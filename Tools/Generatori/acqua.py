# Valdorso - Il lago, il fiume e l'emissario della valle, per il plugin Water di Unreal (v0.1.3). Scritto da Claude il 06/10/2026.
# Legge le altezze della valle (Altezze_3000.png) e i corsi d'acqua (Luoghi_3000.json), fatti entrambi da valle.py,
# e scrive Acqua_3000.json con:
#   lago:      il contorno della riva a quota del pelo dell'acqua (30 m), come punti di una spline chiusa;
#   fiume:     i punti della spline lungo il fondo dell'alveo, con larghezza, profondità e quota dell'acqua;
#   emissario: lo stesso per il torrente che esce dal lago verso ovest.
# Le coordinate sono in centimetri dall'angolo nord-ovest del paesaggio (Unreal X = est, Y = sud, Z = quota):
# lo script dell'editor (Content/Python/crea_acqua_valle.py) aggiunge la posizione dell'attore Landscape.
# Uso: python acqua.py <Altezze_3000.png> <Acqua_3000.json> [anteprima.png]   (serve numpy, Pillow, scipy, scikit-image)
#      Luoghi_3000.json deve stare nella stessa cartella delle altezze.

import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
from skimage import measure

QUOTA_MAX = 800.0             # metri rappresentati dai 16 bit (come valle.py)
# (06/10) L'acqua riempie il 70% dell'alveo scavato da valle.py: resta sotto le sponde, ma si vede.
# Con 1,6 m fissi il fiume era troppo basso, quasi nascosto nell'alveo.
RIEMPIMENTO = 0.7


def main(file_altezze, file_json, file_anteprima=None):
    cartella = os.path.dirname(os.path.abspath(file_altezze))
    lato_txt = os.path.splitext(os.path.basename(file_altezze))[0].split("_")[-1]
    with open(os.path.join(cartella, f"Luoghi_{lato_txt}.json"), encoding="utf-8") as f:
        luoghi = json.load(f)
    LATO = float(luoghi["lato_m"])
    PELO_LAGO = float(luoghi["pelo_del_lago_m"])
    LAGO = (luoghi["luoghi"]["Lago"]["x_m"], luoghi["luoghi"]["Lago"]["y_m"])

    h = np.array(Image.open(file_altezze)).astype(np.float32) / 65535.0 * QUOTA_MAX
    n = h.shape[0]
    passo = LATO / (n - 1)

    def riga_col(x, y):
        return (LATO - y) / passo, x / passo

    def quota(x, y):
        r, c = riga_col(x, y)
        return float(ndimage.map_coordinates(h, [[r], [c]], order=1)[0])

    def cm(x, y, z):
        """Metri del generatore -> centimetri dall'angolo nord-ovest del paesaggio."""
        return [round(x * 100, 1), round((LATO - y) * 100, 1), round(z * 100, 1)]

    # ---------------------------------------------------------------- il lago: la riva a 30 m
    sotto = h < PELO_LAGO
    # Solo il lago: l'emissario scende sotto i 30 m appena esce, e senza questo taglio la riva lo seguirebbe fino ai monti.
    righe = (LATO - np.arange(n, dtype=np.float32) * passo)[:, None]
    colonne = (np.arange(n, dtype=np.float32) * passo)[None, :]
    sotto &= np.hypot(colonne - LAGO[0], righe - LAGO[1]) < 0.12 * LATO
    # E via i canali stretti (l'imbocco dell'emissario, la foce del fiume): la riva resta quella del lago.
    sotto = ndimage.binary_opening(sotto, iterations=20)
    etichette, _ = ndimage.label(sotto)
    r0, c0 = riga_col(*LAGO)
    lago = etichette == etichette[int(r0), int(c0)]
    lago = ndimage.binary_closing(lago, iterations=3)
    contorni = measure.find_contours(lago.astype(np.float32), 0.5)
    riva = max(contorni, key=len)
    # Semplificata: un punto ogni ~25 m di riva (la spline arrotonda il resto).
    riva = measure.approximate_polygon(riva, tolerance=6.0)
    if np.allclose(riva[0], riva[-1]):
        riva = riva[:-1]
    punti_lago = []
    for r, c in riva:
        x, y = c * passo, LATO - r * passo
        punti_lago.append(cm(x, y, PELO_LAGO))
    area = lago.sum() * passo * passo
    profondita_lago = PELO_LAGO - float(h[lago].min())

    # ---------------------------------------------------------------- fiume ed emissario, lungo l'alveo
    def corso(linea, inizio, larghezza_di, profondita_di, quota_iniziale, quota_finale, scivolo=None):
        """Punti della spline ogni ~40 m dal tratto 'inizio' (0..1) in poi; l'acqua sempre in discesa."""
        punti = []
        fatto = 0.0
        lunghezza = sum(math.dist(linea[i], linea[i + 1]) for i in range(len(linea) - 1))
        ultimo = None
        for i, (x, y) in enumerate(linea):
            if i > 0:
                fatto += math.dist(linea[i - 1], linea[i])
            t = fatto / lunghezza
            if t < inizio or not (0 <= x <= LATO and 0 <= y <= LATO):
                continue
            if ultimo is not None and math.dist(ultimo, (x, y)) < 40.0 and i < len(linea) - 1:
                continue
            ultimo = (x, y)
            fondo = min(quota(min(max(x + dx, 0), LATO), min(max(y + dy, 0), LATO)) for dx in (-1, 0, 1) for dy in (-1, 0, 1))
            r, c = riga_col(x, y)
            nel_lago = bool(lago[min(int(round(r)), n - 1), min(int(round(c)), n - 1)])
            punti.append({"x": x, "y": y, "t": t, "larghezza": larghezza_di(t), "fondo": fondo, "nel_lago": nel_lago,
                          # dentro il lago l'acqua è quella del lago (il fondo lì è a 26 m di profondità)
                          "acqua": PELO_LAGO - 0.05 if nel_lago else fondo + RIEMPIMENTO * profondita_di(t)})
        # Dentro il lago basta un punto: l'ultimo prima della riva (per l'emissario) o il primo dopo (per il fiume).
        while len(punti) > 1 and punti[0]["nel_lago"] and punti[1]["nel_lago"]:
            punti.pop(0)
        while len(punti) > 1 and punti[-1]["nel_lago"] and punti[-2]["nel_lago"]:
            punti.pop()
        if scivolo is not None:
            # Uscendo dal lago l'acqua scende piano (scivolo = metri di discesa per metro), senza un salto alla riva:
            # l'alveo appena fuori è più basso perché il lago ripido lo scava.
            strada = 0.0
            for i, p in enumerate(punti):
                if i > 0:
                    strada += math.dist((punti[i - 1]["x"], punti[i - 1]["y"]), (p["x"], p["y"]))
                p["acqua"] = max(p["acqua"], PELO_LAGO - 0.05 - scivolo * strada)
        quota_prima = quota_iniziale if quota_iniziale is not None else 1e9
        for p in punti:
            z = min(p["acqua"], quota_prima - 0.05)
            if quota_finale is not None:
                z = max(z, quota_finale)
            p["acqua"] = z
            quota_prima = z
        return punti, lunghezza * (1 - inizio)

    def per_unreal(punti):
        # Larghezza piena dell'acqua: l'alveo di valle.py ha raggio 'larghezza'; pieno al 70% è largo ~1,3 volte.
        return [{"posizione": cm(p["x"], p["y"], p["acqua"]),
                 "larghezza_cm": round(p["larghezza"] * 1.3 * 100, 1),
                 "profondita_cm": round(max(p["acqua"] - p["fondo"], 0.5) * 100 + 100, 1)}
                for p in punti]

    # Il fiume parte dove entra nella conca dai monti (prima è una cascata nella roccia: arriverà dopo) e finisce nel lago.
    pf, lungo_fiume = corso([tuple(p) for p in luoghi["fiume_m"]], 0.16,
                            lambda t: 6 + 10 * t, lambda t: 3.0 + 2.5 * t, None, PELO_LAGO + 0.05)
    pf[-1]["acqua"] = PELO_LAGO + 0.02
    # L'emissario esce dal lago appena sotto il suo pelo e scende fino al bordo del mondo.
    pe, lungo_em = corso([tuple(p) for p in luoghi["emissario_m"]], 0.0,
                         lambda t: 3.5 + 2 * t, lambda t: 2.0, PELO_LAGO - 0.05, None, scivolo=0.03)
    punti_fiume, punti_emissario = per_unreal(pf), per_unreal(pe)

    dati = {
        "lato_cm": LATO * 100,
        "nota": "Centimetri dall'angolo nord-ovest del paesaggio: aggiungere la posizione dell'attore Landscape.",
        "lago": {"pelo_cm": PELO_LAGO * 100, "area_m2": round(area), "profondita_m": round(profondita_lago, 1),
                 "riva": punti_lago},
        "fiume": {"lunghezza_m": round(lungo_fiume), "punti": punti_fiume},
        "emissario": {"lunghezza_m": round(lungo_em), "punti": punti_emissario},
    }
    with open(file_json, "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)
    print(f"Lago: {len(punti_lago)} punti, {area / 10000:.1f} ettari, profondo {profondita_lago:.1f} m")
    print(f"Fiume: {len(punti_fiume)} punti, da {pf[0]['acqua']:.1f} a {pf[-1]['acqua']:.1f} m")
    print(f"Emissario: {len(punti_emissario)} punti, da {pe[0]['acqua']:.1f} a {pe[-1]['acqua']:.1f} m")

    if file_anteprima:
        L = 1024
        k = L / LATO
        g = (h - h.min()) / (h.max() - h.min())
        img = Image.fromarray((g * 255).astype(np.uint8)).resize((L, L)).convert("RGB")
        d = ImageDraw.Draw(img)
        poly = [(p[0] / 100 * k, p[1] / 100 * k) for p in punti_lago]
        d.polygon(poly, outline=(40, 120, 255))
        for punti in (punti_fiume, punti_emissario):
            for a, b in zip(punti, punti[1:]):
                d.line([(a["posizione"][0] / 100 * k, a["posizione"][1] / 100 * k),
                        (b["posizione"][0] / 100 * k, b["posizione"][1] / 100 * k)], fill=(40, 160, 255), width=3)
        img.save(file_anteprima)


if __name__ == "__main__":
    main(*sys.argv[1:4])
