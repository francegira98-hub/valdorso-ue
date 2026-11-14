# Valdorso - La Gazzetta del passo, fuori dal gioco (13/11/2026, scritto da Claude; idea approvata da Fra: "con
# revisione dello staff fuori dal gioco, poi nel gioco la proclama un ente; e lo stesso per gli annunci").
#
#   python gazzetta.py componi [giorni]   la bozza della settimana dal registro eventi (Saved/Server/Eventi, gli ultimi
#                                         7 giorni o quelli detti) in Saved/Server/Gazzetta/Bozza.txt: lo staff la
#                                         legge e la corregge con il Blocco note
#   python gazzetta.py approva            la bozza corretta diventa il foglio numero N in Saved/Server/Gazzetta/Approvate
#                                         (Gazzetta_00N.json): il Banditore della Corona la proclama nel gioco entro
#                                         30 secondi (ValdorsoBanditore.cpp) e Pubblica (scelta 2) la mette sul sito
#   python gazzetta.py annuncio "testo"   un annuncio dello staff, senza entrare nel gioco (come quello della finestra F9)
#
# Lo lanciano i .cmd accanto a questo file. Usa solo la libreria standard (va bene il Python di Unreal).

import datetime
import glob
import json
import os
import re
import sys
from collections import Counter, OrderedDict

QUI = os.path.dirname(os.path.abspath(__file__))
PROGETTO = os.path.normpath(os.path.join(QUI, "..", ".."))
EVENTI = os.path.join(PROGETTO, "Saved", "Server", "Eventi")
CARTELLA = os.path.join(PROGETTO, "Saved", "Server", "Gazzetta")
APPROVATE = os.path.join(CARTELLA, "Approvate")
BOZZA = os.path.join(CARTELLA, "Bozza.txt")
ENTE = "Il Banditore della Corona"
LETTERE_BACHECA = 190      # come i proclami del Balivo (ValdorsoGildaDati.h, LettereProclama)

MESI = ["gennaio", "febbraio", "marzo", "aprile", "maggio", "giugno", "luglio", "agosto", "settembre", "ottobre",
        "novembre", "dicembre"]

# Le voci della locanda: una per numero (non si sa se vere: le dicerie fanno parte del gioco)
VOCI = [
    "Dicono che di notte, sulle rovine degli Antichi, le rune si accendano da sole.",
    "Il caporale Dino, dopo il terzo boccale, giura che al passo arriveranno ordini nuovi dalla Corona.",
    "Qualcuno ha sentito martellare sotto la frana della grotta del nord, a notte fonda.",
    "Nonna Edda sostiene che l'acqua della palude, bollita, guarisca più di quanto si creda.",
    "Il gatto della locanda passa le sere sempre nello stesso angolo. Chissà perché.",
    "Si dice che la carovana di Sibilla porti più segreti che merci.",
    "Qualcuno ha visto un'ombra grande come un orso salire verso la collina, al tramonto.",
    "Bruno giura che il suo stufato, una volta, ha rimesso in piedi un miliziano ferito.",
]

# I tipi del registro eventi che diventano notizie (ValdorsoRegistroEventi::Annota in tutto il codice)
NOTIZIE_FATTI = {"FaseVillaggio", "Atto", "Sigillo", "Grotta", "Cerimonia", "Allarme", "Storia"}


def plurale(n, uno, tanti):
    return uno if n == 1 else "%d %s" % (n, tanti)


def data_italiana(d):
    return "%d %s" % (d.day, MESI[d.month - 1])


def nome_vero(n):
    """Solo i nomi dei coloni e degli abitanti, non quelli tecnici (BP_..., ValdorsoPNG_UAID_...)."""
    return bool(n) and not re.search(r"[_0-9]", n) and n not in ("?", "-", "WorldSettings", "staff")


def leggi_eventi(giorni):
    # il server chiama i file con la data UTC (ValdorsoRegistroEventi.cpp): si conta da oggi in UTC, un giorno in più
    oggi = datetime.datetime.now(datetime.timezone.utc).date()
    giorni += 1
    eventi = []
    for k in range(giorni):
        giorno = oggi - datetime.timedelta(days=k)
        f = os.path.join(EVENTI, "Eventi_%s.jsonl" % giorno.isoformat())
        if not os.path.exists(f):
            continue
        with open(f, encoding="utf-8") as righe:
            for riga in righe:
                riga = riga.strip()
                if not riga:
                    continue
                try:
                    e = json.loads(riga)
                except ValueError:
                    continue
                if "Prove" in e.get("mappa", "") or "UEDPIE" in e.get("mappa", ""):
                    continue      # le prove nell'editor non fanno notizia
                eventi.append(e)
    return eventi


def componi_testo(eventi, numero, giorni):
    oggi = datetime.date.today()
    dal = oggi - datetime.timedelta(days=giorni - 1)
    per_tipo = {}
    for e in eventi:
        per_tipo.setdefault(e.get("tipo", ""), []).append(e)
    sezioni = OrderedDict()
    titoli = []

    # i coloni sono quelli che sono entrati nella valle (gli abitanti non "entrano")
    coloni = {e.get("chi") for e in per_tipo.get("Entra", []) if nome_vero(e.get("chi"))}
    if coloni:
        sezioni["La valle in numeri"] = ["Questa settimana %s nella valle." % plurale(len(coloni), "un colono ha camminato", "coloni hanno camminato")]

    gilda = [e for e in per_tipo.get("Gilda", []) if nome_vero(e.get("chi"))]
    if gilda:
        sezioni["Dalla Gilda"] = ["%s: %s." % (e["chi"], e.get("dettagli", "").rstrip(".")) for e in gilda[-6:]]
        titoli.append("le novità della Gilda")

    maestri = [e for t in ("Marchio", "Crescere", "Tecnica") for e in per_tipo.get(t, []) if nome_vero(e.get("chi"))]
    if maestri:
        sezioni["Botteghe e maestri"] = ["%s: %s." % (e["chi"], e.get("dettagli", "").rstrip(".")) for e in maestri[-6:]]
        titoli.append("i progressi delle botteghe")

    magazzino = Counter(e["chi"] for e in per_tipo.get("Magazzino", []) if nome_vero(e.get("chi")))
    if magazzino:
        primi = ", ".join("%s (%d)" % (n, q) for n, q in magazzino.most_common(3))
        sezioni["Il magazzino del villaggio"] = ["%s al magazzino. I più generosi: %s." % (
            plurale(sum(magazzino.values()), "Una consegna", "consegne").capitalize(), primi)]

    carovane = len(per_tipo.get("Carovana", []))
    if carovane:
        sezioni["Strade e carovane"] = ["La carovana è passata dal piazzale %d volt%s." % (carovane, "a" if carovane == 1 else "e")]

    cronaca = []
    risse = len([e for e in per_tipo.get("Colpo", []) if nome_vero(e.get("chi"))])
    if risse:
        cronaca.append("Le guardie hanno contato %s in risse e duelli." % plurale(risse, "un colpo dato", "colpi dati"))
    morti = len(per_tipo.get("Morte", []))
    if morti:
        cronaca.append("%s al tempio del Cuore." % plurale(morti, "Un colono è caduto e si è rialzato", "coloni sono caduti e si sono rialzati"))
    cacce = len(per_tipo.get("Scuoia", []))
    if cacce:
        cronaca.append("I cacciatori hanno portato a casa %s." % plurale(cacce, "una preda", "prede"))
    trappole = len(per_tipo.get("Trappola", []))
    if trappole:
        cronaca.append("Nella grotta del nord le trappole sono scattate %s: attenzione a dove si mettono i piedi." % plurale(trappole, "una volta", "volte"))
    if cronaca:
        sezioni["Cronaca"] = cronaca
        titoli.append("la cronaca della settimana")

    scoperte = OrderedDict()
    for e in per_tipo.get("ScopreLuogo", []):
        luogo = e.get("dettagli", "") or e.get("territorio", "")
        if luogo and luogo not in scoperte and nome_vero(e.get("chi")):
            scoperte[luogo] = e["chi"]
    if scoperte:
        sezioni["Scoperte"] = ["%s ha trovato la via per %s." % (chi, luogo) for luogo, chi in list(scoperte.items())[:6]]
        titoli.append("chi ha scoperto nuovi luoghi")

    fatti = [e for e in eventi if e.get("tipo") in NOTIZIE_FATTI and e.get("dettagli")]
    if fatti:
        visti = []
        for e in fatti:
            d = e["dettagli"].rstrip(".")
            if d not in visti:
                visti.append(d)
        sezioni["Fatti della valle"] = [d[0].upper() + d[1:] + "." for d in visti[-6:]]
        titoli.insert(0, "i fatti della valle")

    sezioni["Si dice alla locanda"] = [VOCI[(numero - 1) % len(VOCI)] + " (voce non confermata)"]

    titolo = "Gazzetta del passo n. %d" % numero
    if titoli:
        riassunto = "Esce la Gazzetta del passo n. %d, con %s. Si legge alla locanda e nelle Cronache." % (
            numero, " e ".join(titoli[:2]))
    else:
        riassunto = "Esce la Gazzetta del passo n. %d: una settimana tranquilla nella valle." % numero
    corpo = ["Settimana dal %s al %s." % (data_italiana(dal), data_italiana(oggi)), ""]
    for nome, righe in sezioni.items():
        corpo.append(nome.upper())
        corpo.extend(righe)
        corpo.append("")
    return titolo, riassunto[:LETTERE_BACHECA], "\n".join(corpo).strip()


def prossimo_numero():
    numeri = [int(m.group(1)) for f in glob.glob(os.path.join(APPROVATE, "Gazzetta_*.json"))
              for m in [re.search(r"Gazzetta_(\d+)", f)] if m]
    return (max(numeri) + 1) if numeri else 1


def componi(giorni=7):
    os.makedirs(APPROVATE, exist_ok=True)
    numero = prossimo_numero()
    eventi = leggi_eventi(giorni)
    titolo, riassunto, testo = componi_testo(eventi, numero, giorni)
    with open(BOZZA, "w", encoding="utf-8") as f:
        f.write("# La bozza della Gazzetta del passo n. %d (%d eventi letti dagli ultimi %d giorni).\n" % (numero, len(eventi), giorni))
        f.write("# Correggila pure con il Blocco note: le righe che cominciano con # non si pubblicano.\n")
        f.write("# TITOLO: una riga. RIASSUNTO: quello che va sulla bacheca della piazza (al massimo %d lettere).\n" % LETTERE_BACHECA)
        f.write("# TESTO: tutto il resto (va nelle notizie del launcher e del sito). Poi lancia \"Gazzetta 2 - approva\".\n")
        f.write("TITOLO: %s\nRIASSUNTO: %s\nTESTO:\n%s\n" % (titolo, riassunto, testo))
    print("Bozza della Gazzetta n. %d scritta (%d eventi): %s" % (numero, len(eventi), BOZZA))


def leggi_bozza():
    with open(BOZZA, encoding="utf-8") as f:
        righe = [r.rstrip("\n") for r in f if not r.startswith("#")]
    titolo, riassunto, testo, dentro = "", "", [], False
    for r in righe:
        if dentro:
            testo.append(r)
        elif r.startswith("TITOLO:"):
            titolo = r[len("TITOLO:"):].strip()
        elif r.startswith("RIASSUNTO:"):
            riassunto = r[len("RIASSUNTO:"):].strip()
        elif r.startswith("TESTO:"):
            dentro = True
    return titolo, riassunto, "\n".join(testo).strip()


def scrivi_foglio(nome, tipo, etichetta, titolo, riassunto, testo, autore):
    os.makedirs(APPROVATE, exist_ok=True)
    foglio = {"gioco": "Valdorso", "tipo": tipo, "data": datetime.date.today().isoformat(), "etichetta": etichetta,
              "titolo": titolo, "riassunto": riassunto[:LETTERE_BACHECA], "testo": testo, "ente": ENTE, "autore": autore}
    percorso = os.path.join(APPROVATE, nome)
    with open(percorso, "w", encoding="utf-8") as f:
        json.dump(foglio, f, ensure_ascii=False, indent=1)
    return percorso


def approva():
    if not os.path.exists(BOZZA):
        print("Non c'è una bozza: prima \"Gazzetta 1 - componi la bozza\".")
        return 1
    titolo, riassunto, testo = leggi_bozza()
    if not titolo or not riassunto:
        print("Nella bozza mancano TITOLO o RIASSUNTO: correggila e riprova.")
        return 1
    if len(riassunto) > LETTERE_BACHECA:
        print("Il RIASSUNTO è lungo %d lettere: al massimo %d (va sulla bacheca). Accorcialo e riprova." % (len(riassunto), LETTERE_BACHECA))
        return 1
    numero = prossimo_numero()
    percorso = scrivi_foglio("Gazzetta_%03d.json" % numero, "gazzetta", "Gazzetta", titolo, riassunto, testo, "staff")
    archivio = os.path.join(CARTELLA, "Bozze")
    os.makedirs(archivio, exist_ok=True)
    os.replace(BOZZA, os.path.join(archivio, "Bozza_%03d_approvata.txt" % numero))
    print("Gazzetta n. %d approvata: %s" % (numero, percorso))
    print("Nel gioco la proclama il Banditore entro 30 secondi (se il server è acceso, altrimenti al prossimo avvio).")
    print("Sul sito e nel launcher: Tools\\Pubblica Valdorso.cmd, scelta 2, poi FileZilla.")
    return 0


def annuncio(testo):
    testo = testo.strip()
    if not testo:
        print("Scrivi il testo dell'annuncio.")
        return 1
    nome = "Annuncio_%s.json" % datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    percorso = scrivi_foglio(nome, "annuncio", "Annuncio", "Un annuncio per la valle", testo[:LETTERE_BACHECA], testo, "staff")
    print("Annuncio scritto: %s" % percorso)
    return 0


if __name__ == "__main__":
    comando = sys.argv[1] if len(sys.argv) > 1 else ""
    if comando == "componi":
        componi(int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 7)
    elif comando == "approva":
        sys.exit(approva())
    elif comando == "annuncio":
        sys.exit(annuncio(" ".join(sys.argv[2:])))
    else:
        print("uso: gazzetta.py componi [giorni] | approva | annuncio \"testo\" (vedi le righe in cima a questo file)")
        sys.exit(1)
