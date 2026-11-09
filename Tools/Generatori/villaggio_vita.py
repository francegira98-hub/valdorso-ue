# Valdorso - Il villaggio vivo (v0.1.4, 09/11/2026, chiesto da Fra: "la citta' va sistemata essendo spoglia").
# Scritto da Claude. Le cose di tutti i giorni intorno alle case della fase 1 (quelle di oggi):
#   accanto alle porte: botti, casse, ceste, secchi, vasi di fiori, scope (mai davanti alla porta, mai sulle panche);
#   lungo un fianco della casa: la catasta della legna sotto la gronda; dietro: il bucato steso fra due pali, le aiuole;
#   le insegne appese sopra le porte delle botteghe (locanda: il boccale; fucina: l'incudine; Gilda: le chiavi
#   incrociate; falegname: la sega; conceria: la pelle; mulino: la ruota; stalla: il ferro di cavallo);
#   alla stalla, ai fienili e alla fattoria: l'abbeveratoio, i carri, le balle di fieno;
#   lungo le strade del villaggio le lanterne sui pali (con la luce vera), lontane da quelle della piazza.
# I meli negli orti e dietro le case li mette il bosco (bosco.py). Il fumo dai camini: piu' avanti (roadmap).
# Ogni casa ha un suo pezzo "SM_Vita_<casa>" (le cose seguono il terreno), nella sezione "Vita": crea_villaggio.py
# la puo' rifare da sola (menu Valdorso -> "Villaggio: rifai solo la vita nelle strade").
# Lo chiama villaggio.py (main); sempre uguale (seme fisso).

import math

import numpy as np

from villaggio_mesh import Mesh, normalizza

SEZIONE = "Vita"

FAB_CASSE = ["/Game/Fab/Mercato_Tenda1/Crate_003", "/Game/Fab/Mercato_Tenda1/Crate_03_007"]
FAB_CESTE = ["/Game/Fab/Mercato_Tenda1/vegetable_box_01_005", "/Game/Fab/Mercato_Tenda1/vegetable_box_03_006"]
FAB_SECCHIO = "/Game/Fab/Mercato_Tenda1/wooden_bucket_01_005"
FAB_VASI = ["/Game/Fab/Mercato_Tenda1/Pot_01_006", "/Game/Fab/Mercato_Tenda1/Pot_02_013"]

INSEGNE = {"Locanda_di_Bruno": "boccale", "Fucina_di_Marta": "incudine", "Gilda": "chiavi", "Falegname": "sega",
           "Conceria": "pelle", "Mulino": "ruota", "Stalla_della_porta": "ferro"}
CAMPAGNA = ("Stalla_della_porta", "Fienile_fattoria", "Fattoria_Est", "Fienile_Est")

FIORI = [(0.75, 0.12, 0.10), (0.92, 0.78, 0.20), (0.92, 0.90, 0.85), (0.55, 0.25, 0.60), (0.95, 0.45, 0.15)]
PANNI = [(0.85, 0.83, 0.76), (0.62, 0.20, 0.15), (0.30, 0.38, 0.55), (0.75, 0.68, 0.50), (0.45, 0.50, 0.30)]


# ------------------------------------------------------------------------------------------------ pezzi colorati

def colorato(m, colore, disegna):
    """Disegna con 'disegna(tmp)' e copia in m con il colore dei vertici (per M_VillaggioColore: Canna e' bianca)."""
    tmp = Mesh("tmp")
    disegna(tmp)
    for mat, parte in tmp.parti.items():
        inizio = 0
        for P, N, UV, C, T in zip(*[parte[k] for k in range(5)]):
            m.triangoli(mat, P, N, UV, np.asarray(T) - inizio, [list(colore) + [1.0]] * len(P))
            inizio += len(P)


def _base(gradi):
    a = math.radians(gradi)
    return np.array([math.cos(a), math.sin(a), 0.0]), np.array([-math.sin(a), math.cos(a), 0.0])


def botte(m, c, gradi=0.0):
    c = np.asarray(c, np.float64)
    m.cilindro("Assi", c, 30, 47, lati=12, raggio_alto=36, tappo=False)
    m.cilindro("Assi", c + [0, 0, 47], 36, 48, lati=12, raggio_alto=30, tappo=False)
    m.cilindro("Trave", c + [0, 0, 92], 29.5, 1, lati=12)
    for z0, r0, r1 in ((8, 31.2, 32.6), (78, 32.6, 31.2)):
        m.cilindro("Ferro", c + [0, 0, z0], r0 + 0.8, 7, lati=12, raggio_alto=r1 + 0.8, tappo=False)


def vaso_fiori(m, c, rng, r=24.0):
    c = np.asarray(c, np.float64)
    m.cilindro("Coppi", c, r * 0.75, r * 1.3, lati=10, raggio_alto=r, tappo=True)
    m.cilindro("Terra", c + [0, 0, r * 1.25], r * 0.92, 2, lati=10)
    verde = (0.18, 0.32, 0.10)
    colore = FIORI[int(rng.integers(len(FIORI)))]
    for _ in range(9):
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(0, r * 0.8)
        b = c + [math.cos(a) * d, math.sin(a) * d, r * 1.3]
        h = rng.uniform(14, 30)
        colorato(m, verde, lambda t, b=b, h=h: t.cilindro("Canna", b, 1.2, h, lati=4, tappo=False))
        fiore = b + [0, 0, h]
        colorato(m, colore, lambda t, f=fiore: t.cilindro("Canna", f - [0, 0, 3], 5.5, 6, lati=6, raggio_alto=1.0))
    for _ in range(6):
        a = rng.uniform(0, 2 * math.pi)
        b = c + [math.cos(a) * r * 0.5, math.sin(a) * r * 0.5, r * 1.3]
        d = np.array([math.cos(a), math.sin(a), 0.6])
        colorato(m, verde, lambda t, b=b, d=d: t.trave("Canna", b, b + d * 18, 7, 1.5))


def aiuola(m, a, b, rng, quota):
    """Una striscia di fiori lungo un muro, da a a b (punti sul terreno), con il bordo di sassi."""
    a, b = np.asarray(a, np.float64), np.asarray(b, np.float64)
    L = np.linalg.norm(b[:2] - a[:2])
    if L < 50:
        return
    u = normalizza(b - a)
    for k in range(int(L / 22)):
        p = a + u * (k * 22 + 11)
        p[2] = quota(p)
        m.scatola("Pietra", p + [0, 0, 4], u * 10, normalizza(np.cross([0, 0, 1], u)) * 6, [0, 0, 6])
    colore = FIORI[int(rng.integers(len(FIORI)))]
    verde = (0.20, 0.35, 0.12)
    n = normalizza(np.cross([0, 0, 1], u))
    for k in range(int(L / 18)):
        p = a + u * rng.uniform(0, L) - n * rng.uniform(15, 45)
        p[2] = quota(p)
        h = rng.uniform(18, 40)
        colorato(m, verde, lambda t, p=p, h=h: t.cilindro("Canna", p - [0, 0, 3], 6, h, lati=5, raggio_alto=2, tappo=False))
        if rng.random() < 0.7:
            c2 = colore if rng.random() < 0.8 else FIORI[int(rng.integers(len(FIORI)))]
            colorato(m, c2, lambda t, p=p, h=h: t.cilindro("Canna", p + [0, 0, h - 4], 6, 7, lati=6, raggio_alto=1.0))


def catasta(m, c, gradi, L=220.0, file=5):
    u, n = _base(gradi)
    c = np.asarray(c, np.float64)
    for r in range(file):
        for k in range(file - r + 2):
            p = c + n * ((k - (file - r + 1) / 2) * 24) + [0, 0, 12 + r * 21]
            m.cilindro("Trave", p - u * L / 2, 11, L, lati=7, asse=tuple(u))
    # i due paletti che la tengono
    for s in (-1, 1):
        m.cilindro("Trave", c + u * s * (L / 2 + 8) + [0, 0, -20], 5, file * 21 + 40, lati=5)


def bucato(m, a, b, quota, rng):
    """Due pali e la corda, con i panni stesi."""
    a, b = np.asarray(a, np.float64), np.asarray(b, np.float64)
    for p in (a, b):
        m.cilindro("Trave", p - [0, 0, 40], 6, 230, lati=6)
    alto_a, alto_b = a + [0, 0, 180], b + [0, 0, 180]
    m.trave("Stoffa", alto_a, alto_b, 1.5, 1.5)
    u = normalizza(b - a)
    L = np.linalg.norm(b - a)
    s = 25.0
    while s < L - 60:
        w = rng.uniform(40, 80)
        h = rng.uniform(45, 90)
        p0 = alto_a + (alto_b - alto_a) * (s / L)
        p1 = alto_a + (alto_b - alto_a) * min(1.0, (s + w) / L)
        colore = PANNI[int(rng.integers(len(PANNI)))]
        n = normalizza(np.cross([0, 0, 1], u))
        piega = n * rng.uniform(-6, 6)
        colorato(m, colore, lambda t, p0=p0, p1=p1, h=h, piega=piega: t.quad("Canna", p0, p1, p1 - [0, 0, h] + piega, p0 - [0, 0, h] + piega))
        s += w + rng.uniform(10, 30)


def scopa(m, c, gradi):
    u, n = _base(gradi)
    c = np.asarray(c, np.float64)
    cima = c + [0, 0, 140] + n * 18
    m.trave("Trave", c + [0, 0, 30], cima, 3.5, 3.5)
    m.cilindro("Paglia", c, 14, 34, lati=7, raggio_alto=4, tappo=False)


def abbeveratoio(m, c, gradi, L=260.0):
    u, n = _base(gradi)
    c = np.asarray(c, np.float64)
    W, H = 70.0, 65.0
    for s in (-1, 1):
        m.scatola("Assi", c + n * s * W / 2 + [0, 0, H / 2], u * L / 2, n * 5, [0, 0, H / 2])
        m.scatola("Assi", c + u * s * L / 2 + [0, 0, H / 2], u * 5, n * W / 2, [0, 0, H / 2])
    m.scatola("Assi", c + [0, 0, 6], u * L / 2, n * W / 2, [0, 0, 6])
    m.scatola("Acqua", c + [0, 0, H - 14], u * (L / 2 - 6), n * (W / 2 - 6), [0, 0, 1])
    for s in (-0.35, 0.35):
        m.scatola("Trave", c + u * s * L + [0, 0, -10], u * 6, n * (W / 2 + 10), [0, 0, 18])


def ruota(m, c, asse, r=62.0):
    asse = normalizza(np.asarray(asse, np.float64))
    c = np.asarray(c, np.float64)
    e1 = normalizza(np.cross(asse, [0, 0, 1]))
    e1 = np.array([0, 0, 1.0]) if np.linalg.norm(e1) < 0.1 else e1
    e2 = np.cross(asse, e1)
    anello = [c + (e1 * math.cos(2 * math.pi * k / 14) + e2 * math.sin(2 * math.pi * k / 14)) * r for k in range(14)]
    for k in range(14):
        m.trave("Trave", anello[k], anello[(k + 1) % 14], 8, 8, su=tuple(asse))
    for k in range(0, 14, 2):
        m.trave("Trave", c, anello[k], 4, 4, su=tuple(asse))
    m.cilindro("Trave", c - asse * 9, 10, 18, lati=8, asse=tuple(asse))


def carro(m, c, gradi, rng, carico="fieno"):
    """Un carro a due ruote con le stanghe a terra, il pianale e le sponde; sopra il fieno, i sacchi o le botti."""
    u, n = _base(gradi)
    c = np.asarray(c, np.float64)
    L, W, z = 300.0, 150.0, 80.0
    pian = c + [0, 0, z]
    m.scatola("Assi", pian, u * L / 2, n * W / 2, [0, 0, 4])
    for s in (-1, 1):
        m.scatola("Assi", pian + n * s * W / 2 + [0, 0, 25], u * L / 2, n * 3, [0, 0, 22])
        m.scatola("Assi", pian + u * s * L / 2 + [0, 0, 25], u * 3, n * W / 2, [0, 0, 22])
        ruota(m, c + n * s * (W / 2 + 14) + [0, 0, 62], n)
    # le stanghe, appoggiate a terra davanti
    for s in (-1, 1):
        a = pian + u * L / 2 + n * s * 45 - [0, 0, 4]
        m.trave("Trave", a - u * 60, c + u * (L / 2 + 230) + n * s * 40 + [0, 0, 8], 8, 8)
    m.cilindro("Trave", c + n * (-W / 2 - 14) + [0, 0, 62], 4, W + 28, lati=6, asse=tuple(n))
    if carico == "fieno":
        m.cilindro("Paglia", pian + [0, 0, 4], W * 0.45, 70, lati=8, raggio_alto=W * 0.35, asse=(0, 0, 1))
        m.scatola("Paglia", pian + [0, 0, 50], u * (L / 2 - 20), n * (W / 2 - 10), [0, 0, 45])
    elif carico == "sacchi":
        for k in range(5):
            p = pian + u * rng.uniform(-L / 2 + 40, L / 2 - 40) + n * rng.uniform(-W / 2 + 35, W / 2 - 35) + [0, 0, 18]
            colorato(m, (0.72, 0.63, 0.46), lambda t, p=p: t.cilindro("Canna", p, 26, 32, lati=7, raggio_alto=18))
    elif carico == "botti":
        for s in (-0.25, 0.25):
            botte(m, pian + u * s * L + [0, 0, 4])


def balla(m, c, gradi):
    u, n = _base(gradi)
    m.scatola("Paglia", np.asarray(c, np.float64) + [0, 0, 28], u * 55, n * 38, [0, 0, 28])


def insegna(m, muro, fuori, simbolo):
    """Il braccio di ferro che esce dal muro sopra la porta e la tavola appesa con il simbolo della bottega."""
    muro = np.asarray(muro, np.float64)
    o = normalizza(np.asarray(fuori, np.float64))
    lato = normalizza(np.cross([0, 0, 1], o))
    alto = muro + [0, 0, 285]
    punta = alto + o * 95
    m.trave("Ferro", alto, punta, 4, 4)
    m.trave("Ferro", muro + [0, 0, 240], alto + o * 60, 3, 3)
    for s in (0.25, 0.85):
        a = alto + o * 95 * s
        m.trave("Ferro", a, a - [0, 0, 18], 1.2, 1.2)
    centro = alto + o * 55 - [0, 0, 48]
    # la tavola (perpendicolare al muro: si legge da chi cammina per la strada)
    m.scatola("Assi", centro, o * 36, lato * 2.5, [0, 0, 28])
    m.scatola("Trave", centro + [0, 0, 30], o * 40, lato * 3.2, [0, 0, 2.5])
    for s in (-1, 1):
        f = centro + lato * s * 3.2

        def blocco(x, z, mx, mz, f=f, s=s):
            m.scatola("Ferro", f + o * x + [0, 0, z], o * mx, lato * s * 1.0, [0, 0, mz])

        if simbolo == "boccale":
            blocco(-4, -2, 11, 13)
            blocco(12, -2, 2.5, 8)
            blocco(17, -2, 3, 2.5)
            blocco(-4, 13, 13, 3)
        elif simbolo == "incudine":
            blocco(0, 6, 20, 4)
            blocco(-22, 7, 6, 2.5)
            blocco(0, -4, 7, 7)
            blocco(0, -14, 14, 3)
        elif simbolo == "chiavi":
            for d in (-1, 1):
                for k in range(5):
                    blocco(d * (-14 + k * 7), -14 + k * 7, 3, 3)
                blocco(d * 17, 15, 5, 5)
        elif simbolo == "sega":
            blocco(4, 0, 20, 5)
            for k in range(6):
                blocco(-12 + k * 6.5, -7, 1.6, 2.5)
            blocco(-21, 2, 4, 9)
        elif simbolo == "pelle":
            blocco(0, 0, 16, 13)
            for x, z in ((-19, 12), (19, 12), (-19, -12), (19, -12)):
                blocco(x, z, 4, 4)
        elif simbolo == "ruota":
            for k in range(8):
                a = 2 * math.pi * k / 8
                blocco(math.cos(a) * 15, math.sin(a) * 15, 4, 4)
            blocco(0, 0, 3, 3)
        elif simbolo == "ferro":
            for k in range(7):
                a = math.pi * (0.1 + 0.8 * k / 6)
                blocco(math.cos(a) * 14, -math.sin(a) * 14 + 6, 3.5, 3.5)


def lanterna(m, c, fuori):
    """Palo con il braccio e la lanterna; restituisce dove va la luce."""
    c = np.asarray(c, np.float64)
    o = normalizza(np.asarray(fuori, np.float64))
    m.cilindro("Trave", c - [0, 0, 50], 9, 330, lati=6)
    cima = c + [0, 0, 270]
    m.trave("Trave", cima, cima + o * 55, 7, 7)
    l = cima + o * 55 - [0, 0, 40]
    lato = normalizza(np.cross([0, 0, 1], o))
    m.scatola("Ferro", l + [0, 0, 32], o * 15, lato * 15, [0, 0, 3])
    m.scatola("Fiamma", l + [0, 0, 15], o * 10, lato * 10, [0, 0, 12])
    m.scatola("Ferro", l, o * 15, lato * 15, [0, 0, 3])
    for sx in (-1, 1):
        for sy in (-1, 1):
            m.trave("Ferro", l + o * 14 * sx + lato * 14 * sy, l + o * 14 * sx + lato * 14 * sy + [0, 0, 32], 1.5, 1.5)
    return l + [0, 0, 15]


# ------------------------------------------------------------------------------------------------ dove

def _dentro(p, poligono):
    x, y = p[0], p[1]
    dentro = False
    n = len(poligono)
    for i in range(n):
        x1, y1 = poligono[i]
        x2, y2 = poligono[(i + 1) % n]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1 + 1e-12) + x1:
            dentro = not dentro
    return dentro


def _dist_linea(p, punti):
    best = 1e18
    for a, b in zip(punti[:-1], punti[1:]):
        a, b = np.asarray(a, np.float64), np.asarray(b, np.float64)
        d = b - a
        t = np.clip(np.dot(np.asarray(p[:2]) - a, d) / (np.dot(d, d) + 1e-9), 0, 1)
        best = min(best, np.linalg.norm(np.asarray(p[:2]) - (a + d * t)))
    return best


class Spazio:
    """Dove c'e' gia' qualcosa: le case (poligoni), le strade, le cose del villaggio (porte, panche, botti, luci)."""

    def __init__(self, pr):
        self.case = [(d[3], [np.array(p, np.float64) for p in d[2]]) for d in pr.disegni if d[1] in ("edificio", "fab")]
        self.strade = [(d[2], d[3]) for d in pr.disegni if d[1] == "strada"]
        self.punti = [(np.array(c["pos"][:2], np.float64), 120.0 if c["tipo"] in ("porta", "panca", "bacheca") else 70.0)
                      for c in pr.cose if c["tipo"] in ("porta", "panca", "bacheca", "fab", "luce", "letto")]
        self.messi = []

    def libero(self, p, r, casa_propria=None, strada_ok=False):
        q = np.asarray(p[:2], np.float64)
        for nome, poli in self.case:
            if _dentro(q, poli):
                return False
        for c, rr in self.punti:
            if np.linalg.norm(q - c) < rr + r:
                return False
        for c, rr in self.messi:
            if np.linalg.norm(q - c) < rr + r:
                return False
        if not strada_ok:
            for punti, larga in self.strade:
                if isinstance(larga, (int, float)) and _dist_linea(q, punti) < larga / 2 - 40:
                    return False
        return True

    def prendi(self, p, r):
        self.messi.append((np.asarray(p[:2], np.float64), r))


# ------------------------------------------------------------------------------------------------ il villaggio vivo

def vita(pr, rng):
    t = pr.t
    sp = Spazio(pr)
    porte = {}
    for c in pr.cose:
        if c["tipo"] == "porta" and c["sezione"] == "Villaggio" and c["nome"].endswith(" porta"):
            porte.setdefault(c["nome"][:-6], c)
    case = [d for d, fd in zip(pr.disegni, pr.disegni.fasi) if d[0] == "Villaggio" and d[1] == "edificio"
            and tuple(fd) == (1, 0) and isinstance(d[3], str)]
    conta = {"case": 0, "cose": 0, "insegne": 0, "lanterne": 0}

    def quota(p):
        return t.quota(p[0], p[1])

    for (sez, tipo, punti, nome) in case:
        poli = [np.array(p, np.float64) for p in punti]
        centro = np.mean(poli, 0)
        porta = porte.get(nome)
        if porta is not None:
            P = np.array(porta["pos"][:2], np.float64)
        elif nome in CAMPAGNA or nome in INSEGNE:
            # (stalla e fienili non hanno porte vere) la facciata e' il lato piu' vicino a una strada
            mezzi = [(poli[i] + poli[(i + 1) % len(poli)]) / 2 for i in range(len(poli))]
            P = min(mezzi, key=lambda q: min((_dist_linea(q, pts) for pts, _ in sp.strade), default=0.0))
        else:
            continue
        fuori = P - centro
        if np.linalg.norm(fuori) < 1:
            continue
        o = np.append(normalizza(fuori), 0.0)
        lato = np.array([-o[1], o[0], 0.0])
        # quanto e' larga la facciata (lungo 'lato') e profonda la casa (lungo 'o')
        proj_l = [np.dot(p - centro, lato[:2]) for p in poli]
        proj_o = [np.dot(p - centro, o[:2]) for p in poli]
        mezzo_l, mezzo_o = max(proj_l), max(proj_o)
        porta_l = np.dot(P - centro, lato[:2])
        # quanto la facciata sta davanti al punto della porta (la porta e' dentro il muro di una ventina di cm)
        fronte = max(0.0, mezzo_o - np.dot(P - centro, o[:2]))
        z0 = quota(P)
        m = Mesh("SM_Vita_" + nome)
        origine = np.array([P[0], P[1], z0])
        cose = 0

        def L(p):
            """Dal mondo (x, y) al pezzo della casa: (x, y, quota) meno l'origine."""
            q = np.array([p[0], p[1], quota(p)])
            return q - origine

        # 1. accanto alla porta (a destra o a sinistra, contro il muro): due o tre cose
        lati = [1, -1]
        rng.shuffle(lati)
        for s in lati[: int(rng.integers(1, 3))]:
            for k in range(int(rng.integers(1, 3))):
                d = rng.uniform(150, 260) + k * 85
                if abs(porta_l + s * d) > mezzo_l - 40:
                    continue
                q = P + lato[:2] * s * d + o[:2] * (fronte + rng.uniform(45, 60))
                if not sp.libero(q, 45):
                    continue
                scelta = rng.choice(["botte", "botte", "cassa", "cesta", "vaso", "vaso", "secchio", "scopa"])
                if scelta == "botte":
                    botte(m, L(q) - [0, 0, 3])
                elif scelta == "vaso":
                    vaso_fiori(m, L(q) - [0, 0, 2], rng)
                elif scelta == "scopa":
                    scopa(m, L(q), math.degrees(math.atan2(o[1], o[0])) + 90)
                elif scelta == "cassa":
                    pr.oggetto(SEZIONE, "%s cassa %d" % (nome, k), FAB_CASSE[int(rng.integers(2))], q[0], q[1], rng.uniform(0, 360))
                elif scelta == "cesta":
                    pr.oggetto(SEZIONE, "%s cesta %d" % (nome, k), FAB_CESTE[int(rng.integers(2))], q[0], q[1], rng.uniform(0, 360))
                elif scelta == "secchio":
                    pr.oggetto(SEZIONE, "%s secchio %d" % (nome, k), FAB_SECCHIO, q[0], q[1], rng.uniform(0, 360))
                sp.prendi(q, 45)
                cose += 1
        # 2. la catasta della legna lungo un fianco (meta' delle case)
        if rng.random() < 0.55 and nome not in INSEGNE:
            s = rng.choice([-1, 1])
            q = centro + lato[:2] * s * (mezzo_l + 95) + o[:2] * rng.uniform(-mezzo_o * 0.4, mezzo_o * 0.3)
            if sp.libero(q, 120):
                catasta(m, L(q) - [0, 0, 6], math.degrees(math.atan2(o[1], o[0])), L=min(260.0, mezzo_o * 1.2))
                sp.prendi(q, 130)
                cose += 1
        # 3. dietro: il bucato steso, o un'aiuola lungo il muro
        dietro = centro - o[:2] * (mezzo_o + 160)
        if rng.random() < 0.35:
            a = dietro + lato[:2] * min(220.0, mezzo_l * 0.8)
            b = dietro - lato[:2] * min(220.0, mezzo_l * 0.8)
            if sp.libero(a, 40) and sp.libero(b, 40) and sp.libero(dietro, 60):
                bucato(m, L(a), L(b), quota, rng)
                sp.prendi(a, 40)
                sp.prendi(b, 40)
                sp.prendi(dietro, 80)
                cose += 1
        elif rng.random() < 0.5:
            a = centro - o[:2] * (mezzo_o + 40) + lato[:2] * (mezzo_l * 0.7)
            b = centro - o[:2] * (mezzo_o + 40) - lato[:2] * (mezzo_l * 0.7)
            if sp.libero((a + b) / 2, 60) and sp.libero(a, 30) and sp.libero(b, 30):
                aiuola(m, np.append(a, 0) - origine * [1, 1, 0], np.append(b, 0) - origine * [1, 1, 0], rng,
                       lambda p: t.quota(p[0] + origine[0], p[1] + origine[1]) - origine[2])
                sp.prendi((a + b) / 2, 60)
                cose += 1
        # 4. l'insegna delle botteghe, sopra la porta (a lato)
        if nome in INSEGNE:
            muro = P + o[:2] * fronte + lato[:2] * 110 * (1 if porta_l < mezzo_l - 150 else -1)
            insegna(m, L(muro) + [0, 0, quota(P) - quota(muro)], o, INSEGNE[nome])
            conta["insegne"] += 1
            cose += 1
        # 5. in campagna: l'abbeveratoio, il carro, le balle di fieno
        if nome in CAMPAGNA:
            q = P + o[:2] * (fronte + 400) + lato[:2] * 260
            if sp.libero(q, 170):
                abbeveratoio(m, L(q) - [0, 0, 4], math.degrees(math.atan2(lato[1], lato[0])))
                sp.prendi(q, 170)
                cose += 1
            q = P + o[:2] * (fronte + 500) - lato[:2] * 380
            if sp.libero(q, 260):
                carro(m, L(q) - [0, 0, 2], math.degrees(math.atan2(lato[1], lato[0])) + rng.uniform(-20, 20), rng, "fieno")
                sp.prendi(q, 260)
                cose += 1
            for k in range(int(rng.integers(2, 5))):
                q = centro + lato[:2] * (mezzo_l + 110) * rng.choice([-1, 1]) + o[:2] * rng.uniform(-mezzo_o, mezzo_o)
                if sp.libero(q, 75):
                    balla(m, L(q) - [0, 0, 3], rng.uniform(0, 180))
                    sp.prendi(q, 75)
                    cose += 1
        if cose and m.parti:
            pr.metti_mesh(SEZIONE, "Vita " + nome, m, origine[0], origine[1], origine[2], 0.0, collisione=True)
            conta["case"] += 1
            conta["cose"] += cose

    # 6. due carri in paese: uno di sacchi accanto al mercato, uno di botti davanti alla locanda
    P0 = t.luogo("Piazza del villaggio")
    for nome, offset, carico in (("Carro del mercato", (2300, 1500), "sacchi"), ("Carro delle botti", (-2300, 1300), "botti")):
        for prova in range(12):
            q = np.array([P0[0] + offset[0] + rng.uniform(-600, 600), P0[1] + offset[1] + rng.uniform(-600, 600)])
            if sp.libero(q, 260, strada_ok=False):
                g = rng.uniform(0, 360)
                m = Mesh("SM_Vita_" + nome.replace(" ", "_"))
                carro(m, np.zeros(3), g, rng, carico)
                pr.metti_mesh(SEZIONE, nome, m, q[0], q[1], t.quota(q[0], q[1]) - 2, 0.0, collisione=True)
                sp.prendi(q, 260)
                conta["cose"] += 1
                break

    # 7. le lanterne lungo le strade del villaggio: ogni 34 m, un lato e poi l'altro, lontano dalle lanterne che ci sono gia'
    lanterne_vecchie = [np.array(c["pos"][:2]) for c in pr.cose if c["tipo"] == "luce"]
    lm = Mesh("SM_Vita_Lanterne")
    luci = []
    oz = t.quota(P0[0], P0[1])
    for punti, larga in sp.strade:
        if not isinstance(larga, (int, float)) or larga < 300:
            continue
        pts = [np.array(p, np.float64) for p in punti]
        lung = sum(np.linalg.norm(b - a) for a, b in zip(pts[:-1], pts[1:]))
        if lung < 2000:
            continue
        s = 1400.0
        verso = 1
        while s < lung - 600 and len(luci) < 20:
            # il punto a distanza s lungo la strada
            resto = s
            for a, b in zip(pts[:-1], pts[1:]):
                l = np.linalg.norm(b - a)
                if resto <= l:
                    d = normalizza(b - a)
                    p = a + d * resto
                    break
                resto -= l
            else:
                break
            n = np.array([-d[1], d[0]]) * verso
            q = p + n * (larga / 2 + 70)
            if (all(np.linalg.norm(q - v) > 1500 for v in lanterne_vecchie) and sp.libero(q, 60, strada_ok=True)
                    and np.linalg.norm(q - np.array(P0[:2])) < 26000):
                luce = lanterna(lm, [q[0] - P0[0], q[1] - P0[1], t.quota(q[0], q[1]) - oz], [-n[0], -n[1], 0])
                luci.append(luce)
                lanterne_vecchie.append(q)
                sp.prendi(q, 60)
            s += 3400.0
            verso = -verso
    if luci:
        pr.al_suolo(SEZIONE, "Lanterne delle strade", lm, P0[0], P0[1], oz, collisione=True)
        for k, (lx, ly, lz) in enumerate(luci):
            pr.aggiungi(SEZIONE, "Luce lanterna della strada %d" % k, "luce", P0[0] + lx, P0[1] + ly, oz + lz, intensita=3.0, raggio=900.0)
        conta["lanterne"] = len(luci)
    print("vita del villaggio: %d case con le loro cose (%d cose), %d insegne, %d lanterne" % (
        conta["case"], conta["cose"], conta["insegne"], conta["lanterne"]))
    return conta
