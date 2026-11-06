# Valdorso - Il tempo della valle (v0.1.3): la pioggia. Scritto da Claude il 06/11/2026, mentre Fra dormiva.
# Fa da zero (niente licenze), in una cartella:
#   T_Pioggia.png    le gocce che cadono (512 x 1024, si ripete senza cuciture): righe sottili, bianche su nero
#   S_Pioggia.wav    la pioggia che si ripete senza scatti (12 s): fruscio filtrato e gocce
#   SM_TendaPioggia.glb  la tenda su cui scendono le gocce: un tubo senza tappi (diametro 1 m, alto 1 m, centro nel
#                    mezzo, come il cilindro di Unreal), così guardando in alto non si vede un disco di pioggia
# Li importa e li usa Content/Python/nuovi/crea_tempo.py (materiale M_Pioggia); la logica è in AValdorsoTempo (C++).
# Uso: python tempo.py <uscita>

import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from scipy import signal
from scipy.io import wavfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from villaggio_mesh import Mesh  # noqa: E402

SR = 48000


def texture_pioggia(file, w=512, h=1024, seme=12):
    """Righe di pioggia di lunghezze e luminosità diverse, appena inclinate; disegnate su 3x3 per ripetersi bene."""
    rng = np.random.default_rng(seme)
    im = Image.new("L", (w * 3, h * 3), 0)
    d = ImageDraw.Draw(im)
    for _ in range(550):
        x = rng.uniform(0, w)
        y = rng.uniform(0, h)
        lung = rng.uniform(30, 120)
        luce = int(rng.uniform(70, 255))
        spessore = 1 if rng.random() < 0.8 else 2
        for ox in (0, w, 2 * w):
            for oy in (0, h, 2 * h):
                d.line([(x + ox, y + oy), (x + ox + lung * 0.04, y + oy + lung)], fill=luce, width=spessore)
    centro = im.crop((w, h, 2 * w, 2 * h)).filter(ImageFilter.GaussianBlur(0.6))
    centro.convert("RGB").save(file)


def suono_pioggia(file, secondi=12.0, seme=5):
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    # il fruscio: rumore rosa-ish filtrato (molte gocce lontane)
    rumore = rng.standard_normal(n * 3)
    rumore = signal.lfilter(*signal.butter(2, [400 / (SR / 2), 9000 / (SR / 2)], btype="band"), rumore)
    rumore = rumore[n:2 * n]
    # un respiro lento (raffiche), che torna uguale alla fine del giro
    tt = np.arange(n) / SR
    rumore *= 0.75 + 0.25 * np.sin(2 * np.pi * tt / secondi * 3)
    # le gocce vicine: piccoli colpi secchi (in giro: una goccia che passa la fine riparte dall'inizio)
    gocce = np.zeros(n)
    for _ in range(int(secondi * 25)):
        i = int(rng.uniform(0, n))
        f = rng.uniform(1800, 5000)
        dur = int(SR * rng.uniform(0.008, 0.025))
        k = np.arange(dur)
        colpo = np.sin(2 * np.pi * f * k / SR) * np.exp(-k / (dur / 4)) * rng.uniform(0.2, 1.0)
        idx = (i + k) % n
        gocce[idx] += colpo
    x = rumore / np.max(np.abs(rumore)) * 0.8 + gocce / (np.max(np.abs(gocce)) + 1e-9) * 0.25
    x = x / np.max(np.abs(x)) * 0.8
    wavfile.write(file, SR, (x * 32767).astype(np.int16))


def tenda(file, lati=32):
    m = Mesh("SM_TendaPioggia")
    r = 50.0
    for i in range(lati):
        a0, a1 = 2 * np.pi * i / lati, 2 * np.pi * (i + 1) / lati
        p = [np.array([r * np.cos(a0), r * np.sin(a0), 50.0]), np.array([r * np.cos(a1), r * np.sin(a1), 50.0]),
             np.array([r * np.cos(a1), r * np.sin(a1), -50.0]), np.array([r * np.cos(a0), r * np.sin(a0), -50.0])]
        n0 = [-np.cos(a0), -np.sin(a0), 0.0]
        n1 = [-np.cos(a1), -np.sin(a1), 0.0]
        # u gira intorno, v va dall'alto (0) al basso (1): le gocce scendono con la v
        uv = [[i / lati, 0.0], [(i + 1) / lati, 0.0], [(i + 1) / lati, 1.0], [i / lati, 1.0]]
        m.triangoli("Pioggia", p, [n0, n1, n1, n0], uv, [[0, 1, 2], [0, 2, 3]])
    m.scrivi(file)


if __name__ == "__main__":
    uscita = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(uscita, exist_ok=True)
    texture_pioggia(os.path.join(uscita, "T_Pioggia.png"))
    suono_pioggia(os.path.join(uscita, "S_Pioggia.wav"))
    tenda(os.path.join(uscita, "SM_TendaPioggia.glb"))
    print("tempo: T_Pioggia.png e S_Pioggia.wav in", uscita)
