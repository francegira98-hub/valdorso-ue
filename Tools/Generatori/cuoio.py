# Valdorso - Generatore del cuoio dei pulsanti del Registro (05/10/2026, Claude).
# Crea T_Cuoio.png: cuoio scuro con la grana, i bordi consumati e una cucitura chiara tutto intorno.
# Si usa in Slate come "Box" a nove parti: bordi e cucitura restano uguali, il centro si allunga col pulsante.
# Uso: python cuoio.py [cartella_di_uscita]   (serve numpy, Pillow, scipy)

import sys
import numpy as np
from PIL import Image
from scipy import ndimage

L, A = 512, 160
rng = np.random.default_rng(1312)

y, x = np.mgrid[0:A, 0:L]

# Grana: rumore a celle (pori del cuoio) più rumore morbido.
pori = ndimage.gaussian_filter(rng.random((A, L)), 1.2)
pori = (pori - pori.min()) / (pori.max() - pori.min())
morbido = ndimage.gaussian_filter(rng.random((A // 8 + 2, L // 8 + 2)), 1.0)
morbido = ndimage.zoom(morbido, (A / morbido.shape[0], L / morbido.shape[1]), order=3)[:A, :L]
morbido = (morbido - morbido.min()) / (morbido.max() - morbido.min())

base = np.array([92, 58, 36], float)        # marrone cuoio
scuro = np.array([48, 28, 16], float)
t = np.clip(0.35 * morbido + 0.35 * pori, 0, 1)[..., None]
img = base * (1 - t) + scuro * t

# Bordi consumati: più scuri verso il margine, con angoli arrotondati.
r = 14.0
dx = np.minimum(x, L - 1 - x).astype(float)
dy = np.minimum(y, A - 1 - y).astype(float)
dist = np.where((dx < r) & (dy < r), r - np.sqrt((r - dx) ** 2 + (r - dy) ** 2), np.minimum(dx, dy))
alpha = np.clip(dist / 1.5, 0, 1)
bordo = np.clip(1 - dist / 16.0, 0, 1) ** 1.5
img = img * (1 - bordo[..., None] * 0.55)

# Luce morbida dall'alto (il cuoio è un poco bombato).
luce = np.clip(1 - y / A, 0, 1) ** 2 * 18
img += luce[..., None] * np.array([1.0, 0.8, 0.6])

# Cucitura: trattini chiari a 9 pixel dal bordo.
passo, pieno = 10, 6
cuci = np.zeros((A, L), bool)
d_cuci = 9
riga = (np.abs(dist - d_cuci) < 1.0)
lungo = np.where(dy <= dx, x, y)            # coordinata lungo il bordo
cuci = riga & ((lungo % passo) < pieno)
filo = np.array([196, 160, 104], float)
ombra = ndimage.binary_dilation(cuci, iterations=1) & ~cuci
img[ombra] = img[ombra] * 0.6
img[cuci] = filo * 0.9 + img[cuci] * 0.1

img = np.clip(img, 0, 255)
rgba = np.dstack([img, alpha * 255]).astype(np.uint8)
uscita = sys.argv[1] if len(sys.argv) > 1 else "."
Image.fromarray(rgba, "RGBA").save(f"{uscita}/T_Cuoio.png", optimize=True)
print("Fatto:", f"{uscita}/T_Cuoio.png", rgba.shape)
