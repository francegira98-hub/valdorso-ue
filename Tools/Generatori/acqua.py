# Valdorso - Il lago, il fiume e l'emissario della valle, per il plugin Water di Unreal (v0.1.3). Scritto da Claude il 06/10/2026.
# Legge le altezze della valle (Altezze_3000.png) e i corsi d'acqua (Luoghi_3000.json), fatti entrambi da valle.py,
# e scrive Acqua_3000.json con:
#   lago:      il contorno della riva a quota del pelo dell'acqua (30 m), come punti di una spline chiusa;
#   fiume:     i punti della spline lungo il fondo dell'alveo, con larghezza, profondità e quota dell'acqua;
#   emissario: lo stesso per il torrente che esce dal lago verso ovest;
#   (06/10) torrente: il fiume nei monti sopra la cascata, fino all'orlo;
#   (06/10) pozza: la pozza rotonda dove cade la cascata (un piccolo lago); il fiume ora parte da lì.
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

    def riva_di_pozza(centro, raggio, pelo):
        r0, c0 = riga_col(*centro)
        mezza = int(2.0 * raggio / passo) + 2
        ra, rb = max(int(r0) - mezza, 0), min(int(r0) + mezza + 1, n)
        ca, cb = max(int(c0) - mezza, 0), min(int(c0) + mezza + 1, n)
        finestra = h[ra:rb, ca:cb]
        rr, cc = np.mgrid[ra:rb, ca:cb]
        vicino = np.hypot((rr - r0) * passo, (cc - c0) * passo) < 1.6 * raggio
        sotto_pelo = ndimage.binary_opening((finestra < pelo - 0.05) & vicino, iterations=2)
        etichette_p, _ = ndimage.label(sotto_pelo)
        if etichette_p[int(r0) - ra, int(c0) - ca] == 0:
            # di riserva: il cerchio
            return [cm(centro[0] + raggio * 0.97 * math.cos(a), centro[1] + raggio * 0.97 * math.sin(a), pelo)
                    for a in np.linspace(0, 2 * math.pi, 16, endpoint=False)]
        dentro = etichette_p == etichette_p[int(r0) - ra, int(c0) - ca]
        # un punto più largo: semplificando, le corde tagliano un poco verso l'interno e resterebbe una striscia asciutta
        dentro = ndimage.binary_dilation(dentro, iterations=1)
        contorno = max(measure.find_contours(np.pad(dentro, 1).astype(np.float32), 0.5), key=len)
        contorno = measure.approximate_polygon(contorno, tolerance=1.0 / passo)   # un punto ogni metro di scarto
        if np.allclose(contorno[0], contorno[-1]):
            contorno = contorno[:-1]
        punti = []
        for r, c in contorno:
            r, c = r - 1 + ra, c - 1 + ca
            punti.append(cm(c * passo, LATO - r * passo, pelo))
        return punti

    # ---------------------------------------------------------------- fiume ed emissario, lungo l'alveo
    def corso(linea, inizio, larghezza_di, profondita_di, quota_iniziale, quota_finale, scivolo=None, fine=1.0, ogni=40.0,
              punto_finale=None, acqua_primo=None):
        """Punti della spline ogni ~40 m dal tratto 'inizio' al tratto 'fine' (0..1); l'acqua sempre in discesa."""
        punti = []
        fatto = 0.0
        lunghezza = sum(math.dist(linea[i], linea[i + 1]) for i in range(len(linea) - 1))
        ultimo = None
        for i, (x, y) in enumerate(linea):
            if i > 0:
                fatto += math.dist(linea[i - 1], linea[i])
            t = fatto / lunghezza
            if t < inizio or t > fine or not (0 <= x <= LATO and 0 <= y <= LATO):
                continue
            ultimo_del_tratto = i == len(linea) - 1 or fatto + math.dist(linea[i], linea[i + 1]) > fine * lunghezza
            if ultimo is not None and math.dist(ultimo, (x, y)) < ogni and not ultimo_del_tratto:
                continue
            ultimo = (x, y)
            fondo = min(quota(min(max(x + dx, 0), LATO), min(max(y + dy, 0), LATO)) for dx in (-1, 0, 1) for dy in (-1, 0, 1))
            r, c = riga_col(x, y)
            nel_lago = bool(lago[min(int(round(r)), n - 1), min(int(round(c)), n - 1)])
            punti.append({"x": x, "y": y, "t": t, "larghezza": larghezza_di(t), "fondo": fondo, "nel_lago": nel_lago,
                          # dentro il lago l'acqua è quella del lago (il fondo lì è a 26 m di profondità)
                          "acqua": PELO_LAGO - 0.05 if nel_lago else fondo + RIEMPIMENTO * profondita_di(t)})
        if punto_finale is not None:
            # (07/10) l'ultimo punto proprio lì (per il torrente: l'orlo della cascata), non al punto della linea prima
            x, y = punto_finale
            if punti and math.dist((punti[-1]["x"], punti[-1]["y"]), (x, y)) < 3.0:
                punti.pop()
            # il fondo si legge un metro a monte: un passo più in là c'è già il vuoto della cascata
            bx, by = (punti[-1]["x"] - x, punti[-1]["y"] - y) if punti else (0.0, 0.0)
            lb = math.hypot(bx, by) or 1.0
            fondo = quota(x + bx / lb, y + by / lb)
            punti.append({"x": x, "y": y, "t": fine, "larghezza": larghezza_di(fine), "fondo": fondo, "nel_lago": False,
                          "acqua": fondo + RIEMPIMENTO * profondita_di(fine)})
        # Dentro il lago basta un punto: l'ultimo prima della riva (per l'emissario) o il primo dopo (per il fiume).
        while len(punti) > 1 and punti[0]["nel_lago"] and punti[1]["nel_lago"]:
            punti.pop(0)
        while len(punti) > 1 and punti[-1]["nel_lago"] and punti[-2]["nel_lago"]:
            punti.pop()
        if acqua_primo is not None and punti:
            # (07/10) il primo punto prende l'acqua da dove nasce (la pozza), non dal suo fondo
            punti[0]["acqua"] = acqua_primo
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
        return punti, lunghezza * (fine - inizio)

    def per_unreal(punti):
        # Larghezza piena dell'acqua: l'alveo di valle.py ha raggio 'larghezza'; pieno al 70% è largo ~1,3 volte.
        # (07/10) velocita = quante volte più veloce della corrente normale: dove è ripido l'acqua corre e fa schiuma
        # (1 in piano, fino a 5 sui tratti più ripidi del torrente).
        fuori = []
        for i, p in enumerate(punti):
            a, b = punti[max(i - 1, 0)], punti[min(i + 1, len(punti) - 1)]
            strada = math.dist((a["x"], a["y"]), (b["x"], b["y"])) or 1.0
            pendenza = max(a["acqua"] - b["acqua"], 0) / strada
            fuori.append({"posizione": cm(p["x"], p["y"], p["acqua"]),
                          "larghezza_cm": round(p["larghezza"] * 1.3 * 100, 1),
                          "profondita_cm": round(max(p["acqua"] - p["fondo"], 0.5) * 100 + 100, 1),
                          "velocita": round(min(max(1 + pendenza * 12, 1), 5), 2)})
        return fuori

    linea_fiume = [tuple(p) for p in luoghi["fiume_m"]]
    lunghezza_fiume = sum(math.dist(linea_fiume[i], linea_fiume[i + 1]) for i in range(len(linea_fiume) - 1))
    largo_fiume, fondo_fiume = (lambda t: 6 + 10 * t), (lambda t: 3.0 + 2.5 * t)
    cascata = luoghi.get("cascata")
    if cascata:
        # (06/10) Sopra la cascata: il torrente nei monti fino all'orlo (punti fitti: è ripido).
        # (07/10) Da dove il fiume entra nella valle dal bordo del mondo, non più solo gli ultimi 120 m (sopra era asciutto).
        # (07/10) il fiume ora serpeggia: l'orlo si misura lungo la linea vera (t_orlo_linea), se c'è
        t_orlo = cascata.get("t_orlo_linea", cascata["t_orlo"])
        q_orlo = cascata["quota_orlo_m"]
        pt, _ = corso(linea_fiume, 0.0, largo_fiume, fondo_fiume, None,
                      q_orlo + 0.25, fine=t_orlo, ogni=5.0,   # (07/10) fitti: l'acqua segue gli scalini
                      punto_finale=tuple(cascata["orlo_m"]))
        # La pozza: un cerchio a quota del suo pelo.
        pz = cascata["pozza_m"]
        rp = cascata["raggio_pozza_m"]
        pelo_pozza = cascata["pelo_pozza_m"]
        # (07/10) La riva vera della pozza (non più un cerchio): dove il terreno è sotto il pelo, vicino al centro.
        riva_pozza = riva_di_pozza(pz, rp, pelo_pozza)
        # Il fiume parte dalla riva della pozza, a valle (poco dentro, così le due acque si toccano).
        t_via = t_orlo + (math.hypot(*np.subtract(pz, cascata["orlo_m"])) + 0.85 * rp) / lunghezza_fiume
        pf, lungo_fiume = corso(linea_fiume, t_via, largo_fiume, fondo_fiume, pelo_pozza - 0.03, PELO_LAGO + 0.05,
                                acqua_primo=pelo_pozza - 0.03)
    else:
        # Senza cascata (valle vecchia): il fiume parte dove entra nella conca dai monti.
        pt, riva_pozza, pelo_pozza = None, None, None
        pf, lungo_fiume = corso(linea_fiume, 0.16, largo_fiume, fondo_fiume, None, PELO_LAGO + 0.05)
    pf[-1]["acqua"] = PELO_LAGO + 0.02
    # L'emissario esce dal lago appena sotto il suo pelo e scende fino al bordo del mondo.
    pe, lungo_em = corso([tuple(p) for p in luoghi["emissario_m"]], 0.0,
                         lambda t: 3.5 + 2 * t, lambda t: 2.0, PELO_LAGO - 0.05, None, scivolo=0.03)
    punti_fiume, punti_emissario = per_unreal(pf), per_unreal(pe)
    punti_torrente = per_unreal(pt) if pt else None
    # (07/10) Gli scalini del torrente: dove il letto salta di 3-4 m, una piccola tenda d'acqua (la mette crea_cascata_valle.py).
    scalini = []
    for sc in (cascata or {}).get("scalini", []):
        x, y = sc["x_m"], sc["y_m"]
        ux, uy = sc["direzione"]
        sopra = min(quota(x - ux * dx2, y - uy * dx2) for dx2 in (1.5, 2.5, 3.5))
        sotto = min(quota(x + ux * dx2, y + uy * dx2) for dx2 in (2.0, 3.0, 4.0))
        t = sc["t"]
        acqua_sopra = sopra + RIEMPIMENTO * (3.0 + 2.5 * t)
        acqua_sotto = sotto + RIEMPIMENTO * (3.0 + 2.5 * t)
        salto = acqua_sopra - acqua_sotto
        if salto < 1.0:
            continue
        scalini.append({"orlo_cm": cm(x - ux * 1.0, y - uy * 1.0, acqua_sopra), "direzione": [ux, -uy],   # Unreal Y = sud
                        "salto_cm": round(salto * 100, 1), "larghezza_cm": round((6 + 10 * t) * 100, 1)})

    dati = {
        "lato_cm": LATO * 100,
        "nota": "Centimetri dall'angolo nord-ovest del paesaggio: aggiungere la posizione dell'attore Landscape.",
        "lago": {"pelo_cm": PELO_LAGO * 100, "area_m2": round(area), "profondita_m": round(profondita_lago, 1),
                 "riva": punti_lago},
        "fiume": {"lunghezza_m": round(lungo_fiume), "punti": punti_fiume},
        "emissario": {"lunghezza_m": round(lungo_em), "punti": punti_emissario},
    }
    if punti_torrente:
        dati["torrente"] = {"punti": punti_torrente}
        dati["pozza"] = {"pelo_cm": round(pelo_pozza * 100, 1), "riva": riva_pozza}
        # L'orlo è dato alla quota dell'ACQUA sull'orlo (l'ultimo punto del torrente): da lì parte la tenda di cascata.py,
        # e il salto è da quell'acqua al pelo della pozza, così modello e posto vanno d'accordo.
        acqua_orlo = pt[-1]["acqua"]
        dati["scalini"] = scalini
        dati["cascata"] = {"orlo_cm": cm(*cascata["orlo_m"], acqua_orlo),
                           "roccia_orlo_cm": round(pt[-1]["fondo"] * 100, 1),
                           "pozza_cm": cm(*cascata["pozza_m"], pelo_pozza),
                           "direzione": [cascata["direzione"][0], -cascata["direzione"][1]],   # Unreal Y = sud
                           "salto_cm": round((acqua_orlo - pelo_pozza) * 100, 1),
                           # la tenda è larga come l'acqua sull'orlo (il letto è più largo, ma l'acqua sta nel mezzo)
                           "larghezza_cm": round(cascata["larghezza_orlo_m"] * 100, 1)}
    with open(file_json, "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)
    print(f"Lago: {len(punti_lago)} punti, {area / 10000:.1f} ettari, profondo {profondita_lago:.1f} m")
    print(f"Fiume: {len(punti_fiume)} punti, da {pf[0]['acqua']:.1f} a {pf[-1]['acqua']:.1f} m")
    print(f"Emissario: {len(punti_emissario)} punti, da {pe[0]['acqua']:.1f} a {pe[-1]['acqua']:.1f} m")
    if punti_torrente:
        print(f"Torrente: {len(punti_torrente)} punti, da {pt[0]['acqua']:.1f} a {pt[-1]['acqua']:.1f} m; "
              f"cascata di {cascata['quota_orlo_m'] - pelo_pozza:.1f} m nella pozza (pelo a {pelo_pozza:.1f} m, "
              f"{len(riva_pozza)} punti di riva); {len(scalini)} scalini nel torrente")

    if file_anteprima:
        L = 1024
        k = L / LATO
        g = (h - h.min()) / (h.max() - h.min())
        img = Image.fromarray((g * 255).astype(np.uint8)).resize((L, L)).convert("RGB")
        d = ImageDraw.Draw(img)
        poly = [(p[0] / 100 * k, p[1] / 100 * k) for p in punti_lago]
        d.polygon(poly, outline=(40, 120, 255))
        if riva_pozza:
            d.polygon([(p[0] / 100 * k, p[1] / 100 * k) for p in riva_pozza], outline=(40, 120, 255))
        for punti in (punti_fiume, punti_emissario, punti_torrente or []):
            for a, b in zip(punti, punti[1:]):
                d.line([(a["posizione"][0] / 100 * k, a["posizione"][1] / 100 * k),
                        (b["posizione"][0] / 100 * k, b["posizione"][1] / 100 * k)], fill=(40, 160, 255), width=3)
        img.save(file_anteprima)


if __name__ == "__main__":
    main(*sys.argv[1:4])
