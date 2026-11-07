# Valdorso - L'alfabeto delle rune di Val d'Orso (v0.1.3). Scritto da Claude il 07/11/2026.
# Le rune sono quelle delle Cronache (la tavola "Le rune di Val d'Orso" mandata da Fra il 07/11): una per elemento
# (Aeth il fuoco, Nir l'acqua, Dorn la terra, Sil il vento, Lume la luce), cinque verbi (Var andare/scagliare,
# Ka proteggere, Ren guarire, Vel vedere/guidare, Tor fermare) e Ur, la runa del custode, che si scrive da sola, sopra.
# Gli elementi stanno sulla linea di base; i verbi no, sono un gesto.
# Un incantesimo è elemento + verbo (Aeth-Var, il Dardo infuocato); una terza runa lo piega (le quindici parole a tre
# rune, grado III nelle Cronache). Nel MMO la lingua serve per rituali, iscrizioni, tomi e firme dei maghi; i nomi
# degli incantesimi restano in italiano.
#
# Questo file è la fonte unica della forma delle rune:
#   - i generatori (villaggio_rovine.py, villaggio_grotta.py) le incidono nella pietra con tratti();
#   - python alfabeto_rune.py <cartella Source/Valdorso> scrive ValdorsoRuneTratti.inl, che il C++ (ValdorsoRune.cpp)
#     usa per disegnarle sullo schermo (SValdorsoRuna), e Anteprima_Rune.png per controllarle a occhio.
# Coordinate: da -1 a 1, x a destra, y in su; ogni runa è un elenco di linee spezzate (punti uniti in ordine).

import math
import os
import sys

ELEMENTI = ["Aeth", "Nir", "Dorn", "Sil", "Lume"]
VERBI = ["Var", "Ka", "Ren", "Vel", "Tor"]
TUTTE = ELEMENTI + VERBI + ["Ur"]

SIGNIFICATI = {
    "Aeth": "il fuoco", "Nir": "l'acqua", "Dorn": "la terra", "Sil": "il vento", "Lume": "la luce",
    "Var": "andare, scagliare", "Ka": "proteggere", "Ren": "guarire", "Vel": "vedere, guidare", "Tor": "fermare",
    "Ur": "orso, custode, la porta",
}

# Le parole con un nome (dalle Cronache). Le altre combinazioni valide si possono scrivere, ma non hanno un nome.
PAROLE = {
    "Aeth-Var": "Dardo infuocato", "Nir-Ren": "Guarigione lieve", "Lume-Tor": "Scudo sacro",
    "Aeth-Var-Tor": "Fiamma che inchioda", "Aeth-Var-Ren": "Fuoco che nutre", "Aeth-Ka-Vel": "Brace negli occhi",
    "Nir-Var-Tor": "Lancia di ghiaccio", "Nir-Ren-Ka": "Acqua che veglia", "Nir-Var-Vel": "Pioggia negli occhi",
    "Dorn-Var-Tor": "Frana", "Dorn-Ka-Ren": "Pietra viva", "Dorn-Var-Vel": "Polvere negli occhi",
    "Sil-Var-Tor": "Vento che inchioda", "Sil-Ka-Ren": "Vento che ristora", "Sil-Var-Vel": "Vento che guida",
    "Lume-Vel-Tor": "Luce che ferma", "Lume-Ren-Ka": "Mano di Solara", "Lume-Vel-Ren": "Luce che consola",
}

BASE = -0.85  # la linea di base degli elementi


def _arco(cx, cy, rx, ry, da, a, passi=10):
    """Punti di un arco d'ellisse da 'da' ad 'a' gradi (0 = destra, 90 = su)."""
    return [(cx + rx * math.cos(math.radians(da + (a - da) * k / passi)),
             cy + ry * math.sin(math.radians(da + (a - da) * k / passi))) for k in range(passi + 1)]


def _punto(cx, cy, r=0.09):
    """Un punto: un cerchietto chiuso (con il tratto spesso sembra pieno)."""
    return _arco(cx, cy, r, r, 0, 360, 8)


def _base(mezza=0.8):
    return [(-mezza, BASE), (mezza, BASE)]


FORME = {
    # il fuoco: la fiamma che sale dalla base (un'asta e due gambe)
    "Aeth": [[(0.0, 1.0), (0.0, BASE)], [(-0.4, BASE), (0.0, 0.5), (0.4, BASE)], _base(0.82)],
    # l'acqua: due onde una sopra l'altra
    "Nir": [_arco(0.0, 0.62, 0.62, 0.42, 200, 340), _arco(0.0, 0.12, 0.62, 0.42, 200, 340), _base(0.82)],
    # la terra: il monte
    "Dorn": [[(-0.58, BASE), (0.0, 0.85), (0.58, BASE)], _base(0.95)],
    # il vento: tre soffi, il primo con il ricciolo
    "Sil": [[(-0.52, 0.55)] + _arco(0.55, 0.72, 0.17, 0.17, -90, 150, 8),
            [(-0.82, 0.15), (0.82, 0.15)], [(-0.55, -0.3), (0.55, -0.3)], _base(0.82)],
    # la luce: il sole con quattro raggi
    "Lume": [_arco(0.0, 0.1, 0.3, 0.3, 0, 360, 16), [(0.0, 0.45), (0.0, 0.8)], [(0.0, -0.25), (0.0, -0.55)],
             [(-0.45, 0.1), (-0.78, 0.1)], [(0.45, 0.1), (0.78, 0.1)], _base(0.82)],
    # andare, scagliare: la freccia
    "Var": [[(-0.85, 0.0), (0.85, 0.0)], [(0.4, 0.45), (0.85, 0.0), (0.4, -0.45)]],
    # proteggere: il tetto sopra il punto
    "Ka": [_arco(0.0, -0.3, 0.72, 0.75, 170, 10), _punto(0.0, -0.5)],
    # guarire: la coppa che raccoglie
    "Ren": [[(-0.7, 0.6)] + _arco(0.0, 0.6, 0.62, 0.95, 195, 345)[1:-1] + [(0.7, 0.6)], _punto(0.0, 0.0)],
    # vedere, guidare: l'occhio
    "Vel": [_arco(0.0, -0.45, 0.85, 0.9, 150, 30), _arco(0.0, 0.45, 0.85, 0.9, 210, 330), _punto(0.0, 0.0, 0.13)],
    # fermare: la porta chiusa (la parola del muro)
    "Tor": [[(-0.85, 0.55), (0.85, 0.55)], [(-0.45, 0.55), (-0.45, -0.6)], [(0.45, 0.55), (0.45, -0.6)]],
    # Ur: l'arco del custode sopra tre colonne
    "Ur": [_arco(0.0, 0.05, 0.8, 0.75, 180, 0), [(-0.42, 0.25), (-0.42, -0.95)], [(0.0, 0.32), (0.0, -0.95)],
           [(0.42, 0.25), (0.42, -0.95)]],
}

assert set(FORME) == set(TUTTE)

# quanto è largo il posto di una runa e il trattino tra due rune di una parola (unità della runa)
PASSO = 2.0
GIUNTO = 0.9


def linee(nome):
    """Le linee spezzate di una runa (copia)."""
    return [list(l) for l in FORME[nome]]


def tratti(nome):
    """I tratti (coppie di punti) di una runa, per incidere."""
    fuori = []
    for l in FORME[nome]:
        fuori += list(zip(l[:-1], l[1:]))
    return fuori


def leggi(parola):
    """'Aeth-Var-Tor' -> ['Aeth', 'Var', 'Tor'] (maiuscole e minuscole uguali)."""
    nomi = {n.lower(): n for n in TUTTE}
    return [nomi[p.strip().lower()] for p in parola.replace(" ", "-").split("-") if p.strip()]


def tratti_parola(parola, verticale=False):
    """I tratti di una parola intera, centrata in (0, 0): le rune in fila unite da un trattino a metà altezza
    (in colonna, dall'alto in basso, se verticale). Unità: quelle della runa (alta 2)."""
    nomi = leggi(parola) if isinstance(parola, str) else list(parola)
    n = len(nomi)
    passo = PASSO + GIUNTO
    fuori = []
    for k, nome in enumerate(nomi):
        o = (k - (n - 1) / 2.0) * passo
        for (a, b) in tratti(nome):
            if verticale:
                fuori.append(((a[0], a[1] - o), (b[0], b[1] - o)))
            else:
                fuori.append(((a[0] + o, a[1]), (b[0] + o, b[1])))
        if k < n - 1:
            g0, g1 = o + PASSO / 2 - 0.05, o + PASSO / 2 + GIUNTO + 0.05
            if verticale:
                fuori.append(((0.0, -g0), (0.0, -g1)))
            else:
                fuori.append(((g0, 0.0), (g1, 0.0)))
    return fuori


def incidi(tratto, centro, normale, su, misura, parola, verticale=False):
    """Incide una parola (o una runa) su una faccia: chiama tratto(A, B) per ogni tratto, in centimetri.
    normale = verso chi legge, su = l'alto della scritta, misura = mezza altezza di una runa (cm).
    Attenzione al verso: le coordinate di Unreal sono sinistrorse (x est, y sud, z su), quindi per chi guarda la faccia
    la destra è normale x su (non su x normale come in un sistema destrorso): così la scritta non viene a specchio."""
    import numpy as np
    n = np.asarray(normale, np.float64)
    n = n / (np.linalg.norm(n) + 1e-12)
    u = np.asarray(su, np.float64)
    u = u - n * float(np.dot(u, n))
    u = u / (np.linalg.norm(u) + 1e-12)
    destra = np.cross(n, u)
    c = np.asarray(centro, np.float64)
    for (a, b) in tratti_parola(parola, verticale):
        tratto(c + destra * a[0] * misura + u * a[1] * misura, c + destra * b[0] * misura + u * b[1] * misura)


def valida(parola):
    """Un incantesimo: un elemento e poi uno o due verbi diversi; Ur mai."""
    nomi = leggi(parola) if isinstance(parola, str) else list(parola)
    return (2 <= len(nomi) <= 3 and nomi[0] in ELEMENTI and all(v in VERBI for v in nomi[1:])
            and len(set(nomi[1:])) == len(nomi) - 1)


# ----------------------------------------------------------------------------------------------- per il C++
def scrivi_inl(percorso):
    righe = ["// Valdorso - La forma delle rune. FILE GENERATO da Tools/Generatori/alfabeto_rune.py: non toccarlo a mano,",
             "// cambia le forme lì e rilancia: python alfabeto_rune.py Source/Valdorso",
             "// Per ogni runa (nell'ordine di EValdorsoRuna): le sue linee spezzate; ogni linea = numero di punti, poi x, y.",
             "// Coordinate da -1 a 1, y in su. La runa finisce con 0.",
             "static const float GValdorsoRuneDati[] =", "{"]
    for nome in TUTTE:
        pezzi = []
        for l in FORME[nome]:
            pezzi.append("%d, %s" % (len(l), ", ".join("%.4ff, %.4ff" % (x, y) for x, y in l)))
        righe.append("\t// %s (%s)" % (nome, SIGNIFICATI[nome]))
        for p in pezzi:
            righe.append("\t%s," % p)
        righe.append("\t0,")
    righe.append("};")
    righe.append("")
    righe.append("// Le parole con un nome, dalle Cronache.")
    righe.append("static const TCHAR* const GValdorsoRuneParole[][2] =")
    righe.append("{")
    for p, n in PAROLE.items():
        righe.append("\t{ TEXT(\"%s\"), TEXT(\"%s\") }," % (p, n))
    righe.append("};")
    with open(percorso, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(righe) + "\n")


def anteprima(percorso, lato=120):
    from PIL import Image, ImageDraw
    colonne = 6
    file_ = (len(TUTTE) + colonne - 1) // colonne
    img = Image.new("RGB", (colonne * lato * 2, file_ * lato * 2 + 260), (238, 226, 200))
    d = ImageDraw.Draw(img)
    s = lato * 0.62

    def disegna(segmenti, cx, cy, scala, spessore):
        for (a, b) in segmenti:
            d.line([(cx + a[0] * scala, cy - a[1] * scala), (cx + b[0] * scala, cy - b[1] * scala)],
                   fill=(122, 58, 18), width=spessore)

    for k, nome in enumerate(TUTTE):
        cx = (k % colonne) * lato * 2 + lato
        cy = (k // colonne) * lato * 2 + lato * 0.85
        disegna(tratti(nome), cx, cy, s, 7)
        d.text((cx - 18, cy + s + 18), nome, fill=(40, 25, 15))
    y = file_ * lato * 2 + 80
    for k, parola in enumerate(["Aeth-Var", "Nir-Ren-Ka", "Lume-Vel-Tor"]):
        cx = 260 + k * 470
        disegna(tratti_parola(parola), cx, y, 38, 5)
        d.text((cx - 60, y + 60), "%s - %s" % (parola, PAROLE[parola]), fill=(40, 25, 15))
    img.save(percorso)


if __name__ == "__main__":
    cartella = sys.argv[1] if len(sys.argv) > 1 else "."
    scrivi_inl(os.path.join(cartella, "ValdorsoRuneTratti.inl"))
    print("scritto", os.path.join(cartella, "ValdorsoRuneTratti.inl"))
    if len(sys.argv) > 2:
        anteprima(sys.argv[2])
        print("anteprima", sys.argv[2])
