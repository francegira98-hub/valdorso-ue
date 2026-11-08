# Valdorso - Prepara un aggiornamento per il launcher: il gioco a pezzi per GitHub e la cartella per Altervista.
# Scritto da Claude il 07/11/2026; (08/11) il gioco in parti da 1,9 GB, la cartella del sito pronta, la pagina web.
#
# Si usa dopo aver pacchettizzato il gioco ("Tools\Pacchettizza Valdorso.cmd": il gioco finisce in V:\Pacchetti\Windows).
# Di solito lo lancia "Tools\Pubblica Valdorso.cmd". A mano, dalla cartella del progetto:
#
#   python Tools\Launcher\prepara_aggiornamento.py V:\Pacchetti\Windows 0.1.3 https://github.com/UTENTE/REPO/releases/download/v0.1.3/
#
# (al posto di 0.1.3 si può scrivere "auto": prende il numero da Source/Valdorso/ValdorsoVersione.h; nell'indirizzo
#  "{versione}" diventa il numero: https://github.com/UTENTE/REPO/releases/download/v{versione}/)
#
# Fa due cartelle dentro Build (fuori da GitHub):
#   Build\Aggiornamento\0.1.3\    le parti del gioco (Valdorso_0.1.3.zip.001, .002...), ognuna al massimo 1,9 GB
#                                  (GitHub ne accetta 2 per file), "Carica su GitHub.cmd" e le note per la release.
#                                  Le parti vanno caricate nella release di GitHub (quella dell'indirizzo dato sopra).
#   Build\Altervista\launcher\    TUTTO quello che va sul sito, nella cartella /launcher/ (con FileZilla):
#                                  manifesto.json, notizie.json, patch.json, roadmap.json, cronache.json, immagini/,
#                                  index.html (la pagina "Valdorso - il launcher") e ValdorsoLauncher.zip.
#
# Opzioni:
#   --solo-sito                  rifà solo la cartella per Altervista (per cambiare notizie, note o roadmap senza
#                                ripubblicare il gioco): usa il manifesto dell'ultima versione preparata
#   --nome "La valle"            il nome della versione (di serie quello di ValdorsoVersione.h)
#   --server 1.2.3.4:7777        l'indirizzo del server che il launcher passa al gioco (vuoto: "la valle dorme")
#   --stato http://1.2.3.4:7780/stato   la pagina dello stato del mondo (di serie: dal server, porta 7780)
#   --parte-mb 1900              quanto è grande al massimo ogni parte, in MB
#   --sito https://valdorso.altervista.org/launcher/   dove sta la cartella del sito
#   --launcher 1.1.0 --url-launcher https://...        l'ultima versione del launcher (di serie: quella scritta in
#                                src/Base.cs e lo zip sul sito): chi ne ha una più vecchia vede l'avviso
#   --notizie file.json          (come prima) un elenco di notizie che prende il posto di quelle di contenuti/notizie.json
#   --chiuso                     la valle è chiusa per lavori (il launcher lo dice; si gioca lo stesso offline)
#
# I testi (notizie, note delle patch, roadmap, Cronache) si scrivono in Tools\Launcher\contenuti\*.json. Il campo
# "aggiornato" lo mette questo script (adesso), così il launcher sa che la copia del sito è più nuova della sua.

import argparse
import datetime
import hashlib
import html
import io
import json
import os
import re
import shutil
import sys
import zipfile

QUI = os.path.dirname(os.path.abspath(__file__))
PROGETTO = os.path.abspath(os.path.join(QUI, "..", ".."))
ESCLUSI_CARTELLE = {"Saved", "Intermediate", "DerivedDataCache"}
ESCLUSI_ESTENSIONI = {".pdb"}
# questi sono già compressi: nello zip si mettono così come sono (si fa prima, e lo zip non cresce)
GIA_COMPRESSI = {".pak", ".ucas", ".utoc", ".bk2", ".mp4", ".png", ".jpg", ".jpeg", ".ogg", ".zip", ".7z"}
LIMITE_ALTERVISTA = 2 * 1024 * 1024
CONTENUTI = ["notizie.json", "patch.json", "roadmap.json", "cronache.json"]


def sha256(percorso):
    h = hashlib.sha256()
    with open(percorso, "rb") as f:
        while True:
            pezzo = f.read(1 << 20)
            if not pezzo:
                break
            h.update(pezzo)
    return h.hexdigest()


def leggi_h(chiave):
    h = os.path.join(PROGETTO, "Source", "Valdorso", "ValdorsoVersione.h")
    try:
        with open(h, encoding="utf-8") as f:
            testo = f.read()
        m = re.search(chiave + r'\(\)\s*\{\s*return TEXT\("([^"]+)"\)', testo)
        return m.group(1) if m else ""
    except OSError:
        return ""


def versione_launcher():
    try:
        with open(os.path.join(QUI, "src", "Base.cs"), encoding="utf-8") as f:
            m = re.search(r'const string Versione = "([^"]+)"', f.read())
        return m.group(1) if m else ""
    except OSError:
        return ""


def misura(n):
    if n >= 1 << 30:
        return ("%.1f GB" % (n / (1 << 30))).replace(".", ",")
    if n >= 1 << 20:
        return ("%.1f MB" % (n / (1 << 20))).replace(".", ",")
    return "%d KB" % max(1, n // 1024)


def file_del_gioco(cartella):
    for radice, cartelle, files in os.walk(cartella):
        cartelle[:] = sorted(c for c in cartelle if c not in ESCLUSI_CARTELLE)
        for n in sorted(files):
            if os.path.splitext(n)[1].lower() in ESCLUSI_ESTENSIONI:
                continue
            completo = os.path.join(radice, n)
            yield completo, os.path.relpath(completo, cartella).replace("\\", "/")


class FileAParti(io.RawIOBase):
    """Un file che su disco è fatto di parti uguali (nome.001, nome.002...): lo zip ci scrive dentro come in un file
    solo, anche tornando indietro (zipfile lo fa per scrivere le misure di ogni file). Così non serve mai lo spazio
    per lo zip intero più quello delle parti."""

    def __init__(self, base, grandezza):
        super().__init__()
        self.base = base
        self.grandezza = grandezza
        self.posizione = 0
        self.lunghezza = 0
        self.aperti = {}

    def nome(self, k):
        return "%s.%03d" % (self.base, k + 1)

    def _parte(self, k):
        f = self.aperti.get(k)
        if f is None:
            percorso = self.nome(k)
            f = open(percorso, "r+b" if os.path.exists(percorso) else "w+b")
            self.aperti[k] = f
            # al massimo due parti aperte: quella di prima serve solo se zipfile torna indietro
            for vecchia in [x for x in self.aperti if x < k - 1]:
                self.aperti.pop(vecchia).close()
        return f

    def writable(self):
        return True

    def seekable(self):
        return True

    def readable(self):
        return False

    def tell(self):
        return self.posizione

    def seek(self, dove, da=0):
        if da == 0:
            self.posizione = dove
        elif da == 1:
            self.posizione += dove
        else:
            self.posizione = self.lunghezza + dove
        return self.posizione

    def write(self, dati):
        dati = memoryview(dati).cast("B")
        scritti = 0
        while scritti < len(dati):
            k, dentro = divmod(self.posizione, self.grandezza)
            quanti = min(len(dati) - scritti, self.grandezza - dentro)
            f = self._parte(k)
            f.seek(dentro)
            f.write(dati[scritti:scritti + quanti])
            scritti += quanti
            self.posizione += quanti
        self.lunghezza = max(self.lunghezza, self.posizione)
        return scritti

    def flush(self):
        for f in self.aperti.values():
            f.flush()

    def close(self):
        if not self.closed:
            for f in self.aperti.values():
                f.close()
            self.aperti.clear()
        super().close()

    def parti(self):
        n = (self.lunghezza + self.grandezza - 1) // self.grandezza
        return [self.nome(k) for k in range(max(1, n))]


def fai_parti(cartella, cartella_uscita, versione, grandezza):
    """Lo zip del gioco scritto direttamente a parti. Restituisce (parti, elenco dei file, byte da scompattato)."""
    base = os.path.join(cartella_uscita, "Valdorso_%s.zip" % versione)
    for vecchio in os.listdir(cartella_uscita):
        if vecchio.startswith("Valdorso_") and ".zip" in vecchio:
            os.remove(os.path.join(cartella_uscita, vecchio))
    elenco = []
    installato = 0
    flusso = FileAParti(base, grandezza)
    with zipfile.ZipFile(flusso, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as z:
        tutti = list(file_del_gioco(cartella))
        for i, (completo, relativo) in enumerate(tutti, 1):
            grande = os.path.getsize(completo)
            info = zipfile.ZipInfo.from_file(completo, relativo)
            if info.date_time[0] < 1980:
                info.date_time = (1980, 1, 1, 0, 0, 0)
            info.compress_type = zipfile.ZIP_STORED if os.path.splitext(completo)[1].lower() in GIA_COMPRESSI else zipfile.ZIP_DEFLATED
            info.file_size = grande  # così zipfile sa già se serve lo Zip64 (file oltre i 4 GB)
            h = hashlib.sha256()
            with open(completo, "rb") as dentro, z.open(info, "w") as fuori:
                while True:
                    pezzo = dentro.read(1 << 20)
                    if not pezzo:
                        break
                    h.update(pezzo)
                    fuori.write(pezzo)
            elenco.append({"p": relativo, "s": h.hexdigest(), "b": grande})
            installato += grande
            if i % 50 == 0 or grande > 200 << 20:
                print("  %d di %d file (%s)" % (i, len(tutti), relativo))
    flusso.close()
    return flusso.parti(), elenco, installato


def leggi_json(percorso):
    with open(percorso, encoding="utf-8-sig") as f:
        return json.load(f)


def scrivi_json(percorso, dati, compatto=False):
    with open(percorso, "w", encoding="utf-8", newline="\n") as f:
        if compatto:
            json.dump(dati, f, ensure_ascii=False, separators=(",", ":"))
        else:
            json.dump(dati, f, ensure_ascii=False, indent=1)
            f.write("\n")


def note_markdown(patch, versione):
    """Le note della versione per la pagina della release su GitHub."""
    for v in patch.get("versioni", []):
        if v.get("versione") == versione:
            righe = ["# Valdorso %s \u00ab%s\u00bb" % (versione, v.get("nome", "")), ""]
            if v.get("sommario"):
                righe += [v["sommario"], ""]
            for g in v.get("gruppi", []):
                righe.append("## " + g.get("titolo", ""))
                righe += ["- " + x for x in g.get("voci", [])]
                righe.append("")
            righe += ["Si installa con il launcher: https://valdorso.altervista.org/launcher/", ""]
            return "\n".join(righe)
    return "# Valdorso %s\n\nSi installa con il launcher: https://valdorso.altervista.org/launcher/\n" % versione


def carica_su_github_cmd(cartella, url, versione, parti):
    """Scrive "Carica su GitHub.cmd" (righe CRLF) se l'indirizzo è quello di una release di GitHub."""
    m = re.match(r"https://github\.com/([^/]+)/([^/]+)/releases/download/([^/]+)/?$", url)
    if not m:
        return None
    utente, repo, etichetta = m.groups()
    nomi = " ".join('"%s"' % os.path.basename(p) for p in parti)
    righe = [
        "@echo off",
        "rem Valdorso - Carica le parti del gioco %s nella release %s di GitHub (%s/%s)." % (versione, etichetta, utente, repo),
        "rem Serve gh (GitHub CLI) gia' collegato: gh auth login. Senza gh, si caricano a mano dal browser:",
        "rem https://github.com/%s/%s/releases/new  (etichetta %s, poi trascina i file .001, .002... e Pubblica)." % (utente, repo, etichetta),
        'cd /d "%~dp0"',
        "where gh >nul 2>nul",
        "if errorlevel 1 (",
        "    echo Non trovo gh. Carica le parti a mano dal browser: https://github.com/%s/%s/releases/new" % (utente, repo),
        "    pause",
        "    exit /b 1",
        ")",
        'gh release view %s --repo %s/%s >nul 2>nul' % (etichetta, utente, repo),
        "if errorlevel 1 (",
        '    gh release create %s --repo %s/%s --title "Valdorso %s" --notes-file note_github.md %s' % (etichetta, utente, repo, versione, nomi),
        ") else (",
        "    gh release upload %s --repo %s/%s --clobber %s" % (etichetta, utente, repo, nomi),
        ")",
        "if errorlevel 1 (",
        "    echo CARICAMENTO NON RIUSCITO: manda uno screenshot a Claude.",
        "    pause",
        "    exit /b 1",
        ")",
        "echo.",
        "echo PARTI CARICATE. Ora trascina Build\\Altervista\\launcher su Altervista con FileZilla.",
        "pause",
    ]
    percorso = os.path.join(cartella, "Carica su GitHub.cmd")
    with open(percorso, "w", encoding="ascii", errors="replace", newline="\r\n") as f:
        f.write("\n".join(righe) + "\n")
    return percorso, etichetta


def zip_launcher(cartella_launcher, destinazione):
    """ValdorsoLauncher.zip: il programma, launcher.json di serie e le risorse (niente registro, cache, gioco, scaricati)."""
    fuori = {"cache", "gioco", "gioco_nuovo", "gioco_vecchio", "scaricati"}
    with zipfile.ZipFile(destinazione, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for radice, cartelle, files in os.walk(cartella_launcher):
            cartelle[:] = sorted(c for c in cartelle if c.lower() not in fuori)
            for n in sorted(files):
                if n.lower().endswith((".log", ".part", ".scarico", ".nuovo")) or n.lower() in ("anteprima_icona.png", "launcher_opzioni.json"):
                    continue
                completo = os.path.join(radice, n)
                relativo = os.path.relpath(completo, cartella_launcher).replace("\\", "/")
                if relativo == "launcher.json":
                    # quello di serie, non quello del PC di chi pubblica (che può avere un server di prova)
                    completo = os.path.join(QUI, "launcher.json")
                z.write(completo, "Valdorso/" + relativo)


def prepara_sito(a, manifesto, sito_cartella):
    """La cartella da caricare su Altervista, nella cartella /launcher/ del sito."""
    adesso = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
    oggi = datetime.date.today().isoformat()
    versione = manifesto["versione"]
    if os.path.isdir(sito_cartella):
        shutil.rmtree(sito_cartella)
    os.makedirs(os.path.join(sito_cartella, "immagini"))

    dati = {}
    for nome in CONTENUTI:
        percorso = os.path.join(a.contenuti, nome)
        if not os.path.isfile(percorso):
            print("ATTENZIONE: manca %s: il launcher userà la copia che ha dentro" % percorso)
            continue
        d = leggi_json(percorso)
        d["gioco"] = "Valdorso"
        d["aggiornato"] = adesso
        d.pop("_come", None)
        dati[nome] = d
    if a.notizie and "notizie.json" in dati:
        dati["notizie.json"]["notizie"] = leggi_json(a.notizie)
    # la versione che si pubblica adesso prende la data di oggi, se non ce l'ha
    for v in dati.get("patch.json", {}).get("versioni", []):
        if v.get("versione") == versione and not v.get("data"):
            v["data"] = manifesto.get("data", oggi)
    for t in dati.get("roadmap.json", {}).get("tappe", []):
        if t.get("versione") == versione and t.get("stato") == "fatta" and not t.get("data"):
            t["data"] = manifesto.get("data", oggi)
    # le forme delle rune per la pagina web (dalla fonte unica)
    if "cronache.json" in dati:
        sys.path.insert(0, os.path.join(PROGETTO, "Tools", "Generatori"))
        try:
            import alfabeto_rune
            dati["cronache.json"].setdefault("rune", {})["linee"] = alfabeto_rune.linee_json()
        except Exception as e:  # noqa: BLE001
            print("ATTENZIONE: forme delle rune non aggiunte (%s)" % e)
    for nome, d in dati.items():
        scrivi_json(os.path.join(sito_cartella, nome), d)

    # il manifesto: per il launcher 1.0 anche le prime notizie dentro
    m = dict(manifesto)
    m["aggiornato"] = adesso
    m["notizie"] = [{k: n.get(k, "") for k in ("data", "titolo", "testo", "link")} for n in dati.get("notizie.json", {}).get("notizie", [])[:3]]
    scrivi_json(os.path.join(sito_cartella, "manifesto.json"), m)

    # le immagini: quelle del launcher (risorse\immagini) e quelle in contenuti\immagini, più lo sfondo e l'icona
    risorse = os.path.join(a.launcher_cartella, "risorse")
    for sorgente in (os.path.join(risorse, "immagini"), os.path.join(a.contenuti, "immagini")):
        if os.path.isdir(sorgente):
            for n in os.listdir(sorgente):
                if n.lower().endswith((".jpg", ".jpeg", ".png")):
                    shutil.copyfile(os.path.join(sorgente, n), os.path.join(sito_cartella, "immagini", n))
    for n, d in (("sfondo.jpg", "sfondo.jpg"), ("anteprima_icona.png", "icona.png")):
        if os.path.isfile(os.path.join(risorse, n)):
            shutil.copyfile(os.path.join(risorse, n), os.path.join(sito_cartella, "immagini", d))
    mancanti = [n.get("immagine") for n in dati.get("notizie.json", {}).get("notizie", [])
                if n.get("immagine") and not os.path.isfile(os.path.join(sito_cartella, "immagini", n["immagine"]))]
    if mancanti:
        print("ATTENZIONE: le notizie chiedono immagini che non ci sono: %s" % ", ".join(mancanti))

    # il launcher da scaricare
    zip_launcher_percorso = os.path.join(sito_cartella, "ValdorsoLauncher.zip")
    if os.path.isfile(os.path.join(a.launcher_cartella, "ValdorsoLauncher.exe")):
        zip_launcher(a.launcher_cartella, zip_launcher_percorso)
    else:
        print("ATTENZIONE: non trovo %s: prima compila il launcher (Compila launcher.cmd)" % os.path.join(a.launcher_cartella, "ValdorsoLauncher.exe"))

    # la pagina web
    pagina = PAGINA
    incorporati = {k[:-5]: v for k, v in dati.items()}
    incorporati["manifesto"] = {"versione": m["versione"], "nome": m.get("nome", ""), "data": m.get("data", ""),
                                "byte": m.get("pacchetto", {}).get("byte", 0), "launcher_versione": m.get("launcher_versione", "")}
    incorporati["launcher_zip"] = os.path.getsize(zip_launcher_percorso) if os.path.isfile(zip_launcher_percorso) else 0
    testo_dati = json.dumps(incorporati, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    pagina = pagina.replace("/*DATI*/null", testo_dati)
    with open(os.path.join(sito_cartella, "index.html"), "w", encoding="utf-8", newline="\n") as f:
        f.write(pagina)

    # Altervista gratis: file piccoli
    troppo = []
    for radice, _, files in os.walk(sito_cartella):
        for n in files:
            p = os.path.join(radice, n)
            if os.path.getsize(p) > LIMITE_ALTERVISTA:
                troppo.append("%s (%s)" % (os.path.relpath(p, sito_cartella), misura(os.path.getsize(p))))
    if troppo:
        print("ATTENZIONE: questi file superano i 2 MB e Altervista potrebbe rifiutarli: %s" % ", ".join(troppo))
        print("            (ValdorsoLauncher.zip si può mettere anche nella release di GitHub: poi --url-launcher)")
    return sito_cartella


def ultima_preparata(uscita):
    base = os.path.join(uscita, "Aggiornamento")
    migliore = None
    if os.path.isdir(base):
        for n in os.listdir(base):
            p = os.path.join(base, n, "manifesto.json")
            if os.path.isfile(p) and re.fullmatch(r"\d+(\.\d+)*", n):
                chiave = tuple(int(x) for x in n.split("."))
                if migliore is None or chiave > migliore[0]:
                    migliore = (chiave, p)
    return migliore[1] if migliore else None


def principale(argomenti=None):
    p = argparse.ArgumentParser(description="Prepara il gioco a parti (GitHub) e la cartella del sito (Altervista) per il launcher di Valdorso.")
    p.add_argument("cartella", nargs="?", help="la cartella del gioco pacchettizzato (quella con Valdorso.exe), es. V:\\Pacchetti\\Windows")
    p.add_argument("versione", nargs="?", help="per esempio 0.1.3, oppure auto")
    p.add_argument("url", nargs="?", help="dove stanno le parti: https://github.com/UTENTE/REPO/releases/download/v0.1.3/")
    p.add_argument("--solo-sito", action="store_true")
    p.add_argument("--nome", default=None)
    p.add_argument("--server", default=None)
    p.add_argument("--stato", default=None)
    p.add_argument("--parte-mb", type=int, default=1900)
    p.add_argument("--uscita", default=os.path.join(PROGETTO, "Build"))
    p.add_argument("--eseguibile", default="Valdorso.exe")
    p.add_argument("--sito", default="https://valdorso.altervista.org/launcher/")
    p.add_argument("--launcher", default=None)
    p.add_argument("--url-launcher", default=None)
    p.add_argument("--launcher-cartella", default=os.path.join(PROGETTO, "Build", "Launcher"))
    p.add_argument("--contenuti", default=os.path.join(QUI, "contenuti"))
    p.add_argument("--notizie", default=None)
    p.add_argument("--chiuso", action="store_true")
    a = p.parse_args(argomenti)
    a.sito = a.sito.rstrip("/") + "/"

    if a.solo_sito:
        da = ultima_preparata(a.uscita)
        if not da:
            print("ERRORE: non c'è ancora nessuna versione preparata in %s. Prima prepara il gioco (senza --solo-sito)." % os.path.join(a.uscita, "Aggiornamento"))
            return 1
        manifesto = leggi_json(da)
        print("Rifaccio solo il sito, con il manifesto della v%s (%s)" % (manifesto["versione"], da))
    else:
        if not (a.cartella and a.versione and a.url):
            p.print_usage()
            print("ERRORE: servono la cartella del gioco, la versione e l'indirizzo delle parti (oppure --solo-sito)")
            return 1
        cartella = os.path.abspath(a.cartella)
        if not os.path.isfile(os.path.join(cartella, a.eseguibile)):
            print("ERRORE: in %s non c'è %s. È la cartella giusta (quella del gioco pacchettizzato)?" % (cartella, a.eseguibile))
            return 1
        versione = leggi_h("Numero") if a.versione.lower() == "auto" else a.versione
        if not re.fullmatch(r"\d+(\.\d+)*", versione or ""):
            print("ERRORE: la versione deve essere fatta di numeri e punti (per esempio 0.1.3)")
            return 1
        if not re.match(r"https?://", a.url):
            print("ERRORE: l'indirizzo delle parti deve cominciare con https://")
            return 1
        url = a.url.replace("{versione}", versione).rstrip("/") + "/"
        etichetta = re.search(r"/releases/download/([^/]+)/$", url)
        if etichetta and etichetta.group(1) not in ("v" + versione, versione):
            print("ATTENZIONE: la release dell'indirizzo è %s ma la versione è %s: è giusto?" % (etichetta.group(1), versione))
        uscita_ver = os.path.join(a.uscita, "Aggiornamento", versione)
        os.makedirs(uscita_ver, exist_ok=True)
        grandezza = a.parte_mb * 1024 * 1024
        print("Preparo il gioco v%s da %s (parti da %d MB al massimo)..." % (versione, cartella, a.parte_mb))
        parti, elenco, installato = fai_parti(cartella, uscita_ver, versione, grandezza)
        voci = []
        totale = 0
        for percorso in parti:
            b = os.path.getsize(percorso)
            totale += b
            voci.append({"url": url + os.path.basename(percorso), "sha256": sha256(percorso), "byte": b})
            print("  %s  %s" % (os.path.basename(percorso), misura(b)))
        print("  %d file, %s scompattato, %s in %d parti" % (len(elenco), misura(installato), misura(totale), len(parti)))

        # il server e il resto: se non si dicono, restano quelli dell'ultima volta
        vecchio = {}
        prima = ultima_preparata(a.uscita)
        if prima:
            try:
                vecchio = leggi_json(prima)
            except (OSError, ValueError):
                vecchio = {}
        server_vecchio = vecchio.get("server", {}) if isinstance(vecchio.get("server"), dict) else {}
        pacchetto = {"parti": voci, "byte": totale, "installato": installato}
        if len(voci) == 1:
            # il launcher 1.0 conosce solo lo zip intero
            pacchetto.update({"url": voci[0]["url"], "sha256": voci[0]["sha256"]})
        manifesto = {
            "gioco": "Valdorso",
            "versione": versione,
            "nome": a.nome if a.nome is not None else leggi_h("Nome"),
            "data": datetime.date.today().isoformat(),
            "aperto": not a.chiuso,
            "pacchetto": pacchetto,
            "eseguibile": a.eseguibile,
            "argomenti": "",
            "server": {"indirizzo": a.server if a.server is not None else server_vecchio.get("indirizzo", ""),
                       "stato": a.stato if a.stato is not None else server_vecchio.get("stato", "")},
            "file": elenco,
        }
        scrivi_json(os.path.join(uscita_ver, "manifesto.json"), manifesto)
        contenuti_patch = os.path.join(a.contenuti, "patch.json")
        note = note_markdown(leggi_json(contenuti_patch) if os.path.isfile(contenuti_patch) else {}, versione)
        with open(os.path.join(uscita_ver, "note_github.md"), "w", encoding="utf-8", newline="\n") as f:
            f.write(note)
        gh = carica_su_github_cmd(uscita_ver, url, versione, parti)
        if gh:
            print("  scritto %s" % gh[0])
        else:
            print("  (l'indirizzo non è di una release di GitHub: carica le parti a mano a %s)" % url)

    # in ogni caso: il server, il launcher e l'apertura si possono cambiare anche con --solo-sito
    if a.server is not None or a.stato is not None:
        s = manifesto.setdefault("server", {})
        if a.server is not None:
            s["indirizzo"] = a.server
        if a.stato is not None:
            s["stato"] = a.stato
    if a.chiuso:
        manifesto["aperto"] = False
    manifesto["launcher_versione"] = a.launcher if a.launcher is not None else versione_launcher()
    manifesto["launcher_url"] = a.url_launcher if a.url_launcher is not None else a.sito + "ValdorsoLauncher.zip"
    manifesto["launcher"] = {"versione": manifesto["launcher_versione"], "url": manifesto["launcher_url"]}
    # il manifesto della versione resta aggiornato (server compreso) per la prossima volta
    scrivi_json(os.path.join(a.uscita, "Aggiornamento", manifesto["versione"], "manifesto.json"), manifesto)

    sito = prepara_sito(a, manifesto, os.path.join(a.uscita, "Altervista", "launcher"))
    print("")
    print("FATTO.")
    if not a.solo_sito:
        print("1. Carica le parti di %s nella release di GitHub (doppio clic su \"Carica su GitHub.cmd\" lì dentro," % os.path.join(a.uscita, "Aggiornamento", manifesto["versione"]))
        print("   oppure a mano dal browser).")
        print("2. Con FileZilla trascina la cartella %s dentro /launcher/ di Altervista (sostituisci i file)." % sito)
    else:
        print("Con FileZilla trascina la cartella %s dentro /launcher/ di Altervista (sostituisci i file)." % sito)
    print("Il server nel manifesto: %s" % (manifesto.get("server", {}).get("indirizzo") or "nessuno (il launcher dice \"la valle dorme\")"))
    return 0


PAGINA = r"""<!doctype html>
<html lang="it">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Valdorso — il launcher</title>
<meta name="description" content="Valdorso, la valle dell'Orso: scarica il launcher, leggi le note delle patch, la roadmap e le Cronache.">
<link rel="icon" href="immagini/icona.png">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Cinzel:wght@400;700&family=EB+Garamond:ital,wght@0,400;0,600;1,400&display=swap" rel="stylesheet">
<style>
:root{--fondo:#0E0B09;--pannelli:#15110D;--oro:#D4AF37;--oroc:#F3DC8C;--brace:#D9772B;--perg:#E8D9B5;--sec:#C9B99A;--verde:#78A050}
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{margin:0;background:var(--fondo);color:var(--perg);font-family:"EB Garamond",Georgia,serif;font-size:19px;line-height:1.5}
a{color:var(--oro)}a:hover{color:var(--oroc)}
.cima{position:relative;min-height:min(78vh,760px);display:flex;align-items:flex-end;background:#1a120c url(immagini/sfondo.jpg) center/cover no-repeat}
.cima::after{content:"";position:absolute;inset:0;background:linear-gradient(90deg,rgba(14,11,9,.92),rgba(14,11,9,.35) 60%,rgba(14,11,9,.1)),linear-gradient(0deg,var(--fondo),rgba(14,11,9,0) 45%)}
.cima .dentro{position:relative;z-index:1;max-width:1100px;width:100%;margin:0 auto;padding:48px 24px 64px}
h1{font-family:Cinzel,Georgia,serif;font-weight:700;font-size:clamp(48px,9vw,104px);margin:0;letter-spacing:.04em;line-height:1;background:linear-gradient(180deg,var(--oroc),#A0762A);-webkit-background-clip:text;background-clip:text;color:transparent;filter:drop-shadow(0 3px 2px rgba(0,0,0,.7))}
.motto{font-style:italic;color:var(--sec);font-size:24px;margin:10px 0 28px}
.scarica{display:inline-flex;flex-direction:column;align-items:center;padding:16px 38px;border-radius:6px;text-decoration:none;font-family:Cinzel,Georgia,serif;font-weight:700;font-size:26px;letter-spacing:.06em;color:#2a1806;background:linear-gradient(180deg,#ECCC6C,#AA7C1E);border:1.5px solid #FAE6A0;box-shadow:0 0 28px rgba(212,175,55,.35)}
.scarica:hover{color:#2a1806;background:linear-gradient(180deg,#FAE496,#C4922A)}
.scarica small{font-family:"EB Garamond",Georgia,serif;font-style:italic;font-weight:400;font-size:16px;letter-spacing:0;margin-top:2px}
.nota{color:var(--sec);font-style:italic;font-size:17px;margin-top:14px;max-width:620px}
nav{position:sticky;top:0;z-index:5;background:rgba(14,11,9,.92);border-bottom:1px solid rgba(212,175,55,.35);backdrop-filter:blur(4px)}
nav .dentro{max-width:1100px;margin:0 auto;padding:0 24px;display:flex;gap:28px;overflow-x:auto}
nav a{font-family:Cinzel,Georgia,serif;font-weight:700;font-size:14px;letter-spacing:.08em;color:var(--sec);text-decoration:none;padding:14px 0;white-space:nowrap}
nav a:hover{color:var(--oroc)}
section{max-width:1100px;margin:0 auto;padding:56px 24px 8px}
h2{font-family:Cinzel,Georgia,serif;font-weight:700;color:var(--oro);font-size:16px;letter-spacing:.12em;display:flex;align-items:center;gap:14px;margin:0 0 22px}
h2::after{content:"";flex:1;height:1px;background:linear-gradient(90deg,rgba(212,175,55,.5),transparent)}
h3{font-family:Cinzel,Georgia,serif;color:var(--oroc);font-size:24px;margin:0 0 4px}
.data{color:var(--sec);font-style:italic;font-size:16px}
.griglia{display:grid;gap:18px;grid-template-columns:repeat(auto-fill,minmax(300px,1fr))}
.carta{background:rgba(21,17,13,.85);border:1px solid rgba(212,175,55,.35);border-radius:6px;padding:18px 20px;overflow:hidden}
.carta img{display:block;width:calc(100% + 40px);margin:-18px -20px 14px;height:150px;object-fit:cover}
.carta h4{font-family:Cinzel,Georgia,serif;color:var(--perg);font-size:18px;margin:0 0 2px}
.cartellino{display:inline-block;font-family:Cinzel,Georgia,serif;font-size:11px;font-weight:700;letter-spacing:.08em;padding:1px 7px;border:1px solid currentColor;border-radius:3px;margin-right:8px;vertical-align:2px}
ul.voci{list-style:none;padding:0;margin:8px 0 18px;columns:2 340px;column-gap:32px}
ul.voci li{position:relative;padding-left:20px;margin-bottom:6px;break-inside:avoid}
ul.voci li::before{content:"";position:absolute;left:3px;top:.62em;width:7px;height:7px;background:var(--brace);transform:rotate(45deg)}
.gruppo{font-family:Cinzel,Georgia,serif;font-size:14px;letter-spacing:.1em;color:var(--brace);margin-top:16px}
details{border-top:1px solid rgba(212,175,55,.25);padding:12px 0}
summary{cursor:pointer;font-family:Cinzel,Georgia,serif;color:var(--perg);font-size:18px}
.tappe{position:relative;padding-left:44px}
.tappe::before{content:"";position:absolute;left:13px;top:10px;bottom:20px;width:2px;background:rgba(212,175,55,.35)}
.tappa{position:relative;margin-bottom:26px}
.tappa::before{content:"";position:absolute;left:-40px;top:6px;width:16px;height:16px;border-radius:50%;border:2px solid var(--oro);background:var(--fondo);box-sizing:border-box}
.tappa.fatta::before{background:var(--oro)}
.tappa.in_corso::before{background:var(--brace);border-color:var(--brace);box-shadow:0 0 0 5px rgba(217,119,43,.25)}
.tappa.futura::before{border-color:rgba(201,185,154,.5);transform:scale(.8)}
.tappa.futura h3{color:var(--sec)}
.tappa ul.voci{columns:3 240px;font-size:17px}
.capitoli{display:grid;grid-template-columns:2fr 1fr;gap:32px}
.capitoli p{margin:0 0 14px}
.capitoli>.carta{align-self:start}
.rune{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:14px;margin:14px 0 26px}
.runa{text-align:center;background:rgba(21,17,13,.85);border:1px solid rgba(212,175,55,.35);border-radius:6px;padding:14px 8px}
.runa svg{width:76px;height:76px;filter:drop-shadow(0 0 6px rgba(217,119,43,.55))}
.runa b{display:block;font-family:Cinzel,Georgia,serif;color:var(--oroc);font-size:17px}
.runa i{color:var(--sec);font-size:16px}
.parole{display:grid;grid-template-columns:repeat(auto-fill,minmax(300px,1fr));gap:10px 24px;margin-bottom:26px}
.parola{display:flex;align-items:center;gap:14px}
.parola svg{height:30px;flex:none}
.parola b{font-family:Cinzel,Georgia,serif;font-size:15px;display:block}
.parola i{color:var(--sec);font-size:15px}
.volto b{font-family:Cinzel,Georgia,serif;color:var(--oroc)}
.volto em{display:block;color:#E6A064;font-size:16px}
footer{max-width:1100px;margin:60px auto 0;padding:28px 24px 40px;border-top:1px solid rgba(212,175,55,.3);color:var(--sec);font-style:italic;font-size:16px}
@media (max-width:760px){.capitoli{grid-template-columns:1fr}body{font-size:18px}}
</style>
</head>
<body>
<header class="cima"><div class="dentro">
 <h1>VALDORSO</h1>
 <div class="motto">Ogni gesto lascia un segno. Scegli chi diventare.</div>
 <a class="scarica" href="ValdorsoLauncher.zip" id="scarica">Scarica il launcher<small id="scarica-info">per Windows 10 e 11</small></a>
 <p class="nota">Scompatta lo zip dove vuoi e apri <b>ValdorsoLauncher.exe</b>: il gioco lo scarica e lo aggiorna lui, e ti dice com'è la valle prima di entrare. Se Windows avvisa che il programma non è firmato: «Ulteriori informazioni», poi «Esegui comunque».</p>
</div></header>
<nav><div class="dentro">
 <a href="#notizie">Notizie</a><a href="#patch">Note delle patch</a><a href="#roadmap">Roadmap</a><a href="#cronache">Cronache</a>
</div></nav>
<section id="notizie"><h2>NOTIZIE DALLA VALLE</h2><div class="griglia" id="elenco-notizie"></div></section>
<section id="patch"><h2>NOTE DELLE PATCH</h2><div id="elenco-patch"></div></section>
<section id="roadmap"><h2>ROADMAP</h2><p class="data" id="roadmap-intro"></p><div class="tappe" id="elenco-tappe"></div><p id="roadmap-dopo"></p></section>
<section id="cronache"><h2>CRONACHE DI VAL D'ORSO</h2>
 <div class="capitoli"><div id="capitoli"></div><div class="carta"><h4>Il Cuore</h4><p class="data">Nel Tempio del Cuore batte un frammento del Cuore: due colpi e una pausa, da più di trecento anni. Padre Anselmo dice che finché batte la valle è viva.</p></div></div>
 <h2>L'ALFABETO DELLE RUNE</h2><p id="rune-intro"></p><div class="rune" id="rune"></div>
 <h2>LE PAROLE CON UN NOME</h2><div class="parole" id="parole"></div>
 <h2>I VOLTI DELLA VALLE</h2><p class="data" id="volti-intro"></p><div class="griglia" id="volti"></div>
</section>
<footer>Valdorso è un gioco in costruzione, fatto con pazienza. Le pagine qui sopra sono le stesse che vedi nel launcher.<br><span id="aggiornato"></span></footer>
<script>
var DATI = /*DATI*/null || {};
var MESI = ["gennaio","febbraio","marzo","aprile","maggio","giugno","luglio","agosto","settembre","ottobre","novembre","dicembre"];
function data(t){var m=/^(\d{4})-(\d{2})-(\d{2})/.exec(t||"");return m?(+m[3])+" "+MESI[+m[2]-1]+" "+m[1]:(t||"");}
function el(tag,cls,testo){var e=document.createElement(tag);if(cls)e.className=cls;if(testo!=null)e.textContent=testo;return e;}
function svuota(id){var e=document.getElementById(id);while(e.firstChild)e.removeChild(e.firstChild);return e;}
function misura(n){if(n>=1073741824)return (n/1073741824).toFixed(1).replace(".",",")+" GB";if(n>=1048576)return (n/1048576).toFixed(1).replace(".",",")+" MB";return Math.max(1,Math.round(n/1024))+" KB";}
var NS="http://www.w3.org/2000/svg";
function svgRuna(nome,sp){var l=((DATI.cronache||{}).rune||{}).linee||{};var s=document.createElementNS(NS,"svg");s.setAttribute("viewBox","-1.15 -1.15 2.3 2.3");disegna(s,l[nome],0,sp||0.13);return s;}
function disegna(s,linee,dx,sp){(linee||[]).forEach(function(p){var q=document.createElementNS(NS,"polyline");q.setAttribute("points",p.map(function(v){return (v[0]+dx)+","+(-v[1]);}).join(" "));q.setAttribute("fill","none");q.setAttribute("stroke","#D4AF37");q.setAttribute("stroke-width",sp);q.setAttribute("stroke-linecap","round");q.setAttribute("stroke-linejoin","round");s.appendChild(q);});}
function svgParola(parola){var l=((DATI.cronache||{}).rune||{}).linee||{};var nomi=parola.split("-");var largo=nomi.length*2+(nomi.length-1)*0.9;var s=document.createElementNS(NS,"svg");s.setAttribute("viewBox","-0.1 -1.1 "+(largo+0.2)+" 2.2");s.style.width=(30*largo/2.2)+"px";
 nomi.forEach(function(n,k){var x=1+k*2.9;disegna(s,l[n],x,0.16);if(k<nomi.length-1){var g=document.createElementNS(NS,"line");g.setAttribute("x1",x+0.95);g.setAttribute("x2",x+1.95);g.setAttribute("y1",0);g.setAttribute("y2",0);g.setAttribute("stroke","#D4AF37");g.setAttribute("stroke-width",0.16);g.setAttribute("stroke-linecap","round");s.appendChild(g);}});return s;}
function link(l){if(!l)return null;var a=el("a",null,"");if(l.charAt(0)=="#"){var nomi={"#patch":"Leggi le note della patch","#roadmap":"Guarda la roadmap","#cronache":"Leggi le Cronache"};if(!nomi[l])return null;a.href=l;a.textContent=nomi[l]+" »";}else if(/^https?:\/\//.test(l)){a.href=l;a.textContent="Leggi tutto »";}else return null;return a;}
function mostra(){
 var m=DATI.manifesto||{};
 if(m.versione){document.getElementById("scarica-info").textContent="per Windows 10 e 11 · gioco v"+m.versione+(m.nome?" «"+m.nome+"»":"")+(m.byte?" · "+misura(m.byte):"");}
 var n=svuota("elenco-notizie");((DATI.notizie||{}).notizie||[]).forEach(function(x){var c=el("div","carta");if(x.immagine){var i=el("img");i.src="immagini/"+x.immagine;i.alt="";i.loading="lazy";c.appendChild(i);}var h=el("h4");if(x.etichetta){var t=el("span","cartellino",x.etichetta.toUpperCase());t.style.color="#D9772B";h.appendChild(t);}h.appendChild(document.createTextNode(x.titolo||""));c.appendChild(h);c.appendChild(el("div","data",data(x.data)));c.appendChild(el("p",null,x.testo||""));var a=link(x.link);if(a)c.appendChild(a);n.appendChild(c);});
 var p=svuota("elenco-patch");((DATI.patch||{}).versioni||[]).forEach(function(v,i){var contenitore=i==0?el("div"):el("details");var t=i==0?el("h3",null,"v"+v.versione+(v.nome?" · "+v.nome:"")):el("summary",null,"v"+v.versione+(v.nome?" · "+v.nome:"")+(v.data?" — "+data(v.data):""));contenitore.appendChild(t);if(i==0&&v.data)contenitore.appendChild(el("div","data","Uscita il "+data(v.data)));if(v.sommario)contenitore.appendChild(el("p",null,v.sommario));(v.gruppi||[]).forEach(function(g){contenitore.appendChild(el("div","gruppo",(g.titolo||"").toUpperCase()));var u=el("ul","voci");(g.voci||[]).forEach(function(x){u.appendChild(el("li",null,x));});contenitore.appendChild(u);});p.appendChild(contenitore);});
 var r=DATI.roadmap||{};document.getElementById("roadmap-intro").textContent=r.intro||"";document.getElementById("roadmap-dopo").textContent=r.dopo||"";
 var etichette={"fatta":"FATTA","in_corso":"IN CORSO","prossima":"PROSSIMA","futura":"PIÙ AVANTI"};var colori={"fatta":"#78A050","in_corso":"#D9772B","prossima":"#D4AF37","futura":"#9A8C78"};
 var tp=svuota("elenco-tappe");(r.tappe||[]).forEach(function(t){var d=el("div","tappa "+(t.stato||"futura"));var h=el("h3",null,(t.versione?"v"+t.versione+" · ":"")+(t.nome||"")+" ");var c=el("span","cartellino",(etichette[t.stato]||"PIÙ AVANTI")+(t.stato=="fatta"&&t.data?" · "+data(t.data):""));c.style.color=colori[t.stato]||"#9A8C78";h.appendChild(c);d.appendChild(h);if(t.sommario)d.appendChild(el("div","data",t.sommario));var u=el("ul","voci");(t.voci||[]).forEach(function(x){u.appendChild(el("li",null,x));});d.appendChild(u);tp.appendChild(d);});
 var cr=DATI.cronache||{};var cap=svuota("capitoli");(cr.capitoli||[]).forEach(function(c){cap.appendChild(el("h3",null,c.titolo));(c.testo||"").split(/\n\s*\n/).forEach(function(par){cap.appendChild(el("p",null,par));});});
 var ru=cr.rune||{};document.getElementById("rune-intro").textContent=ru.intro||"";var rg=svuota("rune");[].concat(ru.elementi||[],ru.verbi||[],ru.custode||[]).forEach(function(x){var d=el("div","runa");d.appendChild(svgRuna(x.nome));d.appendChild(el("b",null,x.nome));d.appendChild(el("i",null,x.significato));rg.appendChild(d);});
 var pr=svuota("parole");(ru.parole||[]).forEach(function(x){var d=el("div","parola");d.appendChild(svgParola(x.rune));var t=el("div");t.appendChild(el("b",null,x.nome));t.appendChild(el("i",null,x.rune));d.appendChild(t);pr.appendChild(d);});
 document.getElementById("volti-intro").textContent=cr.volti_intro||"";var vo=svuota("volti");(cr.volti||[]).forEach(function(x){var d=el("div","carta volto");d.appendChild(el("b",null,x.nome));d.appendChild(el("em",null,x.ruolo));d.appendChild(el("span","data",x.testo));vo.appendChild(d);});
 var agg=(DATI.notizie||{}).aggiornato;document.getElementById("aggiornato").textContent=agg?"Pagina aggiornata il "+data(agg)+".":"";
}
mostra();
// se qualcuno ha cambiato solo i file .json sul sito, la pagina li prende freschi
["notizie","patch","roadmap","cronache"].forEach(function(nome){if(!window.fetch)return;fetch(nome+".json",{cache:"no-cache"}).then(function(r){return r.ok?r.json():null;}).then(function(d){if(d&&d.gioco=="Valdorso"){if(nome=="cronache"&&DATI.cronache&&DATI.cronache.rune&&d.rune&&!d.rune.linee){d.rune.linee=DATI.cronache.rune.linee;}DATI[nome]=d;mostra();}}).catch(function(){});});
</script>
</body>
</html>
"""


if __name__ == "__main__":
    sys.exit(principale())
