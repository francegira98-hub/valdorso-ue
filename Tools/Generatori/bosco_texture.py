# Valdorso - Le texture del bosco, fatte da zero (v0.1.4, 09/11/2026, chiesto da Fra). Scritto da Claude.
# Due tipi:
#   le cortecce, che si ripetono senza cuciture (1024 x 1024, colore T_<Nome>.png e rilievo T_<Nome>_N.png, come
#   quelle del villaggio): Abete (rosso-grigia a scaglie), Faggio (grigio liscio con i licheni), Betulla (bianca con
#   i segni neri), Melo (bruna screpolata), Anelli (la faccia tagliata dei ceppi e dei tronchi, non si ripete);
#   le fronde, disegnate su una carta trasparente (RGBA, l'alfa e' la forma): RamoAbete e RamoLarice (un ramo visto
#   dall'alto, dal tronco a sinistra alla punta a destra, 1024 x 512), FoglieFaggio, FoglieBetulla, FoglieMelo (con
#   le mele) e FoglieNocciolo (un ciuffo di foglie sul suo rametto, attaccato in basso al centro, 1024 x 1024).
# Le fronde si disegnano grandi il doppio e si rimpiccioliscono (bordi morbidi); il colore sotto le parti trasparenti
# si allarga dai bordi, cosi' da lontano (mipmap) non vengono gli aloni neri.
# Le usa bosco.py; non servono licenze.

import math
import os

import numpy as np
from PIL import Image, ImageDraw

import villaggio_texture as vt

N = vt.N


# ------------------------------------------------------------------------------------------------ cortecce

def _stirato(rng, scala_x, scala_y, esponente=2.0):
    """Rumore periodico allungato: macchie larghe scala_x e alte scala_y pixel (le venature della corteccia)."""
    f = np.fft.fftfreq(N)
    fx, fy = np.meshgrid(f, f)
    r = np.sqrt((fx * scala_x) ** 2 + (fy * scala_y) ** 2)
    filtro = 1.0 / (1.0 + r ** (2 * esponente))
    filtro[0, 0] = 0
    v = np.real(np.fft.ifft2(np.fft.fft2(rng.standard_normal((N, N))) * filtro))
    v -= v.mean()
    return v / (v.std() + 1e-9)


def _solchi(rng, scala_x, scala_y, base, scuro, placche=0.0):
    """Corteccia a placche: zone larghe in rilievo (le placche, chiare) separate da solchi scuri, allungate in su."""
    n = _stirato(rng, scala_x, scala_y) * 0.8 + _stirato(rng, scala_x * 0.4, scala_y * 0.4) * 0.35
    alt = np.clip((n + 0.35) * 1.6, 0, 1) ** 0.7
    # rotture orizzontali delle placche (le scaglie che si staccano)
    rotte = np.clip(_stirato(rng, scala_y * 0.5, scala_x * 0.6) - 1.8, 0, 1) * placche * 0.6
    alt = np.clip(alt - rotte, 0, 1)
    fine = vt.rumore(rng, 24)
    alt = np.clip(alt + fine * 0.05, 0, 1)
    col = np.array(scuro) * (1 - alt[..., None]) + np.array(base) * alt[..., None]
    col *= (1 + 0.12 * vt.morbido(rng, 5)[..., None] + 0.07 * fine[..., None])
    return col, alt


def abete(rng):
    """Corteccia dell'abete rosso: scaglie sottili rosso-grigie, solchi poco profondi, qualche lichene (1 m)."""
    col, alt = _solchi(rng, 40, 140, (0.47, 0.37, 0.31), (0.15, 0.11, 0.09), placche=0.9)
    grigio = np.clip(vt.morbido(rng, 7) * 0.5 + 0.3, 0, 1)
    col = col * (1 - 0.35 * grigio[..., None]) + np.array([0.36, 0.34, 0.31]) * alt[..., None] * 0.35 * grigio[..., None]
    licheni = np.clip(vt.morbido(rng, 8) * 0.6 - 1.0, 0, 1) * np.clip(alt, 0, 1)
    col = col * (1 - licheni[..., None]) + np.array([0.46, 0.50, 0.40]) * licheni[..., None]
    return col, alt, 5.0


def faggio(rng):
    """Corteccia del faggio: grigio argento quasi liscia, anelli orizzontali leggeri, macchie di licheni (1 m)."""
    fine = vt.rumore(rng, 32)
    righe = np.sin(np.linspace(0, 2 * np.pi * 9, N, endpoint=False))[:, None] * np.ones((1, N))
    alt = 0.5 + fine * 0.06 + righe * 0.03
    col = np.array([0.50, 0.49, 0.46]) * (1 + 0.05 * fine[..., None] + 0.08 * vt.morbido(rng, 4)[..., None])
    for colore_lichene, soglia in (((0.50, 0.54, 0.44), 1.0), ((0.62, 0.62, 0.58), 1.3), ((0.25, 0.27, 0.22), 1.3)):
        m = np.clip(vt.morbido(rng, 9) * 0.7 - soglia, 0, 1) ** 0.7
        col = col * (1 - m[..., None]) + np.array(colore_lichene) * m[..., None]
        alt += m * 0.05
    # gli "occhi" dei rami caduti: archi scuri
    yy, xx = np.mgrid[0:N, 0:N]
    for _ in range(2):
        cx, cy = rng.uniform(0, N, 2)
        dx = (xx - cx + N / 2) % N - N / 2
        dy = (yy - cy + N / 2) % N - N / 2
        d = np.sqrt((dx / 1.8) ** 2 + dy ** 2)
        arco = (np.abs(d - 22) < 2) & (dy < 4)
        col[arco] *= 0.6
        alt[arco] -= 0.1
    return col, alt, 2.0


def betulla(rng):
    """Corteccia della betulla: bianca, i segni neri orizzontali (lenticelle), macchie scure ruvide (1 m)."""
    fine = vt.rumore(rng, 40)
    col = np.array([0.86, 0.85, 0.81]) * (1 + 0.04 * fine[..., None])
    alt = 0.6 + fine * 0.03
    yy, xx = np.mgrid[0:N, 0:N]
    for _ in range(420):
        cx, cy = rng.uniform(0, N, 2)
        lung = rng.uniform(20, 90)
        alto = rng.uniform(2, 6)
        dx = (xx - cx + N / 2) % N - N / 2
        dy = (yy - cy + N / 2) % N - N / 2
        m = (np.abs(dx) < lung / 2) & (np.abs(dy) < alto * (1 - (2 * dx / lung) ** 2).clip(0, 1) ** 0.5)
        col[m] = np.array([0.12, 0.11, 0.10]) * rng.uniform(0.8, 1.6)
        alt[m] = 0.35
    # le chiazze scure e screpolate
    chiazze = np.clip(vt.morbido(rng, 5) * 0.8 - 0.75, 0, 1) ** 0.6
    ruvido = np.abs(vt.rumore(rng, 16))
    scuro = np.array([0.18, 0.16, 0.14]) * (0.8 + 0.4 * ruvido[..., None])
    col = col * (1 - chiazze[..., None]) + scuro * chiazze[..., None]
    alt = alt * (1 - chiazze) + (0.3 + ruvido * 0.3) * chiazze
    # una velatura calda (la betulla non e' bianca di gesso)
    col *= np.array([1.0, 0.98, 0.94])
    return col, alt, 2.5


def melo(rng):
    """Corteccia del melo e del nocciolo: bruna, a placchette strette che si sfogliano (1 m)."""
    col, alt = _solchi(rng, 24, 60, (0.46, 0.38, 0.30), (0.13, 0.10, 0.08), placche=1.4)
    return col, alt, 4.0


def anelli(rng):
    """La faccia tagliata di un tronco: gli anelli, la corteccia intorno, le crepe dal centro (non si ripete)."""
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    dx, dy = (xx - N / 2) / (N / 2), (yy - N / 2) / (N / 2)
    ang = np.arctan2(dy, dx)
    r = np.sqrt(dx * dx + dy * dy) * (1 + 0.04 * np.sin(ang * 3 + 1) + 0.02 * np.sin(ang * 7))
    fine = vt.rumore(rng, 64)
    anello = 0.5 + 0.5 * np.sin(r * 2 * np.pi * 34 + fine * 0.6)
    legno = np.array([0.66, 0.50, 0.33]) * (0.82 + 0.18 * anello[..., None]) * (1 - 0.25 * r[..., None] ** 2)
    alt = 0.5 + anello * 0.08
    crepe = np.zeros((N, N), bool)
    for _ in range(5):
        a = rng.uniform(-np.pi, np.pi)
        d = np.abs(((ang - a + np.pi) % (2 * np.pi)) - np.pi)
        crepe |= (d < 0.012 + 0.02 * (1 - r)) & (r > rng.uniform(0.05, 0.3)) & (r < rng.uniform(0.6, 0.92))
    legno[crepe] = np.array([0.18, 0.12, 0.08])
    alt[crepe] = 0.2
    corteccia = r > 0.92
    legno[corteccia] = np.array([0.30, 0.22, 0.16]) * (0.8 + 0.4 * np.abs(fine[corteccia, None]))
    alt[corteccia] = 0.7 + fine[corteccia] * 0.1
    return legno, alt, 2.0


CORTECCE = {"Abete": abete, "Faggio": faggio, "Betulla": betulla, "Melo": melo, "Anelli": anelli}


# ------------------------------------------------------------------------------------------------ fronde

def _hex(c):
    return tuple(int(max(0, min(255, round(v)))) for v in c)


def _varia(rng, base, quanto=0.12):
    f = rng.uniform(1 - quanto, 1 + quanto)
    return _hex([b * f * rng.uniform(0.96, 1.04) for b in base])


def _foglia(cx, cy, lung, larg, gradi, punta=1.0, base_tonda=0.5, lati=18):
    """Il contorno di una foglia ovale appuntita, dal picciolo (cx, cy) verso 'gradi'."""
    a = math.radians(gradi)
    u = (math.cos(a), math.sin(a))
    v = (-u[1], u[0])
    sx, dx = [], []
    for i in range(lati + 1):
        s = i / lati
        w = larg * 0.5 * (math.sin(math.pi * s ** (0.8 + 0.4 * base_tonda)) ** (0.7 + 0.5 * punta))
        p = (cx + u[0] * s * lung, cy + u[1] * s * lung)
        sx.append((p[0] + v[0] * w, p[1] + v[1] * w))
        dx.append((p[0] - v[0] * w, p[1] - v[1] * w))
    return sx + dx[::-1]


def _chiudi(img):
    """Da 2x a 1x con l'alfa giusto, poi il colore allargato sotto le parti trasparenti (niente aloni nei mipmap)."""
    a = np.asarray(img, np.float32) / 255.0
    h, w = a.shape[0] // 2, a.shape[1] // 2
    a = a.reshape(h, 2, w, 2, 4).mean((1, 3))
    alfa = a[..., 3]
    rgb = a[..., :3] / np.maximum(alfa[..., None], 1e-4)
    pieno = alfa > 0.02
    colore = np.where(pieno[..., None], rgb, 0)
    peso = pieno.astype(np.float32)
    for _ in range(64):
        if peso.min() > 0:
            break
        somma = sum(np.roll(np.roll(colore * peso[..., None], dy, 0), dx, 1) for dy in (-1, 0, 1) for dx in (-1, 0, 1))
        conta = sum(np.roll(np.roll(peso, dy, 0), dx, 1) for dy in (-1, 0, 1) for dx in (-1, 0, 1))
        nuovo = (peso == 0) & (conta > 0)
        colore[nuovo] = somma[nuovo] / conta[nuovo, None]
        peso = np.where(nuovo, 1.0, peso)
    colore[peso == 0] = colore[peso > 0].mean(0) if (peso > 0).any() else 0.3
    out = np.dstack([np.clip(colore, 0, 1), alfa])
    return Image.fromarray((out * 255 + 0.5).astype(np.uint8), "RGBA")


def _aghi_lungo(d, rng, punti, lung, colori, spessore, avanti=55.0, fitti=6.0, giovani=None):
    """Aghi ai due lati di una linea (lista di punti): ogni 'fitti' px due aghi inclinati verso la punta."""
    for i in range(len(punti) - 1):
        (x0, y0), (x1, y1) = punti[i], punti[i + 1]
        seg = math.hypot(x1 - x0, y1 - y0)
        if seg < 1e-3:
            continue
        a = math.atan2(y1 - y0, x1 - x0)
        passi = max(1, int(seg / fitti))
        for k in range(passi):
            t = (k + rng.random()) / passi
            px, py = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            fine_ramo = (i + t) / max(1, len(punti) - 1)
            for lato in (-1, 1):
                ang = a + lato * math.radians(avanti + rng.uniform(-15, 15))
                l = lung * rng.uniform(0.75, 1.15) * (0.75 + 0.25 * (1 - fine_ramo))
                c = colori[rng.integers(len(colori))]
                if giovani is not None and fine_ramo > 0.82 and rng.random() < 0.7:
                    c = giovani[rng.integers(len(giovani))]
                d.line([(px, py), (px + math.cos(ang) * l, py + math.sin(ang) * l)], fill=c + (255,), width=spessore)


def _curva(x0, y0, lung, gradi, piega, passi=10):
    """Una linea che parte da (x0, y0) e si piega a poco a poco (piega in gradi in tutto)."""
    pts = [(x0, y0)]
    a = math.radians(gradi)
    for i in range(passi):
        a += math.radians(piega) / passi
        x0 += math.cos(a) * lung / passi
        y0 += math.sin(a) * lung / passi
        pts.append((x0, y0))
    return pts


def ramo_abete(rng):
    """Un ramo d'abete rosso visto dall'alto: il ramo, i rametti alterni, gli aghi fitti scuri, le punte piu' chiare."""
    W, H = 2048, 1024
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    scuri = [_hex(c) for c in ((34, 56, 32), (40, 64, 36), (30, 50, 30), (46, 70, 40), (38, 60, 42))]
    giovani = [_hex(c) for c in ((78, 112, 52), (88, 120, 56), (70, 100, 48))]
    legno = (72, 54, 38, 255)
    stelo = _curva(0, H / 2, W * 0.97, rng.uniform(-3, 3), rng.uniform(-8, 8), 16)
    rametti = []
    t = 0.10
    lato = 1
    while t < 0.93:
        i = int(t * (len(stelo) - 1))
        x, y = stelo[i]
        lung = (H * 0.46) * (1 - t) ** 0.55 + 70
        ang = lato * rng.uniform(38, 58)
        pts = _curva(x, y, lung, ang, -lato * rng.uniform(8, 22), 8)
        rametti.append(pts)
        # sotto-rametti
        for s in (0.35, 0.65):
            j = int(s * (len(pts) - 1))
            sx, sy = pts[j]
            for lato2 in (-1, 1):
                if rng.random() < 0.75:
                    rametti.append(_curva(sx, sy, lung * rng.uniform(0.25, 0.4), ang + lato2 * rng.uniform(35, 55), 0, 4))
        lato = -lato
        t += rng.uniform(0.045, 0.075)
    # prima gli aghi di sotto (piu' scuri), poi i legni, poi gli aghi sopra
    ombra = [_hex([c * 0.7 for c in s]) for s in scuri]
    for pts in rametti:
        _aghi_lungo(d, rng, pts, 30, ombra, 4, avanti=70, fitti=9)
    _aghi_lungo(d, rng, stelo, 36, ombra, 5, avanti=70, fitti=8)
    d.line(stelo, fill=legno, width=12)
    for pts in rametti:
        d.line(pts, fill=legno, width=5)
    for pts in rametti:
        _aghi_lungo(d, rng, pts, 26, scuri, 3, avanti=50, fitti=5, giovani=giovani)
    _aghi_lungo(d, rng, stelo, 30, scuri, 4, avanti=55, fitti=5, giovani=giovani)
    return _chiudi(img)


def ramo_larice(rng):
    """Un ramo di larice: rametti sottili che pendono, con i ciuffi di aghi morbidi verde chiaro."""
    W, H = 2048, 1024
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    verdi = [_hex(c) for c in ((96, 132, 58), (110, 146, 62), (84, 120, 52), (120, 152, 70))]
    legno = (92, 70, 50, 255)
    stelo = _curva(0, H / 2, W * 0.96, rng.uniform(-3, 3), rng.uniform(-6, 6), 14)
    rametti = [stelo]
    t, lato = 0.12, 1
    while t < 0.9:
        x, y = stelo[int(t * (len(stelo) - 1))]
        lung = H * 0.42 * (1 - t) ** 0.5 + 60
        rametti.append(_curva(x, y, lung, lato * rng.uniform(45, 70), -lato * rng.uniform(10, 25), 8))
        lato = -lato
        t += rng.uniform(0.06, 0.09)
    for pts in rametti:
        d.line(pts, fill=legno, width=6 if pts is stelo else 3)
    for pts in rametti:
        for i in range(1, len(pts)):
            x, y = pts[i]
            for _ in range(2):
                cx, cy = x + rng.uniform(-14, 14), y + rng.uniform(-14, 14)
                n = rng.integers(14, 24)
                for k in range(n):
                    a = 2 * math.pi * k / n + rng.uniform(-0.2, 0.2)
                    l = rng.uniform(22, 40)
                    c = verdi[rng.integers(len(verdi))]
                    d.line([(cx, cy), (cx + math.cos(a) * l, cy + math.sin(a) * l)], fill=c + (255,), width=3)
    return _chiudi(img)


def _ciuffo(rng, colori, lung, larg_rel, quante, punta=1.0, base_tonda=0.5, mele=0, pendono=False, rametti_colore=(70, 56, 40)):
    """Un ciuffo di foglie sui suoi rametti, attaccato in basso al centro (dove la carta tocca il ramo)."""
    W = H = 2048
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    rametti = []
    radice = (W / 2, H * 0.97)
    for k in range(5):
        ang = -90 + rng.uniform(-40, 40)
        pts = _curva(radice[0], radice[1], H * rng.uniform(0.5, 0.75), ang, rng.uniform(-30, 30) + (60 if pendono else 0), 10)
        rametti.append(pts)
        for s in (0.4, 0.7):
            x, y = pts[int(s * (len(pts) - 1))]
            rametti.append(_curva(x, y, H * rng.uniform(0.15, 0.3), ang + rng.choice([-1, 1]) * rng.uniform(30, 60), 25 if pendono else 0, 6))
    for pts in rametti:
        d.line(pts, fill=_hex(rametti_colore) + (255,), width=7)
    foglie = []
    # le foglie riempiono un ovale (la chioma del ciuffo), girate verso fuori dal punto d'attacco
    cx, cy, rx, ry = W / 2, H * 0.47, W * 0.45, H * 0.44
    while len(foglie) < quante:
        x, y = rng.uniform(cx - rx, cx + rx), rng.uniform(cy - ry, cy + ry)
        q = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
        if q > 1 or rng.random() < q ** 3:
            continue
        verso = math.degrees(math.atan2(y - radice[1], x - radice[0]))
        gradi = verso + rng.uniform(-70, 70)
        if pendono:
            gradi = 90 + rng.uniform(-50, 50)
        l = lung * rng.uniform(0.75, 1.2)
        # le foglie di fuori stanno sopra e prendono piu' luce; quelle di dentro sono in ombra
        foglie.append((x, y, l, l * larg_rel * rng.uniform(0.85, 1.1), gradi, min(1.0, rng.random() * 0.6 + q * 0.6)))
    foglie.sort(key=lambda f: f[5])
    for x, y, l, w, g, prof in foglie:
        luce = 0.62 + 0.45 * prof
        base = colori[rng.integers(len(colori))]
        c = _hex([v * luce for v in base])
        contorno = _foglia(x, y, l, w, g, punta, base_tonda)
        d.polygon(contorno, fill=c + (255,))
        # la nervatura e meta' foglia appena piu' chiara (la luce)
        a = math.radians(g)
        fine = (x + math.cos(a) * l * 0.92, y + math.sin(a) * l * 0.92)
        meta = contorno[: len(contorno) // 2] + [fine, (x, y)]
        d.polygon(meta, fill=_hex([v * luce * 1.09 for v in base]) + (255,))
        d.line([(x, y), fine], fill=_hex([v * luce * 0.75 for v in base]) + (255,), width=3)
        for s in (0.3, 0.5, 0.7):
            px, py = x + math.cos(a) * l * s, y + math.sin(a) * l * s
            for lato in (-1, 1):
                b = a + lato * math.radians(50)
                d.line([(px, py), (px + math.cos(b) * w * 0.35, py + math.sin(b) * w * 0.35)],
                       fill=_hex([v * luce * 0.85 for v in base]) + (255,), width=2)
    for _ in range(mele):
        pts = rametti[rng.integers(len(rametti))]
        x, y = pts[rng.integers(3, len(pts))]
        r = rng.uniform(42, 56)
        rosso = rng.random() < 0.7
        base = (170, 38, 30) if rosso else (190, 160, 60)
        d.line([(x, y - r * 1.4), (x, y - r * 0.6)], fill=(60, 45, 30, 255), width=5)
        for k in range(12, 0, -1):
            f = k / 12
            c = _hex([v * (0.55 + 0.6 * (1 - f)) for v in base])
            d.ellipse([x - r * f - (1 - f) * r * 0.25, y - r * f - (1 - f) * r * 0.3, x + r * f - (1 - f) * r * 0.25, y + r * f - (1 - f) * r * 0.3], fill=c + (255,))
        d.ellipse([x - r * 0.45, y - r * 0.55, x - r * 0.15, y - r * 0.3], fill=(235, 210, 170, 255))
    return _chiudi(img)


def foglie_faggio(rng):
    verdi = [(70, 108, 38), (82, 118, 42), (62, 96, 34), (96, 128, 48), (76, 112, 44)]
    return _ciuffo(rng, verdi, 150, 0.58, 520, punta=0.9, base_tonda=0.6)


def foglie_betulla(rng):
    verdi = [(112, 146, 56), (124, 156, 62), (100, 134, 50), (136, 160, 70)]
    return _ciuffo(rng, verdi, 105, 0.7, 620, punta=1.3, base_tonda=0.2, pendono=True, rametti_colore=(60, 44, 34))


def foglie_melo(rng):
    verdi = [(66, 104, 40), (78, 114, 46), (58, 92, 36), (88, 120, 50)]
    return _ciuffo(rng, verdi, 125, 0.55, 480, punta=0.8, base_tonda=0.5, mele=7)


def foglie_nocciolo(rng):
    verdi = [(72, 110, 40), (86, 122, 46), (64, 98, 36), (98, 130, 52)]
    return _ciuffo(rng, verdi, 165, 0.85, 380, punta=0.6, base_tonda=0.9)


FRONDE = {"RamoAbete": ramo_abete, "RamoLarice": ramo_larice, "FoglieFaggio": foglie_faggio,
          "FoglieBetulla": foglie_betulla, "FoglieMelo": foglie_melo, "FoglieNocciolo": foglie_nocciolo}


def scrivi_tutte(cartella, seme=909):
    os.makedirs(cartella, exist_ok=True)
    for i, (nome, funzione) in enumerate(CORTECCE.items()):
        rng = np.random.default_rng(seme + i)
        col, alt, forza = funzione(rng)
        Image.fromarray(vt.colore(col), "RGB").save(os.path.join(cartella, "T_Corteccia%s.png" % nome))
        alt = (alt - alt.min()) / (alt.max() - alt.min() + 1e-9) * 40.0
        Image.fromarray(vt.normale(alt, forza / 10.0), "RGB").save(os.path.join(cartella, "T_Corteccia%s_N.png" % nome))
        print("texture corteccia", nome)
    for i, (nome, funzione) in enumerate(FRONDE.items()):
        rng = np.random.default_rng(seme + 100 + i)
        funzione(rng).save(os.path.join(cartella, "T_%s.png" % nome))
        print("texture fronda", nome)


if __name__ == "__main__":
    import sys
    scrivi_tutte(sys.argv[1] if len(sys.argv) > 1 else "uscita_bosco")
