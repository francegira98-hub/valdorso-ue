# Valdorso - I luoghi della valle per il gioco (bussola e taccuino, v0.1.3). Scritto da Claude il 07/11/2026.
#
# Legge Luoghi_3000.json (lo scrive valle.py: per ogni luogo la posizione in Unreal, "unreal_cm") e scrive
# Source/Valdorso/ValdorsoLuoghiDati.inl, che il C++ (ValdorsoTaccuinoDati.cpp) usa per:
#   - scoprire i luoghi: il server guarda ogni secondo se un colono è entrato nel raggio di un luogo che non conosceva;
#   - la bussola in alto sullo schermo: i segni dei luoghi già scoperti;
#   - il taccuino di viaggio (tasto J): l'elenco dei luoghi scoperti.
# Le posizioni sono in centimetri del mondo, con il Landscape importato con l'angolo nord-ovest in (0, 0, 0) come dice
# valle.py (Unreal X = est, Unreal Y = sud). Se un giorno il Landscape si sposta, si rilancia con --angolo X Y (cm).
# Il raggio di scoperta (metri) si decide qui sotto, luogo per luogo: grande per i luoghi larghi (il lago, la palude,
# i pascoli), piccolo per una cosa sola (il tempio, il mulino, l'ingresso di una grotta).
#
# Uso (dalla cartella del progetto):
#   python Tools/Generatori/luoghi_gioco.py                                   legge V:/Texture/valdorso/valle/Luoghi_3000.json
#   python Tools/Generatori/luoghi_gioco.py <Luoghi_3000.json> [<cartella Source/Valdorso>] [--angolo X Y]

import json
import os
import sys

PREDEFINITO = "V:/Texture/valdorso/valle/Luoghi_3000.json"

# Raggio di scoperta in metri (in piano: la quota non conta). Un luogo che non è qui usa RAGGIO_DI_SERIE.
RAGGI_M = {
    "Piazza del villaggio": 30,
    "Tempio del Cuore": 15,          # a 35 m dalla piazza: si scopre entrando nel tempio o sulla soglia
    "Mulino": 25,
    "Ponte": 25,
    "Collina dell'Orso": 120,        # la cima della collina: il pendio alto basta
    "Rovine degli Antichi": 60,      # la cinta spezzata, la colonnata, il cerchio delle cinque pietre e il tumulo
    "Grotta est": 40,                # il punto di valle.py sta un poco dentro il monte: si scopre davanti all'ingresso
    "Grotta nord": 40,
    "Lago": 220,                     # il centro del lago: la riva è da 125 a 245 m (Acqua_3000.json)
    "Palude": 140,
    "Pascoli": 120,
    "Cimitero": 30,
    "Boschetto": 150,
    "Posto di guardia del passo": 40,
    "Cascata": 50,                   # la pozza sotto la cascata
}
RAGGIO_DI_SERIE = 40

# Voci del file che non sono luoghi da scoprire (per ora nessuna: tutte quelle di valle.py hanno un nome sulla mappa).
DA_SALTARE = set()


def c_testo(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


def scrivi_inl(luoghi, percorso, angolo):
    righe = ["// Valdorso - I luoghi della valle. FILE GENERATO da Tools/Generatori/luoghi_gioco.py: non toccarlo a mano,",
             "// cambia i raggi lì e rilancia: python Tools/Generatori/luoghi_gioco.py <Luoghi_3000.json>",
             "// Per ogni luogo: il nome, la posizione nel mondo (cm: X est, Y sud, Z quota) e il raggio di scoperta (cm).",
             "static const FValdorsoLuogoDati GValdorsoLuoghiDati[] =",
             "{"]
    for nome, dati in luoghi.items():
        if nome in DA_SALTARE:
            continue
        x, y, z = dati["unreal_cm"]
        raggio = RAGGI_M.get(nome, RAGGIO_DI_SERIE) * 100
        righe.append('\t{ TEXT("%s"), %.1f, %.1f, %.1f, %.1ff },' % (c_testo(nome), x + angolo[0], y + angolo[1], z, raggio))
    righe.append("};")
    with open(percorso, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(righe) + "\n")


if __name__ == "__main__":
    argomenti = sys.argv[1:]
    angolo = (0.0, 0.0)
    if "--angolo" in argomenti:
        i = argomenti.index("--angolo")
        angolo = (float(argomenti[i + 1]), float(argomenti[i + 2]))
        del argomenti[i:i + 3]
    sorgente = argomenti[0] if len(argomenti) > 0 else PREDEFINITO
    cartella = argomenti[1] if len(argomenti) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Source", "Valdorso")
    with open(sorgente, encoding="utf-8") as f:
        dati = json.load(f)
    luoghi = dati["luoghi"]
    sconosciuti = [n for n in luoghi if n not in RAGGI_M and n not in DA_SALTARE]
    if sconosciuti:
        print("attenzione, luoghi nuovi senza raggio (uso %d m): %s" % (RAGGIO_DI_SERIE, ", ".join(sconosciuti)))
    uscita = os.path.join(cartella, "ValdorsoLuoghiDati.inl")
    scrivi_inl(luoghi, uscita, angolo)
    print("scritto %s (%d luoghi)" % (os.path.normpath(uscita), len([n for n in luoghi if n not in DA_SALTARE])))
