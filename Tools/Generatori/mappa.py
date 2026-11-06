# Valdorso - La mappa di carta per il gioco (v0.1.3, tasto M). Scritto da Claude il 06/11/2026, mentre Fra dormiva.
#
# Parte dalla mappa disegnata da valle.py (Mappa_3000.png: pergamena, rilievo, fiume, bosco, nomi) e la rifinisce per
# il gioco: più grande (2048 x 2048), con la pianta vera del villaggio della fase 1 (le case a inchiostro, la palizzata,
# le strade di terra), il posto di guardia del passo, il bordo della pergamena consumato e una rosa dei venti.
# Niente puntino "sei qui" (deciso nel Design: la mappa è di carta, ci si orienta da soli).
#
# Uso: python mappa.py <cartella valle (Mappa_3000.png, Luoghi_3000.json)> <Pianta_3000.json> <cartella font> <uscita.png>
# I font sono quelli del gioco (Cinzel, EB Garamond: licenza OFL).

import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from scipy import ndimage

LATO = 2048
INCHIOSTRO = (58, 40, 26)
INCHIOSTRO_ROSSO = (120, 36, 24)


def font(cartella, nome, grandezza):
    try:
        return ImageFont.truetype(os.path.join(cartella, nome), grandezza)
    except OSError:
        return ImageFont.load_default()


def scritta(d, xy, testo, f, colore=INCHIOSTRO, alone=(226, 208, 168), centro=True):
    """Una scritta a mano con un alone di carta sotto (si legge anche sopra il rilievo)."""
    w, h = d.textbbox((0, 0), testo, font=f)[2:]
    x, y = (xy[0] - w / 2, xy[1] - h / 2) if centro else xy
    for dx in (-2, -1, 0, 1, 2):
        for dy in (-2, -1, 0, 1, 2):
            if dx or dy:
                d.text((x + dx, y + dy), testo, font=f, fill=alone)
    d.text((x, y), testo, font=f, fill=colore)


def rosa_dei_venti(d, cx, cy, r, f):
    for k in range(8):
        a = math.radians(k * 45 - 90)
        lungo = r if k % 2 == 0 else r * 0.55
        p = (cx + math.cos(a) * lungo, cy + math.sin(a) * lungo)
        lati = [(cx + math.cos(a + math.pi / 2) * r * 0.12, cy + math.sin(a + math.pi / 2) * r * 0.12),
                (cx + math.cos(a - math.pi / 2) * r * 0.12, cy + math.sin(a - math.pi / 2) * r * 0.12)]
        d.polygon([p, lati[0], (cx, cy)], fill=INCHIOSTRO)
        d.polygon([p, lati[1], (cx, cy)], outline=INCHIOSTRO, fill=(226, 208, 168))
    d.ellipse((cx - r * 0.16, cy - r * 0.16, cx + r * 0.16, cy + r * 0.16), outline=INCHIOSTRO, width=3)
    d.ellipse((cx - r * 1.05, cy - r * 1.05, cx + r * 1.05, cy + r * 1.05), outline=INCHIOSTRO, width=2)
    scritta(d, (cx, cy - r * 1.3), "N", f)


def mappa(cartella_valle, file_pianta, cartella_font, uscita):
    base = Image.open(os.path.join(cartella_valle, "Mappa_3000.png")).convert("RGB").resize((LATO, LATO), Image.LANCZOS)
    with open(os.path.join(cartella_valle, "Luoghi_3000.json"), encoding="utf-8") as f:
        luoghi = json.load(f)
    lato_cm = float(luoghi["lato_m"]) * 100.0
    k = LATO / lato_cm

    def px(p):
        return (p[0] * k, p[1] * k)

    d = ImageDraw.Draw(base)
    pianta = []
    if file_pianta and os.path.exists(file_pianta):
        with open(file_pianta, encoding="utf-8") as f:
            pianta = json.load(f)

    def nella_fase_1(c):
        a, b = c.get("fase", [1, 0])
        return a <= 1 and (b == 0 or b >= 1)

    # la pianta del villaggio, della palude e del posto di guardia (solo la fase 1: com'è oggi)
    for c in pianta:
        if not nella_fase_1(c):
            continue
        punti = [px(p) for p in c["punti"]]
        if c["tipo"] == "strada":
            d.line(punti, fill=(150, 112, 70), width=max(2, int(c["larga"] * k * 0.9)))
    for c in pianta:
        if not nella_fase_1(c):
            continue
        punti = [px(p) for p in c["punti"]]
        if c["tipo"] in ("edificio", "fab"):
            d.polygon(punti, fill=(92, 58, 38), outline=INCHIOSTRO)
        elif c["tipo"] == "palizzata":
            d.line(punti, fill=INCHIOSTRO, width=3)
        elif c["tipo"] == "muro":
            d.line(punti, fill=(70, 66, 62), width=5)
        elif c["tipo"] == "linea" and c["nome"] in ("Orto", "Cimitero", "Recinto", "Cortile"):
            d.line(punti, fill=(110, 80, 50), width=1)
        elif c["tipo"] == "linea" and c["nome"] == "Sigillo":
            # il sigillo della Corona sul passo: una riga d'oro e rosso
            d.line(punti, fill=(200, 150, 50), width=5)
            d.line(punti, fill=INCHIOSTRO_ROSSO, width=2)

    # i nomi in più che la mappa di valle.py non ha
    f_piccolo = font(cartella_font, "EBGaramond-Italic.ttf", 26)
    f_nord = font(cartella_font, "Cinzel-Bold.ttf", 34)
    g = luoghi["luoghi"].get("Posto di guardia del passo")
    if g:
        x, y = px(g["unreal_cm"])
        scritta(d, (x - 40, y - 46), "Guardia della Corona", f_piccolo, colore=INCHIOSTRO_ROSSO)

    # (la freccia del nord c'è già, in alto a destra, disegnata da valle.py: rosa_dei_venti resta per altre mappe)

    # il bordo della pergamena: consumato, più scuro e un poco bruciato agli angoli
    rng = np.random.default_rng(31)
    yy, xx = np.mgrid[0:LATO, 0:LATO]
    bordo = np.minimum.reduce([xx, yy, LATO - 1 - xx, LATO - 1 - yy]).astype(np.float32)
    rumore = ndimage.gaussian_filter(rng.standard_normal((LATO, LATO)), 18) * 160 + ndimage.gaussian_filter(rng.standard_normal((LATO, LATO)), 4) * 25
    margine = bordo + rumore
    scuro = np.clip(1.0 - (margine - 10) / 70.0, 0, 1)            # dove si scurisce
    fuori = margine < 8                                           # dove la carta manca
    c = np.asarray(base).astype(np.float32)
    c = c * (1 - 0.55 * scuro[..., None]) + np.array([70, 42, 20.0]) * 0.55 * scuro[..., None]
    alfa = np.where(fuori, 0, 255).astype(np.uint8)
    alfa = np.asarray(Image.fromarray(alfa).filter(ImageFilter.GaussianBlur(1.2)))
    out = Image.fromarray(np.dstack([np.clip(c, 0, 255).astype(np.uint8), alfa]), "RGBA")
    out.save(uscita)
    print("mappa: %s (%d x %d)" % (uscita, LATO, LATO))


if __name__ == "__main__":
    mappa(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
