# Valdorso - Il Sognatore: il primo pezzo del Battito "Cardinal 2.0" (13/11/2026, scritto da Claude; idee di Fra:
# "l'indipendenza nello scrivere quest nuove e che non siano per tutti, rendere ognuno speciale a modo suo";
# "dividerle per livello e rarita', come in Overgeared: si innesca, esce al centro dello schermo, accetta o rifiuta,
# e anche rifiutare ha conseguenze; serie, trame"; "folklore, fantasmi del passato, zone dove si rivivono le cronache").
#
# Per ora lavora FUORI dal gioco, come la Gazzetta: scrive le storie, lo staff le legge, le approva o le scarta.
# Quando le storie approvate entreranno nel gioco (modulo "PNG veri e il custode"), il Battito le dara' ai coloni con la
# finestra al centro dello schermo (i dati sono gia' pronti: il campo "finestra" di ogni storia approvata).
#
#   python sognatore.py vede [nome]          cosa vede il Sognatore di ogni colono (o di uno): Saved/Server/Sognatore/Profili
#   python sognatore.py sogna [quante]       per ogni colono passato dalla valle negli ultimi 30 giorni scrive fino a [3]
#                                            storie sue in Saved/Server/Sognatore/Bozza.txt (lo staff la legge)
#   python sognatore.py approva              le storie con "si" vanno in Approvate/<colono>/, le altre in Scartate
#   python sognatore.py esito NOME ID COSA   com'e' andata nel gioco (finchè il gioco non lo scrive da solo):
#                                            COSA = accettata | rifiutata | scaduta | fallita | finale1 | finale2 ...
#                                            (un finale con "prossimo" apre il capitolo dopo del filo)
#   python sognatore.py patente              i poteri del Battito: quanti si' di fila ha preso ognuno, chi puo' salire
#   python sognatore.py controlla            controlla tutti i semi (Bibbia, mattoni, rarita', fili) con coloni finti
#   python sognatore.py prova                un giro finto con tre coloni inventati (non tocca niente del server)
#
# Le sei parti (vedi il Piano, Design, "Il Battito come Cardinal 2.0"):
#   1 la Bibbia della valle (bibbia.json)   2 i mattoni, le rarita', gli inneschi, gli effetti e le zone (mattoni.json)
#   3 il Sognatore (qui: i semi in semi*.json e, se acceso, l'intelligenza artificiale)   4 il Giudice (giudica())
#   5 il Battito sceglie chi, quando e quanto spesso (sogna())   6 la memoria e la patente (Memoria.json,
#   Statistiche.json, poteri.json): impara quali storie piacciono allo staff e ai coloni, e guadagna liberta'.
#
# Usa solo la libreria standard (va bene il Python di Unreal). L'intelligenza artificiale e' SPENTA finche' in
# impostazioni.json non si mette "ia": true; la chiave la mette lo staff (variabile ANTHROPIC_API_KEY o il file
# Saved/Server/Sognatore/chiave_api.txt): il Sognatore non la chiede e non la scrive mai da nessuna parte.

import datetime
import glob
import hashlib
import json
import os
import random
import re
import sys
import urllib.request
from collections import Counter

QUI = os.path.dirname(os.path.abspath(__file__))
PROGETTO = os.path.normpath(os.path.join(QUI, "..", ".."))
SAVED = os.path.join(PROGETTO, "Saved", "Server")


def cartelle(radice):
    c = {"eventi": os.path.join(radice, "Eventi"), "personaggi": os.path.join(radice, "Archivio", "Personaggi"),
         "sognatore": os.path.join(radice, "Sognatore")}
    c.update({k: os.path.join(c["sognatore"], v) for k, v in
              {"profili": "Profili", "bozze": "Bozze", "approvate": "Approvate", "scartate": "Scartate"}.items()})
    c["bozza"] = os.path.join(c["sognatore"], "Bozza.txt")
    c["memoria"] = os.path.join(c["sognatore"], "Memoria.json")
    c["statistiche"] = os.path.join(c["sognatore"], "Statistiche.json")
    c["chiave"] = os.path.join(c["sognatore"], "chiave_api.txt")
    return c


C = cartelle(SAVED)


def carica(nome):
    with open(os.path.join(QUI, nome), encoding="utf-8") as f:
        return json.load(f)


BIBBIA = carica("bibbia.json")
MATTONI = carica("mattoni.json")
POTERI = carica("poteri.json")
IMPOSTAZIONI = carica("impostazioni.json") if os.path.exists(os.path.join(QUI, "impostazioni.json")) else {}
RARITA = {k: v for k, v in MATTONI["rarita"].items() if not k.startswith("_")}
ZONE = {k: v for k, v in MATTONI["zone"].items() if not k.startswith("_")}
EFFETTI = {k for k in MATTONI["effetti"] if not k.startswith("_")}
INNESCHI = {k for k in MATTONI["inneschi"] if not k.startswith("_")}


def carica_semi():
    """Tutti i file semi*.json: semi.json (personali), semi_folklore.json, semi_serie.json, e quelli che verranno."""
    semi, voci = [], []
    for f in sorted(glob.glob(os.path.join(QUI, "semi*.json"))):
        with open(f, encoding="utf-8") as testo:
            dati = json.load(testo)
        for s in dati.get("semi", []):
            s["_file"] = os.path.basename(f)
            semi.append(s)
        voci += dati.get("voci", [])
    return semi, voci


SEMI, VOCI = carica_semi()
SEMI_PER_ID = {s["id"]: s for s in SEMI}

PELLI = {"Lupo": "Pelle_Lupo", "Cinghiale": "Pelle_Cinghiale", "Cervo": "Pelle_Cervo", "Lepre": "Pelle_Lepre", "Volpe": "Pelle_Volpe"}
PREMI_IN_PIU = {"Coltello", "Medaglietta", "Quaderno", "Perla", "Chiodi", "Pane_Nero", "Candeliere", "Corda"}
STELLE = {1: "*", 2: "**", 3: "***", 4: "****", 5: "*****"}


def nomi_oggetti():
    """Id -> nome per i giocatori, letto dal codice del gioco (ValdorsoOggetti.cpp): resta sempre allineato."""
    nomi = {}
    f = os.path.join(PROGETTO, "Source", "Valdorso", "ValdorsoOggetti.cpp")
    if os.path.exists(f):
        with open(f, encoding="utf-8", errors="replace") as testo:
            for idx, nome in re.findall(r'TEXT\("([A-Z][A-Za-z_]+)"\),\s*TEXT\("([^"]+)"\)', testo.read()):
                nomi.setdefault(idx, nome)
    return nomi


OGGETTI = nomi_oggetti()


def nome_oggetto(idx):
    return OGGETTI.get(idx, idx.replace("_", " ").lower())


def adesso():
    return datetime.datetime.now()


def oggi():
    return adesso().date()


# ---------------------------------------------------------------------------------------------------------------------
# Cosa vede: i personaggi (le risposte del Registro) e il registro eventi
# ---------------------------------------------------------------------------------------------------------------------

def campo(oggetto, *nomi):
    """Un campo del file del personaggio, senza badare a maiuscole (il gioco li salva come camelCase)."""
    if not isinstance(oggetto, dict):
        return None
    basso = {k.lower(): v for k, v in oggetto.items()}
    for n in nomi:
        if n.lower() in basso:
            return basso[n.lower()]
    return None


def leggi_personaggi():
    coloni = {}
    for f in sorted(glob.glob(os.path.join(C["personaggi"], "*.json"))):
        try:
            with open(f, encoding="utf-8-sig") as testo:
                p = json.load(testo)
        except (ValueError, OSError):
            continue
        nome = campo(p, "nome")
        if not nome:
            continue
        reg = campo(p, "registro") or {}
        coloni[nome] = {
            "nome": nome,
            "registro": {k: (campo(reg, k) or "") for k in
                         ("sesso", "fede", "origine", "mestiere", "motivo", "ricordo", "paura", "onesta", "coraggio",
                          "devozione", "animo", "richiamoMagia")},
            "firmato": bool(campo(reg, "bFirmato", "firmato")),
            "tempo": int(campo(p, "tempoDiGioco") or 0),
            "ultimo": int(campo(p, "ultimoGioco") or 0),
        }
    return coloni


def leggi_eventi(giorni):
    oggi_utc = datetime.datetime.now(datetime.timezone.utc).date()
    eventi = []
    for k in range(giorni + 1):
        f = os.path.join(C["eventi"], "Eventi_%s.jsonl" % (oggi_utc - datetime.timedelta(days=k)).isoformat())
        if not os.path.exists(f):
            continue
        with open(f, encoding="utf-8") as righe:
            for riga in righe:
                try:
                    e = json.loads(riga)
                except ValueError:
                    continue
                if "UEDPIE" in e.get("mappa", "") or "Prove" in e.get("mappa", ""):
                    continue
                eventi.append(e)
    return eventi


# I tratti che si vedono da quello che uno fa (il tipo dell'evento -> il tratto, quanto pesa)
TRATTI = {
    "esploratore": {"ScopreLuogo": 2, "Grotta": 2, "PortaSegreta": 3, "Passaggio": 2, "Scala": 1, "Rovista": 1, "Scheletro": 1},
    "combattente": {"Colpo": 0.3, "Morte": 0.5, "Marchio": 1},
    "cacciatore": {"Animale": 2, "Scuoia": 2, "Trappola": 1},
    "socievole": {"Chat": 0.3, "Siede": 1, "Gilda": 1, "Dialogo": 0.2},
    "mistico": {"GuardaPietra": 2, "Sogno": 2, "Cerimonia": 2, "Segno": 2},
    "curioso": {"Dialogo": 0.5, "Storia": 2, "Gatto": 1},
    "ombroso": {"PortaChiusa": 2, "TesoroAllarme": 3, "Allarme": 1, "Marchio": 2, "Forziere": 1},
}


def profilo(colono, eventi):
    """Quello che il Sognatore sa di un colono. NON legge cosa si scrive in chat: solo quante volte si parla."""
    miei = [e for e in eventi if e.get("chi") == colono["nome"]]
    conta = Counter(e.get("tipo", "") for e in miei)
    tratti = {}
    for tratto, pesi in TRATTI.items():
        valore = sum(conta.get(t, 0) * p for t, p in pesi.items())
        if valore > 0:
            tratti[tratto] = round(valore, 1)
    reg = colono["registro"]
    # il carattere scelto nel Registro conta come un tratto (devoto, scettico, coraggioso, prudente...)
    for chiave in ("onesta", "coraggio", "devozione", "animo"):
        if reg.get(chiave):
            tratti[reg[chiave]] = tratti.get(reg[chiave], 0) + 3
    if reg.get("mestiere") == "cacciatore":
        tratti["cacciatore"] = tratti.get("cacciatore", 0) + 3
    if reg.get("mestiere") == "soldato":
        tratti["combattente"] = tratti.get("combattente", 0) + 2
    if reg.get("richiamoMagia") == "si":
        tratti["mistico"] = tratti.get("mistico", 0) + 2
    legami = Counter()
    risposte = []
    for e in miei:
        if e.get("tipo") != "Dialogo":
            continue
        d = e.get("dettagli", "")
        m = re.match(r"parla con (.+?) \(", d)
        if m:
            legami[m.group(1)] += 1
        m = re.match(r"(.+?): \S+ -> \S+ \((.*)\)$", d)
        if m and m.group(2):
            risposte.append("%s: \"%s\"" % (m.group(1), m.group(2)))
    luoghi = sorted({e.get("dettagli", "") for e in miei if e.get("tipo") == "ScopreLuogo" and e.get("dettagli")})
    storie = [e.get("dettagli", "") for e in miei if e.get("tipo") in ("Storia", "Sogno")]
    return {"tratti": dict(sorted(tratti.items(), key=lambda kv: -kv[1])), "legami": dict(legami.most_common(8)),
            "risposte": risposte[-10:], "luoghi": luoghi, "storie": storie[-10:], "eventi": sum(conta.values()),
            "tipi": set(conta), "chat": conta.get("Chat", 0), "morti": conta.get("Morte", 0)}


def descrivi_registro(reg):
    r = BIBBIA["registro"]
    righe = []
    venuto = "venuta" if reg.get("sesso") == "donna" else "venuto"
    for chiave, etichetta in (("origine", "Viene da"), ("mestiere", "Mestiere"), ("motivo", "Perche' e' " + venuto),
                              ("ricordo", "Cosa porta"), ("paura", "La sua paura"), ("fede", "Fede")):
        v = reg.get(chiave, "")
        if not v:
            continue
        testo = BIBBIA["fedi"].get(v, v) if chiave == "fede" else r.get(chiave, {}).get(v, v)
        righe.append("%s: %s" % (etichetta, testo))
    carattere = [reg.get(k) for k in ("onesta", "coraggio", "devozione", "animo") if reg.get(k)]
    if carattere:
        righe.append("Carattere: " + ", ".join(carattere))
    if reg.get("richiamoMagia"):
        righe.append("Ha sentito la magia rispondere: " + reg["richiamoMagia"])
    return righe


def scheda(colono, prof, mie=()):
    righe = ["COLONO: %s" % colono["nome"], ""]
    righe += ["Dal Registro di Val d'Orso:"] + ["  " + r for r in descrivi_registro(colono["registro"]) or ["(registro non ancora firmato)"]]
    ore = colono["tempo"] // 3600
    righe += ["", "Nella valle: %s, %d cose fatte negli ultimi giorni (parla in chat %s; i testi della chat il Sognatore NON li legge)."
              % ("un'ora" if ore == 1 else "%d ore" % ore, prof["eventi"], "una volta" if prof["chat"] == 1 else "%d volte" % prof["chat"])]
    righe += ["Tratti (da quello che fa e dal carattere): " + (", ".join("%s %s" % kv for kv in prof["tratti"].items()) or "nessuno ancora")]
    righe += ["Legami (con chi parla di piu'): " + (", ".join("%s (%d)" % kv for kv in prof["legami"].items()) or "nessuno ancora")]
    if prof["luoghi"]:
        righe += ["Luoghi scoperti: " + ", ".join(prof["luoghi"])]
    if prof["risposte"]:
        righe += ["Ultime risposte nei dialoghi:"] + ["  " + r for r in prof["risposte"]]
    if prof["storie"]:
        righe += ["Storie e sogni vissuti:"] + ["  " + s for s in prof["storie"]]
    if mie:
        righe += ["Le storie del Battito per lui:"] + ["  %s [%s] %s - %s" % (m.get("data", ""), m.get("rarita", ""), m.get("titolo", ""),
                                                                          m.get("stato", "")) for m in mie]
        fiducia = fiducia_di(mie)
        if fiducia:
            righe += ["Fiducia dei PNG (dalle storie): " + ", ".join("%s %+d" % kv for kv in fiducia.items())]
    return "\n".join(righe)


# ---------------------------------------------------------------------------------------------------------------------
# La memoria e le statistiche
# ---------------------------------------------------------------------------------------------------------------------

def leggi_json(percorso, vuoto):
    if os.path.exists(percorso):
        with open(percorso, encoding="utf-8") as f:
            return json.load(f)
    return vuoto


def scrivi_json(percorso, dati):
    os.makedirs(os.path.dirname(percorso), exist_ok=True)
    temporaneo = percorso + ".nuovo"
    with open(temporaneo, "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)
    os.replace(temporaneo, percorso)


def leggi_memoria():
    """{colono: [ {id, seme, titolo, rarita, data, stato, esito?, finale?} ], "_leggende": {seme: colono},
        "_patente": {potere: si_di_fila}}"""
    return leggi_json(C["memoria"], {})


def coloni_in_memoria(memoria):
    return {k: v for k, v in memoria.items() if not k.startswith("_")}


def fiducia_di(mie):
    """Quanto ogni PNG si fida di quel colono, dagli effetti delle storie finite o rifiutate."""
    tot = Counter()
    for m in mie:
        for e in m.get("effetti_avvenuti", []):
            if e.get("tipo") == "fiducia" and e.get("png"):
                tot[e["png"]] += int(e.get("quanto", 0))
    return {k: max(-5, min(5, v)) for k, v in tot.items() if v}


def leggi_statistiche():
    return leggi_json(C["statistiche"], {})


def conta_statistica(stat, seme, voce, quanto=1):
    if not seme:
        return
    riga = stat.setdefault(seme, {})
    riga[voce] = riga.get(voce, 0) + quanto


def gradimento(stat, seme):
    """Quanto piace un seme (allo staff e ai coloni): da 0.3 a 2. Cosi' il Battito impara cosa proporre."""
    r = stat.get(seme, {})
    buono = 1 + r.get("approvate", 0) + 2 * r.get("finite", 0) + r.get("accettate", 0)
    cattivo = 1 + 2 * r.get("scartate", 0) + r.get("rifiutate", 0) + r.get("scadute", 0)
    return max(0.3, min(2.0, buono / cattivo))


# ---------------------------------------------------------------------------------------------------------------------
# Il Sognatore: sceglie e cuce i semi (e, se acceso, chiede storie nuove all'intelligenza artificiale)
# ---------------------------------------------------------------------------------------------------------------------

def adatto(seme, reg):
    for chiave, valori in seme.get("per", {}).items():
        if reg.get(chiave) not in valori:
            return False
    for chiave, valori in seme.get("non_per", {}).items():
        if reg.get(chiave) in valori:
            return False
    return True


def filo_aperto(seme, mie):
    """Un capitolo dopo il primo si da' solo a chi ha finito il capitolo prima con un finale che porta qui."""
    serie = seme.get("serie")
    if not serie or serie.get("capitolo", 1) <= 1:
        return True
    for m in mie:
        if m.get("prossimo") == seme["id"]:
            return True
    return False


def serve_storia_finita(seme, mie):
    """Se tutti gli inneschi sono 'storia', serve averne finita almeno una."""
    inneschi = seme.get("innesco", [])
    if not inneschi or any(i.get("tipo") != "storia" for i in inneschi):
        return True
    finite = {m.get("seme") for m in mie if str(m.get("esito", "")).startswith("finale")}
    return any(i.get("storia") in finite for i in inneschi)


def innesco_vicino(seme, prof, reg):
    """Vero se nella vita del colono c'e' gia' quello che fa nascere la storia (allora e' la storia giusta adesso)."""
    for i in seme.get("innesco", []):
        t = i.get("tipo")
        if t == "dialogo" and i.get("png") in prof["legami"]:
            return True
        if t == "luogo" and i.get("luogo") in prof["luoghi"]:
            return True
        if t == "evento" and i.get("evento") in prof["tipi"]:
            return True
        if t == "registro" and reg.get(i.get("campo")) in i.get("valori", []):
            return True
        if t == "sogno" and "Sogno" in prof["tipi"]:
            return True
    return False


def punteggio(seme, prof, reg, stat, caso):
    p = 5.0 if seme.get("per") else 0.5           # le storie su di lui (dal Registro) vengono prima
    if seme.get("serie", {}).get("capitolo", 1) > 1:
        p += 6.0                                  # un filo aperto si continua
    for tratto, peso in seme.get("piace", {}).items():
        p += peso * min(prof["tratti"].get(tratto, 0), 10) / 3.0
    if innesco_vicino(seme, prof, reg):
        p += 2.0
    return p * gradimento(stat, seme["id"]) + caso.random() * 1.5


def genere(testo, donna):
    for maschio, femmina in (("{o}", "a" if donna else "o"), ("{lo}", "la" if donna else "lo"),
                             ("{gli}", "le" if donna else "gli"), ("{figlio}", "figlia" if donna else "figlio")):
        testo = testo.replace(maschio, femmina)
    return testo


def premio_testo(p, nascosto=False):
    parti = []
    if p.get("monete"):
        parti.append("? monete" if nascosto else "%d monete" % p["monete"])
    parti += ["???" if nascosto else nome_oggetto(o) for o in p.get("oggetti", [])]
    if p.get("titolo"):
        parti.append("un titolo" if nascosto else "il titolo \"%s\"" % p["titolo"])
    if p.get("iscrizione") and not nascosto:
        parti.append("nel taccuino: \"%s\"" % p["iscrizione"])
    if p.get("segno") and not nascosto:
        parti.append("segno %s" % p["segno"])
    return ", ".join(parti) or "niente"


def finestra(storia):
    """Quello che il gioco mostrera' al centro dello schermo quando la storia nasce (idea di Fra, come in Overgeared)."""
    r = RARITA.get(storia.get("rarita"), RARITA["comune"])
    nascosto = r["ordine"] >= 3
    premi = []
    for f in storia.get("finali", []):
        t = premio_testo(f.get("premio") or {}, nascosto)
        if t not in premi:
            premi.append(t)
    passi = storia.get("passi") or [{}]
    return {
        "intestazione": "Il Battito ha intrecciato una storia",
        "rarita": storia.get("rarita"), "colore": r["colore"], "stelle": storia.get("difficolta", 1),
        "titolo": storia.get("titolo"), "trama": storia.get("aggancio"), "primo_passo": passi[0].get("testo", ""),
        "premi": " / oppure / ".join(premi),
        "se_rifiuti": (storia.get("rifiuto") or {}).get("testo", ""),
        "scadenza_giorni": r["scadenza_giorni"],
        "bottoni": ["Accetta", "Rifiuta", "Ci penso"],
        "serie": ("%s, capitolo %d" % (storia["serie"]["id"], storia["serie"]["capitolo"])) if storia.get("serie") else "",
    }


def cuci(seme, colono, caso, altro=""):
    """Un seme diventa la storia di QUEL colono: sceglie PNG, luogo, animale, oggetto, e riempie i segnaposto."""
    reg = colono["registro"]
    donna = reg.get("sesso") == "donna"
    r = BIBBIA["registro"]
    animale = caso.choice(MATTONI["animali"])
    valori = {
        "colono": colono["nome"],
        "png": caso.choice(seme["png"]) if seme.get("png") else "",
        "luogo": caso.choice(seme["luogo"]) if seme.get("luogo") else "",
        "animale": animale.lower(),
        "pelle": PELLI[animale],
        "oggetto": caso.choice(["Funghi", "Cozze", "Pelle_Volpe", "Erbe_Palude", "Formaggio"]),
        "ricordo": r["ricordo"].get(reg.get("ricordo"), "il tuo ricordo"),
        "paura": r["paura"].get(reg.get("paura"), "la tua paura"),
        "origine": r["origine"].get(reg.get("origine"), "lontano"),
        "mestiere": r["mestiere"].get(reg.get("mestiere"), "colono"),
        "motivo": r["motivo"].get(reg.get("motivo"), "la valle"),
        "fede": BIBBIA["fedi"].get(reg.get("fede"), "La fede").split(":")[0],
        "voce": caso.choice(VOCI or ["il frammento abbia saltato un battito"]),
        "altro": altro,
        "eroe": seme.get("eroe", ""),
    }

    def riempi(testo, campo_oggetto=False):
        for k, v in valori.items():
            if campo_oggetto:
                testo = testo.replace("{%s}" % k, v)
            else:
                testo = testo.replace("{%s}" % k, nome_oggetto(v) if k in ("oggetto", "pelle") else v)
        return genere(testo, donna)

    def riempi_tutto(x):
        if isinstance(x, str):
            return riempi(x)
        if isinstance(x, list):
            return [riempi_tutto(v) for v in x]
        if isinstance(x, dict):
            return {k: riempi_tutto(v) for k, v in x.items()}
        return x

    storia = {
        "id": "%s_%s" % (seme["id"], hashlib.sha1(("%s|%s" % (colono["nome"], seme["id"])).encode("utf-8")).hexdigest()[:6]),
        "seme": seme["id"], "colono": colono["nome"], "titolo": riempi(seme["titolo"]),
        "rarita": seme.get("rarita", "comune"), "difficolta": seme.get("difficolta", 1),
        "patente": RARITA.get(seme.get("rarita", "comune"), RARITA["comune"])["patente"],
        "png": valori["png"], "luogo": valori["luogo"], "zona": seme.get("zona", ""),
        "innesco": riempi_tutto(seme.get("innesco", [])), "aggancio": riempi(seme["aggancio"]),
        "passi": [], "finali": [], "rifiuto": riempi_tutto(seme.get("rifiuto", {})),
        "serie": seme.get("serie"), "scritta_da": "seme", "scritta_il": oggi().isoformat(),
    }
    for chiave in ("per_tutti", "intreccio", "eroe", "ritorno", "annuncio"):
        if seme.get(chiave):
            storia[chiave] = seme[chiave]
    for passo in seme["passi"]:
        nuovo = {"mattone": passo["mattone"], "testo": riempi(passo["testo"])}
        for k in ("png", "luogo"):
            if k in passo:
                nuovo[k] = riempi(passo[k])
        if "oggetto" in passo:
            nuovo["oggetto"] = riempi(passo["oggetto"], campo_oggetto=True)
        if "animale" in passo:
            nuovo["animale"] = animale
        storia["passi"].append(nuovo)
    for f in seme["finali"]:
        nuovo = {"scelta": riempi(f["scelta"]), "esito": riempi(f["esito"]), "premio": riempi_tutto(f.get("premio", {}))}
        if f.get("prossimo"):
            nuovo["prossimo"] = f["prossimo"]
        if f.get("effetti"):
            nuovo["effetti"] = riempi_tutto(f["effetti"])
        if f.get("soglia"):
            nuovo["soglia"] = f["soglia"]
        storia["finali"].append(nuovo)
    storia["finestra"] = finestra(storia)
    return storia


def chiave_api():
    chiave = os.environ.get("ANTHROPIC_API_KEY", "").strip()
    if not chiave and os.path.exists(C["chiave"]):
        with open(C["chiave"], encoding="utf-8") as f:
            chiave = f.read().strip()
    return chiave


def sogna_con_ia(colono, prof, mie, esempi):
    """Una storia NUOVA, scritta dall'intelligenza artificiale con la Bibbia, i mattoni e la scheda del colono.
    Restituisce la storia (da far giudicare) o None."""
    chiave = chiave_api()
    if not chiave:
        print("   (intelligenza artificiale accesa ma senza chiave: salto le storie nuove)")
        return None
    mattoni_ok = {k: v["cosa"] for k, v in MATTONI["passi"].items() if v["oggi"]}
    mattoni_dopo = {k: v["cosa"] for k, v in MATTONI["passi"].items() if not v["oggi"]}
    budget = {k: {"monete_max": v["monete_max"], "oggetti_max": v["oggetti_max"], "titolo": v["titolo"]} for k, v in RARITA.items()}
    istruzioni = (
        "Sei il Sognatore di Valdorso, un gioco di ruolo online fantasy: il Battito, l'anima del mondo, che scrive storie "
        "per i coloni. Scrivi UNA storia (una quest) in italiano per il colono descritto: deve parlare di lui (le risposte "
        "del Registro, quello che ha fatto, i PNG che conosce) oppure venire dal folklore delle valli di montagna o dalla "
        "lore, ma sempre dentro la Bibbia della valle: tono, regole, segreti (i segreti si possono solo far intuire). "
        "Usa SOLO i personaggi e i luoghi della Bibbia. Mattoni che il gioco sa gia' fare: %s. Mattoni che arriveranno "
        "(usali solo se servono davvero): %s. Rarita' e premi massimi: %s; oggetti comuni: %s; oggetti rari (solo da rara in "
        "su): %s. Scegli tu rarita' (comune, insolita o rara; epica solo se la storia e' davvero grande) e difficolta' (1-5). "
        "Da 2 a 5 passi, 2 o 3 finali (nessuno sbagliato), un innesco credibile nel mondo, e cosa succede se il colono "
        "rifiuta (effetti tra: %s). Non ripetere storie gia' date: %s. Rispondi SOLO con un oggetto JSON con i campi: "
        "titolo, rarita, difficolta, png, luogo, innesco [{tipo, ...}], aggancio, passi [{mattone, png?, luogo?, oggetto?, "
        "animale?, testo}], finali [{scelta, esito, premio {monete?, oggetti?, titolo?, segno, iscrizione?}}], rifiuto "
        "{testo, effetti [{tipo, png?, quanto?, testo?}]}. Il segno e' una parola sola in CamelCase. I dialoghi tra « »."
        % (json.dumps(mattoni_ok, ensure_ascii=False), json.dumps(mattoni_dopo, ensure_ascii=False),
           json.dumps(budget, ensure_ascii=False), ", ".join(MATTONI["oggetti_comuni"]), ", ".join(MATTONI["oggetti_rari"]),
           ", ".join(sorted(EFFETTI)), "; ".join(m.get("titolo", "") for m in mie) or "nessuna"))
    contenuto = ("BIBBIA DELLA VALLE:\n%s\n\nSCHEDA DEL COLONO:\n%s\n\nDUE ESEMPI DI STORIE BEN FATTE:\n%s"
                 % (json.dumps(BIBBIA, ensure_ascii=False), scheda(colono, prof, mie), json.dumps(esempi, ensure_ascii=False)))
    corpo = json.dumps({"model": IMPOSTAZIONI.get("modello", "claude-sonnet-4-5"), "max_tokens": 3000,
                        "system": istruzioni, "messages": [{"role": "user", "content": contenuto}]}).encode("utf-8")
    richiesta = urllib.request.Request("https://api.anthropic.com/v1/messages", data=corpo, method="POST", headers={
        "content-type": "application/json", "x-api-key": chiave, "anthropic-version": "2023-06-01"})
    try:
        with urllib.request.urlopen(richiesta, timeout=180) as risposta:
            dati = json.loads(risposta.read().decode("utf-8"))
    except Exception as errore:            # la rete o il servizio: si continua con i semi
        print("   (intelligenza artificiale non raggiungibile: %s)" % str(errore)[:120])
        return None
    testo = "".join(b.get("text", "") for b in dati.get("content", []) if b.get("type") == "text")
    m = re.search(r"\{.*\}", testo, re.S)
    try:
        storia = json.loads(m.group(0)) if m else None
    except ValueError:
        storia = None
    if not isinstance(storia, dict):
        return None
    if storia.get("rarita") not in RARITA:
        storia["rarita"] = "comune"
    storia.update({"id": "ia_%s" % hashlib.sha1((colono["nome"] + testo).encode("utf-8")).hexdigest()[:8], "seme": "",
                   "colono": colono["nome"], "patente": RARITA[storia["rarita"]]["patente"],
                   "scritta_da": "intelligenza artificiale", "scritta_il": oggi().isoformat()})
    storia["finestra"] = finestra(storia)
    return storia


# ---------------------------------------------------------------------------------------------------------------------
# Il Giudice: nessuna storia arriva allo staff (e poi al gioco) senza passare da qui
# ---------------------------------------------------------------------------------------------------------------------

def giudica(storia, gia_titoli=()):
    """Restituisce (stato, motivi): "ok", "in attesa" (serve un mattone o una zona che arrivera') o "scartata"."""
    no, attesa = [], []
    personaggi, luoghi = BIBBIA["personaggi"], BIBBIA["luoghi"]
    nel_gioco = MATTONI["png_nel_gioco"]
    for campo_testo in ("titolo", "aggancio"):
        if not str(storia.get(campo_testo, "")).strip():
            no.append("manca %s" % campo_testo)
    rarita = storia.get("rarita", "comune")
    if rarita not in RARITA:
        no.append("rarita' sconosciuta (%s)" % rarita)
        rarita = "comune"
    budget = RARITA[rarita]
    if not 1 <= int(storia.get("difficolta", 1) or 0) <= 5:
        no.append("la difficolta' va da 1 a 5")
    passi, finali = storia.get("passi") or [], storia.get("finali") or []
    if not 1 <= len(passi) <= 6:
        no.append("i passi devono essere da 1 a 6 (sono %d)" % len(passi))
    if not 2 <= len(finali) <= 4:
        no.append("i finali devono essere da 2 a 4 (sono %d)" % len(finali))
    if storia.get("png") and storia["png"] not in personaggi:
        no.append("PNG sconosciuto alla Bibbia: %s" % storia["png"])
    if storia.get("luogo") and storia["luogo"] not in luoghi:
        no.append("luogo sconosciuto alla Bibbia: %s" % storia["luogo"])
    # l'innesco: sempre un perche' nel mondo
    inneschi = storia.get("innesco") or []
    if not inneschi:
        no.append("manca l'innesco (cosa fa nascere la storia)")
    for i in inneschi:
        t = i.get("tipo")
        if t not in INNESCHI:
            no.append("innesco sconosciuto (%s)" % t)
        elif t == "dialogo" and i.get("png") not in personaggi:
            no.append("innesco: PNG sconosciuto (%s)" % i.get("png"))
        elif t == "luogo" and i.get("luogo") not in luoghi:
            no.append("innesco: luogo sconosciuto (%s)" % i.get("luogo"))
        elif t == "storia" and SEMI_PER_ID and i.get("storia") not in SEMI_PER_ID:
            no.append("innesco: storia che non esiste (%s)" % i.get("storia"))
    # la zona
    zona = storia.get("zona")
    if zona:
        if zona not in ZONE:
            no.append("zona sconosciuta (%s)" % zona)
        else:
            attesa.append("usa la zona '%s' (arriva con le zone del Battito)" % zona)
    for n, p in enumerate(passi, 1):
        m = MATTONI["passi"].get(p.get("mattone"))
        if not m:
            no.append("passo %d: mattone sconosciuto (%s)" % (n, p.get("mattone")))
            continue
        if not m["oggi"]:
            attesa.append("passo %d usa '%s' (arriva piu' avanti)" % (n, p["mattone"]))
        if p["mattone"] in ("eco", "tasca") and not zona:
            no.append("passo %d: '%s' senza una zona" % (n, p["mattone"]))
        if p.get("png"):
            if p["png"] not in personaggi:
                no.append("passo %d: PNG sconosciuto (%s)" % (n, p["png"]))
            elif p["mattone"] in ("parla", "porta", "scegli") and p["png"] not in nel_gioco:
                attesa.append("passo %d: %s non e' ancora un PNG con cui si parla" % (n, p["png"]))
        if p.get("luogo") and p["luogo"] not in luoghi:
            no.append("passo %d: luogo sconosciuto (%s)" % (n, p["luogo"]))
        if p.get("oggetto") and OGGETTI and p["oggetto"] not in OGGETTI:
            no.append("passo %d: oggetto sconosciuto (%s)" % (n, p["oggetto"]))
        if p.get("animale") and p["animale"] not in MATTONI["animali"]:
            no.append("passo %d: animale che non c'e' (%s)" % (n, p["animale"]))
    # i premi, secondo la rarita'
    ammessi = set(MATTONI["oggetti_comuni"]) | PREMI_IN_PIU | (set(MATTONI["oggetti_rari"]) if budget["rari"] else set())
    for n, f in enumerate(finali, 1):
        premio = f.get("premio") or {}
        if int(premio.get("monete", 0) or 0) > budget["monete_max"]:
            no.append("finale %d: troppe monete per una storia %s (%s, al massimo %d)" % (n, rarita, premio.get("monete"), budget["monete_max"]))
        oggetti = premio.get("oggetti") or []
        if len(oggetti) > budget["oggetti_max"]:
            no.append("finale %d: troppi oggetti per una storia %s" % (n, rarita))
        for o in oggetti:
            if o not in ammessi:
                no.append("finale %d: premio non ammesso per una storia %s (%s)" % (n, rarita, o))
        if premio.get("titolo") and not budget["titolo"]:
            no.append("finale %d: un titolo si da' solo con una storia epica o leggendaria" % n)
        if not str(f.get("scelta", "")).strip() or not str(f.get("esito", "")).strip():
            no.append("finale %d senza scelta o esito" % n)
        if f.get("prossimo") and SEMI_PER_ID and f["prossimo"] not in SEMI_PER_ID:
            no.append("finale %d: il capitolo dopo non esiste (%s)" % (n, f["prossimo"]))
    # il rifiuto: ha sempre un testo; gli effetti devono essere quelli conosciuti
    rifiuto = storia.get("rifiuto") or {}
    if not str(rifiuto.get("testo", "")).strip():
        no.append("manca cosa succede se il colono rifiuta")
    for e in rifiuto.get("effetti", []) + [e for f in finali for e in f.get("effetti", [])]:
        if e.get("tipo") not in EFFETTI:
            no.append("effetto sconosciuto (%s)" % e.get("tipo"))
        elif e["tipo"] == "fiducia":
            if e.get("png") not in personaggi:
                no.append("effetto fiducia: PNG sconosciuto (%s)" % e.get("png"))
            if not -2 <= int(e.get("quanto", 0)) <= 2:
                no.append("effetto fiducia: al massimo 2 in piu' o in meno")
        elif e["tipo"] in ("mondo", "voce") and not str(e.get("testo", "")).strip():
            no.append("effetto %s senza testo" % e["tipo"])
    if storia.get("serie") and not isinstance(storia["serie"].get("capitolo"), int):
        no.append("serie senza numero di capitolo")
    tutto = json.dumps({k: v for k, v in storia.items() if k not in ("finestra", "giudice", "motivi")}, ensure_ascii=False)
    resti = re.findall(r"\{[a-z_]+\}", tutto)
    if resti:
        no.append("segnaposto non riempiti: %s" % ", ".join(sorted(set(resti))))
    for parola in BIBBIA.get("parole_vietate", []):
        if re.search(r"\b%s\b" % re.escape(parola), tutto):
            no.append("dice un segreto della Bibbia (%s)" % parola)
    for testo in [storia.get("aggancio", "")] + [p.get("testo", "") for p in passi] + [f.get("esito", "") for f in finali]:
        if len(testo) > 420:
            no.append("un testo e' troppo lungo (%d lettere, al massimo 420)" % len(testo))
            break
    if storia.get("titolo") in gia_titoli:
        no.append("questo colono l'ha gia' avuta")
    if no:
        return "scartata", no
    if attesa:
        return "in attesa", attesa
    return "ok", []


# ---------------------------------------------------------------------------------------------------------------------
# I limiti: quante storie, quanto rare, quanto spesso (perche' una Rara resti rara)
# ---------------------------------------------------------------------------------------------------------------------

def giorni_fa(data):
    try:
        return (oggi() - datetime.date.fromisoformat(data)).days
    except (TypeError, ValueError):
        return 9999


def ordine(rarita):
    return RARITA.get(rarita, RARITA["comune"])["ordine"]


def rarita_permessa(rarita, mie, memoria, gia_stavolta, capitolo=1):
    """Rispetta 'ogni_giorni' (per colono) e 'valle_ogni_giorni' (per tutta la valle), contando anche le bozze di oggi.
    Il capitolo di un filo gia' cominciato non aspetta il turno della sua rarita' (la trama non si ferma), ma la valle si'."""
    r = RARITA[rarita]
    finite = sum(1 for m in mie if str(m.get("esito", "")).startswith("finale"))
    if finite < r.get("finite_min", 0):
        return False
    date = [m for m in mie if not m.get("stato", "").startswith("scartata")] + gia_stavolta.get("colono", [])
    if r["ogni_giorni"] and capitolo <= 1 and any(ordine(m.get("rarita")) >= r["ordine"] and giorni_fa(m.get("data")) < r["ogni_giorni"] for m in date):
        return False
    if r["valle_ogni_giorni"]:
        tutte = [m for v in coloni_in_memoria(memoria).values() for m in v if not m.get("stato", "").startswith("scartata")]
        tutte += gia_stavolta.get("valle", [])
        if any(ordine(m.get("rarita")) >= r["ordine"] and giorni_fa(m.get("data")) < r["valle_ogni_giorni"] for m in tutte):
            return False
    return True


# ---------------------------------------------------------------------------------------------------------------------
# I comandi
# ---------------------------------------------------------------------------------------------------------------------

def attivi(coloni, eventi, giorni):
    """Chi e' passato dalla valle negli ultimi giorni (dal registro eventi o dall'ultimo salvataggio)."""
    visti = {e.get("chi") for e in eventi}
    limite = datetime.datetime.now(datetime.timezone.utc).timestamp() - giorni * 86400

    def recente(t):
        # (l'ultimo gioco e' salvato come secondi o come "tick" di Unreal: si accettano tutti e due)
        if t > 10 ** 15:
            t = (t - 621355968000000000) / 10 ** 7
        return t >= limite
    return [c for n, c in coloni.items() if n in visti or recente(c["ultimo"])]


def vede(nome=None):
    coloni = leggi_personaggi()
    if not coloni:
        print("Non trovo personaggi in %s" % C["personaggi"])
        return 1
    eventi = leggi_eventi(30)
    memoria = leggi_memoria()
    os.makedirs(C["profili"], exist_ok=True)
    for c in coloni.values():
        if nome and c["nome"].lower() != nome.lower():
            continue
        testo = scheda(c, profilo(c, eventi), memoria.get(c["nome"], []))
        with open(os.path.join(C["profili"], re.sub(r"[^\w\- ]", "_", c["nome"]) + ".txt"), "w", encoding="utf-8") as f:
            f.write(testo + "\n")
        print(testo + "\n" + "-" * 60)
    print("Le schede sono in %s" % C["profili"])
    return 0


def ritorno_pronto(seme, mie):
    """Un seme di ritorno arriva solo a chi ha finito la storia 'di' con quel finale, e dopo 'dopo_giorni' giorni."""
    r = seme.get("ritorno")
    if not r:
        return True
    for m in mie:
        if m.get("seme") == r["di"] and m.get("esito") == "finale%d" % r["finale"] \
                and giorni_fa(m.get("esito_il")) >= r.get("dopo_giorni", 0):
            return True
    return False


def leggi_gesta():
    """I semi nati dalle imprese dei coloni (le gesta diventano Eco): Saved/Server/Sognatore/semi_gesta.json."""
    return leggi_json(os.path.join(C["sognatore"], "semi_gesta.json"), {"semi": []}).get("semi", [])


def custode_stato(memoria):
    """Il custode dei custodi: quanto il Battito ha dato e cambiato negli ultimi giorni. Restituisce (conti, superati)."""
    lim = POTERI.get("custode", {})
    giorni = lim.get("giorni", 7)
    conti = {"monete_valle": 0, "oggetti_rari_valle": 0, "titoli_valle": 0, "fatti_nel_mondo": 0, "monete_colono": {}}
    for nome, mie in memoria.items():
        if nome.startswith("_") and nome != "_valle":
            continue
        for m in mie:
            if not m.get("stato", "").startswith("approvata") or giorni_fa(m.get("data")) >= giorni:
                continue
            conti["monete_valle"] += m.get("monete_max", 0)
            if nome != "_valle":
                conti["monete_colono"][nome] = conti["monete_colono"].get(nome, 0) + m.get("monete_max", 0)
            conti["oggetti_rari_valle"] += m.get("rari_max", 0)
            conti["titoli_valle"] += 1 if m.get("titolo_premio") else 0
        for m in mie:
            if giorni_fa(m.get("esito_il")) < giorni:
                conti["fatti_nel_mondo"] += sum(1 for e in m.get("effetti_avvenuti", []) if e.get("tipo") == "mondo")
    superati = [k for k in ("monete_valle", "oggetti_rari_valle", "titoli_valle", "fatti_nel_mondo")
                if k in lim and conti[k] > lim[k]]
    troppi = sorted(n for n, v in conti["monete_colono"].items() if "monete_colono" in lim and v > lim["monete_colono"])
    fiducia_bassa = []
    for nome, mie in coloni_in_memoria(memoria).items():
        for png, v in fiducia_di(mie).items():
            if v <= lim.get("fiducia_bassa", -99):
                fiducia_bassa.append("%s con %s (%+d)" % (nome, png, v))
    return conti, superati, troppi, fiducia_bassa


def vicini(a, b, profili, memoria):
    """Quanto due coloni sono vicini: gli stessi PNG, gli stessi luoghi, gli stessi fili."""
    pa, pb = profili[a["nome"]], profili[b["nome"]]
    fa = {m.get("serie") for m in memoria.get(a["nome"], []) if m.get("serie")}
    fb = {m.get("serie") for m in memoria.get(b["nome"], []) if m.get("serie")}
    return len(set(pa["legami"]) & set(pb["legami"])) + len(set(pa["luoghi"]) & set(pb["luoghi"])) + 2 * len(fa & fb)


def sogna(quante=3, giorni=30, coloni=None, eventi=None):
    coloni = coloni if coloni is not None else leggi_personaggi()
    eventi = eventi if eventi is not None else leggi_eventi(giorni)
    scelti = [c for c in (attivi(coloni, eventi, giorni) or list(coloni.values())) if c.get("firmato", True)]
    if not scelti:
        print("Non trovo coloni (%s)." % C["personaggi"])
        return 1
    memoria = leggi_memoria()
    stat = leggi_statistiche()
    leggende = memoria.get("_leggende", {})
    ia = bool(IMPOSTAZIONI.get("ia"))
    nuove_ia = int(IMPOSTAZIONI.get("storie_nuove_per_colono", 1))
    future_max = int(IMPOSTAZIONI.get("future_per_colono", 1))
    tutti_i_semi = SEMI + leggi_gesta()
    # il custode dei custodi: se il Battito ha dato troppo, niente Rare o piu' finche' si rientra
    _, superati, troppi, _ = custode_stato(memoria)
    if superati:
        print("   Il custode frena il Battito (%s): per ora solo storie Comuni e Insolite." % ", ".join(superati))
    bozze, valle_stavolta, profili = [], [], {}
    for c in scelti:
        prof = profili[c["nome"]] = profilo(c, eventi)
        mie = memoria.get(c["nome"], [])
        gia_semi = {m.get("seme") for m in mie}
        gia_titoli = {m.get("titolo") for m in mie}
        caso = random.Random("%s|%s" % (c["nome"], oggi().isoformat()))
        fili_miei = {m.get("serie") for m in mie if m.get("serie")}
        candidati = [s for s in tutti_i_semi
                     if s["id"] not in gia_semi and s["id"] not in leggende
                     and not s.get("per_tutti") and not s.get("intreccio") and s.get("eroe") != c["nome"]
                     and not ((s.get("serie") or {}).get("capitolo", 1) <= 1 and (s.get("serie") or {}).get("id") in fili_miei)
                     and adatto(s, c["registro"]) and filo_aperto(s, mie) and serve_storia_finita(s, mie)
                     and ritorno_pronto(s, mie)]
        candidati.sort(key=lambda s: -(punteggio(s, prof, c["registro"], stat, caso) + (7 if s.get("ritorno") else 0)))
        prese = future = 0
        mie_stavolta = []
        for seme in candidati:
            if prese >= quante:
                break
            rarita = seme.get("rarita", "comune")
            if (superati or c["nome"] in troppi) and ordine(rarita) >= 3:
                continue
            if not rarita_permessa(rarita, mie, memoria, {"colono": mie_stavolta, "valle": valle_stavolta},
                                   2 if seme.get("ritorno") else (seme.get("serie") or {}).get("capitolo", 1)):
                continue
            storia = cuci(seme, c, caso)
            stato, motivi = giudica(storia, gia_titoli)
            if stato == "in attesa":
                if future >= future_max:
                    continue
                future += 1
            storia.update({"giudice": stato, "motivi": motivi})
            bozze.append(storia)
            conta_statistica(stat, seme["id"], "proposte")
            segno = {"rarita": rarita, "data": oggi().isoformat()}
            mie_stavolta.append(segno)
            valle_stavolta.append(segno)
            prese += 1
        if ia:
            esempi = [cuci(SEMI_PER_ID[i], c, caso) for i in ("il_cacciatore", "anguana_del_lago") if i in SEMI_PER_ID]
            for _ in range(nuove_ia):
                storia = sogna_con_ia(c, prof, mie, esempi)
                if storia:
                    try:
                        stato, motivi = giudica(storia, gia_titoli)
                    except Exception as errore:      # una risposta storta dell'intelligenza artificiale
                        stato, motivi = "scartata", ["storia storta: %s" % str(errore)[:80]]
                    storia.update({"giudice": stato, "motivi": motivi})
                    bozze.append(storia)
        if prese == 0:
            print("   %s: per oggi niente (semi adatti finiti o rarita' gia' date di recente)." % c["nome"])
    frenati = set(troppi) | ({c["nome"] for c in scelti} if superati else set())
    oggi_dati = {}
    for b in bozze:
        oggi_dati.setdefault(b["colono"], []).append({"rarita": b.get("rarita"), "data": oggi().isoformat()})
    bozze += sogna_intreccio(scelti, profili, memoria, stat, frenati, oggi_dati)
    bozze += sogna_valle(memoria, stat, "fatti_nel_mondo" in superati)
    scrivi_json(C["statistiche"], stat)
    scrivi_bozza(bozze)
    conta = Counter(b["giudice"] for b in bozze)
    rar = Counter(b.get("rarita") for b in bozze)
    print("Il Sognatore ha scritto %d storie per %d coloni (%d pronte, %d in attesa di mattoni o zone, %d scartate dal Giudice)."
          % (len(bozze), len(scelti), conta["ok"], conta["in attesa"], conta["scartata"]))
    print("Rarita': " + ", ".join("%s %d" % (k, rar[k]) for k in RARITA if rar[k]))
    print("Leggile in %s, poi \"Sognatore 2 - approva\"." % C["bozza"])
    return 0


def sogna_intreccio(scelti, profili, memoria, stat, frenati=(), oggi_dati=None):
    """Al massimo un intreccio per giro: i due coloni piu' vicini che non ne hanno avuto uno negli ultimi 7 giorni."""
    semi = [s for s in SEMI if s.get("intreccio")]
    if not semi or len(scelti) < 2:
        return []
    def libero(nome):
        return not any(m.get("con") and giorni_fa(m.get("data")) < 7 for m in memoria.get(nome, []))
    coppie = []
    for i, a in enumerate(scelti):
        for b in scelti[i + 1:]:
            v = vicini(a, b, profili, memoria)
            if v > 0 and libero(a["nome"]) and libero(b["nome"]):
                coppie.append((v, a, b))
    oggi_dati = oggi_dati or {}

    def permessa(seme, c):
        r = seme.get("rarita", "comune")
        if c["nome"] in frenati and ordine(r) >= 3:
            return False
        return adatto(seme, c["registro"]) and rarita_permessa(r, memoria.get(c["nome"], []), memoria,
                                                               {"colono": oggi_dati.get(c["nome"], []), "valle": []})
    if not coppie:
        return []
    coppie.sort(key=lambda x: (-x[0], x[1]["nome"], x[2]["nome"]))
    for _, a, b in coppie:                       # la coppia piu' vicina che ha ancora una storia adatta
        usati = {m.get("seme") for m in memoria.get(a["nome"], []) + memoria.get(b["nome"], [])}
        caso = random.Random("%s|%s|%s" % (a["nome"], b["nome"], oggi().isoformat()))
        adatti = [s for s in semi if s["id"] not in usati and permessa(s, a) and permessa(s, b)]
        if adatti:
            break
    else:
        return []
    adatti.sort(key=lambda s: -gradimento(stat, s["id"]) - caso.random())
    seme = adatti[0]
    legame = hashlib.sha1(("%s|%s|%s" % (a["nome"], b["nome"], seme["id"])).encode("utf-8")).hexdigest()[:6]
    fuori = []
    for io, altro in ((a, b), (b, a)):
        storia = cuci(seme, io, random.Random(legame), altro=altro["nome"])
        storia["con"] = altro["nome"]
        storia["intreccio_id"] = legame
        stato, motivi = giudica(storia, {m.get("titolo") for m in memoria.get(io["nome"], [])})
        storia.update({"giudice": stato, "motivi": motivi})
        fuori.append(storia)
    conta_statistica(stat, seme["id"], "proposte")
    return fuori


def sogna_valle(memoria, stat, frenato):
    """Una storia per tutta la valle, se non ce n'e' gia' una aperta e se il custode non frena i fatti nel mondo."""
    if frenato:
        return []
    mie = memoria.get("_valle", [])
    if any(m.get("stato", "").startswith("approvata") and not m.get("esito") for m in mie):
        return []                                      # una alla volta
    if any(giorni_fa(m.get("data")) < 7 for m in mie):
        return []
    usati = {m.get("seme") for m in mie if giorni_fa(m.get("data")) < 60}
    semi = [s for s in SEMI if s.get("per_tutti") and s["id"] not in usati]
    if not semi:
        return []
    caso = random.Random("valle|%s" % oggi().isoformat())
    semi.sort(key=lambda s: -gradimento(stat, s["id"]) - caso.random())
    storia = cuci(semi[0], {"nome": "_valle", "registro": {}}, caso)
    storia["id"] = "%s_%s" % (storia["id"], oggi().strftime("%Y%m%d"))       # la stessa storia della valle puo' tornare, mesi dopo
    stato, motivi = giudica(storia, {m.get("titolo") for m in mie if giorni_fa(m.get("data")) < 60})
    storia.update({"giudice": stato, "motivi": motivi})
    conta_statistica(stat, semi[0]["id"], "proposte")
    return [storia]


def verdetto_iniziale(s):
    if s["giudice"] == "scartata":
        return "no"
    # i poteri di livello 3 (epiche e leggendarie) aspettano sempre il si' dello staff
    potere = potere_di(s)
    return "si" if POTERI["poteri"][potere]["livello"] < 3 else "chiedo"


def potere_di(s):
    if s.get("per_tutti"):
        return "storia_valle"
    if s.get("intreccio"):
        return "intreccio"
    if s.get("eroe"):
        return "gesta_eco"
    if s.get("ritorno"):
        return "ritorno"
    return {"comune": "storia_comune", "insolita": "storia_comune", "rara": "storia_rara",
            "epica": "storia_epica", "leggendaria": "storia_leggendaria"}.get(s.get("rarita"), "storia_comune")


def scrivi_bozza(bozze):
    os.makedirs(C["bozze"], exist_ok=True)
    if os.path.exists(C["bozza"]):
        # una bozza non ancora approvata non si perde: va tra le lette, con il suo nome
        archivio = os.path.join(C["sognatore"], "Bozze_lette")
        os.makedirs(archivio, exist_ok=True)
        os.replace(C["bozza"], os.path.join(archivio, "Bozza_%s_NON_APPROVATA.txt" % adesso().strftime("%Y%m%d_%H%M%S")))
        print("   (c'era una bozza non ancora approvata: l'ho messa in Bozze_lette, con NON_APPROVATA nel nome)")
    for vecchio in glob.glob(os.path.join(C["bozze"], "*.json")):
        os.remove(vecchio)
    righe = ["IL SOGNATORE - storie da leggere (%s)" % adesso().strftime("%d/%m/%Y %H:%M"), "",
             "Per ogni storia, dopo VERDETTO: si = approvata, no = scartata (non tornera' a quel colono), lascia",
             "chiedo = non decidi adesso (le Epiche e le Leggendarie partono con chiedo: serve il vostro si).",
             "Puoi correggere i testi: approvando, il Sognatore prende la tua versione e il Giudice la rilegge.",
             "Poi salva, chiudi e lancia \"Sognatore 2 - approva\".", ""]
    for n, s in enumerate(bozze, 1):
        with open(os.path.join(C["bozze"], "%03d.json" % n), "w", encoding="utf-8") as f:
            json.dump(s, f, ensure_ascii=False, indent=1)
        rifiuto = s.get("rifiuto") or {}
        righe += ["=" * 100,
                  "STORIA %03d  |  per %s  |  %s %s  |  %s" % (n, nome_colono(s), (s.get("rarita") or "").upper(),
                                                           STELLE.get(s.get("difficolta", 1), "*"), s.get("titolo", "")),
                  "VERDETTO: %s" % verdetto_iniziale(s),
                  "IL GIUDICE: %s%s" % (s["giudice"], (" - " + "; ".join(s["motivi"])) if s["motivi"] else ""),
                  "SCRITTA DA: %s%s%s" % (s["scritta_da"], (" (seme %s)" % s["seme"]) if s.get("seme") else "",
                                          ("  |  FILO: %s" % s["finestra"]["serie"]) if s.get("serie") else ""),
                  "NASCE QUANDO: %s" % "; oppure ".join(descrivi_innesco(i) for i in s.get("innesco", [])),
                  "TITOLO: %s" % s.get("titolo", ""),
                  "COMINCIA: %s" % s.get("aggancio", "")]
        if s.get("ritorno"):
            righe.append("   (e' una conseguenza che torna: \"%s\", finale %d, dopo %d giorni)" % (
                SEMI_PER_ID.get(s["ritorno"]["di"], {}).get("titolo", s["ritorno"]["di"]), s["ritorno"]["finale"], s["ritorno"].get("dopo_giorni", 0)))
        if s.get("zona"):
            righe.append("   (zona del Battito: %s - %s)" % (s["zona"], ZONE.get(s["zona"], {}).get("cronaca") or ZONE.get(s["zona"], {}).get("dove", "")))
        for k, p in enumerate(s.get("passi", []), 1):
            righe.append("PASSO %d [%s]: %s" % (k, p.get("mattone"), p.get("testo", "")))
        for k, f in enumerate(s.get("finali", []), 1):
            if f.get("soglia"):
                righe.append("   (questo finale se aiutano almeno %d coloni)" % f["soglia"])
            righe.append("FINALE %d SCELTA: %s" % (k, f.get("scelta", "")))
            righe.append("FINALE %d ESITO: %s" % (k, f.get("esito", "")))
            righe.append("   (premio: %s%s)" % (premio_testo(f.get("premio") or {}),
                                               ("  ->  apre: %s" % f["prossimo"]) if f.get("prossimo") else ""))
        righe.append("SE RIFIUTA: %s" % rifiuto.get("testo", ""))
        if rifiuto.get("effetti"):
            righe.append("   (effetti: %s)" % "; ".join(descrivi_effetto(e) for e in rifiuto["effetti"]))
        righe.append("")
    with open(C["bozza"], "w", encoding="utf-8") as f:
        f.write("\n".join(righe))


def nome_colono(s):
    if s["colono"] == "_valle":
        return "TUTTA LA VALLE"
    if s.get("con"):
        return "%s (intrecciata con %s)" % (s["colono"], s["con"])
    return s["colono"]


def descrivi_innesco(i):
    t = i.get("tipo")
    if t == "dialogo":
        return "parla con %s" % i.get("png")
    if t == "luogo":
        return "arriva a %s%s" % (i.get("luogo"), (" di " + i["ora"]) if i.get("ora") else "")
    if t == "evento":
        return "nel registro: %s" % i.get("evento")
    if t == "storia":
        return "ha finito \"%s\"" % SEMI_PER_ID.get(i.get("storia"), {}).get("titolo", i.get("storia"))
    if t == "registro":
        return "nel Registro %s = %s" % (i.get("campo"), "/".join(i.get("valori", [])))
    if t == "tempo":
        return i.get("quando", "un momento speciale")
    if t == "sogno":
        return "dopo un sogno"
    return t or "?"


def descrivi_effetto(e):
    t = e.get("tipo")
    if t == "fiducia":
        return "%s si fida %s (%+d)" % (e.get("png"), "di piu'" if int(e.get("quanto", 0)) > 0 else "di meno", int(e.get("quanto", 0)))
    if t == "passa":
        return "la storia passa a un altro colono"
    if t == "chiusa":
        return "non tornera' mai piu'"
    return "%s: %s" % (t, e.get("testo", ""))


def leggi_bozza():
    """Dalla Bozza.txt corretta dallo staff: numero -> verdetto e testi."""
    if not os.path.exists(C["bozza"]):
        return {}
    blocchi, n, ultimo = {}, None, None
    try:
        with open(C["bozza"], encoding="utf-8-sig") as f:
            testo = f.read()
    except UnicodeDecodeError:           # il Blocco note l'ha salvata come ANSI
        with open(C["bozza"], encoding="cp1252") as f:
            testo = f.read()
    for riga in testo.splitlines():
        m = re.match(r"STORIA (\d+)", riga)
        if m:
            n = int(m.group(1))
            blocchi[n] = {"passi": {}, "scelte": {}, "esiti": {}}
            continue
        if n is None:
            continue
        b = blocchi[n]
        # una riga senza etichetta continua il testo di prima (lo staff ha premuto Invio dentro un testo)
        if riga.strip() and not re.match(r"(VERDETTO|IL GIUDICE|SCRITTA DA|NASCE QUANDO|TITOLO|COMINCIA|PASSO \d+|FINALE \d+|SE RIFIUTA|=+|   \()", riga):
            if ultimo:
                dove, chiave = ultimo
                dove[chiave] = (dove[chiave] + " " + riga.strip()).strip()
            continue
        ultimo = None
        for etichetta, chiave in (("VERDETTO:", "verdetto"), ("TITOLO:", "titolo"), ("COMINCIA:", "aggancio"),
                                  ("SE RIFIUTA:", "rifiuto")):
            if riga.startswith(etichetta):
                b[chiave] = riga[len(etichetta):].strip()
                ultimo = (b, chiave) if chiave != "verdetto" else None
        m = re.match(r"PASSO (\d+) \[[^\]]*\]: (.*)", riga)
        if m:
            b["passi"][int(m.group(1))] = m.group(2).strip()
            ultimo = (b["passi"], int(m.group(1)))
        m = re.match(r"FINALE (\d+) (SCELTA|ESITO): (.*)", riga)
        if m:
            dove = b["scelte" if m.group(2) == "SCELTA" else "esiti"]
            dove[int(m.group(1))] = m.group(3).strip()
            ultimo = (dove, int(m.group(1)))
    return blocchi


def approva():
    blocchi = leggi_bozza()
    if not blocchi:
        print("Non c'e' una bozza: prima \"Sognatore 1 - sogna le storie\".")
        return 1
    memoria = leggi_memoria()
    stat = leggi_statistiche()
    patente = memoria.setdefault("_patente", {})
    leggende = memoria.setdefault("_leggende", {})
    si = no = rimandate = 0
    # le due meta' di una storia intrecciata hanno lo stesso verdetto: quello della prima
    primo_verdetto = {}
    for n, b in sorted(blocchi.items()):
        f = os.path.join(C["bozze"], "%03d.json" % n)
        if os.path.exists(f):
            legame = leggi_json(f, {}).get("intreccio_id")
            if legame:
                if legame in primo_verdetto:
                    b["verdetto"] = primo_verdetto[legame]
                else:
                    primo_verdetto[legame] = b.get("verdetto", "")
    for n, b in sorted(blocchi.items()):
        f = os.path.join(C["bozze"], "%03d.json" % n)
        if not os.path.exists(f):
            continue
        with open(f, encoding="utf-8") as testo:
            s = json.load(testo)
        verdetto = b.get("verdetto", "").strip().lower().rstrip(".!")
        if verdetto in ("si", "sì", "s", "ok"):
            verdetto = "si"
        elif verdetto in ("no", "n"):
            verdetto = "no"
        else:
            if verdetto not in ("chiedo", "c", ""):
                print("Storia %03d: VERDETTO \"%s\" non lo capisco (si, no o chiedo): la tengo per la prossima volta." % (n, verdetto))
            verdetto = "chiedo"
        if verdetto == "chiedo":
            rimandate += 1                 # "chiedo": resta per la prossima bozza (non si ricorda, puo' tornare)
            os.remove(f)
            continue
        # le correzioni dello staff
        s["titolo"] = b.get("titolo") or s.get("titolo")
        s["aggancio"] = b.get("aggancio") or s.get("aggancio")
        if b.get("rifiuto"):
            s.setdefault("rifiuto", {})["testo"] = b["rifiuto"]
        for k, t in b["passi"].items():
            if 0 < k <= len(s["passi"]):
                s["passi"][k - 1]["testo"] = t
        for k, t in b["scelte"].items():
            if 0 < k <= len(s["finali"]):
                s["finali"][k - 1]["scelta"] = t
        for k, t in b["esiti"].items():
            if 0 < k <= len(s["finali"]):
                s["finali"][k - 1]["esito"] = t
        approvata = verdetto == "si"
        detto_no_dallo_staff = verdetto == "no" and s.get("giudice") != "scartata"
        mie = memoria.setdefault(s["colono"], [])
        stato, motivi = giudica(s, {m.get("titolo") for m in mie})     # il Giudice rilegge anche le correzioni
        if approvata and stato == "scartata":
            print("Storia %03d: il Giudice la scarta anche dopo le correzioni (%s)" % (n, "; ".join(motivi)))
            approvata = False
        if approvata and s.get("rarita") == "leggendaria" and s.get("seme") in leggende:
            print("Storia %03d: questa Leggendaria e' gia' di %s (una sola al mondo)" % (n, leggende[s["seme"]]))
            approvata = False
        s["finestra"] = finestra(s)
        s.update({"giudice": stato, "motivi": motivi, "approvata_il": oggi().isoformat()})
        dove = os.path.join(C["approvate"] if approvata else C["scartate"], re.sub(r"[^\w\- ]", "_", s["colono"]))
        os.makedirs(dove, exist_ok=True)
        with open(os.path.join(dove, s["id"] + ".json"), "w", encoding="utf-8") as out:
            json.dump(s, out, ensure_ascii=False, indent=1)
        premi = [f.get("premio") or {} for f in s.get("finali", [])]
        mie.append({"id": s["id"], "seme": s.get("seme"), "titolo": s["titolo"], "rarita": s.get("rarita"),
                    "serie": (s.get("serie") or {}).get("id", ""), "data": s["approvata_il"], "png": s.get("png", ""),
                    "luogo": s.get("luogo", ""), "con": s.get("con", ""),
                    "monete_max": max([int(x.get("monete", 0) or 0) for x in premi] or [0]),
                    "rari_max": max([sum(1 for o in x.get("oggetti", []) if o in MATTONI["oggetti_rari"]) for x in premi] or [0]),
                    "titolo_premio": any(x.get("titolo") for x in premi),
                    "stato": ("approvata" if approvata else "scartata") + ("" if stato == "ok" else " (" + stato + ")")})
        conta_statistica(stat, s.get("seme"), "approvate" if approvata else "scartate")
        # la patente: un si' in piu' di fila per quel potere, un no la azzera
        potere = potere_di(s)
        if approvata:
            patente[potere] = patente.get(potere, 0) + 1
        elif detto_no_dallo_staff:
            patente[potere] = 0
        if approvata and s.get("rarita") == "leggendaria" and s.get("seme"):
            leggende[s["seme"]] = s["colono"]
        si, no = si + approvata, no + (not approvata)
        os.remove(f)
    scrivi_json(C["memoria"], memoria)
    scrivi_json(C["statistiche"], stat)
    archivio = os.path.join(C["sognatore"], "Bozze_lette")
    os.makedirs(archivio, exist_ok=True)
    os.replace(C["bozza"], os.path.join(archivio, "Bozza_%s.txt" % adesso().strftime("%Y%m%d_%H%M%S")))
    print("Approvate %d storie, scartate %d, rimandate %d. Sono in %s (una cartella per colono)." % (si, no, rimandate, C["approvate"]))
    print("Nel gioco arriveranno con il modulo \"PNG veri e il custode\": per ora sono il raccolto del Sognatore.")
    return 0


def esito(nome, storia_id, cosa):
    """Com'e' andata nel gioco. Finche' il gioco non lo scrive da solo, lo segna lo staff (o le prove)."""
    memoria = leggi_memoria()
    stat = leggi_statistiche()
    if nome.lower() in ("valle", "la valle", "_valle"):
        nome = "_valle"
    mie = memoria.get(nome)
    if not mie:
        print("Non conosco il colono %s." % nome)
        return 1
    trovate = [m for m in mie if m.get("id") == storia_id or m.get("seme") == storia_id]
    riga = next((m for m in reversed(trovate) if not m.get("esito")), trovate[-1] if trovate else None)
    if not riga:
        print("%s non ha la storia %s." % (nome, storia_id))
        return 1
    cosa = cosa.lower()
    if str(riga.get("esito", "")).startswith(("finale", "rifiutata", "scaduta", "fallita")):
        print("La storia \"%s\" e' gia' chiusa (%s): non si cambia." % (riga["titolo"], riga["esito"]))
        return 1
    f = os.path.join(C["approvate"], re.sub(r"[^\w\- ]", "_", nome), riga["id"] + ".json")
    storia = leggi_json(f, None)
    if storia is None:
        print("La storia %s non e' tra le approvate di %s." % (riga["id"], nome))
        return 1
    riga["esito"] = cosa
    riga["esito_il"] = oggi().isoformat()
    effetti = []
    if cosa.isdigit() and nome == "_valle":
        # la valle: il finale dipende da quanti coloni hanno aiutato
        aiutanti = int(cosa)
        validi = [(f.get("soglia", 0), k) for k, f in enumerate(storia["finali"], 1) if f.get("soglia", 0) <= aiutanti]
        if not validi or aiutanti == 0:
            cosa = "rifiutata"
        else:
            cosa = "finale%d" % max(validi)[1]
        riga["aiutanti"] = aiutanti
        riga["esito"] = cosa
    if cosa.startswith("finale"):
        k = int(re.sub(r"\D", "", cosa) or 0)
        if not 1 <= k <= len(storia["finali"]):
            print("La storia ha %d finali." % len(storia["finali"]))
            return 1
        finale = storia["finali"][k - 1]
        riga["esito"] = "finale%d" % k
        effetti = list(finale.get("effetti", []))
        if finale.get("prossimo"):
            riga["prossimo"] = finale["prossimo"]
            annuncio = SEMI_PER_ID.get(finale["prossimo"], {}).get("annuncio")
            if annuncio:
                riga["sogno_annuncio"] = annuncio
        riga["finale_testo"] = finale.get("esito", "")
        conta_statistica(stat, riga.get("seme"), "finite")
        for r in SEMI:
            if (r.get("ritorno") or {}).get("di") == riga.get("seme") and r["ritorno"]["finale"] == k:
                print("   Questa scelta tornera': \"%s\" tra %d giorni." % (r["titolo"], r["ritorno"].get("dopo_giorni", 0)))
        if storia.get("rarita") in ("epica", "leggendaria") and nome != "_valle":
            nasce_gesta(storia, k)
    elif cosa in ("rifiutata", "scaduta", "fallita"):
        effetti = list((storia.get("rifiuto") or {}).get("effetti", []))
        conta_statistica(stat, riga.get("seme"), {"rifiutata": "rifiutate", "scaduta": "scadute", "fallita": "fallite"}[cosa])
        if any(e.get("tipo") == "chiusa" for e in effetti) and riga.get("serie"):
            riga["filo_chiuso"] = True
    elif cosa == "accettata":
        conta_statistica(stat, riga.get("seme"), "accettate")
    else:
        print("COSA deve essere: accettata, rifiutata, scaduta, fallita, finale1, finale2...")
        return 1
    riga["effetti_avvenuti"] = effetti
    scrivi_json(C["memoria"], memoria)
    scrivi_json(C["statistiche"], stat)
    print("Segnato: %s, \"%s\" -> %s." % (nome, riga["titolo"], cosa))
    for e in effetti:
        print("   effetto: " + descrivi_effetto(e))
    if riga.get("sogno_annuncio"):
        print("   Il Battito gli manda un sogno: \"%s\"" % riga["sogno_annuncio"])
    if riga.get("prossimo"):
        print("   Il filo continua: alla prossima \"sogna\" il Battito proporra' \"%s\"." % SEMI_PER_ID.get(riga["prossimo"], {}).get("titolo", riga["prossimo"]))
    return 0


def nasce_gesta(storia, k):
    """Idea 1: l'impresa Epica o Leggendaria di un colono diventa un'Eco che gli altri possono rivivere."""
    f = os.path.join(C["sognatore"], "semi_gesta.json")
    dati = leggi_json(f, {"_come": "Le gesta dei coloni diventate Eco (scritto dal Sognatore, idea 1 approvata da Fra il 13/11). Non si toccano a mano.", "semi": []})
    nuovo_id = "gesta_%s" % hashlib.sha1((storia["id"] + str(k)).encode("utf-8")).hexdigest()[:8]
    if any(s["id"] == nuovo_id for s in dati["semi"]):
        return
    finale = storia["finali"][k - 1]
    luogo = storia.get("luogo") if storia.get("luogo") in BIBBIA["luoghi"] else "Piazza del villaggio"
    png = storia.get("png") if storia.get("png") in MATTONI["png_nel_gioco"] else "Bruno"
    eroe = storia["colono"]
    dati["semi"].append({
        "id": nuovo_id, "titolo": "Eco: %s" % storia["titolo"].replace("Eco: ", ""), "rarita": "rara", "difficolta": storia.get("difficolta", 3),
        "eroe": eroe, "per": {}, "piace": {"curioso": 2, "esploratore": 1},
        "innesco": [{"tipo": "luogo", "luogo": luogo}, {"tipo": "dialogo", "png": png}],
        "zona": "eco_gesta", "png": [png], "luogo": [luogo],
        "aggancio": ("%s abbassa la voce: «Lo sai cosa ha fatto %s, qui? Il Battito se lo ricorda. Se vuoi, puoi vederlo con i tuoi occhi.»" % (png, eroe)),
        "passi": [
            {"mattone": "eco", "luogo": luogo, "testo": "Entra nell'Eco: e' il giorno di \"%s\"." % storia["titolo"]},
            {"mattone": "fantasma", "luogo": luogo, "testo": "Vedi %s com'era quel giorno. %s" % (eroe, finale.get("esito", "")[:200])},
            {"mattone": "scegli", "png": png, "testo": "L'Eco sta per chiudersi. %s, nell'Eco, si volta verso di te." % eroe}
        ],
        "finali": [
            {"scelta": "Gli fai un cenno di saluto.", "esito": "%s ti risponde, come se ti conoscesse. Quando esci, il vero %s, ovunque sia, sente un brivido e non sa perche'." % (eroe, eroe),
             "premio": {"segno": "HaVistoLeGesta", "monete": 30, "iscrizione": "Le gesta di %s" % eroe}},
            {"scelta": "Guardi soltanto, e ricordi.", "esito": "Esci dall'Eco con una storia vera da raccontare alla locanda. Bruno la vuole sentire due volte.",
             "premio": {"segno": "HaRicordatoLeGesta", "monete": 20}}
        ],
        "rifiuto": {"testo": "L'Eco resta li', per chi la vorra' vedere.", "effetti": [{"tipo": "passa"}]}
    })
    scrivi_json(f, dati)
    print("   Le gesta di %s diventano un'Eco: \"%s\" (gli altri coloni potranno riviverla)." % (eroe, storia["titolo"]))


def custode_cmd():
    memoria = leggi_memoria()
    lim = POTERI.get("custode", {})
    conti, superati, troppi, fiducia_bassa = custode_stato(memoria)
    righe = ["IL CUSTODE DEI CUSTODI - %s (ultimi %d giorni)" % (adesso().strftime("%d/%m/%Y %H:%M"), lim.get("giorni", 7)), ""]
    for k, etichetta in (("monete_valle", "Monete promesse in tutta la valle"), ("oggetti_rari_valle", "Oggetti rari promessi"),
                         ("titoli_valle", "Titoli promessi"), ("fatti_nel_mondo", "Fatti nel mondo avvenuti")):
        righe.append("%-36s %5d su %5d %s" % (etichetta, conti[k], lim.get(k, 0), "<- TROPPO: il Battito e' frenato" if k in superati else ""))
    righe.append("")
    righe.append("Coloni con troppe monete (solo storie Comuni e Insolite per loro): " + (", ".join(troppi) or "nessuno"))
    righe.append("PNG che si fidano molto poco di qualcuno (da guardare): " + (", ".join(fiducia_bassa) or "nessuno"))
    testo = "\n".join(righe)
    with open(os.path.join(C["sognatore"], "Custode.txt"), "w", encoding="utf-8") as f:
        f.write(testo + "\n")
    print(testo)
    return 0


def ricordi(nome=None):
    """Idea 4: come i PNG salutano ogni colono dopo le storie (Saved/Server/Sognatore/Ricordi.json, per il gioco)."""
    memoria = leggi_memoria()
    tutti = {}
    for colono, mie in coloni_in_memoria(memoria).items():
        if nome and colono.lower() != nome.lower():
            continue
        righe = {}
        for png, v in fiducia_di(mie).items():
            frasi = BIBBIA.get("ricordi", {}).get(png)
            if not frasi or v == 0:
                continue
            ultima = next((m for m in reversed(mie) if m.get("png") == png and m.get("esito")), None)
            titolo = ultima["titolo"] if ultima else "quella storia"
            righe[png] = (frasi["buono"] if v > 0 else frasi["freddo"]).replace("{titolo}", titolo).replace("{nome}", colono)
        # chi ha finito storie con un PNG senza effetti sulla fiducia: lo ricorda bene
        for m in mie:
            if str(m.get("esito", "")).startswith("finale") and m.get("png") and m["png"] not in righe and m["png"] in BIBBIA.get("ricordi", {}):
                righe[m["png"]] = BIBBIA["ricordi"][m["png"]]["buono"].replace("{titolo}", m["titolo"]).replace("{nome}", colono)
        if righe:
            tutti[colono] = righe
    scrivi_json(os.path.join(C["sognatore"], "Ricordi.json"), tutti)
    for colono, righe in tutti.items():
        print(colono)
        for png, r in righe.items():
            print("   %s: %s" % (png, r))
    print("Scritto %s" % os.path.join(C["sognatore"], "Ricordi.json"))
    return 0


def libro(nome=None):
    """Idea 7: il libro del colono, la sua storia scritta come una cronaca (Libri/<nome>.txt, e Libri.json per il sito)."""
    memoria = leggi_memoria()
    coloni = leggi_personaggi()
    cartella = os.path.join(C["sognatore"], "Libri")
    os.makedirs(cartella, exist_ok=True)
    tutti = {}
    for colono, mie in coloni_in_memoria(memoria).items():
        if nome and colono.lower() != nome.lower():
            continue
        reg = coloni.get(colono, {}).get("registro", {})
        r = BIBBIA["registro"]
        donna = reg.get("sesso") == "donna"
        capitoli = []
        apertura = "%s arrivo' nella valle dell'Orso nell'anno 312 dopo il Crepuscolo" % colono
        if reg.get("origine"):
            apertura += ", da %s" % r["origine"].get(reg["origine"], reg["origine"])
        if reg.get("motivo"):
            apertura += ". Era %s per %s" % ("venuta" if donna else "venuto", r["motivo"].get(reg["motivo"], reg["motivo"]))
        capitoli.append(apertura + ".")
        for m in mie:
            esito_m = str(m.get("esito", ""))
            if esito_m.startswith("finale"):
                testo = "%s. %s" % (m["titolo"], m.get("finale_testo", ""))
                if m.get("con"):
                    testo += " (Insieme a %s.)" % m["con"]
                capitoli.append(testo)
                if m.get("sogno_annuncio"):
                    capitoli.append("Quella notte sogno': %s" % m["sogno_annuncio"])
            elif esito_m in ("rifiutata", "scaduta", "fallita"):
                capitoli.append("%s: %s." % (m["titolo"], {"rifiutata": "non volle", "scaduta": "lascio' passare il tempo",
                                                         "fallita": "non ci riusci'"}[esito_m]))
        if len(capitoli) < 2:
            continue
        tutti[colono] = capitoli
        with open(os.path.join(cartella, re.sub(r"[^\w\- ]", "_", colono) + ".txt"), "w", encoding="utf-8") as f:
            f.write("IL LIBRO DI %s\n\n%s\n" % (colono.upper(), "\n\n".join(capitoli)))
    scrivi_json(os.path.join(cartella, "Libri.json"), tutti)
    for colono, capitoli in tutti.items():
        print("IL LIBRO DI %s: %d pagine" % (colono.upper(), len(capitoli)))
    print("Scritti in %s" % cartella)
    return 0


def patente_cmd():
    memoria = leggi_memoria()
    conti = memoria.get("_patente", {})
    soglia = POTERI.get("approvazioni_per_salire", 20)
    print("LA PATENTE DEL BATTITO  (1 = da solo, 2 = da solo ma lo staff vede e annulla, 3 = serve il si' dello staff)")
    for nome, p in POTERI["poteri"].items():
        di_fila = conti.get(nome, 0)
        nota = ""
        if p["livello"] > p["puo_scendere_a"] and di_fila >= soglia:
            nota = "  <- PUO' SALIRE: %d si' di fila. Se siete d'accordo, in poteri.json mettete \"livello\": %d" % (di_fila, p["livello"] - 1)
        elif p["livello"] > p["puo_scendere_a"]:
            nota = "  (%d si' di fila su %d)" % (di_fila, soglia)
        print("  livello %d  %-20s %s%s" % (p["livello"], nome, p["cosa"], nota))
    return 0


def controlla():
    """Ogni seme, cucito per coloni diversi, deve passare il Giudice; i fili devono stare in piedi."""
    errori = []
    ids = Counter(s["id"] for s in SEMI)
    errori += ["id ripetuto: %s" % i for i, n in ids.items() if n > 1]
    R = BIBBIA["registro"]
    conta = Counter()
    for seme in SEMI:
        for sesso in ("uomo", "donna"):
            for k in range(6):
                caso = random.Random(k)
                reg = {"sesso": sesso, "fede": caso.choice(list(BIBBIA["fedi"])), "richiamoMagia": caso.choice(["si", "no"])}
                for campo_r in ("origine", "mestiere", "motivo", "ricordo", "paura"):
                    reg[campo_r] = caso.choice(list(R[campo_r]))
                for c, v in seme.get("per", {}).items():
                    reg[c] = v[0]
                stato, motivi = giudica(cuci(seme, {"nome": "Prova", "registro": reg}, caso, altro="Altro"))
                if stato == "scartata":
                    errori.append("%s (%s): %s" % (seme["id"], seme["_file"], "; ".join(motivi)))
                    break
        conta[(seme.get("rarita", "comune"), giudica(cuci(seme, {"nome": "P", "registro": {}}, random.Random(0), altro="A"))[0])] += 1
        r = seme.get("ritorno")
        if r and (r.get("di") not in SEMI_PER_ID or not 1 <= r.get("finale", 0) <= len(SEMI_PER_ID[r["di"]]["finali"])):
            errori.append("%s: torna da una storia o un finale che non c'e' (%s)" % (seme["id"], r))
        if seme.get("per_tutti") and not all(f.get("soglia") for f in seme["finali"]):
            errori.append("%s: storia della valle con un finale senza soglia" % seme["id"])
        if seme.get("intreccio") and "{altro}" not in json.dumps(seme, ensure_ascii=False):
            errori.append("%s: intreccio che non nomina mai {altro}" % seme["id"])
        for f in seme.get("finali", []):
            dopo = SEMI_PER_ID.get(f.get("prossimo", ""))
            if f.get("prossimo") and (not dopo or not dopo.get("serie")):
                errori.append("%s: il finale apre %s, che non e' un capitolo" % (seme["id"], f.get("prossimo")))
    errori = sorted(set(errori))
    print("Semi: %d in %d file." % (len(SEMI), len({s["_file"] for s in SEMI})))
    for r in RARITA:
        print("  %-12s pronte %2d, in attesa %2d" % (r, conta[(r, "ok")], conta[(r, "in attesa")]))
    fili = Counter(s["serie"]["id"] for s in SEMI if s.get("serie"))
    print("Fili: " + ", ".join("%s (%d capitoli)" % kv for kv in sorted(fili.items())))
    print("Ritorni %d, storie della valle %d, intrecci %d." % (sum(1 for s in SEMI if s.get("ritorno")),
          sum(1 for s in SEMI if s.get("per_tutti")), sum(1 for s in SEMI if s.get("intreccio"))))
    if errori:
        print("PROBLEMI:")
        for e in errori:
            print("  - " + e)
        return 1
    print("Tutto a posto: ogni seme passa il Giudice.")
    return 0


def prova():
    """Un giro finto in una cartella a parte, con tre coloni inventati: per vedere come scrive, senza toccare niente."""
    global C
    import tempfile
    radice = tempfile.mkdtemp(prefix="sognatore_prova_")
    C = cartelle(radice)
    coloni = {
        "Alda Ferri": {"nome": "Alda Ferri", "registro": {"sesso": "donna", "fede": "solara", "origine": "minatori", "mestiere": "orfano",
                       "motivo": "scomparso", "ricordo": "lettera", "paura": "buio", "onesta": "onesto", "coraggio": "prudente",
                       "devozione": "devoto", "animo": "gentile", "richiamoMagia": "si"}, "tempo": 7200, "ultimo": 0},
        "Bastian": {"nome": "Bastian", "registro": {"sesso": "uomo", "fede": "nessuna", "origine": "capitale", "mestiere": "soldato",
                    "motivo": "fuga", "ricordo": "spada", "paura": "solitudine", "onesta": "furbo", "coraggio": "coraggioso",
                    "devozione": "scettico", "animo": "duro", "richiamoMagia": "no"}, "tempo": 3600, "ultimo": 0},
        "Mira": {"nome": "Mira", "registro": {"sesso": "donna", "fede": "vecchidei", "origine": "dimenticato", "mestiere": "cacciatore",
                 "motivo": "sogno", "ricordo": "amuleto", "paura": "acqua", "onesta": "onesto", "coraggio": "coraggioso",
                 "devozione": "devoto", "animo": "gentile", "richiamoMagia": "si"}, "tempo": 1800, "ultimo": 0},
    }
    eventi = [{"chi": "Mira", "tipo": "Animale", "dettagli": "Lupo ucciso"}] * 4 + \
             [{"chi": "Mira", "tipo": "Dialogo", "dettagli": "parla con Gaspare (la prima volta)"}] + \
             [{"chi": "Alda Ferri", "tipo": "ScopreLuogo", "dettagli": "Grotta est"}] * 2 + \
             [{"chi": "Bastian", "tipo": "PortaChiusa", "dettagli": "ha provato ad aprire"}] * 3
    for c in coloni.values():
        print(scheda(c, profilo(c, eventi)) + "\n" + "-" * 60)
    sogna(3, coloni=coloni, eventi=eventi)
    print("\n(prova in %s)" % radice)
    return 0


if __name__ == "__main__":
    comando = sys.argv[1] if len(sys.argv) > 1 else ""
    if comando == "vede":
        sys.exit(vede(" ".join(sys.argv[2:]) or None))
    elif comando == "sogna":
        sys.exit(sogna(int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 3))
    elif comando == "approva":
        sys.exit(approva())
    elif comando == "esito" and len(sys.argv) >= 5:
        sys.exit(esito(" ".join(sys.argv[2:-2]), sys.argv[-2], sys.argv[-1]))
    elif comando == "custode":
        sys.exit(custode_cmd())
    elif comando == "ricordi":
        sys.exit(ricordi(" ".join(sys.argv[2:]) or None))
    elif comando == "libro":
        sys.exit(libro(" ".join(sys.argv[2:]) or None))
    elif comando == "patente":
        sys.exit(patente_cmd())
    elif comando == "controlla":
        sys.exit(controlla())
    elif comando == "prova":
        sys.exit(prova())
    print("uso: sognatore.py vede [nome] | sogna [quante] | approva | esito NOME ID COSA | custode | ricordi [nome] | libro [nome] | patente | controlla | prova")
    sys.exit(1)
