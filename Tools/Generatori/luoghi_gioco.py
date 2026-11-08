# Valdorso - I luoghi della valle per il gioco (bussola e taccuino, v0.1.3). Scritto da Claude il 07/11/2026.
#
# Legge Luoghi_3000.json (lo scrive valle.py: per ogni luogo la posizione in Unreal, "unreal_cm") e scrive
# Source/Valdorso/ValdorsoLuoghiDati.inl, che il C++ (ValdorsoTaccuinoDati.cpp) usa per:
#   - scoprire i luoghi: il server guarda ogni secondo se un colono è entrato nel raggio di un luogo che non conosceva;
#   - la bussola in alto sullo schermo: i segni dei luoghi già scoperti;
#   - il taccuino di viaggio (tasto J): l'elenco dei luoghi scoperti.
# Le posizioni sono in centimetri del mondo (Unreal X = est, Unreal Y = sud): quelle del file (cm dall'angolo nord-ovest
# del paesaggio, come per gli script dell'editor) più l'angolo del Landscape nel mondo.
# (08/11) L'angolo NON è (0, 0) come si pensava: in L_Valle il Landscape è centrato, l'angolo nord-ovest sta in
# (-150000, -150000). Lo dice il gioco: in PIE Ottone stava in (112297, 99495), cioè la porta del passo del file
# (262200, 249960) più il suo posto (+97, -465 cm, metti_png.py) meno 150000. Con l'angolo a zero tutti i luoghi
# erano spostati di 1,5 km verso sud-est (il passo, per esempio, a 2,1 km dalla porta, fuori dal paesaggio) e non se ne
# scopriva nessuno. Gli script dell'editor leggono l'angolo dal Landscape; qui è scritto in ANGOLO_DI_SERIE.
# Se un giorno il Landscape si sposta, si cambia ANGOLO_DI_SERIE o si rilancia con --angolo X Y (cm).
# Il raggio di scoperta (metri) si decide qui sotto, luogo per luogo: grande per i luoghi larghi (il lago, la palude,
# i pascoli), piccolo per una cosa sola (il tempio, il mulino, l'ingresso di una grotta).
#
# Uso (dalla cartella del progetto):
#   python Tools/Generatori/luoghi_gioco.py                                   legge V:/Texture/valdorso/valle/Luoghi_3000.json
#   python Tools/Generatori/luoghi_gioco.py <Luoghi_3000.json> [<cartella Source/Valdorso>] [--angolo X Y]
#   (senza --angolo: ANGOLO_DI_SERIE, l'angolo del Landscape di L_Valle)

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
    "Posto di guardia del passo": 45,   # (08/11) dal centro qui sotto: dal sigillo fino a 55 m giù per la strada
    "Cascata": 50,                   # la pozza sotto la cascata
}
RAGGIO_DI_SERIE = 40

# (08/11) L'angolo nord-ovest del Landscape nel mondo (cm): vedi sopra.
ANGOLO_DI_SERIE = (-150000.0, -150000.0)

# (08/11) Centri spostati (cm, come nel file: dall'angolo del paesaggio). Il passo: il punto di valle.py è la porta
# stessa e il sigillo sta 6 m dietro, verso il passo, quindi metà del cerchio cadeva dove non si va. Il centro scende
# di 10 m lungo la strada verso la valle (la direzione della strada di villaggio.py, (-0.80105, -0.59860)): così si
# scopre arrivando al posto di guardia, davanti alla sbarra e alle guardie, fino alla porta e al sigillo.
CENTRI_CM = {
    "Posto di guardia del passo": (262200.0 - 0.80105 * 1000.0, 249960.0 - 0.59860 * 1000.0),
}

# Voci del file che non sono luoghi da scoprire (per ora nessuna: tutte quelle di valle.py hanno un nome sulla mappa).
DA_SALTARE = set()


def c_testo(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


def scrivi_inl(luoghi, percorso, angolo):
    righe = ["// Valdorso - I luoghi della valle. FILE GENERATO da Tools/Generatori/luoghi_gioco.py: non toccarlo a mano,",
             "// cambia i raggi lì e rilancia: python Tools/Generatori/luoghi_gioco.py <Luoghi_3000.json>",
             "// Per ogni luogo: il nome, la posizione nel mondo (cm: X est, Y sud, Z quota) e il raggio di scoperta (cm).",
             "// L'angolo nord-ovest del Landscape nel mondo, sommato alle posizioni del file: (%.0f, %.0f) cm." % (angolo[0], angolo[1]),
             "static const FValdorsoLuogoDati GValdorsoLuoghiDati[] =",
             "{"]
    for nome, dati in luoghi.items():
        if nome in DA_SALTARE:
            continue
        x, y, z = dati["unreal_cm"]
        if nome in CENTRI_CM:
            x, y = CENTRI_CM[nome]
        raggio = RAGGI_M.get(nome, RAGGIO_DI_SERIE) * 100
        righe.append('\t{ TEXT("%s"), %.1f, %.1f, %.1f, %.1ff },' % (c_testo(nome), x + angolo[0], y + angolo[1], z, raggio))
    righe.append("};")
    with open(percorso, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(righe) + "\n")


if __name__ == "__main__":
    argomenti = sys.argv[1:]
    angolo = ANGOLO_DI_SERIE
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
