# Valdorso - Il sigillo della Corona sul passo (v0.1.3). Scritto da Claude il 06/11/2026.
# Il Design ("La barriera del passo", idea di Fra del 28/09): dietro la sbarra delle guardie, un velo luminoso con le
# rune che scorrono attraversa la gola da parete a parete, più forte di notte, con un ronzio basso; chi lo tocca
# viene respinto ("Il sigillo della Corona ti respinge"). Approvato da Fra il 06/11 (passo B).
#
# Fa tre cose, tutte da zero (niente licenze), nella cartella di uscita del villaggio:
#   T_SigilloRune.png      le rune che scorrono (1024 x 1024, si ripete senza cuciture), bianco su nero
#   S_SigilloRonzio.wav    il ronzio (8 s che si ripetono senza scatti)
#   velo(...)              la mesh del velo: una tenda che segue il fondo della gola, nelle coordinate dell'attore
#                          (X = verso la valle, Y = di traverso, Z = su), sfumata in alto e ai lati con l'alfa
# La logica (respingere, aprirsi, pulsare) è nel C++: AValdorsoSigillo.

import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from scipy import signal
from scipy.io import wavfile

from villaggio_mesh import Mesh

SR = 48000


# ------------------------------------------------------------------------------------------------ le rune

def _runa(dr, cx, cy, h, rng, spessore):
    """Una runa inventata, angolosa, alta h pixel: un'asta e da 1 a 3 rami (come le rune degli Antichi del menu)."""
    w = h * 0.55
    alto, basso = cy - h / 2, cy + h / 2
    tratti = [((cx, alto), (cx, basso))]
    for _ in range(int(rng.integers(1, 4))):
        y0 = rng.uniform(alto, basso - h * 0.2)
        lato = rng.choice([-1, 1])
        y1 = y0 + rng.uniform(-0.35, 0.35) * h
        tratti.append(((cx, y0), (cx + lato * w * rng.uniform(0.5, 1.0), y1)))
    if rng.random() < 0.3:
        tratti.append(((cx - w * 0.5, cy), (cx + w * 0.5, cy - h * 0.25)))
    for a, b in tratti:
        dr.line([a, b], fill=255, width=spessore)


def texture_rune(file, lato=1024, seme=88):
    """Colonne di rune a diverse grandezze, con un alone morbido; si ripete in orizzontale e in verticale."""
    rng = np.random.default_rng(seme)
    grande = lato * 3                       # si disegna su 3x3 e si tiene il centro: così i bordi si ripetono
    im = Image.new("L", (grande, grande), 0)
    dr = ImageDraw.Draw(im)
    colonne = 9
    for c in range(colonne):
        x = (c + 0.5) * lato / colonne + rng.uniform(-12, 12)
        y = rng.uniform(0, 60)
        while y < lato - 30:
            h = rng.uniform(34, 70)
            for ox in (-lato, 0, lato):
                for oy in (-lato, 0, lato):
                    _runa(dr, lato + x + ox, lato + y + oy, h, np.random.default_rng(int(x * 7 + y * 13)), 4)
            y += h * rng.uniform(1.5, 2.6)
    centro = im.crop((lato, lato, 2 * lato, 2 * lato))
    alone = centro.filter(ImageFilter.GaussianBlur(9))
    a = np.asarray(centro, np.float32) / 255.0
    b = np.asarray(alone, np.float32) / 255.0
    v = np.clip(a * 0.85 + b * 1.6, 0, 1)
    Image.fromarray((v * 255).astype(np.uint8), "L").convert("RGB").save(file)


# ------------------------------------------------------------------------------------------------ il ronzio

def ronzio(file, secondi=8.0, seme=7):
    """Un ronzio basso e cupo: frequenze che stanno intere negli 8 secondi (così il giro torna uguale), un lento
    battere tra due toni vicini e un crepitio leggero, come elettricità nell'aria."""
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    tt = np.arange(n) / SR
    passo = 1.0 / secondi                    # 0,125 Hz: ogni frequenza multipla torna a zero alla fine
    x = np.zeros(n)
    for f, a in ((55.0, 1.0), (55.0 + 4 * passo, 0.6), (110.0, 0.45), (165.0, 0.18), (220.0 + 2 * passo, 0.12)):
        x += a * np.sin(2 * np.pi * f * tt)
    # un respiro lento (un giro ogni 4 s)
    x *= 0.8 + 0.2 * np.sin(2 * np.pi * (2 * passo) * tt)
    # il crepitio: rumore a colpetti, filtrato in alto, ripetuto in giro
    crepitio = rng.standard_normal(n) * (rng.random(n) < 0.004)
    # filtrato su tre giri attaccati e poi si tiene quello di mezzo: così anche il crepitio si ripete senza scatti
    tre = np.tile(crepitio, 3)
    tre = signal.lfilter(*signal.butter(2, [1500 / (SR / 2), 6000 / (SR / 2)], btype="band"), tre)
    tre = np.convolve(tre, np.exp(-np.arange(200) / 40.0), mode="same")
    crepitio = tre[n:2 * n]
    x = x / np.max(np.abs(x)) + crepitio / (np.max(np.abs(crepitio)) + 1e-9) * 0.12
    x = x / np.max(np.abs(x)) * 0.8
    wavfile.write(file, SR, (x * 32767).astype(np.int16))


# ------------------------------------------------------------------------------------------------ il velo

def velo(t, P, along, quota_porta, traverso=(-45.0, 45.0), alto_sopra=1400.0, passo=1.0):
    """La tenda del sigillo, di traverso alla gola, 'along' metri dalla porta (negativo = verso il passo).
    P(along, across) -> cm del paesaggio. Va da parete a parete: dove il fondo sale più di alto_sopra sopra la porta,
    lì c'è la roccia e il velo finisce. Restituisce (mesh, dati) con le misure per il C++ (cm, relative all'attore,
    che sta a P(along, 0) alla quota della porta)."""
    m = Mesh("SM_Guardia_Sigillo")
    O = np.array(P(along, 0.0), np.float64)
    cima = quota_porta + alto_sopra
    punti = []
    for s in np.arange(traverso[0], traverso[1] + 1e-6, passo):
        X, Y = P(along, s)
        q = t.quota(X, Y)
        punti.append((s, q))
    # il tratto libero (dove il fondo sta sotto la cima) che contiene il mezzo della strada
    liberi = [q < cima - 200.0 for s, q in punti]
    i0 = i1 = len(punti) // 2
    while i0 > 0 and liberi[i0 - 1]:
        i0 -= 1
    while i1 < len(punti) - 1 and liberi[i1 + 1]:
        i1 += 1
    # un metro dentro la roccia per parte, così non resta una fessura
    i0, i1 = max(0, i0 - 1), min(len(punti) - 1, i1 + 1)
    tratto = punti[i0:i1 + 1]
    s0, s1 = tratto[0][0], tratto[-1][0]
    file_ = 6                                          # righe di vertici dal fondo alla cima (per sfumare)
    for (sa, qa), (sb, qb) in zip(tratto[:-1], tratto[1:]):
        for k in range(file_):
            def vert(s, q, j):
                fondo = q - 80.0 - quota_porta
                z = fondo + (cima - quota_porta - fondo) * j / file_
                return np.array([0.0, s * 100.0, z])

            a, b = vert(sa, qa, k), vert(sb, qb, k)
            c, d = vert(sb, qb, k + 1), vert(sa, qa, k + 1)

            def alfa(s, j):
                # sfuma in cima (ultima riga) e un poco ai due lati
                in_alto = 1.0 - max(0.0, (j - (file_ - 2)) / 2.0)
                ai_lati = min(1.0, (s - s0) / 4.0, (s1 - s) / 4.0)
                return float(np.clip(in_alto * max(ai_lati, 0.15), 0.0, 1.0))

            uv = [[sa / 6.0, -a[2] / 600.0], [sb / 6.0, -b[2] / 600.0], [sb / 6.0, -c[2] / 600.0], [sa / 6.0, -d[2] / 600.0]]
            C = [[1, 1, 1, alfa(sa, k)], [1, 1, 1, alfa(sb, k)], [1, 1, 1, alfa(sb, k + 1)], [1, 1, 1, alfa(sa, k + 1)]]
            m.triangoli("Sigillo", [a, b, c, d], [[1.0, 0, 0]] * 4, uv, [[0, 1, 2], [0, 2, 3]], C)
    fondo_min = min(q for s, q in tratto) - 80.0 - quota_porta
    dati = {"traverso_da": round(s0 * 100, 1), "traverso_a": round(s1 * 100, 1),
            "larghezza": round((s1 - s0) * 100, 1), "centro_traverso": round((s0 + s1) / 2 * 100, 1),
            "base": round(fondo_min, 1), "altezza": round(cima - quota_porta - fondo_min, 1)}
    return m, dati


def file_sigillo(cartella):
    texture_rune(os.path.join(cartella, "T_SigilloRune.png"))
    ronzio(os.path.join(cartella, "S_SigilloRonzio.wav"))
