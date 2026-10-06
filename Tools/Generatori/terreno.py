# Valdorso - Generatore delle texture del terreno della valle (v0.1.3). Scritto da Claude il 06/10/2026.
# Crea, senza licenze e sempre uguali (seme fisso), cinque superfici che si ripetono senza cuciture (tileable):
#   T_Erba, T_Roccia, T_Terra, T_Ghiaia, T_Neve   -> _C (colore, sRGB) e _N (normal map, DirectX come Unreal)
# e T_Variazione (macchie grandi in scala di grigi, per rompere la ripetizione da lontano).
# Le usa il materiale M_Valle (Content/Python/crea_materiale_valle.py).
# Uso: python terreno.py [cartella_di_uscita] [lato_pixel]   (serve numpy, Pillow, scipy)

import os
import sys

import numpy as np
from PIL import Image
from scipy.spatial import cKDTree

USCITA = sys.argv[1] if len(sys.argv) > 1 else "."
L = int(sys.argv[2]) if len(sys.argv) > 2 else 1024
rng = np.random.default_rng(2610)


def periodico(n, frequenza, pendenza=2.0):
    """Rumore che si ripete esattamente sui bordi: si costruisce nelle frequenze (FFT). Valori circa -1..1."""
    fy = np.fft.fftfreq(n)[:, None] * n
    fx = np.fft.fftfreq(n)[None, :] * n
    f = np.sqrt(fx * fx + fy * fy)
    f[0, 0] = 1.0
    spettro = (rng.standard_normal((n, n)) + 1j * rng.standard_normal((n, n))) / f ** pendenza
    spettro *= np.exp(-(f / (frequenza * 4.0)) ** 2)       # taglia i dettagli più fini della scala chiesta
    spettro *= 1 - np.exp(-(f / max(frequenza * 0.25, 0.5)) ** 2)
    spettro[0, 0] = 0
    r = np.real(np.fft.ifft2(spettro))
    return r / (np.abs(r).max() + 1e-9)


def ciottoli(n, quanti, raggio):
    """Celle di Voronoi periodiche: ciottoli e sassi. Restituisce altezza (cupole) e un valore casuale per ciottolo."""
    punti = rng.random((quanti, 2)) * n
    copie = np.concatenate([punti + np.array([dx, dy]) * n for dx in (-1, 0, 1) for dy in (-1, 0, 1)])
    indice_vero = np.tile(np.arange(quanti), 9)
    albero = cKDTree(copie)
    yy, xx = np.mgrid[0:n, 0:n]
    d, i = albero.query(np.column_stack([xx.ravel(), yy.ravel()]), k=2)
    d1 = d[:, 0].reshape(n, n)
    d2 = d[:, 1].reshape(n, n)
    bordo = np.clip((d2 - d1) / raggio, 0, 1)                # 0 sul bordo tra due ciottoli, 1 al centro
    cupola = np.sqrt(bordo) * np.clip(1 - d1 / (raggio * 2.2), 0, 1) ** 0.3
    valore = rng.random(quanti)[indice_vero[i[:, 0]]].reshape(n, n)
    return cupola, valore


def normale(altezza, forza):
    """Normal map dalla mappa delle altezze (bordi periodici). Verde invertito: convenzione DirectX di Unreal."""
    dx = (np.roll(altezza, -1, axis=1) - np.roll(altezza, 1, axis=1)) * 0.5 * forza
    dy = (np.roll(altezza, -1, axis=0) - np.roll(altezza, 1, axis=0)) * 0.5 * forza
    n = np.dstack([-dx, dy, np.ones_like(altezza)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return np.clip((n * 0.5 + 0.5) * 255, 0, 255).astype(np.uint8)


def tinta(valore, colori):
    """Da 0 a 1 attraverso una lista di colori."""
    v = np.clip(valore, 0, 1) * (len(colori) - 1)
    i = np.clip(np.floor(v).astype(int), 0, len(colori) - 2)
    t = (v - i)[..., None]
    c = np.array(colori, float)
    return c[i] * (1 - t) + c[i + 1] * t


def salva(nome, colore, altezza, forza):
    Image.fromarray(np.clip(colore, 0, 255).astype(np.uint8), "RGB").save(os.path.join(USCITA, f"T_{nome}_C.png"), optimize=True)
    Image.fromarray(normale(altezza, forza), "RGB").save(os.path.join(USCITA, f"T_{nome}_N.png"), optimize=True)
    print("Fatto:", nome)


os.makedirs(USCITA, exist_ok=True)

# ------------------------------------------------------------------ Erba: prato di montagna, fili e chiazze
macchie = periodico(L, 3, 1.6)
medio = periodico(L, 16, 1.4)
fili = periodico(L, 160, 0.6)
striature = np.abs(periodico(L, 90, 0.9))
alt = 0.5 * fili + 0.35 * medio - 0.4 * striature
valore = 0.5 + 0.25 * macchie + 0.18 * medio + 0.22 * fili
colore = tinta(valore, [(38, 52, 22), (64, 86, 34), (96, 116, 50), (138, 140, 72)])
secco = np.clip(macchie * 1.5 - 0.4, 0, 1)[..., None]        # qualche chiazza d'erba più secca
colore = colore * (1 - 0.35 * secco) + np.array([128, 112, 66.0]) * 0.35 * secco
salva("Erba", colore, alt, 3.0)

# ------------------------------------------------------------------ Roccia: strati, crepe e licheni
grande = periodico(L, 4, 1.8)
dettaglio = periodico(L, 40, 1.2)
yy = np.mgrid[0:L, 0:L][0] / L
strati = np.sin((yy * 9 + 0.35 * grande) * 2 * np.pi) * 0.5 + 0.5
crepe = 1 - np.abs(periodico(L, 22, 1.3)) ** 0.35
alt = 0.6 * grande + 0.25 * strati + 0.3 * dettaglio - 0.5 * np.clip(crepe - 0.75, 0, 1) * 4
valore = 0.45 + 0.25 * grande + 0.15 * dettaglio + 0.1 * strati
colore = tinta(valore, [(58, 54, 50), (96, 90, 82), (132, 124, 112), (168, 160, 146)])
colore *= (1 - 0.55 * np.clip(crepe - 0.75, 0, 1) * 4)[..., None]
licheni = np.clip(periodico(L, 12, 1.5) * 2 - 1.1, 0, 1)[..., None]
colore = colore * (1 - 0.4 * licheni) + np.array([118, 122, 70.0]) * 0.4 * licheni
salva("Roccia", colore, alt, 6.0)

# ------------------------------------------------------------------ Terra: sentieri e campi, con sassolini
base = periodico(L, 6, 1.6)
fine = periodico(L, 120, 0.8)
sassi, tono = ciottoli(L, 900, L / 160)
sassi_vis = (sassi > 0.05) & (tono > 0.55)
alt = 0.4 * base + 0.25 * fine + 0.8 * sassi * sassi_vis
valore = 0.5 + 0.3 * base + 0.15 * fine
colore = tinta(valore, [(62, 44, 30), (98, 72, 48), (130, 100, 68)])
colore = np.where(sassi_vis[..., None], tinta(0.3 + 0.5 * tono, [(80, 76, 70), (140, 134, 124)]) * (0.6 + 0.4 * sassi[..., None]), colore)
salva("Terra", colore, alt, 4.0)

# ------------------------------------------------------------------ Ghiaia: sassi tondi di fiume
cupole, tono = ciottoli(L, 520, L / 60)
piccoli, tono2 = ciottoli(L, 2600, L / 140)
alt = 1.0 * cupole + 0.35 * piccoli
valore = 0.35 + 0.5 * tono + 0.1 * periodico(L, 30, 1.0)
colore = tinta(valore, [(70, 66, 60), (118, 110, 98), (158, 150, 136), (186, 176, 160)])
colore *= (0.45 + 0.55 * np.clip(cupole * 1.4, 0, 1))[..., None]
salva("Ghiaia", colore, alt, 5.0)

# ------------------------------------------------------------------ Neve: morbida, con lievi increspature dal vento
morbida = periodico(L, 5, 1.8)
vento = periodico(L, 50, 1.0)
alt = 0.7 * morbida + 0.15 * vento
valore = 0.75 + 0.15 * morbida + 0.06 * vento
colore = tinta(valore, [(170, 182, 200), (222, 230, 240), (248, 250, 252)])
salva("Neve", colore, alt, 2.0)

# ------------------------------------------------------------------ Variazione: macchie grandi (scala di grigi)
var = periodico(L, 2, 2.0) * 0.6 + periodico(L, 7, 1.5) * 0.4
var = (var - var.min()) / (var.max() - var.min())
Image.fromarray((var * 255).astype(np.uint8), "L").save(os.path.join(USCITA, "T_Variazione.png"), optimize=True)
print("Fatto: Variazione")
