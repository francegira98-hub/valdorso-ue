# Valdorso - I pericoli fatti da zero, provvisori (v0.1.4, 09/11/2026, chiesto da Fra: "fai tu, dopo si cambiano").
# Scritto da Claude. Tre cose, finche' non arrivano quelle di Fab (poi si cambia solo il modello, il resto resta):
#   il lupo a pezzi: SM_Lupo_Corpo, SM_Lupo_Testa, SM_Lupo_Zampa (la stessa per le quattro), SM_Lupo_Coda. Ogni pezzo ha
#     l'origine dove si piega (la spalla, l'anca, il collo, l'attacco della coda): il C++ (ValdorsoLupo.h) li muove
#     mentre il lupo cammina, corre, morde e cade. Il lupo guarda +X. Il pelo e' nel colore dei vertici (grigio, il
#     dorso piu' scuro, la pancia e il muso piu' chiari, il naso nero), materiale Canna (bianco x colore dei vertici);
#   le armi: SM_Arma_<Id> per gli oggetti dello zaino che si impugnano (coltello, pugnale, falcetto, martello, bastone
#     da pastore, spada corta, scudo di legno). L'impugnatura sta nell'origine, la lama verso +X; lo scudo ha la
#     maniglia nell'origine e la faccia verso -Y. Legno (Trave/Assi), ferro (Canna grigia: niente metallo vero per ora);
#   i suoni dei pericoli (WAV, fatti con i numeri): i colpi (S_Colpo_Pugno, _Lama, _Botta, _Morso, _Scudo), il lupo
#     (S_Lupo_Ringhio, S_Lupo_Ululato, S_Lupo_Guaito), l'acqua (S_Bolle, S_Respiro).
# (09/11) anche i suoni del fodero: S_Fodero_Riponi, S_Fodero_Sfodera.
# (10/11) gli animali (animali.py: sei specie, i colori come materiali), l'armeria (20 armi e scudi), i versi degli
# animali e lo scuoiare; Colori.json per crea_pericoli.py.
# (14/11, v0.2.3 "Armi vere") anche l'arco e la freccia.
# Uso: python pericoli.py <uscita>   (poi pericoli.zip in V:/Texture/valdorso, lo legge crea_pericoli.py)

import math
import os
import sys
import wave

import numpy as np

from villaggio_mesh import Mesh, normalizza

RNG_SEME = 909

GRIGIO = np.array([0.42, 0.40, 0.37])
DORSO = np.array([0.24, 0.22, 0.20])
PANCIA = np.array([0.72, 0.68, 0.60])
NERO = np.array([0.05, 0.05, 0.05])


# ------------------------------------------------------------------------------------------------ forme

def tubo(m, mat, centri, raggi_y, raggi_z, colore, lati=12, chiudi=(True, True)):
    """Un tubo lungo una linea (lungo X di solito): sezioni ellittiche, colore per vertice (funzione di punto e normale)."""
    P, N, UV, C, T = [], [], [], [], []
    for i, c in enumerate(centri):
        c = np.asarray(c, np.float64)
        d = np.asarray(centri[min(i + 1, len(centri) - 1)], np.float64) - np.asarray(centri[max(i - 1, 0)], np.float64)
        a = normalizza(d)
        e1 = normalizza(np.cross(a, [0, 0, 1]) if abs(a[2]) < 0.95 else np.cross(a, [1, 0, 0]))
        e2 = np.cross(e1, a)
        for k in range(lati + 1):
            ang = 2 * math.pi * k / lati
            n = e1 * math.cos(ang) * raggi_z[i] + e2 * math.sin(ang) * raggi_y[i]
            p = c + e1 * math.cos(ang) * raggi_y[i] + e2 * math.sin(ang) * raggi_z[i]
            nn = normalizza(e1 * math.cos(ang) / max(raggi_y[i], 0.1) + e2 * math.sin(ang) / max(raggi_z[i], 0.1))
            P.append(p)
            N.append(nn)
            UV.append([k / lati, i / max(1, len(centri) - 1)])
            C.append(list(colore(p, nn)) + [1.0])
    for i in range(len(centri) - 1):
        for k in range(lati):
            a0 = i * (lati + 1) + k
            b0 = a0 + lati + 1
            T += [[a0, b0, a0 + 1], [a0 + 1, b0, b0 + 1]]
    m.triangoli(mat, P, N, UV, T, C)
    for fine, idx in ((0, 0), (1, len(centri) - 1)):
        if not chiudi[fine]:
            continue
        c = np.asarray(centri[idx], np.float64)
        anello = P[idx * (lati + 1): idx * (lati + 1) + lati]
        d = normalizza(np.asarray(centri[-1], np.float64) - np.asarray(centri[0], np.float64)) * (1 if fine else -1)
        col = list(colore(c, d)) + [1.0]
        Tc = [[0, k + 1, (k + 1) % lati + 1] for k in range(lati)]
        m.triangoli(mat, [c] + anello, [d] * (lati + 1), [[0.5, 0.5]] * (lati + 1), Tc, [col] * (lati + 1))


def colore_pelo(dorso_z=0.0, scala=1.0):
    def f(p, n):
        # il dorso (normale in su) piu' scuro, la pancia (normale in giu') chiara, con un po' di variazione
        su = n[2]
        c = GRIGIO.copy()
        if su > 0.2:
            c = c * (1 - (su - 0.2)) + DORSO * (su - 0.2)
        elif su < -0.3:
            c = c * (1 + (su + 0.3)) + PANCIA * (-(su + 0.3))
        rumore = 0.92 + 0.16 * (math.sin(p[0] * 0.37 + p[1] * 0.71) * 0.5 + 0.5)
        return np.clip(c * rumore, 0, 1)
    return f


# (10/11) il lupo ora lo fa animali.py, con gli altri animali.


# (10/11) i colori sono materiali (MV_<Colore>, crea_pericoli.py fa MI_Pericoli_<Colore>): il 09/11 il colore dei
# vertici non arrivava e tutto usciva bianco. I nomi senza trattini bassi.
LEGNO, LEGNOSCURO, CUOIO, FERRO, ACCIAIO, CORNO, OTTONE, ROSSO, ORO, BLU = (
    "Legno", "LegnoScuro", "Cuoio", "Ferro", "Acciaio", "Corno", "Ottone", "Rosso", "Oro", "Blu")
COLORI_ARMI = {
    "Legno": (0.42, 0.29, 0.17), "LegnoScuro": (0.22, 0.14, 0.08), "Cuoio": (0.30, 0.18, 0.10), "Ferro": (0.36, 0.37, 0.38),
    "Acciaio": (0.62, 0.63, 0.65), "Corno": (0.80, 0.74, 0.62), "Ottone": (0.72, 0.52, 0.22), "Rosso": (0.50, 0.08, 0.06),
    "Oro": (0.85, 0.65, 0.20), "Blu": (0.10, 0.16, 0.40),
}


def _colorato(m, colore, disegna):
    """Quello che disegna va nel materiale del colore (il colore dei vertici resta bianco)."""
    tmp = Mesh("tmp")
    disegna(tmp)
    for mat, parte in tmp.parti.items():
        inizio = 0
        for P, N, UV, C, T in zip(*[parte[k] for k in range(5)]):
            m.triangoli(colore, P, N, UV, np.asarray(T) - inizio, [[1.0, 1.0, 1.0, 1.0]] * len(P))
            inizio += len(P)


def lama(m, da, a, largo, spesso, punta=0.25, colore=ACCIAIO, base=None):
    """Una lama piatta lungo X (da 'da' ad 'a'), larga in Z, sottile in Y, che finisce a punta (con il filo piu' chiaro)."""
    base = largo if base is None else base

    def d(t):
        L = a - da
        xp = a - L * punta
        sopra = [np.array([da, 0, base / 2]), np.array([xp, 0, largo / 2]), np.array([a, 0, largo * 0.05])]
        sotto = [np.array([da, 0, -base / 2]), np.array([xp, 0, -largo / 2 * 0.7]), np.array([a, 0, -largo * 0.05])]
        for s_ in (-1, 1):
            n = np.array([0, s_, 0.0])
            off = np.array([0, s_ * spesso / 2, 0])
            for i in range(2):
                t.quad("x", sotto[i] + off, sotto[i + 1] + off, sopra[i + 1] + off, sopra[i] + off, normale=n)
        for lato in (sopra, sotto):
            for i in range(2):
                a1, b1 = lato[i], lato[i + 1]
                t.quad("x", a1 + [0, -spesso / 2, 0], b1 + [0, -spesso / 2, 0], b1 + [0, spesso / 2, 0], a1 + [0, spesso / 2, 0],
                       normale=normalizza(np.array([0, 0, 1.0 if lato is sopra else -1.0])))
    _colorato(m, colore, d)


def manico(m, da, a, r, colore=LEGNO, lati=8):
    _colorato(m, colore, lambda t: t.cilindro("x", [da, 0, 0], r, a - da, lati=lati, asse=(1, 0, 0)))


def fasce(m, da, a, r, quante, colore=CUOIO):
    """Le fasce di cuoio avvolte sull'impugnatura."""
    passo = (a - da) / quante
    for k in range(quante):
        x = da + k * passo
        _colorato(m, colore, lambda t, x=x: t.cilindro("x", [x, 0, 0], r * 1.12, passo * 0.6, lati=8, asse=(1, 0, 0)))


def piastra(m, colore, punti, y0, y1):
    """Una forma piatta (contorno nel piano XZ) spessa da y0 a y1: lame d'ascia, alette, punte."""
    P = [np.array([p[0], 0, p[1]], np.float64) for p in punti]

    def d(t):
        for y, n in ((y0, -1.0), (y1, 1.0)):
            Q = [p + [0, y, 0] for p in P]
            if n < 0:
                Q = Q[::-1]
            t.poligono("x", Q)
        for i in range(len(P)):
            a1, b1 = P[i], P[(i + 1) % len(P)]
            nn = normalizza(np.cross(b1 - a1, [0, 1, 0]))
            t.quad("x", a1 + [0, y0, 0], b1 + [0, y0, 0], b1 + [0, y1, 0], a1 + [0, y1, 0], normale=-nn)
    _colorato(m, colore, d)


def scudo_tondo(m, raggio, colore_faccia, colore_bordo, umbone, assi=True, dipinto=None):
    """Uno scudo tondo: la faccia verso -Y, la maniglia nell'origine."""
    lati = 24

    def faccia(t, y, n):
        P = [np.array([0, y, 0])] + [np.array([raggio * math.cos(2 * math.pi * k / lati), y, raggio * math.sin(2 * math.pi * k / lati)]) for k in range(lati)]
        UV = [[0.5, 0.5]] + [[0.5 + 0.5 * math.cos(2 * math.pi * k / lati), 0.5 + 0.5 * math.sin(2 * math.pi * k / lati)] for k in range(lati)]
        T = [[0, k + 1, (k + 1) % lati + 1] for k in range(lati)] if n < 0 else [[0, (k + 1) % lati + 1, k + 1] for k in range(lati)]
        t.triangoli("Assi" if assi else colore_faccia, P, [np.array([0, n, 0.0])] * len(P), UV, T)
    if assi:
        faccia(m, -6, -1.0)
        faccia(m, -4, 1.0)
    else:
        _colorato(m, colore_faccia, lambda t: (faccia(t, -6, -1.0), faccia(t, -4, 1.0)))
    if dipinto:
        # quattro spicchi dipinti, un filo davanti alla faccia
        for k in range(4):
            a0, a1 = k * math.pi / 2, k * math.pi / 2 + math.pi / 4
            pts = [np.array([0, -6.2, 0])] + [np.array([raggio * 0.97 * math.cos(a), -6.2, raggio * 0.97 * math.sin(a)]) for a in np.linspace(a0, a1, 5)]
            _colorato(m, dipinto, lambda t, pts=pts: t.triangoli("x", pts, [np.array([0, -1.0, 0])] * len(pts), [[0, 0]] * len(pts),
                                                                [[0, i + 1, i] for i in range(1, len(pts) - 1)]))
    _colorato(m, colore_bordo, lambda t: t.cilindro("x", [0, -3.6, 0], raggio + 0.6, 3.0, lati=lati, asse=(0, -1, 0), tappo=False))
    if umbone:
        _colorato(m, colore_bordo, lambda t: t.cilindro("x", [0, -6, 0], umbone, umbone * 0.5, lati=12, asse=(0, -1, 0), raggio_alto=umbone * 0.55))
    manico(m, -6, 6, 1.3, CUOIO)


def arma(nome):
    m = Mesh("SM_Arma_" + nome)
    if nome == "Coltello":
        manico(m, -10, 1, 1.4, CORNO)
        _colorato(m, OTTONE, lambda t: t.cilindro("x", [0.6, 0, 0], 1.8, 0.8, lati=8, asse=(1, 0, 0)))
        lama(m, 1.4, 15, 2.6, 0.4, punta=0.3)
    elif nome == "Pugnale":
        manico(m, -10, 1, 1.4, CUOIO)
        fasce(m, -9, 0, 1.4, 4, LEGNOSCURO)
        _colorato(m, FERRO, lambda t: t.cilindro("x", [-11.5, 0, 0], 2.0, 1.6, lati=8, asse=(1, 0, 0)))
        _colorato(m, FERRO, lambda t: t.blocco("x", 1, 2.2, -1.2, 1.2, -5, 5))
        lama(m, 2.2, 26, 3.4, 0.5, punta=0.45)
    elif nome == "Falcetto":
        manico(m, -12, 1, 1.6, LEGNO)
        _colorato(m, FERRO, lambda t: t.cilindro("x", [0.5, 0, 0], 1.9, 1.5, lati=8, asse=(1, 0, 0)))
        pts = [np.array([1.5 + 12 * math.sin(a), 0, 12 - 12 * math.cos(a) * 0.9]) for a in np.linspace(0, 2.5, 10)]
        for i in range(len(pts) - 1):
            _colorato(m, ACCIAIO, lambda t, i=i: t.trave("x", pts[i], pts[i + 1], 2.6 - i * 0.18, 0.4, su=(0, 1, 0)))
    elif nome == "Martello":
        manico(m, -14, 26, 1.6, LEGNO)
        fasce(m, -13, -4, 1.6, 3)
        _colorato(m, FERRO, lambda t: t.blocco("x", 21, 29, -2.6, 2.6, -3, 9))
        _colorato(m, FERRO, lambda t: t.cilindro("x", [25, 0, -3], 3.2, 5, lati=8, asse=(0, 0, -1), raggio_alto=1.5))
    elif nome == "Bastone_Pastore":
        manico(m, -60, 100, 1.8, LEGNO)
        pts = [np.array([100 + 8 * math.sin(a), 0, 8 - 8 * math.cos(a)]) for a in np.linspace(0, math.pi, 8)]
        for i in range(len(pts) - 1):
            _colorato(m, LEGNO, lambda t, i=i: t.trave("x", pts[i], pts[i + 1], 3.4, 3.4, su=(0, 1, 0)))
    elif nome == "Spada_Corta":
        manico(m, -12, 0, 1.6, CUOIO)
        fasce(m, -11, -1, 1.6, 5, LEGNOSCURO)
        _colorato(m, FERRO, lambda t: t.cilindro("x", [-14.5, 0, 0], 2.4, 2.5, lati=8, asse=(1, 0, 0)))
        _colorato(m, FERRO, lambda t: t.blocco("x", 0, 2, -1.5, 1.5, -9, 9))
        lama(m, 2, 62, 4.4, 0.6, punta=0.15)
    elif nome == "Scudo_Legno":
        scudo_tondo(m, 30, None, FERRO, 7)
    # --- (10/11) l'armeria nuova
    elif nome == "Accetta":
        manico(m, -14, 30, 1.6, LEGNO)
        piastra(m, FERRO, [(22, -2), (30, -2), (31, 4), (34, 10), (24, 9), (22, 4)], -1.2, 1.2)
        _colorato(m, ACCIAIO, lambda t: t.trave("x", [34, 0, 10], [31, 0, 4], 1.2, 2.6, su=(0, 1, 0)))
    elif nome == "Ascia_Guerra":
        manico(m, -30, 90, 1.9, LEGNOSCURO)
        fasce(m, -28, -6, 1.9, 6)
        _colorato(m, FERRO, lambda t: t.cilindro("x", [-32, 0, 0], 2.2, 2.5, lati=8, asse=(1, 0, 0)))
        # la mezzaluna
        arco = [(80 + 4 * math.sin(a), 20 * math.cos(a)) for a in np.linspace(-1.2, 1.2, 9)]
        piastra(m, FERRO, [(74, -4), (78, -14)] + [(x + 10, z) for x, z in arco] + [(78, 14), (74, 4)], -1.4, 1.4)
        # il becco dietro
        piastra(m, FERRO, [(76, -3), (76, 3), (70, 0)], -1.1, 1.1)
    elif nome == "Mazza":
        manico(m, -16, 46, 1.7, LEGNO)
        fasce(m, -15, -3, 1.7, 4)
        _colorato(m, FERRO, lambda t: t.cilindro("x", [36, 0, 0], 2.6, 14, lati=8, asse=(1, 0, 0)))
        for k in range(6):
            a = 2 * math.pi * k / 6
            d = np.array([0, math.cos(a), math.sin(a)])
            piastra_pts = [(37, 0), (49, 0), (47, 6), (39, 6)]
            # ogni aletta: un blocchetto inclinato attorno all'asse
            c = np.array([43, 0, 0]) + d * 4.5
            _colorato(m, FERRO, lambda t, c=c, d=d: t.scatola("x", c, [6, 0, 0], d * 3.2, np.cross([1, 0, 0], d) * 0.6))
        _colorato(m, FERRO, lambda t: t.cilindro("x", [50, 0, 0], 2.6, 2, lati=8, asse=(1, 0, 0), raggio_alto=0.5))
    elif nome == "Clava":
        pts = [np.array([x, 0, 0.6 * math.sin(x * 0.09)]) for x in np.linspace(-14, 52, 9)]
        for i in range(len(pts) - 1):
            r0 = 1.8 + max(0, (pts[i][0] - 5) / 47) * 3.5
            _colorato(m, LEGNO, lambda t, i=i, r0=r0: t.cilindro("x", pts[i], r0, np.linalg.norm(pts[i + 1] - pts[i]), lati=8,
                                                               asse=pts[i + 1] - pts[i], raggio_alto=r0 + 0.45))
        for x, a in ((30, 0.5), (38, 2.4), (44, 4.1), (48, 1.4)):
            d = np.array([0, math.cos(a), math.sin(a)])
            _colorato(m, LEGNOSCURO, lambda t, x=x, d=d: t.cilindro("x", np.array([x, 0, 0]) + d * 4, 1.4, 2.5, lati=6, asse=d, raggio_alto=0.4))
    elif nome == "Lancia":
        manico(m, -80, 135, 1.6, LEGNO)
        _colorato(m, FERRO, lambda t: t.cilindro("x", [-82, 0, 0], 1.8, 3, lati=8, asse=(1, 0, 0), raggio_alto=1.2))
        _colorato(m, FERRO, lambda t: t.cilindro("x", [132, 0, 0], 1.9, 6, lati=8, asse=(1, 0, 0), raggio_alto=1.4))
        lama(m, 138, 162, 5.0, 0.8, punta=0.5, base=1.6)
    elif nome == "Forcone":
        manico(m, -60, 110, 1.7, LEGNO)
        _colorato(m, FERRO, lambda t: t.blocco("x", 108, 112, -0.8, 0.8, -9, 9))
        for z in (-8, 0, 8):
            _colorato(m, FERRO, lambda t, z=z: t.cilindro("x", [111, 0, z], 0.6, 26, lati=6, asse=(1, 0, 0.05 * np.sign(z)), raggio_alto=0.25))
    elif nome == "Bastone_Ferrato":
        manico(m, -70, 80, 1.9, LEGNOSCURO)
        for x in (-72, 76):
            _colorato(m, FERRO, lambda t, x=x: t.cilindro("x", [x, 0, 0], 2.2, 6, lati=8, asse=(1, 0, 0)))
        for x in (-60, 62):
            _colorato(m, FERRO, lambda t, x=x: t.cilindro("x", [x, 0, 0], 2.1, 1.5, lati=8, asse=(1, 0, 0)))
    elif nome == "Spada_Lunga":
        manico(m, -20, 0, 1.6, CUOIO)
        fasce(m, -19, -1, 1.6, 7, LEGNOSCURO)
        _colorato(m, ACCIAIO, lambda t: t.cilindro("x", [-23.5, 0, 0], 2.8, 3.2, lati=10, asse=(1, 0, 0)))
        _colorato(m, ACCIAIO, lambda t: t.blocco("x", 0, 2.2, -1.6, 1.6, -13, 13))
        _colorato(m, OTTONE, lambda t: t.blocco("x", 0.4, 1.8, -1.8, 1.8, -2.5, 2.5))
        lama(m, 2.2, 92, 4.8, 0.7, punta=0.12, base=5.2)
        # la sguscia al centro della lama, un filo piu' scura
        _colorato(m, FERRO, lambda t: t.blocco("x", 3, 60, -0.4, 0.4, -0.7, 0.7))
    elif nome == "Gladio":
        manico(m, -10, 0, 1.8, LEGNOSCURO)
        _colorato(m, LEGNOSCURO, lambda t: t.cilindro("x", [-15, 0, 0], 3.4, 5, lati=10, asse=(1, 0, 0)))   # il pomo tondo
        _colorato(m, OTTONE, lambda t: t.blocco("x", 0, 3, -2.2, 2.2, -4.5, 4.5))
        lama(m, 3, 52, 5.6, 0.7, punta=0.22, base=5.0)
    elif nome == "Brocchiere":
        scudo_tondo(m, 16, FERRO, FERRO, 6, assi=False)
    elif nome == "Scudo_Rotondo":
        scudo_tondo(m, 38, None, FERRO, 8, dipinto=ROSSO)
        # il cerchio d'oro intorno all'umbone
        _colorato(m, ORO, lambda t: t.cilindro("x", [0, -6.3, 0], 13, 0.3, lati=20, asse=(0, -1, 0), tappo=False))
    elif nome in ("Scudo_Aquilone", "Scudo_Legione"):
        if nome == "Scudo_Aquilone":
            # la mandorla: larga in alto, a punta in basso (lunga 95 cm, larga 50); la faccia appena curva
            contorno = [(25 * math.cos(a), 30 + 18 * math.sin(a)) for a in np.linspace(0, math.pi, 9)]
            contorno += [(-23, 10), (-12, -40), (0, -62), (12, -40), (23, 10)]
            colore, bordo = BLU, OTTONE
        else:
            contorno = [(-32, -55), (32, -55), (32, 55), (-32, 55)]
            colore, bordo = ROSSO, OTTONE
        # la faccia: il contorno nel piano XZ e curvato all'indietro (Y) ai lati
        def curva(x):
            return -6 + (x / 32.0) ** 2 * (5 if nome == "Scudo_Legione" else 3)
        Q = [np.array([x, curva(x), z]) for x, z in contorno]
        c = np.array([0, -6, 0])

        def faccia(t, spost, verso):
            P = [c + [0, spost, 0]] + [q + [0, spost, 0] for q in Q]
            n = len(Q)
            T = [[0, k + 1, (k + 1) % n + 1] for k in range(n)]
            if verso > 0:
                T = [[a, cc, b] for a, b, cc in T]
            t.triangoli("x", P, [np.array([0, -verso, 0.0])] * len(P), [[0, 0]] * len(P), T)
        _colorato(m, colore, lambda t: (faccia(t, 0, -1), faccia(t, 2, 1)))
        for i in range(len(Q)):
            a1, b1 = Q[i], Q[(i + 1) % len(Q)]
            _colorato(m, bordo, lambda t, a1=a1, b1=b1: t.trave("x", a1 + [0, -0.4, 0], b1 + [0, -0.4, 0], 2.2, 2.6, su=(0, 1, 0)))
        _colorato(m, bordo, lambda t: t.cilindro("x", [0, -6, 0], 9, 4, lati=14, asse=(0, -1, 0), raggio_alto=5))
        if nome == "Scudo_Legione":
            # i fulmini d'oro delle legioni (quattro raggi dall'umbone)
            for sx, sz in ((1, 1), (-1, 1), (1, -1), (-1, -1)):
                _colorato(m, ORO, lambda t, sx=sx, sz=sz: t.trave("x", [sx * 8, -6.6, sz * 8], [sx * 24, -6.6 + 0.8, sz * 40], 1.6, 0.4, su=(0, 1, 0)))
        manico(m, -7, 7, 1.3, CUOIO)
    elif nome == "Piccone":
        # (11/11, v0.1.7) il piccone per liberare la frana del nord (dalla fucina): manico lungo, testa a due punte
        manico(m, -30, 50, 1.8, LEGNO)
        fasce(m, -29, -18, 1.8, 3)
        _colorato(m, FERRO, lambda t: t.blocco("x", 44, 52, -2.4, 2.4, -3.5, 3.5))
        _colorato(m, FERRO, lambda t: t.trave("x", [48, 0, 3], [46, 0, 22], 3.2, 2.4, su=(1, 0, 0)))
        _colorato(m, FERRO, lambda t: t.trave("x", [48, 0, -3], [46, 0, -20], 3.2, 2.4, su=(1, 0, 0)))
        _colorato(m, ACCIAIO, lambda t: t.cilindro("x", [46, 0, 22], 1.4, 3.5, lati=6, raggio_alto=0.2))
        _colorato(m, ACCIAIO, lambda t: t.blocco("x", 44.5, 47.5, -2.0, 2.0, -23, -19))
    elif nome == "Pugnale_Nani":
        # (11/11, v0.1.7) il pugnale dei nani (dalle ossa della grotta del nord): largo, squadrato, con l'ottone
        manico(m, -9, 1, 1.6, LEGNOSCURO)
        fasce(m, -8, 0, 1.6, 3, OTTONE)
        _colorato(m, OTTONE, lambda t: t.blocco("x", -11.5, -9, -2.2, 2.2, -2.2, 2.2))
        _colorato(m, OTTONE, lambda t: t.blocco("x", 1, 3, -1.4, 1.4, -6, 6))
        lama(m, 3, 24, 4.4, 0.6, punta=0.3)
    elif nome == "Martello_Nani":
        # (11/11) il martello del guardiano della forgia: testa nera squadrata, fasce d'ottone
        manico(m, -34, 48, 2.2, LEGNOSCURO)
        fasce(m, -33, -18, 2.2, 4, OTTONE)
        _colorato(m, FERRO, lambda t: t.blocco("x", 40, 58, -6.5, 6.5, -9, 9))
        _colorato(m, OTTONE, lambda t: t.blocco("x", 39, 59, -6.8, 6.8, -1, 1))
        _colorato(m, FERRO, lambda t: t.blocco("x", 44, 54, -7.2, 7.2, -10, -8))
    # --- (14/11, v0.2.3 "Armi vere") l'arco di tasso e la freccia
    elif nome == "Arco":
        # l'impugnatura nell'origine, i flettenti lungo X (la punta in alto quando e' in mano), la pancia verso +Z e la
        # corda dietro (z negativa)
        manico(m, -6, 6, 1.9, CUOIO)
        pts = [np.array([x, 0, 9.0 * (1 - (x / 64.0) ** 2) - 9.0]) for x in np.linspace(-64, 64, 17)]
        for i in range(len(pts) - 1):
            spessore = 2.4 - 1.4 * abs(pts[i][0] + pts[i + 1][0]) / 128.0
            _colorato(m, LEGNOSCURO, lambda t, i=i, sp=spessore: t.trave("x", pts[i], pts[i + 1], sp, 1.6, su=(0, 1, 0)))
        _colorato(m, CORNO, lambda t: t.trave("x", pts[0] + [0, 0, -1.2], pts[-1] + [0, 0, -1.2], 0.25, 0.25, su=(0, 1, 0)))
    elif nome == "Freccia":
        # lunga 70 cm, la punta in +X; le penne rosse in fondo
        manico(m, -35, 30, 0.45, LEGNO, lati=6)
        lama(m, 30, 37, 1.8, 0.3, punta=0.6, colore=FERRO, base=0.9)
        for a in (0.0, 2.094, 4.189):
            d = np.array([0, math.cos(a), math.sin(a)])
            _colorato(m, ROSSO, lambda t, d=d: t.trave("x", np.array([-33.5, 0, 0]) + d * 0.9, np.array([-26, 0, 0]) + d * 0.9 + d * 1.4,
                                                        0.15, 1.6, su=tuple(np.cross([1, 0, 0], d))))
    return m


ARMI = ["Coltello", "Pugnale", "Falcetto", "Martello", "Bastone_Pastore", "Spada_Corta", "Scudo_Legno",
        "Accetta", "Ascia_Guerra", "Mazza", "Clava", "Lancia", "Forcone", "Bastone_Ferrato", "Spada_Lunga", "Gladio",
        "Brocchiere", "Scudo_Rotondo", "Scudo_Aquilone", "Scudo_Legione",
        # (11/11, v0.1.7) la grotta del nord
        "Piccone", "Pugnale_Nani", "Martello_Nani",
        # (14/11, v0.2.3) l'arco e la freccia (la freccia la usa ValdorsoFreccia.cpp: SM_Arma_Freccia)
        "Arco", "Freccia"]


# ------------------------------------------------------------------------------------------------ suoni

FREQ = 44100


def _scrivi(cartella, nome, x, picco=0.85):
    x = np.asarray(x, np.float64)
    x = x / (np.max(np.abs(x)) + 1e-9) * picco
    with wave.open(os.path.join(cartella, nome + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(FREQ)
        w.writeframes((x * 32767).astype(np.int16).tobytes())


def _env(n, attacco, caduta):
    t = np.arange(n) / FREQ
    return np.minimum(1.0, t / max(attacco, 1e-4)) * np.exp(-t / max(caduta, 1e-4))


def _filtra(x, basso, alto):
    f = np.fft.rfftfreq(len(x), 1 / FREQ)
    X = np.fft.rfft(x)
    X[(f < basso) | (f > alto)] = 0
    return np.fft.irfft(X, len(x))


def suoni(cartella, rng):
    n = lambda s: int(FREQ * s)
    # pugno: un tonfo sordo
    x = _filtra(rng.standard_normal(n(0.25)), 40, 900) * _env(n(0.25), 0.002, 0.04)
    x += np.sin(2 * np.pi * 85 * np.arange(n(0.25)) / FREQ) * _env(n(0.25), 0.001, 0.05) * 0.8
    _scrivi(cartella, "S_Colpo_Pugno", x)
    # lama: un fendente (fruscio che sale) e il taglio
    t = np.arange(n(0.35)) / FREQ
    fr = _filtra(rng.standard_normal(n(0.35)), 1500, 7000) * np.exp(-((t - 0.08) / 0.05) ** 2)
    taglio = _filtra(rng.standard_normal(n(0.35)), 300, 3000) * _env(n(0.35), 0.001, 0.05) * np.roll(np.ones(n(0.35)), 0) * (t > 0.1)
    _scrivi(cartella, "S_Colpo_Lama", fr * 0.6 + taglio)
    # botta: legno pieno
    x = _filtra(rng.standard_normal(n(0.3)), 60, 1500) * _env(n(0.3), 0.001, 0.06)
    x += np.sin(2 * np.pi * 140 * np.arange(n(0.3)) / FREQ) * _env(n(0.3), 0.001, 0.08)
    _scrivi(cartella, "S_Colpo_Botta", x)
    # morso: denti che schioccano e un ringhio corto
    t = np.arange(n(0.4)) / FREQ
    schiocco = _filtra(rng.standard_normal(n(0.4)), 800, 6000) * _env(n(0.4), 0.0005, 0.015)
    ringhio = np.sin(2 * np.pi * (95 + 20 * np.sin(2 * np.pi * 31 * t)) * t) * (0.5 + 0.5 * _filtra(rng.standard_normal(n(0.4)), 20, 80)) * _env(n(0.4), 0.02, 0.15)
    _scrivi(cartella, "S_Colpo_Morso", schiocco + ringhio * 0.5)
    # scudo: legno cavo e ferro che vibra
    t = np.arange(n(0.6)) / FREQ
    x = _filtra(rng.standard_normal(n(0.6)), 80, 1200) * _env(n(0.6), 0.001, 0.05)
    x += (np.sin(2 * np.pi * 620 * t) + 0.6 * np.sin(2 * np.pi * 1340 * t)) * _env(n(0.6), 0.001, 0.25) * 0.4
    _scrivi(cartella, "S_Colpo_Scudo", x)
    # il lupo che ringhia (2 s, in giro)
    t = np.arange(n(2.0)) / FREQ
    mod = 0.6 + 0.4 * np.sin(2 * np.pi * 0.7 * t)
    x = np.sin(2 * np.pi * (80 + 15 * np.sin(2 * np.pi * 27 * t)) * t) * _filtra(rng.standard_normal(n(2.0)), 15, 60) * 4
    x = x * mod + _filtra(rng.standard_normal(n(2.0)), 100, 600) * 0.2 * mod
    _scrivi(cartella, "S_Lupo_Ringhio", x, 0.7)
    # l'ululato: una nota che sale, tiene e scende, con un po' di vibrato e il fiato
    durata = 4.5
    t = np.arange(n(durata)) / FREQ
    f = 380 + 260 * np.clip(t / 0.8, 0, 1) - 180 * np.clip((t - 3.0) / 1.5, 0, 1) + 6 * np.sin(2 * np.pi * 5.5 * t)
    fase = 2 * np.pi * np.cumsum(f) / FREQ
    voce = np.sin(fase) + 0.35 * np.sin(2 * fase) + 0.12 * np.sin(3 * fase)
    inv = np.clip(t / 0.4, 0, 1) * np.clip((durata - t) / 0.9, 0, 1)
    fiato = _filtra(rng.standard_normal(len(t)), 500, 3000) * 0.08
    x = (voce * 0.7 + fiato) * inv
    # l'eco della valle
    eco = np.zeros_like(x)
    ritardo = int(0.45 * FREQ)
    eco[ritardo:] = x[:-ritardo] * 0.3
    _scrivi(cartella, "S_Lupo_Ululato", x + eco, 0.8)
    # il guaito di chi scappa
    t = np.arange(n(0.6)) / FREQ
    f = 900 - 500 * t / 0.6
    fase = 2 * np.pi * np.cumsum(f) / FREQ
    _scrivi(cartella, "S_Lupo_Guaito", (np.sin(fase) + 0.3 * np.sin(2 * fase)) * _env(n(0.6), 0.01, 0.25))
    # le bolle sott'acqua (in giro)
    x = np.zeros(n(3.0))
    for _ in range(40):
        k = int(rng.uniform(0, n(2.9)))
        L = int(rng.uniform(0.02, 0.06) * FREQ)
        ff = rng.uniform(300, 900)
        tt = np.arange(L) / FREQ
        bolla = np.sin(2 * np.pi * ff * (1 + 3 * tt) * tt) * np.exp(-tt / 0.015)
        x[k:k + L] += bolla[: max(0, min(L, len(x) - k))] * rng.uniform(0.3, 1.0)
    x += _filtra(rng.standard_normal(len(x)), 50, 300) * 0.15
    _scrivi(cartella, "S_Bolle", x, 0.5)
    # il respiro a pieni polmoni quando si riemerge
    t = np.arange(n(0.9)) / FREQ
    x = _filtra(rng.standard_normal(len(t)), 300, 2500) * np.clip(t / 0.15, 0, 1) * np.exp(-np.clip(t - 0.3, 0, None) / 0.2)
    _scrivi(cartella, "S_Respiro", x, 0.6)
    # (09/11) il fodero: la lama che scivola nel cuoio e il colpo dell'elsa; sfoderare: lo scivolo e il ferro che canta
    durata = 0.7
    t = np.arange(n(durata)) / FREQ
    scivolo = _filtra(rng.standard_normal(len(t)), 1800, 7000) * np.clip(t / 0.05, 0, 1) * np.clip((0.45 - t) / 0.1, 0, 1)
    colpo = _filtra(rng.standard_normal(len(t)), 150, 2500) * np.exp(-np.clip(t - 0.47, 0, None) / 0.03) * (t > 0.47)
    cuoio = _filtra(rng.standard_normal(len(t)), 80, 600) * np.exp(-np.clip(t - 0.5, 0, None) / 0.08) * (t > 0.5) * 0.5
    _scrivi(cartella, "S_Fodero_Riponi", scivolo * 0.5 + colpo + cuoio, 0.7)
    durata = 1.2
    t = np.arange(n(durata)) / FREQ
    scivolo = _filtra(rng.standard_normal(len(t)), 2000, 8000) * np.clip(t / 0.03, 0, 1) * np.clip((0.4 - t) / 0.1, 0, 1)
    canto = (np.sin(2 * np.pi * 2350 * t) + 0.5 * np.sin(2 * np.pi * 3720 * t) + 0.25 * np.sin(2 * np.pi * 5110 * t))
    canto = canto * np.exp(-np.clip(t - 0.35, 0, None) / 0.35) * (t > 0.33) * 0.25
    _scrivi(cartella, "S_Fodero_Sfodera", scivolo * 0.6 + canto, 0.7)

    # (10/11) gli altri animali
    def voce(durata, f0, f1, aspro, vibrato=0.0, armoniche=(1.0, 0.5, 0.3)):
        t = np.arange(n(durata)) / FREQ
        f = f0 + (f1 - f0) * t / durata + vibrato * np.sin(2 * np.pi * 6 * t)
        fase = 2 * np.pi * np.cumsum(f) / FREQ
        x = sum(a * np.sin((k + 1) * fase) for k, a in enumerate(armoniche))
        x *= (1 - aspro) + aspro * _filtra(rng.standard_normal(len(t)), 30, 200) * 3
        return x * np.clip(t / 0.05, 0, 1) * np.clip((durata - t) / 0.15, 0, 1)
    # il cinghiale: tre grugniti corti e nasali
    x = np.zeros(n(1.4))
    for k0 in (0.0, 0.42, 0.8):
        g = voce(0.28, 140, 110, 0.7, armoniche=(1.0, 0.8, 0.6, 0.4))
        i0 = int(k0 * FREQ)
        x[i0:i0 + len(g)] += g
    _scrivi(cartella, "S_Cinghiale_Grugnito", x + _filtra(rng.standard_normal(len(x)), 100, 900) * 0.05, 0.75)
    # lo strillo del cinghiale ferito
    _scrivi(cartella, "S_Cinghiale_Strillo", voce(0.8, 900, 600, 0.5, vibrato=40, armoniche=(1.0, 0.6, 0.5, 0.3)), 0.75)
    # l'orso: un ruggito basso e lungo, con il fiato
    t = np.arange(n(1.8)) / FREQ
    r = voce(1.8, 95, 70, 0.85, vibrato=6, armoniche=(1.0, 0.9, 0.7, 0.5, 0.3))
    fiato = _filtra(rng.standard_normal(len(t)), 200, 1500) * np.clip(t / 0.2, 0, 1) * np.clip((1.8 - t) / 0.5, 0, 1) * 0.3
    _scrivi(cartella, "S_Orso_Ruggito", r + fiato, 0.85)
    # il bramito del cervo: un lamento lungo che sale e si rompe
    b1 = voce(1.6, 160, 240, 0.45, vibrato=12, armoniche=(1.0, 0.7, 0.4))
    b2 = voce(0.6, 220, 120, 0.6, armoniche=(1.0, 0.6))
    x = np.concatenate([b1, np.zeros(n(0.1)), b2])
    _scrivi(cartella, "S_Cervo_Bramito", x, 0.8)
    # la volpe: due guaiti acuti e rauchi
    v = voce(0.25, 1100, 800, 0.35, armoniche=(1.0, 0.5, 0.2))
    x = np.concatenate([v, np.zeros(n(0.15)), v * 0.8])
    _scrivi(cartella, "S_Volpe_Verso", x, 0.7)
    # scuoiare: la lama che taglia e la pelle che si stacca
    t = np.arange(n(1.2)) / FREQ
    taglio = _filtra(rng.standard_normal(len(t)), 1500, 6000) * (0.5 + 0.5 * np.sin(2 * np.pi * 3 * t)) ** 2 * np.clip((1.2 - t) / 0.3, 0, 1)
    strappo = _filtra(rng.standard_normal(len(t)), 100, 800) * np.exp(-np.clip(t - 0.9, 0, None) / 0.08) * (t > 0.9)
    _scrivi(cartella, "S_Scuoia", taglio * 0.4 + strappo, 0.6)


def main(uscita):
    os.makedirs(uscita, exist_ok=True)
    rng = np.random.default_rng(RNG_SEME)
    import animali
    for nome in animali.FORME:
        for pezzo, m in animali.animale(nome).items():
            m.scrivi(os.path.join(uscita, pezzo + ".glb"))
    # i colori per crea_pericoli.py (MI_Pericoli_<Colore>)
    import json
    colori = dict(COLORI_ARMI)
    colori.update({k: list(v) for k, v in animali.TAVOLOZZA.items()})
    with open(os.path.join(uscita, "Colori.json"), "w", encoding="utf-8") as f:
        json.dump(colori, f, indent=1)
    for nome in ARMI:
        m = arma(nome)
        m.scrivi(os.path.join(uscita, m.nome + ".glb"))
        lo, hi = m.scatola_limiti()
        print(m.nome, m.conta(), "triangoli", lo.round(), hi.round())
    suoni(uscita, rng)
    print("suoni fatti")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "uscita_pericoli")
