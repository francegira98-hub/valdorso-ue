# Valdorso - Prepara un aggiornamento per il launcher: lo zip del gioco e il manifesto. Scritto da Claude il 07/11/2026.
#
# Si usa dopo aver pacchettizzato il gioco (Piattaforme -> Windows -> Pacchettizza progetto, cartella per esempio
# V:\Pacchetti\Windows). Dalla cartella del progetto, una riga sola:
#
#   python Tools\Launcher\prepara_aggiornamento.py V:\Pacchetti\Windows 0.1.3 https://indirizzo/dove/sta/lo/zip/
#
# Opzioni:
#   --nome "La valle"            il nome della versione (di serie quello di Source/Valdorso/ValdorsoVersione.h)
#   --server 1.2.3.4:7777        l'indirizzo del server che il launcher passa al gioco
#   --stato http://1.2.3.4:7780/stato   la pagina dello stato del mondo (di serie: dal server, porta 7780)
#   --notizie notizie.json       le notizie (un elenco: [{"data": "07/11", "titolo": "...", "testo": "...", "link": ""}])
#   --uscita cartella            dove scrivere zip e manifesto (di serie V:\Valdorso\Build\Aggiornamento)
#   --eseguibile Valdorso.exe    il programma da avviare, dentro la cartella del gioco
#   --launcher 1.0.0 --url-launcher https://...   l'ultima versione del launcher (chi ne ha una vecchia vede l'avviso)
#
# Fa: Valdorso_<versione>.zip (senza le cartelle Saved e i file .pdb), manifesto.json (versione, impronta SHA-256 e
# misura dello zip, impronta di ogni file per "Controlla i file", notizie, server). Poi si caricano sul sito:
# manifesto.json all'indirizzo scritto in launcher.json ("manifesto"), lo zip all'indirizzo dato qui.
# Se nella cartella d'uscita c'è già un manifesto.json e non si passa --notizie, le notizie vecchie restano.

import argparse
import datetime
import hashlib
import json
import os
import re
import sys
import zipfile

ESCLUSI_CARTELLE = {"Saved", "Intermediate", "DerivedDataCache"}
ESCLUSI_ESTENSIONI = {".pdb"}


def sha256(percorso):
    h = hashlib.sha256()
    with open(percorso, "rb") as f:
        while True:
            pezzo = f.read(1 << 20)
            if not pezzo:
                break
            h.update(pezzo)
    return h.hexdigest()


def nome_di_serie():
    qui = os.path.dirname(os.path.abspath(__file__))
    h = os.path.join(qui, "..", "..", "Source", "Valdorso", "ValdorsoVersione.h")
    try:
        with open(h, encoding="utf-8") as f:
            testo = f.read()
        m = re.search(r'Nome\(\)\s*\{\s*return TEXT\("([^"]+)"\)', testo)
        return m.group(1) if m else ""
    except OSError:
        return ""


def file_del_gioco(cartella):
    for radice, cartelle, files in os.walk(cartella):
        cartelle[:] = sorted(c for c in cartelle if c not in ESCLUSI_CARTELLE)
        for n in sorted(files):
            if os.path.splitext(n)[1].lower() in ESCLUSI_ESTENSIONI:
                continue
            completo = os.path.join(radice, n)
            yield completo, os.path.relpath(completo, cartella).replace("\\", "/")


def principale(argomenti=None):
    p = argparse.ArgumentParser(description="Prepara lo zip e il manifesto per il launcher di Valdorso.")
    p.add_argument("cartella", help="la cartella del gioco pacchettizzato (quella con Valdorso.exe)")
    p.add_argument("versione", help="per esempio 0.1.3")
    p.add_argument("url", help="l'indirizzo della cartella sul web dove si carica lo zip (finisce con /) o dello zip")
    p.add_argument("--nome", default=None)
    p.add_argument("--server", default="")
    p.add_argument("--stato", default="")
    p.add_argument("--notizie", default=None)
    p.add_argument("--uscita", default=os.path.join("Build", "Aggiornamento"))
    p.add_argument("--eseguibile", default="Valdorso.exe")
    p.add_argument("--launcher", default="")
    p.add_argument("--url-launcher", default="")
    a = p.parse_args(argomenti)

    cartella = os.path.abspath(a.cartella)
    if not os.path.isfile(os.path.join(cartella, a.eseguibile)):
        print("ERRORE: in %s non c'è %s. È la cartella giusta (quella del gioco pacchettizzato)?" % (cartella, a.eseguibile))
        return 1
    if not re.fullmatch(r"\d+(\.\d+)*", a.versione):
        print("ERRORE: la versione deve essere fatta di numeri e punti (per esempio 0.1.3)")
        return 1
    os.makedirs(a.uscita, exist_ok=True)
    nome_zip = "Valdorso_%s.zip" % a.versione
    zip_percorso = os.path.join(a.uscita, nome_zip)

    elenco = []
    print("Preparo %s da %s ..." % (nome_zip, cartella))
    with zipfile.ZipFile(zip_percorso, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for completo, relativo in file_del_gioco(cartella):
            z.write(completo, relativo)
            elenco.append({"p": relativo, "s": sha256(completo), "b": os.path.getsize(completo)})
    print("  %d file, %.1f MB nello zip" % (len(elenco), os.path.getsize(zip_percorso) / 1048576))

    url = a.url if a.url.lower().endswith(".zip") else a.url.rstrip("/") + "/" + nome_zip
    manifesto_percorso = os.path.join(a.uscita, "manifesto.json")
    notizie = []
    if a.notizie:
        with open(a.notizie, encoding="utf-8") as f:
            notizie = json.load(f)
    elif os.path.isfile(manifesto_percorso):
        with open(manifesto_percorso, encoding="utf-8") as f:
            notizie = json.load(f).get("notizie", [])

    manifesto = {
        "gioco": "Valdorso",
        "versione": a.versione,
        "nome": a.nome if a.nome is not None else nome_di_serie(),
        "data": datetime.date.today().isoformat(),
        "aperto": True,
        "pacchetto": {"url": url, "sha256": sha256(zip_percorso), "byte": os.path.getsize(zip_percorso)},
        "eseguibile": a.eseguibile,
        "argomenti": "",
        "server": {"indirizzo": a.server, "stato": a.stato},
        "launcher": {"versione": a.launcher, "url": a.url_launcher},
        "notizie": notizie,
        "file": elenco,
    }
    with open(manifesto_percorso, "w", encoding="utf-8") as f:
        json.dump(manifesto, f, ensure_ascii=False, indent=1)
    print("Fatto: %s e %s" % (zip_percorso, manifesto_percorso))
    print("Ora carica lo zip a %s e il manifesto all'indirizzo scritto in launcher.json." % url)
    return 0


if __name__ == "__main__":
    sys.exit(principale())
