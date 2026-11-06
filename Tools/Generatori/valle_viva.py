# Valdorso - La valle che reagisce (v0.1.3): gli uccellini e i pesci, e i loro suoni. Scritto da Claude il 06/11/2026.
# Fa da zero (niente licenze), in una cartella:
#   SM_Uccello.glb   un passero stilizzato (35 cm di apertura d'ali), X avanti, ali lungo Y, colori nei vertici
#                    (il materiale M_Uccello li usa e muove le ali con il World Position Offset, più forte lontano
#                    dal corpo: niente ossa, costa pochissimo anche con tanti uccelli)
#   SM_Pesce.glb     una trota (35 cm), X avanti, colori nei vertici
#   S_Ali.wav        lo stormo che si alza: un frullo d'ali e due pigolii d'allarme
#   S_Tuffo.wav      il pesce che ricade in acqua
# Li importa Content/Python/nuovi/crea_valle_viva.py; la logica è in ValdorsoValleViva.h/.cpp.
# Uso: python valle_viva.py <uscita>

import math
import os
import sys

import numpy as np
from scipy import signal
from scipy.io import wavfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from villaggio_mesh import Mesh  # noqa: E402

SR = 48000


def _fuso(m, mat, lunghezza, raggi, colori, asse_x0=0.0, lati=10, schiaccia_z=1.0):
    """Un corpo a fuso lungo X: anelli di raggio 'raggi' (da coda a testa), con i colori (sopra, sotto) per anello."""
    n = len(raggi)
    P, N, UV, C, T = [], [], [], [], []
    for i, r in enumerate(raggi):
        x = asse_x0 + lunghezza * i / (n - 1)
        sopra, sotto = colori[i]
        for k in range(lati + 1):
            a = 2 * math.pi * k / lati
            y, z = math.cos(a) * r, math.sin(a) * r * schiaccia_z
            P.append([x, y, z])
            N.append([0.0, math.cos(a), math.sin(a)])
            UV.append([i / (n - 1), k / lati])
            t = 0.5 + 0.5 * math.sin(a)            # 1 sopra, 0 sotto
            C.append([sopra[j] * t + sotto[j] * (1 - t) for j in range(3)] + [1.0])
    for i in range(n - 1):
        for k in range(lati):
            a = i * (lati + 1) + k
            b = a + lati + 1
            T += [[a, b, a + 1], [a + 1, b, b + 1]]
    m.triangoli(mat, P, N, UV, T, C)


def uccello(file):
    m = Mesh("SM_Uccello")
    marrone, panna, scuro, becco = [0.36, 0.24, 0.14], [0.78, 0.70, 0.58], [0.20, 0.14, 0.09], [0.25, 0.22, 0.18]
    # il corpo, dalla coda (x -7) alla testa (x +7)
    raggi = [0.8, 2.4, 3.6, 3.9, 3.6, 2.8, 3.0, 2.8, 1.4]
    colori = [(marrone, panna)] * 6 + [([0.30, 0.20, 0.12], [0.85, 0.80, 0.70])] * 3
    _fuso(m, "Uccello", 14.0, raggi, colori, asse_x0=-7.0, schiaccia_z=0.9)
    # il becco
    c = [becco + [1.0]] * 3
    m.triangoli("Uccello", [[7.0, -0.8, 0.0], [7.0, 0.8, 0.0], [9.2, 0.0, -0.3]], [[0, 0, 1]] * 3, [[0, 0], [1, 0], [0.5, 1]], [[0, 1, 2], [0, 2, 1]], c)
    # la coda: un ventaglio piatto
    cc = [scuro + [1.0]] * 4
    m.triangoli("Uccello", [[-6.0, -1.2, 0.5], [-6.0, 1.2, 0.5], [-12.0, 2.6, 0.2], [-12.0, -2.6, 0.2]], [[0, 0, 1]] * 4,
                [[0, 0], [1, 0], [1, 1], [0, 1]], [[0, 1, 2], [0, 2, 3], [0, 2, 1], [0, 3, 2]], cc)
    # le ali: piatte, dal fianco (y = 3) a 17.5 cm, con la punta più scura (il materiale le batte con |y|)
    for s in (-1, 1):
        P = [[3.5, 3.0 * s, 1.0], [-3.0, 3.0 * s, 1.0], [-5.0, 11.0 * s, 1.2], [-1.0, 17.5 * s, 1.0], [3.0, 12.0 * s, 1.2]]
        C = [marrone + [1.0], marrone + [1.0], scuro + [1.0], scuro + [1.0], scuro + [1.0]]
        T = [[0, 1, 2], [0, 2, 4], [4, 2, 3]]
        T2 = [[a, c2, b] for a, b, c2 in T]                 # due facce
        m.triangoli("Uccello", P, [[0, 0, 1]] * 5, [[0, 0], [1, 0], [1, 1], [0.5, 1], [0, 1]], T + T2, C)
    m.scrivi(file)


def pesce(file):
    m = Mesh("SM_Pesce")
    dorso, ventre = [0.25, 0.30, 0.24], [0.85, 0.85, 0.80]
    raggi = [0.6, 1.8, 3.2, 4.2, 4.6, 4.4, 3.8, 2.6, 0.8]
    _fuso(m, "Pesce", 28.0, raggi, [(dorso, ventre)] * 9, asse_x0=-14.0, lati=10, schiaccia_z=1.0)
    # schiacciato ai lati: si stringe Y a posteriori
    for parte in m.parti.values():
        for P in parte[0]:
            P[:, 1] *= 0.45
    # la coda a forbice
    c = [dorso + [1.0]] * 4
    m.triangoli("Pesce", [[-13.0, 0.0, 0.6], [-13.0, 0.0, -0.6], [-20.0, 0.0, -5.0], [-20.0, 0.0, 5.0]], [[0, 1, 0]] * 4,
                [[0, 0], [1, 0], [1, 1], [0, 1]], [[0, 1, 2], [0, 2, 3], [0, 2, 1], [0, 3, 2]], c)
    # la pinna sul dorso
    m.triangoli("Pesce", [[2.0, 0.0, 3.8], [-6.0, 0.0, 3.4], [-3.0, 0.0, 7.0]], [[0, 1, 0]] * 3, [[0, 0], [1, 0], [0.5, 1]],
                [[0, 1, 2], [0, 2, 1]], c[:3])
    m.scrivi(file)


# ------------------------------------------------------------------------------------------------ i suoni

def ali(seme=21):
    """Il frullo d'ali dello stormo che si alza (1,4 s) e due pigolii d'allarme."""
    rng = np.random.default_rng(seme)
    n = int(1.4 * SR)
    x = np.zeros(n)
    for uccello in range(8):
        inizio = rng.uniform(0.0, 0.25)
        rate = rng.uniform(14, 20)
        forza = rng.uniform(0.4, 1.0)
        for k in range(int(rate * 1.0)):
            t0 = inizio + k / rate
            i = int(t0 * SR)
            m = int(0.03 * SR)
            if i + m >= n:
                break
            colpo = rng.standard_normal(m) * np.exp(-np.arange(m) / (m / 4))
            x[i:i + m] += colpo * forza * math.exp(-k / (rate * 0.6))
    sos = signal.butter(2, [250 / (SR / 2), 3500 / (SR / 2)], btype="band", output="sos")
    x = signal.sosfilt(sos, x)
    for t0 in (0.05, 0.22):
        m = int(0.07 * SR)
        tt = np.arange(m) / SR
        f = np.linspace(rng.uniform(5200, 6000), rng.uniform(3800, 4400), m)
        cip = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.sin(np.pi * tt / tt[-1]) ** 2
        i = int(t0 * SR)
        x[i:i + m] += cip * 0.35
    return x / np.max(np.abs(x)) * 0.8


def tuffo(seme=22):
    """Il pesce che ricade in acqua (0,8 s): lo schiaffo, qualche goccia e le bolle."""
    rng = np.random.default_rng(seme)
    n = int(0.8 * SR)
    tt = np.arange(n) / SR
    x = rng.standard_normal(n) * np.exp(-tt / 0.06)
    sos = signal.butter(2, [400 / (SR / 2), 6000 / (SR / 2)], btype="band", output="sos")
    x = signal.sosfilt(sos, x)
    for _ in range(10):
        d = rng.uniform(0.02, 0.06)
        m = int(d * SR)
        t2 = np.arange(m) / SR
        f0 = rng.uniform(600, 1600)
        b = np.sin(2 * np.pi * np.cumsum(f0 * (1 + 2.0 * t2 / d)) / SR) * np.exp(-t2 / (d / 3))
        i = int(rng.uniform(0.05, 0.6) * SR)
        x[i:i + m] += b * rng.uniform(0.1, 0.35)
    return x / np.max(np.abs(x)) * 0.8


if __name__ == "__main__":
    uscita = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(uscita, exist_ok=True)
    uccello(os.path.join(uscita, "SM_Uccello.glb"))
    pesce(os.path.join(uscita, "SM_Pesce.glb"))
    for nome, x in (("S_Ali", ali()), ("S_Tuffo", tuffo())):
        wavfile.write(os.path.join(uscita, nome + ".wav"), SR, (x * 32767).astype(np.int16))
    print("valle viva: SM_Uccello, SM_Pesce, S_Ali, S_Tuffo in", uscita)
