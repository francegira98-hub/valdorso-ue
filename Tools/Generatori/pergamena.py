# Valdorso - Generatore della pergamena del Registro (05/10/2026, Claude).
# Crea T_Pergamena.png: carta antica con fibre, macchie, bordi scuriti e bruciati (bordo trasparente e irregolare).
# Si usa in Slate come "Box" a nove parti: i bordi bruciati restano uguali, il centro si allunga.
# Uso: python pergamena.py [cartella_di_uscita]   (serve numpy, Pillow, scipy)

import sys
import numpy as np
from PIL import Image
from scipy import ndimage

L, A = 1024, 1280          # larghezza e altezza
SEME = 312                 # l'anno dopo il Crepuscolo: sempre la stessa pergamena
rng = np.random.default_rng(SEME)


def rumore(scala, ottave=5, persistenza=0.55):
    """Rumore frattale morbido (somma di rumori sfocati a scale diverse), da 0 a 1."""
    tot = np.zeros((A, L))
    amp, peso = 1.0, 0.0
    s = scala
    for _ in range(ottave):
        base = rng.random((max(2, A // s + 2), max(2, L // s + 2)))
        grande = ndimage.zoom(base, (A / base.shape[0] * 1.02, L / base.shape[1] * 1.02), order=3)[:A, :L]
        tot += grande * amp
        peso += amp
        amp *= persistenza
        s = max(1, s // 2)
    tot /= peso
    return (tot - tot.min()) / (tot.max() - tot.min())


# --- Colore di base: pergamena calda, più chiara al centro ---
chiaro = np.array([236, 222, 188], float)
scuro = np.array([196, 166, 118], float)
n1 = rumore(256)
n2 = rumore(48, 4)
mix = 0.55 * n1 + 0.45 * n2

y, x = np.mgrid[0:A, 0:L]
cx, cy = (x - L / 2) / (L / 2), (y - A / 2) / (A / 2)
vignetta = np.clip(np.sqrt(cx ** 2 * 0.9 + cy ** 2 * 0.8), 0, 1.4)
t = np.clip(0.25 + 0.45 * mix + 0.35 * vignetta ** 2.2, 0, 1)[..., None]
img = chiaro * (1 - t) + scuro * t

# --- Fibre: tratti sottili orizzontali appena visibili ---
fibre = rng.random((A, L))
fibre = ndimage.gaussian_filter(fibre, sigma=(0.9, 4))
fibre = (fibre - fibre.mean()) / fibre.std()
img += fibre[..., None] * np.array([1.8, 1.6, 1.2])

# --- Grana fine ---
grana = ndimage.gaussian_filter(rng.standard_normal((A, L)), 0.7)
img += grana[..., None] * 3.0

# --- Macchie (gore d'acqua e di tempo): anelli più scuri ---
for _ in range(6):
    r = rng.uniform(40, 170)
    mx, my = rng.uniform(0, L), rng.uniform(0, A)
    d = np.sqrt((x - mx) ** 2 + (y - my) ** 2) / r
    forma = rumore(48, 3) * 1.1
    d = d + forma - 0.25
    alone = np.exp(-((d - 1.0) ** 2) / 0.02) * 0.35 + np.clip(1 - d, 0, 1) * 0.18
    img -= alone[..., None] * np.array([18, 22, 26]) * rng.uniform(0.4, 1.0)

# --- Bordo bruciato: distanza dal bordo deformata dal rumore ---
dist = np.minimum.reduce([x, L - 1 - x, y, A - 1 - y]).astype(float)
deforma = (rumore(96, 4) - 0.5) * 44 + (rumore(24, 3) - 0.5) * 12
d = dist + deforma
taglio = 18.0                                   # dove finisce la carta
alpha = np.clip((d - taglio) / 2.5, 0, 1)        # bordo netto ma morbido
brucia = np.clip(1 - (d - taglio) / 26.0, 0, 1)  # fascia bruciata verso il bordo
brucia = brucia ** 1.6
nero = np.array([46, 26, 12], float)
img = img * (1 - brucia[..., None] * 0.92) + nero * (brucia[..., None] * 0.92)

# Bordo scurito più largo (la carta vecchia ingiallisce ai lati)
ingiallisce = np.clip(1 - (d - taglio) / 140.0, 0, 1) ** 2
img -= ingiallisce[..., None] * np.array([14, 22, 34])

img = np.clip(img, 0, 255)
rgba = np.dstack([img, alpha * 255]).astype(np.uint8)

uscita = sys.argv[1] if len(sys.argv) > 1 else "."
Image.fromarray(rgba, "RGBA").save(f"{uscita}/T_Pergamena.png", optimize=True)
print("Fatto:", f"{uscita}/T_Pergamena.png", rgba.shape)
