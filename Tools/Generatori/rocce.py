# Valdorso - Dove mettere le rocce vere nella gola della cascata (v0.1.3). Scritto da Claude il 07/10/2026.
# Legge le altezze e i luoghi della valle (fatti da valle.py) e scrive Rocce_3000.json: per ogni roccia il gruppo,
# la posizione (cm dall'angolo nord-ovest del paesaggio, Unreal Y = sud, Z = quota vera), la rotazione, la misura
# in metri e quanto va affondata nel terreno. Lo legge Content/Python/piazza_rocce.py nell'editor.
#   pareti: grandi rocce sui fianchi ripidi della gola, sopra e sotto la cascata, e sull'anfiteatro della pozza
#           (coprono le "pieghe" del terreno, che da vicino sembrano gesso);
#   massi:  massi intorno alla pozza e sulle rive del torrente, dove è meno ripido;
#   sassi:  sassi nel letto del torrente e sulle sponde, mezzi affondati nell'acqua.
# Lascia libero il velo della cascata e l'acqua. Sempre uguale (seme fisso).
# Uso: python rocce.py <Altezze_3000.png> <Rocce_3000.json>   (Luoghi_3000.json nella stessa cartella; numpy, Pillow)

import json
import math
import os
import sys

import numpy as np
from PIL import Image

QUOTA_MAX = 800.0
SEME = 710


def main(file_altezze, file_uscita):
    cartella = os.path.dirname(os.path.abspath(file_altezze))
    with open(os.path.join(cartella, "Luoghi_3000.json"), encoding="utf-8") as f:
        luoghi = json.load(f)
    LATO = float(luoghi["lato_m"])
    h = np.array(Image.open(file_altezze)).astype(np.float32) / 65535.0 * QUOTA_MAX
    n = h.shape[0]
    passo = LATO / (n - 1)
    rng = np.random.default_rng(SEME)
    c = luoghi["cascata"]
    orlo = np.array(c["orlo_m"])
    avanti = np.array(c["direzione"])
    pozza = np.array(c["pozza_m"])
    R = c["raggio_pozza_m"]

    def quota(x, y):
        fr, fc = (LATO - y) / passo, x / passo
        r0, c0 = int(fr), int(fc)
        r0, c0 = min(max(r0, 0), n - 2), min(max(c0, 0), n - 2)
        a, b = fr - r0, fc - c0
        return float(h[r0, c0] * (1 - a) * (1 - b) + h[r0, c0 + 1] * (1 - a) * b
                     + h[r0 + 1, c0] * a * (1 - b) + h[r0 + 1, c0 + 1] * a * b)

    def pendio(x, y, d=1.5):
        """Pendenza in gradi e direzione in discesa (x est, y nord)."""
        gx = (quota(x + d, y) - quota(x - d, y)) / (2 * d)
        gy = (quota(x, y + d) - quota(x, y - d)) / (2 * d)
        return math.degrees(math.atan(math.hypot(gx, gy))), (-gx, -gy)

    def normale(x, y, d=3.0):
        """(07/10, v4) La normale del pendio in Unreal (X est, Y sud, Z su), media su 6 m."""
        gx = (quota(x + d, y) - quota(x - d, y)) / (2 * d)
        gy = (quota(x, y + d) - quota(x, y - d)) / (2 * d)
        l = math.sqrt(gx * gx + gy * gy + 1)
        return [round(-gx / l, 4), round(gy / l, 4), round(1 / l, 4)]

    # Il corso del fiume (che serpeggia) con la distanza lungo il corso.
    linea = np.array(luoghi["fiume_m"])
    lunghi = np.concatenate([[0], np.cumsum(np.hypot(*np.diff(linea, axis=0).T))])
    L = lunghi[-1]
    s_orlo = c.get("t_orlo_linea", c["t_orlo"]) * L
    s_fine = s_orlo + 230.0                         # fino a dove il fiume esce dalla gola, nelle rapide sotto la pozza

    def dal_fiume(x, y):
        """Distanza dal fiume e posizione lungo il corso (m), solo nel tratto della gola."""
        p = np.array([x, y])
        a, b = linea[:-1], linea[1:]
        ab = b - a
        t = np.clip(((p - a) * ab).sum(1) / (ab * ab).sum(1), 0, 1)
        q = a + ab * t[:, None]
        d = np.hypot(*(q - p).T)
        i = int(np.argmin(d))
        return float(d[i]), float(lunghi[i] + t[i] * math.dist(a[i], b[i]))

    def larghezza_acqua(s):
        t = s / L
        return 0.65 * (6 + 10 * t)                  # metà della larghezza dell'acqua (acqua.py: 1,3 x larghezza)

    def sul_velo(x, y, raggio=0.0):
        """Davanti alla cascata e sopra l'orlo, dove cade il velo: niente rocce (contando anche quanto è grande la roccia)."""
        v = np.array([x, y]) - orlo
        lungo = v @ avanti
        lato = abs(-v[0] * avanti[1] + v[1] * avanti[0])
        return -6 - raggio < lungo < 26 + raggio and lato < 9 + raggio

    def piede(x, y, raggio):
        """La quota più bassa sotto la roccia (centro e 8 punti intorno): così sul pendio non resta sospesa a valle."""
        return min([quota(x, y)] + [quota(x + raggio * math.cos(a), y + raggio * math.sin(a))
                                    for a in np.linspace(0, 2 * math.pi, 8, endpoint=False)])

    def cm(x, y, z):
        return [round(x * 100, 1), round((LATO - y) * 100, 1), round(z * 100, 1)]

    def yaw_di(dx, dy):
        return math.degrees(math.atan2(-dy, dx))    # Unreal Y = sud

    rocce = []
    messi = {"pareti": [], "massi": [], "sassi": []}

    def lontano(gruppo, x, y, minimo):
        for g, d_min in minimo.items():
            for (px, py) in messi[g]:
                if (px - x) ** 2 + (py - y) ** 2 < d_min * d_min:
                    return False
        return True

    # Candidati: una griglia mossa intorno alla gola (finestra intorno a fiume e pozza).
    xs = linea[:, 0][lunghi <= s_fine + 50]
    ys = linea[:, 1][lunghi <= s_fine + 50]
    x0, x1 = xs.min() - 90, xs.max() + 90
    y0, y1 = ys.min() - 90, ys.max() + 90

    def candidati(spazio):
        gx = np.arange(x0, x1, spazio)
        gy = np.arange(y0, y1, spazio)
        punti = [(x + rng.uniform(-0.45, 0.45) * spazio, y + rng.uniform(-0.45, 0.45) * spazio) for x in gx for y in gy]
        rng.shuffle(punti)
        return punti

    # ---------------------------------------------------------------- pareti
    # (07/10, v6) solo le scogliere "piene" (le due Megascans beige): Nordic Forest Cliff e Coastal Cliff 04 sono gusci
    # vuoti dietro, e non c'è un verso in cui stiano bene da sole su un pendio (provato con Fra: si vede sempre il vuoto).
    grandi = ["mega_huge", "mega_massive"]
    medie = ["rock_face_01", "rock_face_02"]
    for x, y in candidati(6.0):
        d, s = dal_fiume(x, y)
        rz = math.dist((x, y), pozza) / R
        # (07/10, v2) solo la fascia ripida vicino all'acqua: le pareti vere della gola, non i pendii lontani
        nella_gola = s <= s_fine and larghezza_acqua(s) + 2 < d < 38
        anfiteatro = 1.05 < rz < 2.4 and (np.array([x, y]) - pozza) @ avanti < 6
        if not (nella_gola or anfiteatro) or sul_velo(x, y) or rz < 1.05:
            continue
        p, giu = pendio(x, y)
        if p < 35:
            continue
        # (07/10, v2) pareti grandi (prima sembravano sassi sparsi su un pendio liscio): 16-32 m, e si sovrappongono
        misura = rng.uniform(20, 32) if p > 50 else rng.uniform(14, 22)
        if not lontano("pareti", x, y, {"pareti": misura * 0.5}):
            continue
        l = math.hypot(*giu) or 1.0
        ux, uy = giu[0] / l, giu[1] / l
        # un poco dentro la parete, così la roccia esce dal pendio invece di posarsi sopra
        # (07/10, v3) le scogliere scansionate sono gusci aperti dietro: vanno ben dentro il monte, con il dietro nascosto
        px, py = x - ux * misura * 0.35, y - uy * misura * 0.35
        if sul_velo(px, py, misura / 2) or math.dist((px, py), pozza) < R + misura / 2:
            continue
        # (07/10, v2) solo le scogliere vere: le rock_face di Poly Haven sono facciate sottili e sdraiate sembravano crepe
        chiave = grandi[rng.integers(len(grandi))]
        rocce.append({"gruppo": "pareti", "chiave": chiave, "variante": int(rng.integers(100)),
                      "pos_cm": cm(px, py, piede(px, py, misura * 0.4)),
                      "rot": [round(rng.uniform(-6, 6), 1), round(rng.uniform(-8, 8), 1),
                              round(yaw_di(ux, uy) + rng.uniform(-15, 15), 1)],   # imbardata = verso valle
                      "misura_m": round(misura, 2), "affonda": 0.3, "faccia_a_valle": False,
                      "normale": normale(px, py), "giro": round(rng.uniform(-25, 25), 1)})
        messi["pareti"].append((x, y))

    # ---------------------------------------------------------------- massi
    for x, y in candidati(2.5):
        d, s = dal_fiume(x, y)
        rz = math.dist((x, y), pozza) / R
        riva_pozza = 1.08 < rz < 1.9
        riva_fiume = s <= s_fine and larghezza_acqua(s) + 0.5 < d < larghezza_acqua(s) + 9
        misura = rng.uniform(1.5, 4.2)
        if not (riva_pozza or riva_fiume) or sul_velo(x, y, misura / 2) or rz < 1.08 + misura / 2 / R:
            continue
        p, giu = pendio(x, y)
        if p > 42:
            continue
        if not lontano("massi", x, y, {"massi": misura * 1.1, "pareti": 4.0}):
            continue
        rocce.append({"gruppo": "massi", "chiave": ["boulder_01", "rock_moss_set_01"][int(rng.integers(2))],
                      "variante": int(rng.integers(100)), "pos_cm": cm(x, y, piede(x, y, misura * 0.4)),
                      "rot": [round(rng.uniform(-20, 20), 1), round(rng.uniform(-20, 20), 1), round(rng.uniform(0, 360), 1)],
                      "misura_m": round(misura, 2), "affonda": 0.35})
        messi["massi"].append((x, y))
        if len(messi["massi"]) >= 70:
            break

    # ---------------------------------------------------------------- sassi
    for x, y in candidati(1.6):
        d, s = dal_fiume(x, y)
        if s > s_fine or d > larghezza_acqua(s) + 1.5 or sul_velo(x, y) or math.dist((x, y), pozza) < R * 1.02:
            continue
        misura = rng.uniform(0.5, 1.5)
        if not lontano("sassi", x, y, {"sassi": misura * 1.6, "massi": 2.5}):
            continue
        rocce.append({"gruppo": "sassi", "chiave": ["rock_moss_set_02", "rock_moss_set_01"][int(rng.integers(2))],
                      "variante": int(rng.integers(100)), "pos_cm": cm(x, y, quota(x, y)),
                      "rot": [round(rng.uniform(-25, 25), 1), round(rng.uniform(-25, 25), 1), round(rng.uniform(0, 360), 1)],
                      "misura_m": round(misura, 2), "affonda": 0.45})
        messi["sassi"].append((x, y))
        if len(messi["sassi"]) >= 160:
            break

    with open(file_uscita, "w", encoding="utf-8") as f:
        json.dump({"nota": "cm dall'angolo nord-ovest del paesaggio; Z = quota vera; rot = rollio, beccheggio, imbardata",
                   "rocce": rocce}, f, ensure_ascii=False, indent=0)
    print("Rocce: {} pareti, {} massi, {} sassi".format(len(messi["pareti"]), len(messi["massi"]), len(messi["sassi"])))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
