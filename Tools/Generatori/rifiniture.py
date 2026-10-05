# Valdorso - Generatore delle rifiniture del palco del Registro (passo 4.2c, 06/10/2026, Claude).
# Crea, senza licenze:
#   T_Cornice.png  la cornice d'oro intagliata del ritratto (a nove pezzi: si allunga senza deformare gli angoli);
#   T_Sigillo.png  il sigillo di ceralacca con l'impronta della zampa dell'Orso, le colature e il bordo schiacciato;
#   T_Penna.png    la penna d'oca, cursore del mouse nel Registro (la punta è nell'angolo in alto a sinistra).
# Uso: python rifiniture.py [cartella_di_uscita]   (serve numpy, Pillow, scipy)

import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from scipy import ndimage

USCITA = sys.argv[1] if len(sys.argv) > 1 else "."
rng = np.random.default_rng(1306)


def luce(altezza, forza=1.0):
    """Ombreggiatura da una mappa di altezze: luce dall'alto a sinistra, come le candele del palco."""
    gy, gx = np.gradient(altezza)
    n = np.dstack([-gx * forza, -gy * forza, np.ones_like(altezza)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    l = np.array([-0.55, -0.65, 0.75])
    l /= np.linalg.norm(l)
    diffusa = np.clip((n * l).sum(axis=2), 0, 1)
    h = l + np.array([0, 0, 1.0])
    h /= np.linalg.norm(h)
    speculare = np.clip((n * h).sum(axis=2), 0, 1) ** 40
    return diffusa, speculare


def tinta(valore, scuro, medio, chiaro):
    """Da 0 a 1 attraverso tre colori."""
    v = np.clip(valore, 0, 1)[..., None]
    basso = scuro + (medio - scuro) * np.clip(v * 2, 0, 1)
    alto = medio + (chiaro - medio) * np.clip(v * 2 - 1, 0, 1)
    return np.where(v < 0.5, basso, alto)


# ---------------------------------------------------------------- La cornice d'oro (384x384, bordo largo 64)
L = 384
B = 64
y, x = np.mgrid[0:L, 0:L].astype(float)
d = np.minimum.reduce([x, y, L - 1 - x, L - 1 - y])      # distanza dal bordo esterno (angoli a 45 gradi)
lungo = np.where(np.minimum(x, L - 1 - x) < np.minimum(y, L - 1 - y), y, x)   # coordinata lungo il lato

alt = np.zeros((L, L))
# Profilo: filo esterno, grande modanatura tonda, gola, fila di perline, filo interno.
alt += np.exp(-((d - 4) / 2.5) ** 2) * 0.6
alt += np.clip(1 - ((d - 20) / 11) ** 2, 0, 1) ** 0.7 * 1.6
alt -= np.exp(-((d - 33) / 2.5) ** 2) * 0.5
perline = (np.cos(2 * np.pi * lungo / 12.0) * 0.5 + 0.5) ** 2.5
alt += np.exp(-((d - 43) / 4.0) ** 2) * (0.35 + 0.8 * perline)
alt += np.exp(-((d - 55) / 2.0) ** 2) * 0.7
alt += np.clip(1 - np.abs(d - 60) / 4, 0, 1) * 0.3
# Intagli a foglia sulla modanatura grande (onde leggere lungo il lato).
alt += np.exp(-((d - 20) / 9) ** 2) * (np.sin(2 * np.pi * lungo / 36.0) * 0.25)
# Rosette negli angoli.
for cx, cy in [(B / 2, B / 2), (L - 1 - B / 2, B / 2), (B / 2, L - 1 - B / 2), (L - 1 - B / 2, L - 1 - B / 2)]:
    r = np.sqrt((x - cx) ** 2 + (y - cy) ** 2)
    ang = np.arctan2(y - cy, x - cx)
    petali = (np.cos(8 * ang) * 0.5 + 0.5)
    rosa = np.clip(1 - r / 24, 0, 1)
    alt = np.where(r < 26, np.maximum(alt, 0.6 + rosa * (1.3 + 0.5 * petali) + np.exp(-(r / 5) ** 2) * 0.8), alt)
alt = ndimage.gaussian_filter(alt, 0.8) * 9
diff, spec = luce(alt, 1.0)
grana = ndimage.gaussian_filter(rng.standard_normal((L, L)), 1.0) * 0.05
oro = tinta(diff * 0.9 + grana + 0.08, np.array([70, 46, 14.0]), np.array([168, 128, 52.0]), np.array([246, 222, 150.0]))
oro += spec[..., None] * np.array([255, 240, 200.0]) * 0.55
# Patina scura nelle gole (lo sporco dei secoli).
cavita = np.clip(ndimage.gaussian_filter(alt, 3) - alt, 0, None)
oro *= (1 - np.clip(cavita * 0.18, 0, 0.5))[..., None]
alfa = np.clip((B - d) / 1.2, 0, 1) * np.clip((d + 0.5) / 1.0, 0, 1)
img = np.dstack([np.clip(oro, 0, 255), alfa * 255]).astype(np.uint8)
Image.fromarray(img, "RGBA").save(f"{USCITA}/T_Cornice.png", optimize=True)
print("Fatto: T_Cornice")

# ---------------------------------------------------------------- Il sigillo di ceralacca (256x256)
S = 256
y, x = np.mgrid[0:S, 0:S].astype(float)
cx, cy = S / 2, S / 2 - 6
ang = np.arctan2(y - cy, x - cx)
bordo = 96 + 6 * np.sin(5 * ang + 1) + 4 * np.sin(11 * ang + 2) + 3 * np.sin(17 * ang)
r = np.sqrt((x - cx) ** 2 + (y - cy) ** 2)
dentro = r < bordo
# Colature: due gocce che scendono.
gocce = np.zeros((S, S), bool)
for gx, lunga, larga in [(cx - 34, 34, 13), (cx + 22, 24, 10)]:
    gy0 = cy + np.sqrt(max(0, bordo.mean() ** 2 - (gx - cx) ** 2)) - 10
    gocce |= ((x - gx) / larga) ** 2 + ((y - gy0) / lunga) ** 2 < 1
massa = dentro | gocce
alt = np.where(massa, 1.0, 0.0)
alt = ndimage.gaussian_filter(alt, 6) * 3.0                 # cupola morbida
# Anello schiacciato dal timbro e piano interno.
alt -= np.exp(-((r - 70) / 4) ** 2) * 0.55 * dentro
alt = np.where(r < 66, np.minimum(alt, 2.2), alt)
# L'impronta della zampa dell'Orso, incisa (cuscinetto e quattro dita).
zampa = np.zeros((S, S))
zampa += (((x - cx) / 26) ** 2 + ((y - (cy + 14)) / 21) ** 2 < 1).astype(float)
for dx, dy, rr in [(-27, -14, 9.5), (-10, -27, 10.5), (10, -27, 10.5), (27, -14, 9.5)]:
    zampa += (((x - cx - dx) / rr) ** 2 + ((y - cy - dy) / (rr * 1.15)) ** 2 < 1).astype(float)
zampa = ndimage.gaussian_filter(np.clip(zampa, 0, 1), 1.6)
alt -= zampa * 0.75
alt += ndimage.gaussian_filter(rng.standard_normal((S, S)), 2) * 0.02
diff, spec = luce(alt * 14, 1.0)
cera = tinta(diff * 0.95 + 0.05, np.array([48, 8, 6.0]), np.array([118, 24, 20.0]), np.array([186, 62, 50.0]))
cera += spec[..., None] * np.array([255, 210, 200.0]) * 0.8
alfa = ndimage.gaussian_filter(massa.astype(float), 0.8)
img = np.dstack([np.clip(cera, 0, 255), np.clip(alfa, 0, 1) * 255]).astype(np.uint8)
Image.fromarray(img, "RGBA").save(f"{USCITA}/T_Sigillo.png", optimize=True)
print("Fatto: T_Sigillo")

# ---------------------------------------------------------------- La penna d'oca (64x64, punta in alto a sinistra)
G = 4                                                       # si disegna grande e si rimpicciolisce: bordi morbidi
P = 64 * G
penna = Image.new("RGBA", (P, P), (0, 0, 0, 0))
d = ImageDraw.Draw(penna)
punta = np.array([2.0, 2.0]) * G
fine = np.array([60.0, 60.0]) * G
asse = (fine - punta) / np.linalg.norm(fine - punta)
normale = np.array([-asse[1], asse[0]])
# La piuma (vessillo): una vela allungata da un solo lato del calamo, con le barbe.
vela = []
for t in np.linspace(0.22, 1.0, 40):
    larg = np.sin(np.pi * (t - 0.22) / 0.78) ** 0.7 * 15 * G
    p = punta + (fine - punta) * t + normale * larg
    vela.append(tuple(p))
for t in np.linspace(1.0, 0.22, 40):
    larg = np.sin(np.pi * (t - 0.22) / 0.78) ** 0.9 * 4 * G
    p = punta + (fine - punta) * t - normale * larg
    vela.append(tuple(p))
d.polygon(vela, fill=(236, 226, 204, 255), outline=(120, 100, 76, 255))
for t in np.linspace(0.26, 0.97, 22):
    base = punta + (fine - punta) * t
    larg = np.sin(np.pi * (t - 0.22) / 0.78) ** 0.7 * 15 * G
    cima = base + normale * larg * 0.95 + asse * 6 * G
    d.line([tuple(base), tuple(cima)], fill=(176, 160, 136, 255), width=G)
# Il calamo e il pennino tinto d'inchiostro.
d.line([tuple(punta + asse * 3 * G), tuple(fine)], fill=(96, 74, 50, 255), width=2 * G)
d.polygon([tuple(punta), tuple(punta + asse * 9 * G + normale * 2 * G), tuple(punta + asse * 9 * G - normale * 2 * G)], fill=(30, 20, 14, 255))
penna = penna.resize((64, 64), Image.LANCZOS)
ombra = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
ombra.paste((0, 0, 0, 110), (2, 2), penna.split()[3])
ombra = ombra.filter(ImageFilter.GaussianBlur(1.2))
ombra.alpha_composite(penna)
ombra.save(f"{USCITA}/T_Penna.png", optimize=True)
print("Fatto: T_Penna")
