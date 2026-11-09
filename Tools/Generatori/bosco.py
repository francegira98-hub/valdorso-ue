# Valdorso - Il bosco della valle (v0.1.4, 09/11/2026, chiesto da Fra: "il bosco e gli alberi ovunque").
# Scritto da Claude. Le regole sono quelle del Design ("La natura: bosco, prati, rocce ed erba sparsi con regole:
# piu' fitto nel bosco, radure vicino al villaggio, niente alberi sulle strade") e della pianta della valle:
#   il bosco grande a ovest e nord-ovest (abeti rossi; faggi in basso, larici in alto, betulle ai margini; le rovine
#   degli Antichi dentro, in una radura), il boschetto a est (faggi, betulle, qualche abete, noccioli sotto);
#   i margini: abeti giovani e noccioli; nei prati alberi sparsi a gruppi (pochi), sui pendii dei monti larici e
#   abeti radi fino al limite del bosco; vicino all'acqua le betulle; nel bosco i tronchi caduti, al margine verso il
#   villaggio i ceppi (la legna tagliata); negli orti e fra le case i meli.
#   Mai: sulle strade (dipinte e del villaggio), nell'acqua, sulle case e sui loro cortili, sulle palizzate e i muri
#   (anche quelli delle fasi 2 e 3 del villaggio), sulle rocce ripide, nella gola della cascata, nei pascoli, nella
#   piazza e nelle radure dei luoghi (rovine, grotte, belvedere, guardia del passo).
#
# Scrive nella cartella di uscita (di solito V:/Texture/valdorso/bosco, poi bosco.zip):
#   T_*.png             cortecce e fronde (bosco_texture.py)
#   SM_Bosco_*.glb      gli alberi (bosco_alberi.py)
#   Bosco_3000.json     i modelli (altezza, raggio del tronco, collisione) e gli alberi: per ognuno il modello, la
#                       posizione (cm dall'angolo nord-ovest del paesaggio, Z = quota vera in cm), i gradi, la scala
#                       e la pendenza; lo legge Content/Python/nuovi/crea_bosco.py nell'editor
#   Anteprima_Bosco.png la valle vista dall'alto con gli alberi, per controllare
# Uso: python bosco.py <cartella valle (Altezze_3000.png, Luoghi_3000.json, Strati/)> <cartella villaggio
#      (Villaggio_3000.json, Pianta_3000.json)> <uscita>
# Sempre uguale (seme fisso).

import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw

import bosco_alberi
import bosco_texture

QUOTA_MAX = 800.0
SEME = 1109

# Quanti alberi: 1 = il bosco pieno. (Nell'editor crea_bosco.py ha anche DENSITA, che sfoltisce senza rifare niente.)
FITTO = 1.0

# Le specie e i loro modelli
MODELLI = {
    "abete": ["SM_Bosco_Abete_1", "SM_Bosco_Abete_2", "SM_Bosco_Abete_3", "SM_Bosco_Abete_4"],
    "giovane": ["SM_Bosco_AbeteGiovane_1", "SM_Bosco_AbeteGiovane_2"],
    "larice": ["SM_Bosco_Larice_1", "SM_Bosco_Larice_2"],
    "faggio": ["SM_Bosco_Faggio_1", "SM_Bosco_Faggio_2", "SM_Bosco_Faggio_3"],
    "betulla": ["SM_Bosco_Betulla_1", "SM_Bosco_Betulla_2"],
    "melo": ["SM_Bosco_Melo_1", "SM_Bosco_Melo_2"],
    "nocciolo": ["SM_Bosco_Nocciolo_1", "SM_Bosco_Nocciolo_2", "SM_Bosco_Nocciolo_3"],
    "ceppo": ["SM_Bosco_Ceppo_1", "SM_Bosco_Ceppo_2"],
    "tronco": ["SM_Bosco_Tronco_1", "SM_Bosco_Tronco_2"],
}
# Quanto spazio vuole intorno (m): due alberi della stessa "taglia" non stanno piu' vicini di cosi'
SPAZIO = {"abete": 5.5, "larice": 6.0, "faggio": 7.0, "betulla": 4.5, "giovane": 2.5, "melo": 5.0, "nocciolo": 2.2,
          "ceppo": 1.5, "tronco": 3.0}
# Le taglie: gli alberi grandi si tengono lontani fra loro; i piccoli possono stare sotto ai grandi (non sul tronco)
GRANDI = ("abete", "larice", "faggio", "betulla", "melo")


class Valle:
    def __init__(self, cartella):
        with open(os.path.join(cartella, "Luoghi_3000.json"), encoding="utf-8") as f:
            self.luoghi = json.load(f)
        self.h = np.array(Image.open(os.path.join(cartella, "Altezze_3000.png"))).astype(np.float64) / 65535.0 * QUOTA_MAX
        self.n = self.h.shape[0]
        self.lato = float(self.luoghi["lato_m"])
        self.S = self.lato * 100.0 / (self.n - 1)    # cm per pixel
        self.strati = {}
        for nome in ("Bosco", "Strade", "Villaggio", "Acqua", "Roccia", "Palude", "Neve"):
            p = os.path.join(cartella, "Strati", nome + ".png")
            self.strati[nome] = np.array(Image.open(p)).astype(np.float32) / 255.0 if os.path.exists(p) else np.zeros_like(self.h, np.float32)
        gy, gx = np.gradient(self.h, self.S / 100.0)
        self.pendenza = np.degrees(np.arctan(np.hypot(gx, gy)))

    def px(self, x, y):
        return (np.clip(np.round(np.asarray(y) / self.S).astype(int), 0, self.n - 1),
                np.clip(np.round(np.asarray(x) / self.S).astype(int), 0, self.n - 1))

    def quota_cm(self, x, y):
        c, r = x / self.S, y / self.S
        c0, r0 = int(math.floor(c)), int(math.floor(r))
        c0, r0 = min(max(c0, 0), self.n - 2), min(max(r0, 0), self.n - 2)
        fc, fr = c - c0, r - r0
        h = self.h
        q = (h[r0, c0] * (1 - fc) * (1 - fr) + h[r0, c0 + 1] * fc * (1 - fr) + h[r0 + 1, c0] * (1 - fc) * fr
             + h[r0 + 1, c0 + 1] * fc * fr)
        return q * 100.0

    def luogo(self, nome):
        return self.luoghi["luoghi"][nome]["unreal_cm"]


def maschera_occupata(valle, villaggio, pianta, uscita_anteprima=None):
    """Dove non si pianta niente (vero = occupato), a 1 m per pixel: case, strade, palizzate e muri del villaggio (tutte
    le fasi), le cose messe dal villaggio, le radure dei luoghi."""
    lato_px = int(valle.lato)
    k = lato_px / (valle.lato * 100.0)   # pixel per cm
    im = Image.new("L", (lato_px, lato_px), 0)
    dr = ImageDraw.Draw(im)
    for d in pianta:
        punti = [(x * k, y * k) for x, y in d["punti"]]
        if len(punti) < 2:
            continue
        if d["tipo"] in ("edificio", "fab"):
            c = np.mean(punti, 0)
            grandi = [(c[0] + (p[0] - c[0]) * 1.25 + np.sign(p[0] - c[0]) * 3, c[1] + (p[1] - c[1]) * 1.25 + np.sign(p[1] - c[1]) * 3)
                      for p in punti]
            dr.polygon(grandi, fill=255)
        elif d["tipo"] == "strada":
            dr.line(punti, fill=255, width=max(3, int(d["larga"] * k + 6)))
        elif d["tipo"] in ("palizzata", "muro"):
            dr.line(punti, fill=255, width=7)
        elif d["tipo"] == "linea":
            if d["nome"] in ("Orto",):
                dr.polygon(punti, fill=255)
            else:
                dr.line(punti, fill=255, width=5)
        elif d["tipo"] == "canneto":
            dr.polygon(punti, fill=255) if len(punti) > 2 else dr.line(punti, fill=255, width=4)
    for c in villaggio["cose"]:
        x, y = c["pos"][0] * k, c["pos"][1] * k
        r = 4 if c["tipo"] in ("mesh", "fab") else 3
        dr.ellipse((x - r, y - r, x + r, y + r), fill=255)
    # le radure dei luoghi (m)
    radure = {"Piazza del villaggio": 60, "Tempio del Cuore": 30, "Rovine degli Antichi": 32, "Grotta est": 25,
              "Grotta nord": 25, "Posto di guardia del passo": 45, "Cimitero": 25, "Pascoli": 130, "Mulino": 25,
              "Ponte": 20, "Collina dell'Orso": 18, "Cascata": 45}
    for nome, r in radure.items():
        if nome in valle.luoghi["luoghi"]:
            x, y, _ = valle.luogo(nome)
            dr.ellipse((x * k - r, y * k - r, x * k + r, y * k + r), fill=255)
    # altri posti da lasciare liberi (cm, m): Gaspare al margine del bosco (metti_png.py) e la sua strada per le rovine
    for x, y, r in ((110240.0, 145945.0, 16),):
        dr.ellipse((x * k - r, y * k - r, x * k + r, y * k + r), fill=255)
    occ = np.array(im) > 0
    return occ, k


def scegli(rng, pesi):
    nomi = list(pesi)
    p = np.array([pesi[n] for n in nomi], np.float64)
    if p.sum() <= 0:
        return None
    return nomi[rng.choice(len(nomi), p=p / p.sum())]


def pianta_alberi(valle, occ, k, rng):
    """I punti candidati (una griglia mossa a caso, 3 m), ognuno con la sua probabilita' e la specie dal posto."""
    passo = 3.0
    lato = valle.lato
    nx = int(lato / passo)
    gx, gy = np.meshgrid(np.arange(nx), np.arange(nx))
    X = (gx + rng.random(gx.shape)) * passo * 100.0
    Y = (gy + rng.random(gy.shape)) * passo * 100.0
    X, Y = X.ravel(), Y.ravel()
    r, c = valle.px(X, Y)
    bosco = valle.strati["Bosco"][r, c]
    strade = valle.strati["Strade"][r, c]
    acqua = valle.strati["Acqua"][r, c]
    vill = valle.strati["Villaggio"][r, c]
    roccia = valle.strati["Roccia"][r, c]
    palude = valle.strati["Palude"][r, c]
    neve = valle.strati["Neve"][r, c]
    pend = valle.pendenza[r, c]
    quota = valle.h[r, c]
    # quanto e' vicina l'acqua (per le betulle): l'acqua sfocata
    from scipy.ndimage import gaussian_filter, maximum_filter
    vicino_acqua = gaussian_filter(valle.strati["Acqua"], 12)[r, c]
    vicino_strada = maximum_filter(valle.strati["Strade"], size=5)[r, c]
    oi = np.clip((Y * k).astype(int), 0, occ.shape[0] - 1)
    oj = np.clip((X * k).astype(int), 0, occ.shape[1] - 1)
    occupato = occ[oi, oj]
    # il fondovalle: la quota tipica dei prati; sopra si va verso il limite del bosco
    fondo = np.percentile(valle.h[valle.strati["Villaggio"] > 0.5], 50) if (valle.strati["Villaggio"] > 0.5).any() else 50.0
    sopra = quota - fondo
    vietato = (occupato | (strade > 0.08) | (vicino_strada > 0.3) | (acqua > 0.05) | (vill > 0.25) | (pend > 38)
               | (neve > 0.3) | (sopra > 420))
    # rumore a macchie (gruppi di alberi nei prati, radure nel bosco)
    from scipy.ndimage import zoom
    macchie_basse = rng.standard_normal((48, 48))
    macchie = gaussian_filter(macchie_basse, 1.5, mode="wrap")
    macchie = zoom(macchie, valle.n / 48.0, order=1)[:valle.n, :valle.n]
    macchie = (macchie - macchie.mean()) / (macchie.std() + 1e-9)
    m = macchie[r, c]
    # le probabilita' (per cella di 3 m)
    p_bosco = np.clip((bosco - 0.12) / 0.5, 0, 1) * np.clip(1.0 + 0.35 * m, 0.25, 1.3) * 0.36
    p_margine = np.clip(1 - np.abs(bosco - 0.12) / 0.12, 0, 1) * 0.10
    p_prato = np.where((bosco < 0.05) & (roccia < 0.5), np.clip(m - 0.6, 0, 2) * 0.05 + 0.004, 0.0)
    # lungo il fiume e le rive del lago: una fascia di betulle, noccioli e faggi
    p_riva = np.where((vicino_acqua > 0.03) & (vicino_acqua < 0.35) & (bosco < 0.12), 0.05, 0.0)
    p_monte = np.where((roccia >= 0.35) & (pend < 37) & (sopra > 40), 0.07 * np.clip(1 - sopra / 420, 0, 1) * np.clip(1 - (pend - 20) / 15, 0, 1), 0.0)
    p_palude = np.where(palude > 0.3, 0.02, 0.0)
    p = (p_bosco + p_margine + p_prato + p_riva + p_monte + p_palude) * FITTO
    p[vietato] = 0
    tieni = rng.random(len(p)) < p
    candidati = []
    for i in np.nonzero(tieni)[0]:
        b, q, s = bosco[i], sopra[i], pend[i]
        acq = vicino_acqua[i]
        if b > 0.12:
            # nel bosco: dentro abeti; i faggi in basso e nel boschetto (est), i larici in alto, le betulle ai margini
            est = X[i] > valle.lato * 100 * 0.55
            pesi = {"abete": 0.70 if not est else 0.28, "faggio": (0.14 if not est else 0.45) * np.clip(1.4 - q / 150, 0.2, 1.4),
                    "larice": 0.10 * np.clip(q / 100, 0.1, 2.0), "betulla": 0.05 + 0.25 * max(0.0, 0.35 - b) + 0.4 * acq,
                    "giovane": 0.10, "nocciolo": 0.08 + (0.12 if est else 0.0)}
            if rng.random() < 0.025:
                pesi = {"tronco": 1.0}
        elif b > 0.0 and p_margine[i] > 0.02:
            pesi = {"giovane": 0.35, "nocciolo": 0.45, "betulla": 0.15, "abete": 0.05}
        elif p_riva[i] > 0:
            pesi = {"betulla": 0.45, "nocciolo": 0.35, "faggio": 0.15, "giovane": 0.05}
        elif p_palude[i] > 0:
            pesi = {"betulla": 0.6, "nocciolo": 0.4}
        elif p_monte[i] > 0:
            pesi = {"larice": 0.6, "abete": 0.25, "giovane": 0.15}
        else:
            pesi = {"faggio": 0.22, "betulla": 0.15 + 0.6 * acq, "abete": 0.18, "larice": 0.15 * np.clip(q / 60, 0, 2),
                    "nocciolo": 0.25, "giovane": 0.08}
        specie = scegli(rng, pesi)
        if specie:
            candidati.append((specie, X[i], Y[i], s))
    return candidati


def ceppi_e_meli(valle, occ, k, pianta, rng, villaggio_cose=()):
    """Vicino al villaggio: i ceppi della legna tagliata al margine del bosco piu' vicino; i meli accanto agli orti."""
    fuori = []
    P = valle.luogo("Piazza del villaggio")
    bosco = valle.strati["Bosco"]
    # i ceppi: nella fascia di bosco rado piu' vicina alla piazza (entro 600 m)
    for _ in range(4000):
        x = P[0] + rng.uniform(-60000, 60000)
        y = P[1] + rng.uniform(-60000, 60000)
        if not (0 < x < valle.lato * 100 and 0 < y < valle.lato * 100):
            continue
        r, c = valle.px(x, y)
        b = bosco[r, c]
        if 0.05 < b < 0.4 and not occ[min(int(y * k), occ.shape[0] - 1), min(int(x * k), occ.shape[1] - 1)] \
                and valle.strati["Strade"][r, c] < 0.05 and valle.pendenza[r, c] < 25:
            fuori.append(("ceppo", x, y, valle.pendenza[r, c]))
        if len(fuori) >= 160:
            break
    # i meli dietro le case di oggi (una su tre): 5-7 m dietro il muro di fondo, dalla parte opposta alla porta
    porte = {c["nome"][:-6]: c["pos"] for c in villaggio_cose if c["tipo"] == "porta" and c["nome"].endswith(" porta")}
    for d in pianta:
        if d["tipo"] != "edificio" or d["sezione"] != "Villaggio" or list(d.get("fase", [1, 0])) != [1, 0] or d["nome"] not in porte:
            continue
        if rng.random() > 0.5:
            continue
        pts = np.array(d["punti"], np.float64)
        centro = pts.mean(0)
        o = np.array(porte[d["nome"]][:2]) - centro
        if np.linalg.norm(o) < 1:
            continue
        o /= np.linalg.norm(o)
        prof = max(np.dot(p - centro, o) for p in pts)
        lato = np.array([-o[1], o[0]])
        for _ in range(6):
            p = centro - o * (prof + rng.uniform(450, 750)) + lato * rng.uniform(-450, 450)
            if not occ[min(max(int(p[1] * k), 0), occ.shape[0] - 1), min(max(int(p[0] * k), 0), occ.shape[1] - 1)]:
                fuori.append(("melo", p[0], p[1], 0.0))
                break
    # i meli: due o tre lungo i lati di ogni orto, a 3-5 m (fuori dall'orto)
    for d in pianta:
        if d["tipo"] != "linea" or d["nome"] != "Orto" or len(d["punti"]) < 3:
            continue
        pts = np.array(d["punti"], np.float64)
        centro = pts.mean(0)
        for _ in range(int(rng.integers(2, 4))):
            i = int(rng.integers(len(pts)))
            a, b = pts[i], pts[(i + 1) % len(pts)]
            m = a + (b - a) * rng.uniform(0.2, 0.8)
            fuori_dir = m - centro
            fuori_dir /= (np.linalg.norm(fuori_dir) + 1e-9)
            p = m + fuori_dir * rng.uniform(300, 500)
            fuori.append(("melo", p[0], p[1], 0.0))
    return fuori


def distanzia(candidati, valle, occ, k, rng):
    """Toglie gli alberi troppo vicini (una griglia per cercare i vicini): i grandi fra loro, i piccoli dai tronchi."""
    cella = 800.0
    griglia = {}
    tenuti = []
    ordine = sorted(range(len(candidati)), key=lambda i: (candidati[i][0] not in GRANDI, rng.random()))
    for i in ordine:
        specie, x, y, s = candidati[i]
        oi, oj = min(int(y * k), occ.shape[0] - 1), min(int(x * k), occ.shape[1] - 1)
        if occ[oi, oj] and specie not in ("melo",):
            continue
        if specie == "melo" and occ[oi, oj]:
            continue
        grande = specie in GRANDI
        r_me = SPAZIO[specie] * 100.0
        gi, gj = int(x // cella), int(y // cella)
        ok = True
        for di in (-1, 0, 1):
            for dj in (-1, 0, 1):
                for (sp2, x2, y2) in griglia.get((gi + di, gj + dj), ()):
                    d = math.hypot(x - x2, y - y2)
                    if grande and sp2 in GRANDI:
                        lim = (r_me + SPAZIO[sp2] * 100.0) * 0.5
                    elif sp2 in GRANDI or grande:
                        lim = 180.0 if specie != "tronco" and sp2 != "tronco" else 450.0
                    else:
                        lim = (r_me + SPAZIO[sp2] * 100.0) * 0.5
                    if d < lim:
                        ok = False
                        break
                if not ok:
                    break
            if not ok:
                break
        if ok:
            griglia.setdefault((gi, gj), []).append((specie, x, y))
            tenuti.append(candidati[i])
    return tenuti


def anteprima(valle, alberi, indice, percorso, lato_px=1500):
    h = valle.h
    piccolo = h[::max(1, valle.n // lato_px), ::max(1, valle.n // lato_px)]
    gy, gx = np.gradient(piccolo)
    luce = np.clip(0.6 + (gx - gy) * 0.08, 0.2, 1.0)
    acqua = valle.strati["Acqua"][::max(1, valle.n // lato_px), ::max(1, valle.n // lato_px)]
    strade = valle.strati["Strade"][::max(1, valle.n // lato_px), ::max(1, valle.n // lato_px)]
    rgb = np.stack([0.55 * luce, 0.62 * luce, 0.40 * luce], -1)
    rgb = rgb * (1 - acqua[..., None]) + np.array([0.25, 0.4, 0.6]) * acqua[..., None]
    rgb = rgb * (1 - strade[..., None]) + np.array([0.75, 0.65, 0.5]) * strade[..., None]
    im = Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8)).resize((lato_px, lato_px))
    dr = ImageDraw.Draw(im)
    colori = {"abete": (20, 60, 30), "giovane": (60, 110, 50), "larice": (120, 160, 60), "faggio": (60, 120, 30),
              "betulla": (220, 220, 200), "melo": (200, 40, 40), "nocciolo": (100, 140, 60), "ceppo": (120, 80, 40),
              "tronco": (90, 60, 30)}
    scala = lato_px / (valle.lato * 100.0)
    nomi = {v: k for k, v in indice.items()}
    for a in alberi:
        sp = nomi[a[0]].split("_")[2].lower()
        sp = {"abetegiovane": "giovane"}.get(sp, sp)
        x, y = a[1] * scala, a[2] * scala
        r = 1.6 if sp in GRANDI else 0.8
        dr.ellipse((x - r, y - r, x + r, y + r), fill=colori.get(sp, (0, 0, 0)))
    im.save(percorso)


def main(cartella_valle, cartella_villaggio, uscita):
    os.makedirs(uscita, exist_ok=True)
    rng = np.random.default_rng(SEME)
    bosco_texture.scrivi_tutte(uscita)
    modelli = bosco_alberi.tutti()
    info = {}
    for nome, (m, i) in modelli.items():
        m.scrivi(os.path.join(uscita, nome + ".glb"))
        info[nome] = {k: (round(v, 1) if isinstance(v, float) else v) for k, v in i.items()}
        info[nome]["triangoli"] = m.conta()
    valle = Valle(cartella_valle)
    with open(os.path.join(cartella_villaggio, "Villaggio_3000.json"), encoding="utf-8") as f:
        villaggio = json.load(f)
    with open(os.path.join(cartella_villaggio, "Pianta_3000.json"), encoding="utf-8") as f:
        pianta = json.load(f)
    occ, k = maschera_occupata(valle, villaggio, pianta)
    candidati = pianta_alberi(valle, occ, k, rng) + ceppi_e_meli(valle, occ, k, pianta, rng, villaggio["cose"])
    tenuti = distanzia(candidati, valle, occ, k, rng)
    indice = {n: i for i, n in enumerate(sorted(info))}
    alberi = []
    conta = {}
    for specie, x, y, pend in tenuti:
        nome = MODELLI[specie][int(rng.integers(len(MODELLI[specie])))]
        z = valle.quota_cm(x, y)
        scala = float(rng.uniform(0.85, 1.15))
        if specie == "tronco":
            # un tronco caduto sta lungo il pendio (girato a caso), un poco affondato
            pass
        piega = float(rng.uniform(0, 2.5)) if specie in ("abete", "larice", "faggio", "betulla", "giovane") else 0.0
        alberi.append([indice[nome], round(float(x), 1), round(float(y), 1), round(float(z), 1),
                       round(float(rng.uniform(0, 360)), 1), round(scala, 3), round(piega, 2), round(float(rng.uniform(0, 360)), 1)])
        conta[specie] = conta.get(specie, 0) + 1
    dati = {
        "nota": "Fatto da Tools/Generatori/bosco.py: cm dall'angolo nord-ovest del paesaggio, Z = quota vera in cm. "
                "Ogni albero: [modello (indice in 'ordine'), x, y, z, gradi, scala, piega (gradi di inclinazione), verso della piega].",
        "ordine": sorted(info),
        "modelli": info,
        "alberi": alberi,
        "quanti": conta,
    }
    with open(os.path.join(uscita, "Bosco_3000.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, separators=(",", ":"))
    anteprima(valle, alberi, indice, os.path.join(uscita, "Anteprima_Bosco.png"))
    print("alberi:", len(alberi), conta)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3])
