# Valdorso - Le immagini del launcher: lo sfondo (la valle al tramonto, dipinta da zero), l'icona e i caratteri.
# Scritto da Claude il 07/11/2026; (08/11) le immagini delle notizie e delle Cronache. Richiede Pillow e numpy.
#
# Uso:  python launcher_immagini.py <cartella dei caratteri> <cartella di uscita>
#   sul PC: python Tools\Generatori\launcher_immagini.py V:\Font V:\Valdorso\Build\Launcher\risorse
# Fa:   sfondo.jpg (2200 x 1280), icona.ico (16-256 px), anteprima_icona.png e copia Cinzel ed EB Garamond
#       (licenza SIL OFL 1.1: si possono dare insieme al launcher, con la licenza) accanto, per la finestra;
#       in immagini/: valle.jpg e rune.jpg (le cartoline delle notizie) e cronache.jpg (l'Orso sotto la luna, la
#       testata delle Cronache). prepara_aggiornamento.py le copia anche sul sito (cartella immagini/).
# Cerca i caratteri sia come file statici (Cinzel/static/Cinzel-Bold.ttf) sia come file variabili (Cinzel.ttf).

import math
import os
import random
import shutil
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

FONDO = (14, 11, 9)
ORO = (212, 175, 55)
OROC = (243, 220, 140)
BRACE = (217, 119, 43)


def trova(cartella, nomi):
    for radice, _, files in os.walk(cartella):
        for n in nomi:
            if n in files:
                return os.path.join(radice, n)
    return None


def cresta(n, rng, ruvido, base, ampiezza):
    """Una linea di monti: spostamento del punto medio (frattale), da 0 a 1 in altezza."""
    lati = 2 ** int(math.ceil(math.log2(n)))
    y = np.zeros(lati + 1)
    y[0], y[-1] = rng.random(), rng.random()
    passo, scala = lati, 1.0
    while passo > 1:
        meta = passo // 2
        for i in range(meta, lati, passo):
            y[i] = (y[i - meta] + y[i + meta]) / 2 + (rng.random() - 0.5) * scala
        scala *= ruvido
        passo = meta
    y = y[:n]
    y = (y - y.min()) / max(1e-6, y.max() - y.min())
    return base - y * ampiezza


def sfondo(W=2200, H=1280, seme=312):
    rng = random.Random(seme)
    yy = np.linspace(0, 1, H)[:, None]
    xx = np.linspace(0, 1, W)[None, :]
    sole = (0.70, 0.50)
    # il cielo: in alto viola notte, poi rosa e oro verso l'orizzonte
    alto = np.array([28, 22, 44], float)
    medio = np.array([120, 58, 62], float)
    basso = np.array([238, 150, 70], float)
    t = np.clip(yy / 0.56, 0, 1)
    sopra = alto + (medio - alto) * np.clip(t / 0.55, 0, 1) ** 1.2
    sotto = medio + (basso - medio) * np.clip((t - 0.55) / 0.45, 0, 1) ** 0.9
    cielo = np.where(t < 0.55, sopra, sotto)                      # (H, 3)
    img = np.broadcast_to(cielo[:, None, :], (H, W, 3)).copy()
    # il bagliore del sole
    d = np.sqrt(((xx - sole[0]) * W / H) ** 2 + (yy - sole[1]) ** 2)
    img += (np.exp(-(d / 0.09) ** 2) * 255 + np.exp(-(d / 0.35) ** 2) * 70)[..., None] * np.array([1.0, 0.78, 0.45])
    # nuvole sottili, allungate, illuminate da sotto
    nuvole = Image.new("L", (W, H), 0)
    dn = ImageDraw.Draw(nuvole)
    for _ in range(26):
        cx, cy = rng.uniform(0, W), rng.uniform(H * 0.12, H * 0.44)
        lw, lh = rng.uniform(180, 620), rng.uniform(8, 26)
        dn.ellipse([cx - lw, cy - lh, cx + lw, cy + lh], fill=int(rng.uniform(40, 110)))
    nuvole = np.asarray(nuvole.filter(ImageFilter.GaussianBlur(14))).astype(float)[..., None] / 255
    img = img * (1 - nuvole * 0.5) + nuvole * 0.5 * np.array([250, 170, 120])

    # i monti: 5 creste dalla più lontana (chiara, velata) alla più vicina (scura)
    colonne = np.arange(W)
    piani = [
        (0.585, 0.20, 0.55, (150, 92, 96), 0.80),
        (0.625, 0.22, 0.58, (104, 62, 74), 0.62),
        (0.680, 0.20, 0.60, (66, 40, 50), 0.42),
        (0.760, 0.17, 0.62, (36, 24, 28), 0.22),
        (0.860, 0.14, 0.64, (18, 13, 13), 0.0),
    ]
    for k, (base, amp, ruvido, colore, velo) in enumerate(piani):
        linea = cresta(W, rng, ruvido, base, amp) * H
        # la valle: una conca al centro destro, dove scende il fiume verso il lago
        conca = np.exp(-((colonne / W - 0.66) / 0.16) ** 2) * (0.06 + 0.03 * k) * H
        linea = linea + conca
        maschera = yy * H >= linea[None, :]
        tono = np.array(colore, float)
        # i monti lontani prendono il colore del cielo (aria), i vicini restano scuri; i bordi dal sole un filo d'oro
        tinta = tono * (1 - velo) + img * velo
        img = np.where(maschera[..., None], tinta, img)
        bordo = np.abs(yy * H - linea[None, :]) < 2.2
        luce_bordo = np.exp(-((colonne / W - sole[0]) / 0.25) ** 2)[None, :]
        img = np.where(bordo[..., None], img + (luce_bordo * 50)[..., None] * np.array([1.0, 0.75, 0.4]), img)

    # il lago in basso al centro destro, che riflette il sole
    lago_y0, lago_y1 = int(H * 0.835), int(H * 0.875)
    for y in range(lago_y0, lago_y1):
        a = (y - lago_y0) / max(1, lago_y1 - lago_y0)
        larg = 0.13 * (1 - abs(a - 0.5) * 1.6)
        x0, x1 = int(W * (0.66 - larg)), int(W * (0.66 + larg))
        if x1 > x0:
            riflesso = np.exp(-((np.arange(x0, x1) / W - sole[0]) / 0.03) ** 2)[:, None] * np.array([240, 170, 90])
            img[y, x0:x1] = img[y, x0:x1] * 0.35 + np.array([70, 48, 52]) * 0.5 + riflesso * 0.6

    # le luci del villaggio, piccole e calde, sulla riva
    luci = Image.new("RGB", (W, H), (0, 0, 0))
    dl = ImageDraw.Draw(luci)
    for _ in range(38):
        x = rng.gauss(W * 0.61, W * 0.035)
        y = rng.gauss(H * 0.818, H * 0.008)
        r = rng.uniform(1.2, 2.6)
        dl.ellipse([x - r, y - r, x + r, y + r], fill=(255, int(rng.uniform(150, 200)), 80))
    luci = np.asarray(luci.filter(ImageFilter.GaussianBlur(1.6))).astype(float) * 1.6
    img = img + luci

    # le braci che salgono e la vignettatura
    img = np.clip(img, 0, 255)
    im = Image.fromarray(img.astype(np.uint8), "RGB").convert("RGBA")
    braci = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    db = ImageDraw.Draw(braci)
    for _ in range(140):
        x = rng.uniform(0, W)
        y = H * (0.55 + rng.random() * 0.45)
        r = rng.uniform(0.8, 2.6)
        db.ellipse([x - r, y - r, x + r, y + r], fill=(255, int(rng.uniform(120, 190)), int(rng.uniform(40, 90)), int(rng.uniform(60, 200) * (y / H))))
    im.alpha_composite(braci.filter(ImageFilter.GaussianBlur(3)))
    im.alpha_composite(braci)
    arr = np.asarray(im.convert("RGB")).astype(float)
    vign = np.clip(1 - 0.55 * ((xx - 0.55) * 1.5) ** 2 - 0.5 * ((yy - 0.45) * 1.4) ** 2, 0.35, 1)[..., None]
    arr = arr * vign
    return Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGB")


def rumore(W, H, rng, scale=(4, 16, 64, 160), pesi=(0.5, 0.3, 0.15, 0.05)):
    """Un rumore morbido 0..1 (la grana della pietra): somma di rumori sfocati a più scale."""
    tot = np.zeros((H, W))
    for sc, p in zip(scale, pesi):
        w, h = max(2, W // sc), max(2, H // sc)
        r = Image.fromarray((rng.random((h, w)) * 255).astype(np.uint8), "L").resize((W, H), Image.BICUBIC)
        tot += np.asarray(r).astype(float) / 255 * p
    return (tot - tot.min()) / max(1e-6, tot.max() - tot.min())


def cartolina_valle(W=1300, H=450):
    """Un'altra valle al tramonto (altro seme), tagliata larga: la cartolina della notizia della v0.1.3."""
    grande = sfondo(W=1500, H=880, seme=77)
    y0 = int(880 * 0.38)
    return grande.crop((100, y0, 100 + W, y0 + H))


def cartolina_rune(W=1300, H=450):
    """Le rune incise nella pietra scura, accese d'oro e di brace (la cartolina della notizia delle rune)."""
    import alfabeto_rune as ar
    rng = np.random.default_rng(311)
    n = rumore(W, H, rng)
    pietra = np.stack([28 + n * 34, 24 + n * 28, 22 + n * 24], -1)
    # una luce calda da sinistra in basso
    yy = np.linspace(0, 1, H)[:, None]
    xx = np.linspace(0, 1, W)[None, :]
    luce = np.exp(-(((xx - 0.3) / 0.55) ** 2 + ((yy - 0.9) / 0.7) ** 2))
    pietra = pietra * (0.55 + 0.6 * luce[..., None])
    img = Image.fromarray(np.clip(pietra, 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    # le rune: la riga degli elementi grande e una parola sotto
    tratti = Image.new("L", (W, H), 0)
    dt = ImageDraw.Draw(tratti)

    def incidi(segmenti, cx, cy, scala, spessore):
        for (a, b) in segmenti:
            dt.line([(cx + a[0] * scala, cy - a[1] * scala), (cx + b[0] * scala, cy - b[1] * scala)], fill=255, width=spessore)
            for (x, y) in (a, b):
                r = spessore / 2
                dt.ellipse([cx + x * scala - r, cy - y * scala - r, cx + x * scala + r, cy - y * scala + r], fill=255)

    nomi = ar.ELEMENTI + ["Ur"] + ar.VERBI
    passo = W / (len(nomi) + 1)
    for k, nome in enumerate(nomi):
        cx = passo * (k + 1)
        cy = H * 0.36 + (12 if k % 2 else -8)
        incidi(ar.tratti(nome), cx, cy, 44 if nome != "Ur" else 56, 8)
    incidi(ar.tratti_parola("Aeth-Var-Tor"), W * 0.5, H * 0.76, 40, 7)
    alone = tratti.filter(ImageFilter.GaussianBlur(14))
    s = Image.new("RGBA", (W, H), BRACE + (0,))
    s.putalpha(alone.point(lambda v: int(v * 0.9)))
    img.alpha_composite(s)
    # il solco: scuro sotto, oro sopra
    solco = Image.new("RGBA", (W, H), (10, 6, 4, 0))
    solco.putalpha(tratti.point(lambda v: int(v * 0.8)))
    img.alpha_composite(solco, (2, 3))
    img.alpha_composite(gradiente_oro(tratti))
    arr = np.asarray(img.convert("RGB")).astype(float)
    vign = np.clip(1 - 0.6 * ((xx - 0.5) * 1.3) ** 2 - 0.5 * ((yy - 0.5) * 1.3) ** 2, 0.4, 1)[..., None]
    return Image.fromarray(np.clip(arr * vign, 0, 255).astype(np.uint8), "RGB")


def punti_orso():
    """La sagoma dell'orso di orso.py (l'ombra che si vede nel bosco): si leggono i punti senza eseguirlo."""
    import ast
    import re
    testo = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "orso.py"), encoding="utf-8").read()
    m = re.search(r"pts\s*=\s*(\[.*?\])\s*\nP\s*=", testo, re.S)
    return ast.literal_eval(m.group(1))


def testata_cronache(W=1800, H=330):
    """L'Orso sulla collina più alta, sotto la luna: la testata delle Cronache."""
    rng = random.Random(312)
    yy = np.linspace(0, 1, H)[:, None]
    xx = np.linspace(0, 1, W)[None, :]
    # il cielo di notte, con l'ultima brace del tramonto sull'orizzonte
    alto = np.array([10, 10, 22], float)
    basso = np.array([92, 44, 30], float)
    cielo = alto + (basso - alto) * (yy ** 2.2)[..., None]
    img = np.broadcast_to(cielo, (H, W, 3)).copy()
    luna = (0.78, 0.30)
    d = np.sqrt(((xx - luna[0]) * W / H) ** 2 + (yy - luna[1]) ** 2)
    img += (np.exp(-(d / 0.35) ** 2) * 40)[..., None] * np.array([0.8, 0.85, 1.0])
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    dr = ImageDraw.Draw(im)
    for _ in range(170):
        x, y = rng.uniform(0, W), rng.uniform(0, H * 0.7)
        r = rng.uniform(0.5, 1.6)
        a = int(rng.uniform(60, 220) * (1 - y / H))
        dr.ellipse([x - r, y - r, x + r, y + r], fill=(240, 232, 210, a))
    # la falce: il disco meno un disco spostato (niente cerchio scuro sul cielo)
    lx, ly, lr = luna[0] * W, luna[1] * H, H * 0.11
    falce = Image.new("L", (W, H), 0)
    df = ImageDraw.Draw(falce)
    df.ellipse([lx - lr, ly - lr, lx + lr, ly + lr], fill=255)
    df.ellipse([lx - lr + lr * 0.5, ly - lr - lr * 0.2, lx + lr + lr * 0.5, ly + lr - lr * 0.2], fill=0)
    chiara = Image.new("RGBA", (W, H), (236, 228, 205, 0))
    chiara.putalpha(falce.filter(ImageFilter.GaussianBlur(0.8)))
    im.alpha_composite(chiara)
    # i monti lontani e la collina
    nrng = np.random.default_rng(5)
    colonne = np.arange(W)
    for base, amp, colore in ((0.80, 0.30, (40, 30, 40)), (0.92, 0.18, (22, 17, 20))):
        y = np.zeros(W)
        for f, a in ((2, 1.0), (5, 0.45), (13, 0.2), (37, 0.08)):
            fase = nrng.uniform(0, 6.28)
            y += a * np.sin(colonne / W * f * 6.28 + fase)
        y = (y - y.min()) / (y.max() - y.min())
        linea = (base - y * amp) * H
        poli = [(0, H)] + [(int(i), float(linea[i])) for i in range(0, W, 4)] + [(W, H)]
        ImageDraw.Draw(im).polygon(poli, fill=colore + (255,))
    collina = [(int(W * 0.10), H), (int(W * 0.24), int(H * 0.80)), (int(W * 0.33), int(H * 0.73)), (int(W * 0.44), int(H * 0.79)), (int(W * 0.62), H)]
    ImageDraw.Draw(im).polygon(collina, fill=(12, 9, 9, 255))
    # l'orso, nero con un filo d'oro sul dorso (la luna da destra, in alto) e l'occhio di brace
    pts = punti_orso()
    scala = H * 0.40 / 760.0
    ox, oy = W * 0.33 - 1170 * scala, H * 0.75 - 978 * scala
    P = [(ox + x * scala, oy + y * scala) for x, y in pts]
    m = Image.new("L", (W, H), 0)
    ImageDraw.Draw(m).polygon(P, fill=255)
    m = m.filter(ImageFilter.GaussianBlur(0.8))
    from PIL import ImageChops
    # il filo di luce: i punti dell'orso che, spostandosi verso la luna, escono dalla sagoma
    spostata = ImageChops.offset(m, -3, 3)
    filo = ImageChops.subtract(m, spostata).filter(ImageFilter.GaussianBlur(1.0))
    oro = Image.new("RGBA", (W, H), OROC + (0,))
    oro.putalpha(filo.point(lambda v: int(min(255, v * 0.9))))
    nero = Image.new("RGBA", (W, H), (8, 6, 6, 0))
    nero.putalpha(m)
    im.alpha_composite(nero)
    im.alpha_composite(oro)
    ex, ey = ox + 1838 * scala, oy + 474 * scala
    occhio = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(occhio).ellipse([ex - 6, ey - 6, ex + 6, ey + 6], fill=BRACE + (160,))
    im.alpha_composite(occhio.filter(ImageFilter.GaussianBlur(4)))
    ImageDraw.Draw(im).ellipse([ex - 1.6, ey - 1.6, ex + 1.6, ey + 1.6], fill=(255, 190, 110, 255))
    # la nebbia bassa
    nebbia = np.clip(rumore(W, H, np.random.default_rng(9), scale=(8, 30, 120), pesi=(0.2, 0.4, 0.4)) * 1.4 - 0.3, 0, 1)
    nebbia = nebbia * np.clip((yy - 0.65) / 0.35, 0, 1) * 0.55
    arr = np.asarray(im.convert("RGB")).astype(float)
    arr = arr * (1 - nebbia[..., None]) + nebbia[..., None] * np.array([120, 104, 110])
    vign = np.clip(1 - 0.5 * ((xx - 0.45) * 1.2) ** 2, 0.45, 1)[..., None]
    return Image.fromarray(np.clip(arr * vign, 0, 255).astype(np.uint8), "RGB")


def gradiente_oro(maschera):
    w, h = maschera.size
    y = np.linspace(0, 1, h)[:, None]
    a = np.array(OROC, float)
    b = np.array(ORO, float)
    c = np.array((150, 110, 35), float)
    col = np.where(y < 0.55, a + (b - a) * (y / 0.55), b + (c - b) * ((y - 0.55) / 0.45))
    arr = np.repeat(col[:, None, :], w, axis=1).astype(np.uint8)
    img = Image.fromarray(arr, "RGB").convert("RGBA")
    img.putalpha(maschera)
    return img


def icona(cinzel, N=256, S=4):
    """Come l'icona del gioco (logo.py): la V d'oro su fondo scuro con il bordo d'oro e il rombo di brace."""
    n = N * S
    im = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    m = Image.new("L", (n, n), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, n - 1, n - 1], radius=int(n * 0.2), fill=255)
    yy = np.linspace(0, 1, n)[:, None]
    xx = np.linspace(-1, 1, n)[None, :]
    g = np.exp(-((xx / 0.8) ** 2 + ((yy - 0.95) / 0.45) ** 2))
    base = np.array(FONDO, float)[None, None, :] + g[..., None] * np.array([70, 30, 8])[None, None, :]
    fondo = Image.fromarray(np.clip(base, 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    fondo.putalpha(m)
    im.alpha_composite(fondo)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([int(n * 0.03)] * 2 + [n - 1 - int(n * 0.03)] * 2, radius=int(n * 0.18), outline=ORO + (255,), width=int(n * 0.025))
    r = int(n * 0.36)
    c = n // 2
    d.ellipse([c - r, c - r + int(n * 0.02), c + r, c + r + int(n * 0.02)], outline=ORO + (150,), width=int(n * 0.012))
    f = ImageFont.truetype(cinzel, int(n * 0.66))
    try:
        f.set_variation_by_name("Bold")
    except Exception:  # noqa: BLE001
        pass
    mm = Image.new("L", (n, n), 0)
    dm = ImageDraw.Draw(mm)
    bb = f.getbbox("V")
    vw, vh = bb[2] - bb[0], bb[3] - bb[1]
    dm.text((c - vw / 2 - bb[0], c - vh / 2 - bb[1] + int(n * 0.01)), "V", font=f, fill=255)
    al = mm.filter(ImageFilter.GaussianBlur(n * 0.04)).point(lambda v: int(v * 0.6))
    s = Image.new("RGBA", (n, n), BRACE + (0,))
    s.putalpha(al)
    im.alpha_composite(s)
    bx = mm.getbbox()
    sub = mm.crop((0, bx[1], n, bx[3]))
    im.alpha_composite(gradiente_oro(sub), (0, bx[1]))
    rr = int(n * 0.04)
    y0 = int(n * 0.80)
    d = ImageDraw.Draw(im)
    d.polygon([(c, y0 - rr), (c + rr, y0), (c, y0 + rr), (c - rr, y0)], fill=(240, 150, 60, 255))
    return im.resize((N, N), Image.LANCZOS)


def principale():
    caratteri = sys.argv[1] if len(sys.argv) > 1 else "V:/Font"
    uscita = sys.argv[2] if len(sys.argv) > 2 else "risorse"
    os.makedirs(uscita, exist_ok=True)

    sfondo().save(os.path.join(uscita, "sfondo.jpg"), quality=90, optimize=True)
    print("sfondo.jpg fatto")
    immagini = os.path.join(uscita, "immagini")
    os.makedirs(immagini, exist_ok=True)
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))  # per alfabeto_rune
    cartolina_valle().save(os.path.join(immagini, "valle.jpg"), quality=86, optimize=True)
    cartolina_rune().save(os.path.join(immagini, "rune.jpg"), quality=86, optimize=True)
    testata_cronache().save(os.path.join(immagini, "cronache.jpg"), quality=86, optimize=True)
    print("immagini/valle.jpg, rune.jpg, cronache.jpg fatte")

    cinzel = trova(caratteri, ["Cinzel-Bold.ttf", "Cinzel.ttf", "Cinzel-VariableFont_wght.ttf"])
    garamond = trova(caratteri, ["EBGaramond-Regular.ttf", "EBGaramond.ttf", "EBGaramond-VariableFont_wght.ttf"])
    corsivo = trova(caratteri, ["EBGaramond-Italic.ttf", "EBGaramond-Italic-VariableFont_wght.ttf"])
    if cinzel:
        ic = icona(cinzel)
        ic.save(os.path.join(uscita, "anteprima_icona.png"))
        ic.save(os.path.join(uscita, "icona.ico"), sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
        print("icona.ico fatta")
    else:
        print("ATTENZIONE: Cinzel non trovato in %s: niente icona" % caratteri)
    for sorgente, nome in ((cinzel, "Cinzel.ttf"), (garamond, "EBGaramond.ttf"), (corsivo, "EBGaramond-Italic.ttf")):
        if sorgente:
            shutil.copyfile(sorgente, os.path.join(uscita, nome))
            print("copiato %s" % nome)
        else:
            print("ATTENZIONE: %s non trovato: la finestra userà Georgia" % nome)
    # le licenze dei caratteri (SIL OFL 1.1) vanno insieme ai caratteri
    licenze = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Licenze")
    for nome in ("OFL_Cinzel.txt", "OFL_EBGaramond.txt"):
        if os.path.isfile(os.path.join(licenze, nome)):
            shutil.copyfile(os.path.join(licenze, nome), os.path.join(uscita, nome))


if __name__ == "__main__":
    principale()
