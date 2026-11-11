# Valdorso - Il gatto della locanda (v0.1.6, La locanda viva). Scritto da Claude l'11/11/2026.
# Fa i tre pezzi del gatto (ValdorsoGatto.cpp: corpo con le zampe, testa con le orecchie, coda) e due suoni (le fusa,
# il miagolio). Grigio tigrato: il colore e' nei vertici (righe scure), il materiale e' MV_Pelo (un colore x vertici).
# Il perno: corpo al centro del busto (le zampe arrivano a -18, cioe' a terra), testa sul collo, coda sull'attacco.
# Uso: python gatto.py <uscita>      (di solito V:/Texture/valdorso/gatto; poi lo zip accanto)

import math
import os
import sys
import wave

import numpy as np

import villaggio_mesh
from villaggio_mesh import Mesh

villaggio_mesh.TILE.setdefault("Pelo", 50.0)
villaggio_mesh.TILE.setdefault("Occhi", 50.0)

GRIGIO = (0.42, 0.40, 0.37, 1.0)
SCURO = (0.18, 0.17, 0.16, 1.0)
CHIARO = (0.70, 0.67, 0.60, 1.0)


def ellissoide(m, mat, centro, rx, ry, rz, nlat=10, nlon=16, colore=None, righe=0.0):
    """Un ellissoide (x avanti, y lato, z su). righe > 0: strisce scure trasversali (il tigrato), lungo x."""
    c = np.asarray(centro, float)
    P, N, UV, C, T = [], [], [], [], []
    for i in range(nlat + 1):
        th = -math.pi / 2 + math.pi * i / nlat
        for j in range(nlon + 1):
            ph = 2 * math.pi * j / nlon
            d = np.array([math.cos(th) * math.cos(ph), math.cos(th) * math.sin(ph), math.sin(th)])
            p = c + d * [rx, ry, rz]
            n = d / np.array([rx, ry, rz])
            n = n / (np.linalg.norm(n) + 1e-9)
            P.append(p)
            N.append(n)
            UV.append([j / nlon, i / nlat])
            col = colore or GRIGIO
            if righe > 0 and d[2] > -0.2 and math.sin(d[0] * righe * math.pi) > 0.55:
                col = SCURO
            if d[2] < -0.55:
                col = CHIARO       # la pancia chiara
            C.append(col)
    for i in range(nlat):
        for j in range(nlon):
            a = i * (nlon + 1) + j
            b = a + nlon + 1
            T.append([a, a + 1, b + 1])
            T.append([a, b + 1, b])
    m.triangoli(mat, P, N, UV, T, C)


def zampa(m, x, y, alto):
    m.cilindro("Pelo", [x, y, -18.0], 2.6, alto, lati=8, raggio_alto=3.2)
    ellissoide(m, "Pelo", [x + 1.0, y, -17.0], 3.4, 2.8, 1.8, 5, 8, colore=CHIARO)


def corpo():
    m = Mesh("SM_Gatto_Corpo")
    ellissoide(m, "Pelo", [0.0, 0.0, 0.0], 21.0, 9.0, 9.5, 12, 18, righe=4.5)
    ellissoide(m, "Pelo", [15.0, 0.0, 4.0], 8.0, 8.0, 9.0, 8, 12)     # il petto
    for x in (13.0, -13.0):
        for y in (-5.0, 5.0):
            zampa(m, x, y, 14.0)
    return m


def testa():
    m = Mesh("SM_Gatto_Testa")
    ellissoide(m, "Pelo", [4.0, 0.0, 2.0], 7.0, 7.5, 6.5, 10, 14, righe=2.0)
    ellissoide(m, "Pelo", [9.5, 0.0, -0.5], 3.4, 4.2, 2.8, 6, 10, colore=CHIARO)    # il muso
    m.cilindro("Stoffa", [12.6, 0.0, 0.6], 0.9, 0.8, lati=6, asse=(1, 0, 0))           # il naso
    for s in (-1, 1):
        # le orecchie: piramidi sottili
        base = np.array([2.5, s * 4.0, 7.0])
        P = [base + [-1.5, -s * 1.8, 0], base + [1.8, -s * 0.2, 0], base + [-0.5, s * 2.2, 0], base + [0.2, s * 0.6, 6.5]]
        P = [np.asarray(p, float) for p in P]
        for tri in ((0, 1, 3), (1, 2, 3), (2, 0, 3)):
            a, b, c = P[tri[0]], P[tri[1]], P[tri[2]]
            n = np.cross(b - a, c - a)
            n = n / (np.linalg.norm(n) + 1e-9)
            m.triangoli("Pelo", [a, b, c], [n] * 3, [[0, 0], [1, 0], [0.5, 1]], [[0, 1, 2]], [SCURO] * 3)
        # gli occhi (verde-oro, un po' lucidi)
        ellissoide(m, "Occhi", [9.2, s * 3.0, 3.2], 1.0, 1.5, 1.2, 5, 8, colore=(1.0, 1.0, 1.0, 1.0))
    return m


def coda():
    m = Mesh("SM_Gatto_Coda")
    # dall'attacco (0,0,0) all'indietro (-x), con una curva in su in fondo; segmenti a righe
    prev = np.array([0.0, 0.0, 0.0])
    k = 10
    for i in range(1, k + 1):
        t = i / k
        p = np.array([-24.0 * t, 0.0, 6.0 * t * t])
        r = 2.4 - 0.9 * t
        col = SCURO if i % 2 == 0 else GRIGIO
        d = p - prev
        L = float(np.linalg.norm(d))
        ellissoide(m, "Pelo", (prev + p) / 2, L / 2 + 0.8, r, r, 4, 8, colore=col)
        prev = p
    return m


def scrivi_wav(percorso, segnale, frequenza=44100):
    s = np.clip(segnale, -1.0, 1.0)
    with wave.open(percorso, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(frequenza)
        w.writeframes((s * 32000).astype(np.int16).tobytes())


def suoni(uscita):
    fr = 44100
    rng = np.random.default_rng(5)
    # le fusa: un rombo basso a ~26 colpi al secondo, inspira ed espira (2,5 s ciascuno), 5 secondi
    n = int(fr * 5.0)
    t = np.arange(n) / fr
    rumore = rng.normal(0, 1, n)
    for _ in range(3):
        rumore = np.convolve(rumore, np.ones(40) / 40, mode="same")
    rumore /= np.abs(rumore).max() + 1e-9
    colpi = (0.5 + 0.5 * np.sin(2 * np.pi * 26.0 * t)) ** 3
    respiro = 0.55 + 0.45 * np.abs(np.sin(np.pi * t / 2.5))
    fusa = rumore * colpi * respiro * np.minimum(1.0, t / 0.3) * np.minimum(1.0, (5.0 - t) / 0.4)
    scrivi_wav(os.path.join(uscita, "S_Gatto_Fusa.wav"), fusa * 0.7)
    # il miagolio: una voce che sale e scende (mi-a-o), con le armoniche
    n = int(fr * 0.75)
    t = np.arange(n) / fr
    f0 = 520 + 260 * np.sin(np.pi * t / 0.75) - 120 * t
    fase = 2 * np.pi * np.cumsum(f0) / fr
    voce = sum(np.sin(fase * h) / h for h in range(1, 7))
    involucro = np.minimum(1.0, t / 0.05) * np.minimum(1.0, (0.75 - t) / 0.2)
    formante = 0.6 + 0.4 * np.sin(np.pi * t / 0.75)
    scrivi_wav(os.path.join(uscita, "S_Gatto_Miao.wav"), voce * involucro * formante * 0.35)


def principale(uscita):
    os.makedirs(uscita, exist_ok=True)
    nomi = []
    for m in (corpo(), testa(), coda()):
        m.scrivi(os.path.join(uscita, m.nome + ".glb"))
        nomi.append(m.nome)
    prova = Mesh("SM_Gatto_ProvaAssi")
    prova.blocco("Pelo", 0, 100, -5, 5, 0, 10)
    prova.blocco("Pelo", -5, 5, 0, 300, 0, 10)
    prova.blocco("Pelo", -5, 5, -5, 5, 0, 50)
    prova.scrivi(os.path.join(uscita, prova.nome + ".glb"))
    suoni(uscita)
    print("gatto: %s, due suoni" % ", ".join(nomi))


if __name__ == "__main__":
    principale(sys.argv[1] if len(sys.argv) > 1 else "gatto")
