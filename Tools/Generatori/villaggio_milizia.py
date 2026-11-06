# Valdorso - La milizia della cittadina, a fasi (v0.1.3). Scritto da Claude il 06/11/2026 (richiesta di Fra: "una zona
# per la milizia della cittadina, sempre a fasi, iniziando da piccola fino alla 3, una bella caserma e campo, con letti").
#
#   Fase 1  (c'è già: la torre della milizia alla porta di nord-est e la casa della milizia) + il cortile: due fantocci
#           di paglia e una rastrelliera di lance tra la casa e la palizzata.
#   Fase 2  la zona della milizia fuori dalla palizzata di oggi, tra le due strade di est (grotta e passo): una caserma
#           piccola (16 x 8 m, 5 brande, un tavolo con le panche) e un campo d'addestramento recintato (22 x 16 m:
#           quattro fantocci, due bersagli per l'arco, la rastrelliera), con una lanterna.
#   Fase 3  sullo stesso posto, più grande: la caserma di pietra a due piani (30 x 10 m, 18 brande al piano terra, due
#           tavoli con le panche), l'armeria, il campo grande (34 x 18 m: sei fantocci, il tiro con l'arco con tre
#           bersagli, due rastrelliere, lo stendardo), le lanterne.
# Il posto della fase 3 è tenuto libero già dalla fase 2 (le case dell'anello non lo prendono).
# Coordinate: metri dal centro della piazza (x est, y sud), come L() di villaggio.py.

import math

import numpy as np

import villaggio_edifici as ed
from villaggio_mesh import Mesh

# il posto: tutta la zona (fase 3) è larga 38 m (x) e profonda 30 m (y), intorno al centro scelto
ZONA_W, ZONA_D = 38.0, 30.0
# dove cercarla: tra le due strade di est, fuori dalla palizzata di oggi e prima della via dell'anello
CANDIDATI = [(158.0, -8.0), (160.0, -4.0), (156.0, -12.0), (162.0, -10.0), (154.0, -6.0), (164.0, -6.0),
             (158.0, 0.0), (152.0, -10.0), (166.0, -2.0)]


# ------------------------------------------------------------------------------------------------ le cose del campo

def fantoccio(m, x, y, z, gradi=0.0):
    """Un fantoccio di paglia per allenarsi: palo, braccia di legno, corpo di paglia legato, testa di sacco."""
    a = math.radians(gradi)
    u = np.array([math.cos(a), math.sin(a), 0.0])
    m.cilindro("Trave", [x, y, z - 50], 8, 230, lati=6)
    m.trave("Trave", [x - u[0] * 70, y - u[1] * 70, z + 140], [x + u[0] * 70, y + u[1] * 70, z + 140], 8, 8)
    m.cilindro("Paglia", [x, y, z + 80], 26, 85, lati=8, raggio_alto=22)
    m.cilindro("Stoffa", [x, y, z + 118], 27, 8, lati=8)          # la corda
    m.cilindro("Stoffa", [x, y, z + 165], 15, 28, lati=8, raggio_alto=11)


def bersaglio(m, x, y, z, gradi=0.0):
    """Un bersaglio per l'arco: un disco di paglia su un cavalletto, girato verso chi tira (gradi)."""
    a = math.radians(gradi)
    v = np.array([math.cos(a), math.sin(a), 0.0])           # verso chi tira
    s = np.array([-v[1], v[0], 0.0])                        # di lato
    c = np.array([x, y, z + 120])
    m.cilindro("Paglia", list(c - v * 10), 55, 20, lati=16, asse=tuple(v))
    m.cilindro("Stoffa", list(c + v * 10.5), 18, 1.5, lati=12, asse=tuple(v))     # il centro
    for k in (-1, 1):
        piede = np.array([x, y, z]) + s * k * 45 - v * 35
        m.trave("Trave", list(piede), list(c + s * k * 30 - v * 12), 6, 6)
    m.trave("Trave", [x, y, z], list(c - v * 15), 6, 6)


def rastrelliera(m, x, y, z, gradi=0.0, lance=5):
    """La rastrelliera delle lance (come quella del posto di guardia del passo)."""
    a = math.radians(gradi)
    u = np.array([math.cos(a), math.sin(a), 0.0])
    w = np.array([-u[1], u[0], 0.0])
    o = np.array([x, y, z])
    for k in range(lance):
        p = o + u * (-(lance - 1) * 50 + k * 100)
        m.trave("Trave", list(p), list(p + w * 30 + [0, 0, 190]), 6, 6)
        m.cilindro("Ferro", list(p + w * 31 + [0, 0, 190]), 3, 22, lati=4, raggio_alto=0.5)
    m.trave("Trave", list(o - u * (lance * 50) + [0, 0, 150]), list(o + u * (lance * 50) + [0, 0, 150]), 8, 8)
    for s in (-1, 1):
        p = o + u * s * lance * 50
        m.cilindro("Trave", list(p + [0, 0, -30]), 6, 190, lati=5)


def stendardo(m, x, y, z):
    m.cilindro("Trave", [x, y, z - 60], 9, 760, lati=6)
    m.scatola("Stoffa", [x, y + 90, z + 600], [1.5, 0, 0], [0, 85, 0], [0, 0, 110])


# ------------------------------------------------------------------------------------------------ fase 1

def cortile_fase1(pr, S, L, t):
    """Il cortile della milizia di oggi: tra la casa della milizia (103, -4) e la palizzata (x 114)."""
    X, Y = L(111.0, -6.0)
    q = t.quota(X, Y)
    m = Mesh("SM_Milizia_Cortile")
    for k, (dx, dy) in enumerate(((0.0, -3.5), (0.0, 0.5))):
        p = L(111.0 + dx, -6.0 + dy)
        fantoccio(m, p[0] - X, p[1] - Y, t.quota(*p) - q, 90.0)
    p = L(112.6, 2.2)
    rastrelliera(m, p[0] - X, p[1] - Y, t.quota(*p) - q, 90.0, lance=3)
    pr.al_suolo(S, "Cortile della milizia", m, X, Y, q, collisione=True)
    pr.disegni.append((S, "edificio", [L(109.5, -10.5), L(113.5, -10.5), L(113.5, 3.5), L(109.5, 3.5)], "Cortile della milizia"))


# ------------------------------------------------------------------------------------------------ fasi 2 e 3

def zona(pr, S, L, t, mappa, lastricato, rettangolo, dentro, poli_largo, poli_oggi):
    """Sceglie il posto (libero, piano, tra la palizzata di oggi e la cinta larga), lo tiene libero e mette le cose
    delle fasi 2 e 3. Restituisce il centro (metri) o None."""
    scelto = None
    for c in CANDIDATI:
        rett = rettangolo(c, ZONA_W, ZONA_D, 90.0)
        if not all(dentro(p, poli_largo) and not dentro(p, poli_oggi) for p in rett):
            continue
        if not mappa.libero(rett):
            continue
        X, Y = L(*c)
        qmin, qmax = t.sotto(X, Y, ZONA_W * 100, ZONA_D * 100, 0.0)
        if qmax - qmin > 350:
            continue
        scelto = c
        break
    if scelto is None:
        pr.avviso("milizia: nessun posto libero per la zona della fase 2-3")
        return None
    cx, cy = scelto
    mappa.occupa(rettangolo(scelto, ZONA_W, ZONA_D, 90.0), margine=3.0)
    fase_prima = pr.fase

    def M(x, y):
        """Un punto della zona (metri dal centro della zona) nel mondo (cm)."""
        return L(cx + x, cy + y)

    # ---------------------------------------------------------------- fase 2 (solo fase 2)
    pr.fase = (2, 2)
    X, Y = M(-9.0, -10.0)
    pav, gr, info = pr.edificio(S, "Caserma_della_milizia",
                                lambda nome, W, D, **kw: caserma(nome, W, D, kw["fondazione"], piani=1, file=1),
                                X, Y, 90.0, 1600, 800, chiave=False)
    _tavoli(pr, S, "Tavolo della caserma", X, Y, gr, pav, info)
    _campo(pr, S, M, t, lastricato, "SM_Milizia_Campo2", "Campo della milizia", (8.0, 3.0), 22.0, 16.0, fantocci=4, bersagli=2,
           rastrelliere=1, stendardo_=False)
    _lanterne(pr, S, M, t, "SM_Milizia_Lanterne2", "Lanterna della caserma", [(-9.0, -4.6)])
    # due botti e una cassa davanti alla caserma
    for k, (x, y) in enumerate(((-15.5, -4.8), (-14.7, -4.5), (-2.5, -4.8))):
        pr.oggetto(S, "Botte della milizia %d" % k, ["/Game/Fab/Mercato_Tenda2/wooden_barrel", "/Game/Fab/Mercato_Tenda2/wooden_barrel",
                                                     "/Game/Fab/Mercato_Tenda1/Crate_003"][k], *M(x, y), 15.0 * k)

    # ---------------------------------------------------------------- fase 3 (per sempre)
    pr.fase = (3, 0)
    X, Y = M(-4.0, -10.0)
    pav, gr, info = pr.edificio(S, "Caserma_grande_della_milizia",
                                lambda nome, W, D, **kw: caserma(nome, W, D, kw["fondazione"], piani=2, file=2),
                                X, Y, 90.0, 3000, 1000, chiave=False)
    _tavoli(pr, S, "Tavolo della caserma grande", X, Y, gr, pav, info)
    X, Y = M(15.0, -11.0)
    pr.edificio(S, "Armeria_della_milizia", ed.casa, X, Y, 90.0, 800, 600, chiave=True, piani=1, base="Pietra",
                sopra="Pietra", tetto="Coppi", pendenza=30, seme=4401, camino=False)
    _campo(pr, S, M, t, lastricato, "SM_Milizia_Campo3", "Campo grande della milizia", (0.0, 5.5), 34.0, 18.0, fantocci=6,
           bersagli=3, rastrelliere=2, stendardo_=True)
    _lanterne(pr, S, M, t, "SM_Milizia_Lanterne3", "Lanterna della caserma grande", [(-12.0, -4.0), (4.0, -4.0), (17.0, -7.4)])
    for k, (x, y) in enumerate(((-18.0, -4.3), (-17.2, -4.0), (12.0, -7.6), (12.8, -7.8))):
        pr.oggetto(S, "Botte della caserma %d" % k, ["/Game/Fab/Mercato_Tenda2/wooden_barrel", "/Game/Fab/Mercato_Tenda1/Crate_003"][k % 2],
                   *M(x, y), 20.0 * k)
    pr.fase = fase_prima
    print("milizia: zona a (%.0f, %.0f) m dalla piazza" % (cx, cy))
    return scelto


def caserma(nome, W, D, fondazione, piani=1, file=1):
    """La caserma: una casa lunga con la porta in mezzo alla facciata; dentro le brande in fila contro il muro di fondo
    (e, con file=2, anche contro la facciata, lasciando libera la porta), tavoli e panche nel mezzo."""
    m, info = ed.casa(nome, W, D, piani=piani, base="Pietra", sopra="Intonaco" if piani > 1 else "Pietra", tetto="Coppi",
                      pendenza=30, fondazione=fondazione, porta_x=0.0, finestre_aperte=False, seme=4400 + int(W), camino=True)
    passo = 260.0
    n = int((W - 200) // passo)
    xs = [(-(n - 1) / 2 + k) * passo for k in range(n)]
    cose = []
    for x in xs:
        cose.append(("letto", x, D / 2 - 150, 90.0, 0.0))
    if file >= 2:
        for x in xs:
            if abs(x) > 260:                       # davanti alla porta niente brande
                cose.append(("letto", x, -D / 2 + 150, 90.0, 0.0))
    # tavoli (li mette villaggio.py come i tavoli della locanda) e panche
    tavoli = [(0.0, 0.0)] if file < 2 else [(-600.0, 0.0), (600.0, 0.0)]
    for tx, ty in tavoli:
        cose.append(("panca", tx, ty - 90, 0.0))
        cose.append(("panca", tx, ty + 90, 180.0))
    info["cose"] = info.get("cose", []) + cose
    info["tavoli"] = tavoli
    return m, info


def _tavoli(pr, S, nome, X, Y, gradi, pavimento, info):
    for k, (lx, ly) in enumerate(info.get("tavoli", [])):
        px, py = pr.al_mondo(X, Y, gradi, lx, ly)
        pr.oggetto(S, "%s %d" % (nome, k), "/Game/Fab/Mercato_Tenda1/table_002", px, py, gradi, z=pavimento)


def _campo(pr, S, M, t, lastricato, nome_mesh, nome, centro, W, D, fantocci, bersagli, rastrelliere, stendardo_):
    """Il campo d'addestramento: terra battuta, staccionata con l'entrata a ovest, fantocci a est, bersagli in fondo."""
    cx, cy = centro
    X0, Y0 = M(cx, cy)
    q0 = t.quota(X0, Y0)
    m = Mesh(nome_mesh)
    # la terra battuta
    lastricato(m, t, X0, Y0, q0, X0, Y0, W * 50 - 50, D * 50 - 50, mat="Terra", sfuma=150, alza=4, angoli=150)
    pr.zone_senza_erba.append((X0, Y0, W * 50, D * 50))

    def loc(x, y):
        p = M(cx + x, cy + y)
        return p[0] - X0, p[1] - Y0, t.quota(*p) - q0

    # la staccionata: un giro con l'entrata (4 m) a metà del lato ovest
    giro = [(-W / 2, 2.0), (-W / 2, D / 2), (W / 2, D / 2), (W / 2, -D / 2), (-W / 2, -D / 2), (-W / 2, -2.0)]
    m.unisci(ed.recinto("x", [loc(*p) for p in giro]))
    # i fantocci: in fila, a ovest della metà
    for k in range(fantocci):
        fx = -W / 2 + 4.0 + (k % 3) * 3.5
        fy = -D / 4 + (k // 3) * (D / 2)
        fantoccio(m, *loc(fx, fy), 0.0)
    # i bersagli in fondo a est, girati verso ovest (chi tira sta a ovest)
    for k in range(bersagli):
        by = (-(bersagli - 1) / 2 + k) * 4.0
        bersaglio(m, *loc(W / 2 - 2.5, by), 180.0)
    for k in range(rastrelliere):
        rastrelliera(m, *loc(-2.0 + k * 6.0, -D / 2 + 1.2), 0.0)
    if stendardo_:
        stendardo(m, *loc(-W / 2 + 1.5, -3.5))
    pr.al_suolo(S, nome, m, X0, Y0, q0, collisione=True)
    pr.disegni.append((S, "linea", [M(cx + p[0], cy + p[1]) for p in giro], nome))
    # una panca per chi guarda
    p = M(cx - W / 2 + 2.0, cy + D / 2 - 1.5)
    pr.aggiungi(S, nome + " panca", "panca", p[0], p[1], t.quota(*p), 0.0)


def _lanterne(pr, S, M, t, nome_mesh, nome, punti):
    X0, Y0 = M(0.0, 0.0)
    q0 = t.quota(X0, Y0)
    lan = Mesh(nome_mesh)
    luci = []
    for (x, y) in punti:
        X, Y = M(x, y)
        luci.append(ed.lanterna(lan, X - X0, Y - Y0, t.quota(X, Y) - q0))
    pr.al_suolo(S, nome, lan, X0, Y0, q0, collisione=True)
    for k, (lx, ly, lz) in enumerate(luci):
        pr.aggiungi(S, "%s luce %d" % (nome, k), "luce", X0 + lx, Y0 + ly, q0 + lz, intensita=4.0, raggio=1000.0)
