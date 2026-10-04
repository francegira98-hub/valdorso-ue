# Valdorso - Il controllo prima del commit. Scritto da Claude il 04/10/2026 (idea "subito" approvata da Fra).
#
# Parte da solo a ogni "git commit" (lo chiama .githooks/pre-commit) e guarda i file che stanno per entrare
# nel commit, cioè quelli già aggiunti con "git add". Se trova qualcosa che non deve andare su GitHub,
# ferma il commit e spiega cosa fare. Niente viene cancellato o cambiato: il controllo legge e basta.
#
# Cosa ferma (errori):
#   1. Cartelle e file che non devono mai uscire dal PC: Saved/ (archivio degli account, chiavi del server,
#      registri), Binaries/, Intermediate/, DerivedDataCache/, i contenuti con licenza (Content/Fab,
#      Content/Mixamo), gli asset pesanti del menu, "Claude outputs", chiavi e certificati (.pem, .key, .pfx...).
#   2. Segreti scritti dentro i file: chiavi private, token di GitHub, chiavi di accesso di servizi cloud.
#   3. File binari o pesanti fuori da Git LFS (per esempio un .uasset finito nel Git normale).
#   4. Segni di un conflitto di Git lasciati nel codice (<<<<<<< e >>>>>>>).
#   5. Nomi doppi nei namespace anonimi dei .cpp: Unreal compila i .cpp a gruppi (unity build), e due funzioni
#      interne con lo stesso nome in file diversi prima o poi finiscono insieme (lezione del 04/10, ProblemaPassword).
# Cosa segnala soltanto (avvisi): file grandi anche se sono in LFS (più di 50 MB), file vuoti, nomi con spazi
# finali o caratteri che Windows non accetta.
#
# Uso a mano (dalla cartella del progetto):
#   python Tools/Controlli/prima_del_commit.py            controlla i file aggiunti con "git add"
#   python Tools/Controlli/prima_del_commit.py --tutto    controlla tutto il repository (prova generale)
# In caso di vera necessità si salta con "git commit --no-verify" (meglio di no).

import os
import re
import subprocess
import sys
from collections import defaultdict

# --- Regole ----------------------------------------------------------------------------------------------

# Percorsi vietati (inizio del percorso, con le barre "/"), con il motivo.
VIETATI = [
    ("Saved/", "dentro Saved ci sono l'archivio degli account, le chiavi del server e i registri"),
    ("Binaries/", "file compilati: si rifanno da soli"),
    ("Intermediate/", "file temporanei della compilazione"),
    ("DerivedDataCache/", "cache di Unreal: si rifà da sola"),
    ("Build/Windows/", None),  # vedi sotto: solo Application.ico è permesso
    ("Content/Fab/", "contenuti di Fab: la licenza non permette di pubblicarli"),
    ("Content/Mixamo/", "contenuti Mixamo: la licenza non permette di pubblicarli"),
    ("Content/Menu/Orso/", "asset pesante del menu, tenuto fuori da GitHub (decisione del 03/10)"),
    ("Content/Menu/Texture/", "asset pesante del menu, tenuto fuori da GitHub (decisione del 03/10)"),
    ("Content/Menu/Montagne/", "asset pesante del menu, tenuto fuori da GitHub (decisione del 03/10)"),
    ("Content/Menu/Montagne2/", "asset pesante del menu, tenuto fuori da GitHub (decisione del 03/10)"),
    ("Claude outputs/", "anteprime dell'app di Claude, non fanno parte del gioco"),
    (".vs/", "impostazioni personali di Visual Studio"),
]
PERMESSI = {"Build/Windows/Application.ico"}

# Estensioni vietate ovunque: chiavi, certificati, archivi di account.
ESTENSIONI_VIETATE = {
    ".pem": "chiave o certificato (la chiave pubblica va solo in DefaultGame.ini, su una riga)",
    ".key": "chiave privata",
    ".pfx": "certificato con chiave privata",
    ".p12": "certificato con chiave privata",
    ".keystore": "archivio di chiavi",
    ".jks": "archivio di chiavi",
    ".env": "file di variabili, spesso con password",
}
NOMI_VIETATI = {
    "inviti.json": "è l'elenco degli inviti del server",
    "registro_accessi.log": "è il registro degli accessi del server",
}

# Segreti dentro i file. I pezzi sono spezzati, così questo file non segnala se stesso.
SEGRETI = [
    (re.compile(r"-----BEGIN (?:RSA |EC |DSA |OPENSSH |ENCRYPTED )?" + "PRIVATE" + r" KEY-----"), "una chiave privata"),
    (re.compile(r"\b(?:gh" + r"[pousr]_[A-Za-z0-9]{36,}|github" + r"_pat_[A-Za-z0-9_]{60,})"), "un token di GitHub"),
    (re.compile(r"\bAK" + r"IA[0-9A-Z]{16}\b"), "una chiave di accesso di Amazon (AWS)"),
    (re.compile(r"\bAI" + r"za[0-9A-Za-z_\-]{35}\b"), "una chiave di Google"),
    (re.compile(r"\bsk-" + r"(?:ant-)?[A-Za-z0-9_\-]{32,}"), "una chiave segreta di un servizio (sk-...)"),
    (re.compile(r"https://(?:discord(?:app)?\.com)/api/" + r"webhooks/\d+/[\w\-]+"), "l'indirizzo segreto di un webhook di Discord"),
]

# Estensioni che devono sempre stare in Git LFS.
SEMPRE_LFS = {".uasset", ".umap", ".png", ".jpg", ".jpeg", ".tga", ".psd", ".exr", ".hdr", ".fbx", ".obj",
              ".glb", ".gltf", ".wav", ".mp3", ".ogg", ".mp4", ".ttf", ".otf", ".bmp", ".ico", ".zip", ".7z"}

LIMITE_SENZA_LFS = 1 * 1024 * 1024        # 1 MB: oltre, un file normale va in LFS
AVVISO_LFS = 50 * 1024 * 1024              # 50 MB: avviso anche in LFS (lo spazio LFS è limitato)
LIMITE_LETTURA = 5 * 1024 * 1024           # i file più grandi non si leggono per cercare segreti

ESTENSIONI_TESTO = {".cpp", ".h", ".hpp", ".c", ".cs", ".py", ".ini", ".json", ".txt", ".md", ".cmd", ".bat",
                    ".ps1", ".sh", ".uproject", ".uplugin", ".gitignore", ".gitattributes", ".xml", ".yml",
                    ".yaml", ".html", ".css", ".js", ".csv", ".usf", ".ush", ".hlsl", ""}

CARATTERI_WINDOWS = re.compile(r'[<>:"|?*]')


# --- Git -------------------------------------------------------------------------------------------------

def git(*argomenti, binario=False):
    risultato = subprocess.run(["git", *argomenti], capture_output=True)
    if risultato.returncode != 0:
        raise RuntimeError("git " + " ".join(argomenti) + ": " + risultato.stderr.decode("utf-8", "replace").strip())
    return risultato.stdout if binario else risultato.stdout.decode("utf-8", "replace")


def file_da_controllare(tutto):
    if tutto:
        uscita = git("ls-files", "-z")
    else:
        uscita = git("diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z")
    return [f for f in uscita.split("\0") if f]


def attributo_lfs(percorsi):
    """Per ogni percorso, vero se .gitattributes lo manda in LFS."""
    risultato = {}
    for i in range(0, len(percorsi), 200):
        gruppo = percorsi[i:i + 200]
        uscita = git("check-attr", "-z", "filter", "--", *gruppo)
        pezzi = uscita.split("\0")
        for j in range(0, len(pezzi) - 2, 3):
            risultato[pezzi[j]] = pezzi[j + 2] == "lfs"
    return risultato


def dimensioni(percorsi):
    """I pesi di tutti i file in una sola chiamata a Git (su Windows ogni chiamata costa tempo)."""
    risultato = {}
    richiesta = "".join(":" + p + "\n" for p in percorsi).encode("utf-8")
    uscita = subprocess.run(["git", "cat-file", "--batch-check=%(objectsize)"], input=richiesta,
                            capture_output=True).stdout.decode("utf-8", "replace").splitlines()
    for percorso, riga in zip(percorsi, uscita):
        risultato[percorso] = int(riga) if riga.strip().isdigit() else 0
    return risultato


def contenuto(percorso):
    try:
        return git("show", ":" + percorso, binario=True)
    except RuntimeError:
        return b""


def estensione(percorso):
    nome = os.path.basename(percorso).lower()
    if nome.startswith(".") and nome.count(".") == 1:
        return nome
    return os.path.splitext(nome)[1]


def numero_riga(testo, posizione):
    return testo.count("\n", 0, posizione) + 1


# --- Namespace anonimi --------------------------------------------------------------------------------

DEF_NOME = re.compile(
    r"^\s*(?:template\s*<[^>]*>\s*)?(?:(?:static|inline|constexpr|const|extern|FORCEINLINE)\s+)*"
    r"(?!return\b|if\b|for\b|while\b|switch\b|case\b|else\b|using\b|typedef\b|namespace\b|struct\b|class\b|enum\b)"
    r"[\w:<>,\*&\s]*?[\w>\*&]\s+[\*&]*(\w+)\s*(?:\(|=|\[|\{|;)"
)
TIPI = re.compile(r"^\s*(?:struct|class|enum(?:\s+class)?)\s+(\w+)")


def senza_commenti_e_stringhe(testo):
    # Toglie commenti e stringhe tenendo gli a capo, così i numeri di riga restano giusti.
    def sostituisci(m):
        return re.sub(r"[^\n]", " ", m.group(0))
    schema = re.compile(r'//[^\n]*|/\*.*?\*/|R"(\w*)\(.*?\)\1"|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
    return schema.sub(sostituisci, testo)


def nomi_anonimi(testo):
    """I nomi dichiarati direttamente dentro "namespace { ... }" (funzioni, variabili, tipi), con la riga."""
    pulito = senza_commenti_e_stringhe(testo)
    nomi = []
    for inizio in re.finditer(r"\bnamespace\s*\{", pulito):
        livello = 0
        pos = inizio.end() - 1
        riga_corrente = []
        inizio_riga = pos + 1
        i = pos
        while i < len(pulito):
            c = pulito[i]
            if c == "{":
                livello += 1
                if livello == 2:
                    riga_corrente.append(pulito[inizio_riga:i + 1])
                    dichiarazione = "".join(riga_corrente)
                    nomi.extend(_nomi_da(dichiarazione, pulito, inizio_riga))
                    riga_corrente = []
            elif c == "}":
                livello -= 1
                if livello == 0:
                    break
                if livello == 1:
                    inizio_riga = i + 1
            elif c == ";" and livello == 1:
                riga_corrente.append(pulito[inizio_riga:i + 1])
                nomi.extend(_nomi_da("".join(riga_corrente), pulito, inizio_riga))
                riga_corrente = []
                inizio_riga = i + 1
            i += 1
    return nomi


def _nomi_da(dichiarazione, pulito, inizio):
    # Le righe del preprocessore (#if, #endif...) non fanno parte della dichiarazione.
    righe = [r for r in dichiarazione.split("\n") if not r.lstrip().startswith("#")]
    testo = " ".join(" ".join(righe).split())
    if not testo:
        return []
    m = TIPI.match(testo)
    if m:
        return [(m.group(1), numero_riga(pulito, inizio + len(dichiarazione) - len(dichiarazione.lstrip())))]
    m = DEF_NOME.match(testo)
    if m and m.group(1) not in ("operator",):
        return [(m.group(1), numero_riga(pulito, inizio + len(dichiarazione) - len(dichiarazione.lstrip())))]
    return []


def controlla_nomi_doppi(errori):
    """Su tutti i .cpp del gioco (come sono nel commit), cerca nomi uguali in namespace anonimi di file diversi."""
    tutti = [f for f in git("ls-files", "-z", "--", "Source/").split("\0") if f.endswith(".cpp")]
    dove = defaultdict(list)
    for percorso in tutti:
        testo = contenuto(percorso).decode("utf-8", "replace").lstrip("﻿")
        visti = set()
        for nome, riga in nomi_anonimi(testo):
            if nome not in visti:
                visti.add(nome)
                dove[nome].append((percorso, riga))
    for nome, posti in sorted(dove.items()):
        file_diversi = {p for p, _ in posti}
        if len(file_diversi) > 1:
            elenco = ", ".join(p.split("/")[-1] + " riga " + str(r) for p, r in posti)
            errori.append("Il nome \"" + nome + "\" è usato in namespace anonimi di file diversi (" + elenco + "). "
                          "Con la unity build di Unreal finiranno insieme: rinomina uno dei due "
                          "(per esempio aggiungendo il nome del file, come ProblemaPasswordSchermata).")


# --- Controllo -------------------------------------------------------------------------------------------

def controlla(tutto):
    errori, avvisi = [], []
    percorsi = file_da_controllare(tutto)
    if not percorsi:
        print("[Valdorso] Controllo prima del commit: nessun file da controllare.")
        return 0

    print("[Valdorso] Controllo prima del commit: guardo " + str(len(percorsi)) + " file...", flush=True)
    lfs = attributo_lfs(percorsi)
    pesi = dimensioni(percorsi)

    for percorso in percorsi:
        basso = percorso.lower()
        nome = os.path.basename(basso)
        est = estensione(percorso)

        # 1. Percorsi e nomi vietati.
        if percorso not in PERMESSI:
            for prefisso, motivo in VIETATI:
                if percorso.startswith(prefisso):
                    if prefisso == "Build/Windows/":
                        motivo = "da Build va su GitHub solo l'icona (Build/Windows/Application.ico)"
                    errori.append(percorso + ": non va su GitHub (" + motivo + ").")
                    break
        if est in ESTENSIONI_VIETATE:
            errori.append(percorso + ": non va su GitHub (" + ESTENSIONI_VIETATE[est] + ").")
        if nome in NOMI_VIETATI:
            errori.append(percorso + ": non va su GitHub (" + NOMI_VIETATI[nome] + ").")
        if "/archivio/" in "/" + basso:
            errori.append(percorso + ": sembra l'archivio degli account del server, che non va su GitHub.")

        # Nomi che Windows non accetta.
        for pezzo in percorso.split("/"):
            if CARATTERI_WINDOWS.search(pezzo) or pezzo != pezzo.rstrip(" ."):
                avvisi.append(percorso + ": il nome ha caratteri che Windows non accetta o spazi/punti finali.")
                break

        # 3. LFS.
        in_lfs = lfs.get(percorso, False)
        peso = pesi.get(percorso, 0)
        if est in SEMPRE_LFS and not in_lfs:
            errori.append(percorso + ": i file " + est + " vanno in Git LFS, ma questo no. "
                          "Aggiungi la riga \"*" + est + " filter=lfs diff=lfs merge=lfs -text\" a .gitattributes, "
                          "poi \"git rm --cached\" del file e di nuovo \"git add\".")
        elif not in_lfs and peso > LIMITE_SENZA_LFS:
            errori.append(percorso + ": pesa " + str(round(peso / 1048576, 1)) + " MB ed è fuori da Git LFS "
                          "(limite " + str(LIMITE_SENZA_LFS // 1048576) + " MB). Va in LFS, oppure fuori da GitHub.")
        if in_lfs and peso > AVVISO_LFS:
            avvisi.append(percorso + ": pesa " + str(round(peso / 1048576)) + " MB anche in LFS: lo spazio di LFS è limitato.")
        if peso == 0 and est in {".cpp", ".h", ".py", ".ini"}:
            avvisi.append(percorso + ": è vuoto.")

        # 2 e 4. Dentro i file di testo: segreti e segni di conflitto.
        if in_lfs or peso > LIMITE_LETTURA or (est not in ESTENSIONI_TESTO):
            continue
        dati = contenuto(percorso)
        if b"\0" in dati[:8000]:
            continue
        testo = dati.decode("utf-8", "replace")
        for schema, cosa in SEGRETI:
            trovato = schema.search(testo)
            if trovato:
                errori.append(percorso + " riga " + str(numero_riga(testo, trovato.start())) + ": contiene " + cosa +
                              ". Togli il segreto dal file (se è già finito su GitHub, va anche cambiato).")
        if est in {".cpp", ".h", ".hpp", ".cs", ".py", ".ini", ".uproject"}:
            for m in re.finditer(r"^(<<<<<<< |>>>>>>> )", testo, re.M):
                errori.append(percorso + " riga " + str(numero_riga(testo, m.start())) +
                              ": c'è un segno di conflitto di Git (" + m.group(1).strip() + "): il conflitto non è risolto.")
                break

    # 5. Nomi doppi nei namespace anonimi, solo se il commit tocca dei .cpp (o con --tutto).
    if tutto or any(p.startswith("Source/") and p.endswith(".cpp") for p in percorsi):
        controlla_nomi_doppi(errori)

    # Risultato.
    for a in avvisi:
        print("  avviso: " + a)
    if errori:
        print("")
        print("[Valdorso] COMMIT FERMATO. Da sistemare:")
        for e in errori:
            print("  - " + e)
        print("")
        print("Per togliere un file dal commit senza cancellarlo: git restore --staged \"<file>\"")
        return 1
    print("[Valdorso] Tutto a posto.")
    return 0


if __name__ == "__main__":
    # Le lettere accentate devono uscire bene anche nel Prompt dei comandi di Windows.
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except (AttributeError, ValueError):
        pass
    try:
        # Si lavora sempre dalla cartella principale del repository.
        os.chdir(git("rev-parse", "--show-toplevel").strip())
        sys.exit(controlla("--tutto" in sys.argv[1:]))
    except RuntimeError as errore:
        print("[Valdorso] Il controllo prima del commit non è riuscito a leggere Git: " + str(errore))
        sys.exit(1)
