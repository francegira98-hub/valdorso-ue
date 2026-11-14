# Valdorso - La carta e la coccarda di ceralacca della tessera del colono (13/11/2026, scritto da Claude; chiesto da Fra
# dopo la prova del 12/11: "tessera con effetto carta e coccarda di ceralacca rossa in basso").
#
# Scrive in <uscita>:
#   T_Tessera_Carta.png   1024 x 720: carta pesante color crema, con le fibre, due pieghe (era piegata in tre nella
#                          bisaccia), qualche macchia e gli angoli consumati; i bordi un poco irregolari (trasparenza).
#   T_Ceralacca.png       256 x 320: la coccarda: due nastri rossi con il filo d'oro, sopra il sigillo di ceralacca rossa
#                          della capitana Livia Serra (13/11: la serra di tre cime sotto la stella, e in giro
#                          "L. SERRA · CAPITANERIA DEL PASSO"): prende il posto del timbro d'inchiostro.
# sigillo() fa anche i sigilli di altre persone e famiglie (un segno, una scritta, un colore di cera).
# Le importa nell'editor Content/Python/nuovi/importa_icone.py (in /Game/UI/Icone, insieme alle icone); le usa
# SValdorsoTessera.cpp (se mancano, la tessera resta come prima).
# Uso: python tessera.py <uscita>       Sempre uguale (semi fissi).

import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter


def rumore(w, h, scala, rng, ottave=4):
    tot = np.zeros((h, w))
    peso, somma = 1.0, 0.0
    for o in range(ottave):
        kx = max(2, int(w / scala * (2 ** o)))
        ky = max(2, int(h / scala * (2 ** o)))
        g = rng.random((ky, kx))
        img = Image.fromarray((g * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)
        tot += np.asarray(img, np.float64) / 255.0 * peso
        somma += peso
        peso *= 0.5
    tot /= somma
    return (tot - tot.min()) / (tot.max() - tot.min() + 1e-9)


def carta(uscita):
    rng = np.random.default_rng(31)
    w, h = 1024, 720
    base = np.array([238, 226, 196], np.float64)
    scura = np.array([206, 184, 140], np.float64)
    y, x = np.mgrid[0:h, 0:w]
    u, v = x / (w - 1) * 2 - 1, y / (h - 1) * 2 - 1
    # le macchie larghe e l'ingiallito dei bordi
    macchie = rumore(w, h, 180, rng) * 0.7 + rumore(w, h, 40, rng, 3) * 0.3
    bordo = np.clip((np.maximum(abs(u) * 1.0, abs(v) * 1.0) - 0.7) / 0.3, 0, 1) ** 2
    t = np.clip(0.22 * macchie + 0.5 * bordo, 0, 1)
    rgb = base[None, None] * (1 - t[..., None]) + scura[None, None] * t[..., None]
    # le fibre: tante righine corte e chiare/scure in tutte le direzioni
    fibre = Image.new("L", (w, h), 128)
    d = ImageDraw.Draw(fibre)
    for _ in range(9000):
        cx, cy = rng.uniform(0, w), rng.uniform(0, h)
        a = rng.uniform(0, math.pi)
        lung = rng.uniform(4, 16)
        c = int(rng.choice([96, 160]))
        d.line([(cx, cy), (cx + math.cos(a) * lung, cy + math.sin(a) * lung)], fill=c, width=1)
    fibre = np.asarray(fibre.filter(ImageFilter.GaussianBlur(0.6)), np.float64) / 128.0 - 1.0
    rgb *= (1.0 + 0.05 * fibre)[..., None]
    rgb *= (0.985 + 0.03 * rumore(w, h, 2.5, rng, 1))[..., None]
    # le pieghe: era piegata in tre (due righe verticali), con la luce da sinistra: un lato chiaro e uno scuro
    for px in (w / 3, 2 * w / 3):
        dx = x - (px + 6 * np.sin(y / h * 3.1))
        rgb *= (1.0 - 0.045 * np.exp(-(dx - 1.5) ** 2 / 4.0) + 0.025 * np.exp(-(dx + 3.0) ** 2 / 10.0))[..., None]
    # un paio di aloni (una goccia d'acqua, il pollice sporco della capitana)
    for cx, cy, r, f in ((w * 0.83, h * 0.22, 46, 0.05), (w * 0.12, h * 0.8, 30, 0.04)):
        dist = np.sqrt((x - cx) ** 2 + (y - cy) ** 2)
        anello = np.exp(-(dist - r) ** 2 / 18.0)
        rgb *= (1.0 - f * anello)[..., None]
    # i bordi: un poco irregolari, gli angoli consumati (arrotondati e più scuri)
    alfa = Image.new("L", (w, h), 0)
    ImageDraw.Draw(alfa).rounded_rectangle([6, 6, w - 7, h - 7], radius=22, fill=255)
    alfa = np.asarray(alfa, np.float64)
    sfrangia = rumore(w, h, 6, rng, 2)
    distanza_bordo = np.minimum(np.minimum(x, w - 1 - x), np.minimum(y, h - 1 - y))
    alfa = np.where(distanza_bordo < 12, alfa * np.clip((distanza_bordo - 4 + sfrangia * 8) / 6, 0, 1), alfa)
    rgb *= (1.0 - 0.18 * np.clip(1 - distanza_bordo / 14.0, 0, 1))[..., None]
    img = np.dstack([np.clip(rgb, 0, 255), np.clip(alfa, 0, 255)]).astype(np.uint8)
    Image.fromarray(img, "RGBA").save(os.path.join(uscita, "T_Tessera_Carta.png"))


def nastro(d, punti, colore, ombra):
    d.polygon(punti, fill=colore)
    d.line(punti + [punti[0]], fill=ombra, width=3)


# --------------------------------------------------------------------------------------------- i segni dei sigilli
# (13/11, chiesto da Fra) Ogni sigillo ha il suo segno, così si capisce che è vero. Un segno disegna in una maschera
# (bianco = inciso nella cera) intorno al centro (cx, cy), dentro un cerchio di raggio circa 40.

def segno_zampa(d, cx, cy):
    """La zampa d'orso (la valle)."""
    d.ellipse([cx - 20, cy - 2, cx + 20, cy + 26], fill=255)
    for dx, dy, r in ((-24, -16, 8), (-9, -26, 8), (9, -26, 8), (24, -16, 8)):
        d.ellipse([cx + dx - r, cy + dy - r, cx + dx + r, cy + dy + r], fill=255)


def segno_serra(d, cx, cy):
    """Il segno della capitana Livia Serra: la serra (una cresta di tre cime, il suo nome) sotto la stella del passo
    (la stella che c'era nel suo timbro d'inchiostro)."""
    cime = [(cx - 34, cy + 22), (cx - 20, cy - 2), (cx - 10, cy + 10), (cx, cy - 14), (cx + 10, cy + 10),
            (cx + 20, cy - 2), (cx + 34, cy + 22)]
    d.line(cime, fill=255, width=6, joint="curve")
    d.line([(cx - 30, cy + 28), (cx + 30, cy + 28)], fill=255, width=4)
    stella = []
    for k in range(11):
        a = -math.pi / 2 + math.pi * k / 5
        r = 13 if k % 2 == 0 else 5.5
        stella.append((cx + math.cos(a) * r, cy - 30 + math.sin(a) * r))
    d.polygon(stella, fill=255)


SEGNI = {"zampa": segno_zampa, "serra": segno_serra}


def scritta_in_giro(d, cx, cy, raggio, testo, carattere):
    """Le lettere incise tutto intorno, dall'alto in senso orario."""
    if carattere is None:
        for k in range(28):
            a = 2 * math.pi * k / 28
            px, py = cx + math.cos(a) * raggio, cy + math.sin(a) * raggio
            d.ellipse([px - 1.5, py - 1.5, px + 1.5, py + 1.5], fill=255)
        return
    n = len(testo)
    for k, lettera in enumerate(testo):
        a = -math.pi / 2 + 2 * math.pi * k / n
        lato = 18
        l = Image.new("L", (lato, lato), 0)
        ImageDraw.Draw(l).text((lato / 2, lato / 2), lettera, fill=255, font=carattere, anchor="mm")
        l = l.rotate(-math.degrees(a + math.pi / 2), resample=Image.BICUBIC)
        px, py = cx + math.cos(a) * raggio, cy + math.sin(a) * raggio
        d._image.paste(255, (int(px - lato / 2), int(py - lato / 2)), l)


def trova_carattere(grandezza):
    for f in ("/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf", "C:/Windows/Fonts/timesbd.ttf"):
        if os.path.exists(f):
            from PIL import ImageFont
            return ImageFont.truetype(f, grandezza)
    return None


def sigillo(uscita, nome_file, segno, scritta, seme, colore=(168, 30, 28), nastri=True):
    """Una coccarda (se nastri) con il sigillo di ceralacca: la goccia schiacciata col bordo irregolare, il rilievo con
    la luce da sinistra, l'anello, la scritta in giro e il segno di chi l'ha impresso."""
    rng = np.random.default_rng(seme)
    w, h = 256, 320
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx, cy = w / 2, 118
    scuro = tuple(int(c * 0.6) for c in colore) + (255,)
    medio = tuple(int(c * 0.85) for c in colore) + (255,)
    if nastri:
        # 1. i due nastri, a coda di rondine, con il filo d'oro nel mezzo
        for lato in (-1, 1):
            a = (cx + lato * 18, cy + 40)
            b = (cx + lato * 58, cy + 40)
            c = (cx + lato * 74, h - 14)
            e = (cx + lato * 52, h - 40)
            f = (cx + lato * 34, h - 8)
            nastro(d, [a, b, c, e, f], medio, scuro)
            d.line([((a[0] + b[0]) / 2, cy + 44), ((c[0] + f[0]) / 2 + lato * 2, h - 30)], fill=(206, 164, 64, 255), width=4)
        # 2. la rosetta di nastro arricciato dietro il sigillo (pieghe tonde, una chiara e una scura)
        d.ellipse([cx - 84, cy - 84, cx + 84, cy + 84], fill=scuro)
        for k in range(20):
            ang = 2 * math.pi * k / 20
            px, py = cx + math.cos(ang) * 74, cy + math.sin(ang) * 74
            r = 13
            d.ellipse([px - r, py - r, px + r, py + r], fill=tuple(min(255, int(c * 1.05)) for c in colore) + (255,) if k % 2 == 0 else medio)
        d.ellipse([cx - 70, cy - 70, cx + 70, cy + 70], fill=medio)
    # 3. la ceralacca: una goccia schiacciata, il bordo irregolare e qualche colatura
    goccia = Image.new("L", (w, h), 0)
    dg = ImageDraw.Draw(goccia)
    punti = []
    for k in range(64):
        ang = 2 * math.pi * k / 64
        r = 66 + 5 * math.sin(ang * 3 + 1.3) + 3 * math.sin(ang * 7) + rng.uniform(-1.5, 1.5)
        punti.append((cx + math.cos(ang) * r, cy + math.sin(ang) * r))
    dg.polygon(punti, fill=255)
    for ang, lung in ((1.2, 16), (2.3, 10), (5.6, 12)):
        px, py = cx + math.cos(ang) * 62, cy + math.sin(ang) * 62
        dg.ellipse([px - 9, py - 9, px + 9, py + 9 + lung * 0.4], fill=255)
    goccia = goccia.filter(ImageFilter.GaussianBlur(1.0))
    g = np.asarray(goccia, np.float64) / 255.0
    yy, xx = np.mgrid[0:h, 0:w]
    dist = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    luce = np.clip(1.0 - ((xx - cx + 22) ** 2 + (yy - cy + 26) ** 2) / (95.0 ** 2), 0, 1)
    incavo = np.clip(1 - np.abs(dist - 46) / 3.5, 0, 1)            # l'anello dell'impronta
    rgb = np.zeros((h, w, 3))
    rgb[:] = np.array(colore, np.float64)
    rgb *= (0.62 + 0.55 * luce)[..., None]
    rgb *= (1.0 - 0.35 * incavo)[..., None]
    # il segno e la scritta, incisi (più scuri) con il bordo chiaro sotto a destra (il rilievo)
    impronta = Image.new("L", (w, h), 0)
    di = ImageDraw.Draw(impronta)
    SEGNI[segno](di, cx, cy)
    scritta_in_giro(di, cx, cy, 56, scritta, trova_carattere(11) if scritta else None)
    imp = np.asarray(impronta.filter(ImageFilter.GaussianBlur(1.0)), np.float64) / 255.0
    imp_sotto = np.roll(np.roll(imp, 2, 0), 2, 1)
    rgb *= (1.0 - 0.38 * imp)[..., None]
    rgb += (np.clip(imp_sotto - imp, 0, 1) * 70)[..., None]
    # il lucido della cera
    lucido = np.exp(-(((xx - cx + 30) ** 2) / 120.0 + ((yy - cy + 34) ** 2) / 50.0))
    rgb += (lucido * 90)[..., None]
    rgb *= (0.96 + 0.08 * rumore(w, h, 3, rng, 1))[..., None]
    cera = np.dstack([np.clip(rgb, 0, 255), g * 255]).astype(np.uint8)
    # l'ombra sotto la cera
    ombra = Image.new("RGBA", (w, h), (40, 10, 8, 0))
    ombra.putalpha(goccia.filter(ImageFilter.GaussianBlur(5)).point(lambda v: int(v * 0.5)))
    img = Image.alpha_composite(img, Image.fromarray(np.roll(np.roll(np.asarray(ombra), 4, 0), 3, 1), "RGBA"))
    img = Image.alpha_composite(img, Image.fromarray(cera, "RGBA"))
    img.save(os.path.join(uscita, nome_file))


def ceralacca(uscita):
    # (13/11, Fra: "il timbro della capitana mi piaceva, fallo in cera con un suo segno") il sigillo della capitana Livia
    # Serra, al posto del timbro d'inchiostro: la serra e la stella, e in giro la sua capitaneria
    sigillo(uscita, "T_Ceralacca.png", "serra", "L·SERRA·CAPITANERIA·DEL·PASSO·", 47)


if __name__ == "__main__":
    uscita = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(uscita, exist_ok=True)
    carta(uscita)
    ceralacca(uscita)
    print("tessera: T_Tessera_Carta.png e T_Ceralacca.png in", uscita)
