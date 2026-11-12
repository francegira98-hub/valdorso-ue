# Valdorso - La milizia della cittadina, a fasi (v0.1.3). Scritto da Claude il 06/11/2026 (richiesta di Fra: "una zona
# per la milizia della cittadina, sempre a fasi, iniziando da piccola fino alla 3, una bella caserma e campo, con letti").
#
#   Fase 1  (c'è già: la torre della milizia alla porta di nord-est e la casa della milizia) + il cortile: due fantocci
#           di paglia e una rastrelliera di lance tra la casa e la palizzata.
#   Fase 2  la zona della milizia fuori dalla palizzata di oggi, tra le due strade di est (grotta e passo): una caserma
#           piccola (16 x 8 m, 5 brande, un tavolo con le panche) e un campo d'addestramento recintato (22 x 16 m:
#           quattro fantocci, due bersagli per l'arco, la rastrelliera), con una lanterna.
#   Fase 3  sullo stesso posto, più grande. (12/11, rifatta: chiesta da Fra, "letti messi a caso, coprono l'ingresso;
#           voglio strutture belle, non il solito quadrato") Un quartiere vero, a ferro di cavallo intorno al campo:
#           - a ovest l'ala degli alloggi (17,5 x 9 m, due piani) con il portico di legno sul cortile: dentro la
#             camerata, le brande in due file di testa contro i muri lunghi, ognuna con il suo baule ai piedi, il
#             corridoio libero nel mezzo e l'entrata libera (la porta è in fondo, verso la torre);
#           - nel mezzo la torre di guardia (5 x 5 m, 11 m, con la scala a pioli): il perno tra le due ali;
#           - a est la mensa (11 x 8 m) con il focolare, il bancone e due tavoli;
#           - sul lato est del campo, girata verso il campo, l'armeria;
#           - il campo grande (25 x 15,5 m: sei fantocci, tre bersagli, due rastrelliere, lo stendardo), le lanterne.
#           Le porte di alloggi e mensa sono tutte e due vicino alla torre: chi esce si trova sul cortile.
# Il posto della fase 3 è tenuto libero già dalla fase 2 (le case dell'anello non lo prendono).
# Coordinate: metri dal centro della piazza (x est, y sud), come L() di villaggio.py.

import math

import numpy as np

import villaggio_edifici as ed
from villaggio_mesh import Mesh, normalizza

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
    # (12/11) rifatta: alloggi con il portico a ovest, la torre nel mezzo, la mensa a est, l'armeria sul lato del campo
    pr.fase = (3, 0)
    X, Y = M(-10.0, -10.5)
    pr.edificio(S, "Alloggi_della_milizia", lambda nome, W, D, **kw: alloggi(nome, W, D, kw["fondazione"]),
                X, Y, 90.0, 1750, 900, chiave=False)
    X, Y = M(1.5, -11.0)
    pr.edificio(S, "Torre_di_guardia_della_milizia",
                lambda nome, W, D, **kw: ed.torre(nome, lato=W, altezza=1100, fondazione=kw["fondazione"]),
                X, Y, 90.0, 500, 500, chiave=False)
    X, Y = M(10.5, -11.0)
    pav, gr, info = pr.edificio(S, "Mensa_della_milizia", lambda nome, W, D, **kw: mensa(nome, W, D, kw["fondazione"]),
                                X, Y, 90.0, 1100, 800, chiave=False)
    _tavoli(pr, S, "Tavolo della mensa", X, Y, gr, pav, info)
    X, Y = M(16.8, 4.5)
    pr.edificio(S, "Armeria_della_milizia", ed.casa, X, Y, 180.0, 700, 450, chiave=True, piani=1, base="Pietra",
                sopra="Pietra", tetto="Coppi", pendenza=30, seme=4401, camino=False)
    _campo(pr, S, M, t, lastricato, "SM_Milizia_Campo3", "Campo grande della milizia", (0.0, 7.1), 25.0, 15.5, fantocci=6,
           bersagli=3, rastrelliere=2, stendardo_=True)
    _lanterne(pr, S, M, t, "SM_Milizia_Lanterne3", "Lanterna della caserma grande",
              [(-14.0, -1.9), (-4.0, -1.9), (6.0, -6.2), (14.6, -0.2)])
    for k, (x, y) in enumerate(((-18.3, -1.9), (-17.5, -1.6), (16.6, -6.0), (17.3, -5.7))):
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


BRANDA_L, BRANDA_P = 140.0, 232.0      # il letto di Fab (BP_Letto): 140 cm di larghezza, 232 di lunghezza (Y locale)


def baule(m, x, y, gradi=0.0):
    """Il baule di un soldato ai piedi della branda: assi, coperchio, due cerchi di ferro (90 x 50 x 50 cm)."""
    a = math.radians(gradi)
    u = np.array([math.cos(a), math.sin(a), 0.0]) * 45
    v = np.array([-math.sin(a), math.cos(a), 0.0]) * 25
    m.scatola({"altri": "Assi", "+z": "Trave"}, [x, y, 22], u, v, [0, 0, 22])
    m.scatola("Trave", [x, y, 47], u * 1.04, v * 1.08, [0, 0, 4])
    for s_ in (-0.6, 0.6):
        m.scatola("Ferro", [x + u[0] * s_, y + u[1] * s_, 26], u * 0.04, v * 1.06, [0, 0, 25])


def portico(m, x0, x1, y_muro, prof, h_muro, h_fuori, fondazione, passo=300.0, porta_x=None):
    """Un portico di legno appoggiato al muro (la facciata è a -Y): pavimento di pietra rialzato, pali sul davanti,
    la trave, il tetto a una falda di coppi che scende dal muro verso fuori."""
    yf = y_muro - prof                                   # il filo dei pali
    m.blocco({"altri": "Pietra", "+z": "Pietra"}, x0, x1, yf - 30, y_muro, -fondazione, 0)
    m.blocco("Pietra", x0, x1, yf - 70, yf - 30, -fondazione, -22)     # il gradino lungo
    # i pali: uno ogni 'passo', con la porta (se c'è) a metà tra due pali, e sempre un palo ai due capi
    a0, a1 = x0 + 20, x1 - 20
    if porta_x is None:
        n = max(2, int(round((a1 - a0) / passo)) + 1)
        pali = [a0 + (a1 - a0) * k / (n - 1) for k in range(n)]
    else:
        pali = []
        x = porta_x - passo / 2
        while x - passo >= a0:
            x -= passo
        while x <= a1:
            if x >= a0:
                pali.append(x)
            x += passo
        if not pali:
            pali = [a0, a1]
        if pali[0] - a0 > 100:
            pali.insert(0, a0)
        else:
            pali[0] = a0
        if a1 - pali[-1] > 100:
            pali.append(a1)
        else:
            pali[-1] = a1
    for x in pali:
        m.blocco("Pietra", x - 22, x + 22, yf - 22, yf + 22, 0, 18)    # la base del palo
        m.cilindro("Trave", [x, yf, 18], 13, h_fuori - 18, lati=8)
        m.trave("Trave", [x, yf, h_fuori - 60], [x, yf + 70, h_fuori], 10, 10)  # il saettone
    m.trave("Trave", [x0, yf, h_fuori + 10], [x1, yf, h_fuori + 10], 20, 22)
    a = np.array([0.0, yf - 45, h_fuori + 10 - 45 * (h_muro - h_fuori) / prof])
    b = np.array([0.0, y_muro, h_muro])
    su = normalizza(b - a)
    nn = normalizza(np.cross([1.0, 0, 0], su))
    if nn[2] < 0:
        nn = -nn
    m.scatola({"+z": "Coppi", "-z": "Assi", "altri": "Trave"}, (a + b) / 2 + nn * 8 + [(x0 + x1) / 2, 0, 0],
              [(x1 - x0) / 2 + 20, 0, 0], su * np.linalg.norm(b - a) / 2, nn * 8)
    # i travetti sotto il tetto, uno per palo
    for x in pali:
        m.trave("Trave", [x, yf, h_fuori + 4], [x, y_muro, h_muro - 14], 10, 14)


def lume_appeso(m, cose, x, y, soffitto, intensita=5.0, raggio=900.0):
    """(12/11) Un lume appeso al soffitto: la catena, la gabbia di ferro con la fiamma; e la sua luce calda."""
    z = soffitto - 75                                   # la fiamma a 2 m e più: non si sbatte la testa
    m.trave("Ferro", [x, y, soffitto - 15], [x, y, z + 30], 3, 3)
    m.blocco("Ferro", x - 16, x + 16, y - 16, y + 16, z + 26, z + 30)
    m.blocco("Ferro", x - 14, x + 14, y - 14, y + 14, z - 6, z - 2)
    for sx in (-1, 1):
        for sy in (-1, 1):
            m.blocco("Ferro", x + sx * 14 - 2, x + sx * 14 + 2, y + sy * 14 - 2, y + sy * 14 + 2, z - 2, z + 26)
    m.blocco("Fiamma", x - 5, x + 5, y - 5, y + 5, z, z + 14)
    cose.append(("luce", x, y, 0.0, z + 8, {"intensita": intensita, "raggio": raggio}))


def alloggi(nome, W, D, fondazione):
    """(12/11) L'ala degli alloggi: due piani, la porta in fondo verso la torre, il portico sul cortile. Dentro, la
    camerata: brande di testa contro i muri lunghi, un baule ai piedi di ognuna, il corridoio libero nel mezzo; davanti
    alla porta niente brande, e accanto alla porta, contro il muro di fondo, la rastrelliera delle lance."""
    T = ed.SPESSORE
    # (la facciata guarda il cortile e la casa è girata di 180 gradi: il +X di qui è l'ovest, quindi la porta, verso la
    # torre a est, sta a -X)
    porta_x = -W / 2 + 220.0
    m, info = ed.casa(nome, W, D, piani=2, base="Pietra", sopra="Intonaco", tetto="Coppi", pendenza=32,
                      fondazione=fondazione, porta_x=porta_x, finestre_aperte=False, seme=4402, camino=True)
    portico(m, -W / 2 + 30, W / 2 - 30, -D / 2, 250.0, 290.0, 250.0, fondazione, porta_x=porta_x)
    cose = []
    yb = D / 2 - T - BRANDA_P / 2 - 6            # la fila contro il muro di fondo (testa al muro)
    yf = -yb                                      # la fila contro la facciata
    passo = BRANDA_L + 95.0
    x = W / 2 - T - BRANDA_L / 2 - 45
    while x - BRANDA_L / 2 > -W / 2 + T + 330:  # dal fondo della camerata verso la porta, fino alla rastrelliera
        cose.append(("letto", x, yb, 180.0, 0.0))
        baule(m, x, yb - BRANDA_P / 2 - 32)
        if x - BRANDA_L / 2 > porta_x + 260:     # la facciata: libera per 2,6 m davanti alla porta
            cose.append(("letto", x, yf, 0.0, 0.0))
            baule(m, x, yf + BRANDA_P / 2 + 32)
        x -= passo
    rastrelliera(m, -W / 2 + T + 170, D / 2 - T - 12, 0, 180.0, lance=3)
    for lx in (-W / 2 + 330, 0.0, W / 2 - 330):        # (12/11) i lumi del corridoio: la camerata non è più buia
        lume_appeso(m, cose, lx, 0.0, 300.0 - 22)
    info["cose"] = info.get("cose", []) + cose
    return m, info


def mensa(nome, W, D, fondazione):
    """(12/11) La mensa della milizia: un piano alto, la porta verso la torre, il focolare contro il muro di fianco,
    il bancone della cucina contro il muro di fondo, due tavoli con le panche."""
    T = ed.SPESSORE
    # (girata di 180 gradi come gli alloggi: +X è l'ovest, la torre; il focolare sta a -X, sul fianco est)
    m, info = ed.casa(nome, W, D, piani=1, base="Pietra", sopra="Pietra", tetto="Coppi", pendenza=36, h1=360.0,
                      fondazione=fondazione, porta_x=W / 2 - 200, finestre_aperte=False, seme=4403, camino=False,
                      finestre_lato=False)
    xc = -W / 2 + T
    m.blocco("Pietra", xc, xc + 70, -110, 110, 0, 120)                 # il focolare
    m.blocco("Fiamma", xc + 20, xc + 60, -50, 50, 40, 70)
    m.blocco("Pietra", xc, xc + 80, -130, 130, 120, 140)                # la mensola
    m.blocco("Pietra", xc, xc + 60, -90, 90, 140, 360)                  # la cappa
    cima = info["cima"]
    # (il comignolo sta a 150 cm dal bordo del tetto, dove lo cerca il fumo di ValdorsoVitaValle)
    m.blocco({"altri": "Pietra", "+z": "Ferro"}, xc - 20, xc + 80, -55, 55, cima - 15, cima + D / 2 * math.tan(math.radians(36)) + 90)
    ed.bancone(m, -W / 2 + 220, W / 2 - 320, D / 2 - T - 50)
    tavoli = [(-230.0, -60.0), (150.0, -60.0)]
    cose = []
    for tx, ty in tavoli:                              # (12/11) un lume sopra ogni tavolo
        lume_appeso(m, cose, tx, ty, 360.0, intensita=4.0, raggio=800.0)
    # e la luce del focolare, arancione, che fa ombra
    cose.append(("luce", xc + 45, 0.0, 0.0, 70.0, {"intensita": 6.0, "raggio": 700.0, "colore": [1.0, 0.45, 0.15], "ombre": True}))
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
