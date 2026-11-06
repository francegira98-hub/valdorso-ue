# Valdorso - Le texture del villaggio, fatte da zero (v0.1.3). Scritto da Claude il 05/11/2026.
# Texture che si ripetono senza cuciture (tutto il rumore è periodico), 1024 x 1024: colore (T_<Nome>.png) e rilievo
# (T_<Nome>_N.png, normal map alla DirectX come la vuole Unreal). Le usa villaggio.py; non servono licenze.
#   Pietra    muratura di pietre grezze a corsi, con la malta (2 m)
#   Intonaco  calce bianco sporco con macchie e qualche crepa (2 m)
#   Assi      assi verticali di legno con le venature e i chiodi (2 m)
#   Trave     legno scuro stagionato, venatura lungo U (1 m)
#   Coppi     tegole di cotto a file (2 m; V scende lungo la falda)
#   Paglia    tetto di paglia a strati (2 m; V scende lungo la falda)
#   Ciottoli  selciato di ciottoli (2 m)
#   Terra     terra battuta con sassolini (4 m); nell'alfa un rumore per i bordi sfrangiati delle strade
#   Roccia    (06/11, sera) roccia viva della grotta e delle rupi: grigio caldo a strati, fratture, licheni (4 m)

import os

import numpy as np
from PIL import Image

N = 1024


def rumore(rng, scala, n=N, esponente=2.0):
    """Rumore periodico 'a nuvole': rumore bianco filtrato nelle frequenze (1/f^esponente sopra la scala)."""
    f = np.fft.fftfreq(n)
    fx, fy = np.meshgrid(f, f)
    r = np.sqrt(fx * fx + fy * fy)
    r[0, 0] = 1.0
    filtro = 1.0 / (r ** esponente)
    filtro[r < 1.0 / scala] = 0.0 if scala < n else filtro[r < 1.0 / scala]
    filtro[0, 0] = 0.0
    spettro = np.fft.fft2(rng.standard_normal((n, n))) * filtro
    v = np.real(np.fft.ifft2(spettro))
    v -= v.mean()
    return v / (v.std() + 1e-9)


def morbido(rng, celle, n=N):
    """Rumore liscio periodico con circa 'celle' macchie per lato."""
    f = np.fft.fftfreq(n)
    fx, fy = np.meshgrid(f, f)
    r = np.sqrt(fx * fx + fy * fy)
    filtro = np.exp(-(r * n / celle) ** 2)
    filtro[0, 0] = 0
    v = np.real(np.fft.ifft2(np.fft.fft2(rng.standard_normal((n, n))) * filtro))
    v -= v.mean()
    return v / (v.std() + 1e-9)


def sfoca(a, raggio):
    f = np.fft.fftfreq(a.shape[0])
    fx, fy = np.meshgrid(f, np.fft.fftfreq(a.shape[0]))
    g = np.exp(-2 * (np.pi ** 2) * (raggio ** 2) * (fx * fx + fy * fy))
    return np.real(np.fft.ifft2(np.fft.fft2(a) * g))


def normale(altezza, forza):
    """Normal map DirectX (verde = giù) da una mappa di altezze periodica (in pixel)."""
    dx = (np.roll(altezza, -1, 1) - np.roll(altezza, 1, 1)) * 0.5 * forza
    dy = (np.roll(altezza, -1, 0) - np.roll(altezza, 1, 0)) * 0.5 * forza
    nx, ny, nz = -dx, dy, np.ones_like(dx)
    # DirectX: il verde è -dh/dv (v verso il basso nell'immagine) -> ny = +dy con questo segno, poi si inverte il verde
    ny = -ny
    l = np.sqrt(nx * nx + ny * ny + nz * nz)
    rgb = np.stack([nx / l, ny / l, nz / l], -1) * 0.5 + 0.5
    return (np.clip(rgb, 0, 1) * 255 + 0.5).astype(np.uint8)


def colore(rgb):
    return (np.clip(rgb, 0, 1) * 255 + 0.5).astype(np.uint8)


def celle_voronoi(rng, punti, n=N, stira=(1.0, 1.0)):
    """Voronoi periodico: per ogni pixel l'indice della cella, la distanza dal bordo (in pixel) e dal centro."""
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    d1 = np.full((n, n), 1e9, np.float32)
    d2 = np.full((n, n), 1e9, np.float32)
    idx = np.zeros((n, n), np.int32)
    for i, (px, py) in enumerate(punti):
        dx = np.abs(xx - px)
        dx = np.minimum(dx, n - dx) * stira[0]
        dy = np.abs(yy - py)
        dy = np.minimum(dy, n - dy) * stira[1]
        d = np.sqrt(dx * dx + dy * dy)
        nuovo = d < d1
        d2 = np.where(nuovo, d1, np.minimum(d2, d))
        idx = np.where(nuovo, i, idx)
        d1 = np.where(nuovo, d, d1)
    return idx, (d2 - d1) * 0.5, d1


# ------------------------------------------------------------------------------------------------ le texture

def pietra(rng):
    """Pietre squadrate a corsi irregolari (2 m: una pietra è 20-50 cm), malta chiara rientrata."""
    alt = np.zeros((N, N), np.float32)
    col = np.zeros((N, N, 3), np.float32)
    malta = np.ones((N, N), bool)
    y = 0
    file = []
    while y < N:
        h = int(rng.uniform(70, 130))
        if N - y - h < 60:
            h = N - y
        file.append((y, h))
        y += h
    yy, xx = np.mgrid[0:N, 0:N]
    base = np.array([0.55, 0.52, 0.47])
    fine = rumore(rng, 64) * 0.5 + rumore(rng, 8) * 0.5
    for (y0, h) in file:
        x = rng.uniform(0, N)
        inizio = x
        while x < inizio + N:
            w = rng.uniform(110, 260)
            if inizio + N - (x + w) < 80:
                w = inizio + N - x
            # la pietra: rettangolo arrotondato con i bordi un po' irregolari, giunti di 6-10 px
            g = rng.uniform(5, 9)
            cx0, cx1 = x + g, x + w - g
            cy0, cy1 = y0 + g, y0 + h - g
            px = (xx - cx0) % N
            dentro_x = px < (cx1 - cx0)
            dentro_y = (yy >= cy0) & (yy < cy1)
            m = dentro_x & dentro_y
            if m.any():
                ux = px[m] / max(1.0, cx1 - cx0)
                uy = (yy[m] - cy0) / max(1.0, cy1 - cy0)
                bordo = np.minimum(np.minimum(ux, 1 - ux) * (cx1 - cx0), np.minimum(uy, 1 - uy) * (cy1 - cy0))
                bombato = np.clip(bordo / 22.0, 0, 1) ** 0.6
                alt[m] = 0.55 + 0.45 * bombato + rng.uniform(-0.1, 0.1)
                tinta = base * rng.uniform(0.8, 1.15) + rng.normal(0, 0.02) + rng.normal(0, 0.008, 3)
                if rng.random() < 0.15:
                    tinta = tinta * np.array([1.08, 1.0, 0.9])     # qualche pietra più calda
                col[m] = tinta
                malta[m] = False
            x += w
    alt += fine * 0.06
    alt[malta] = 0.15 + fine[malta] * 0.03
    col[malta] = np.array([0.66, 0.63, 0.57]) * (1 + fine[malta, None] * 0.05)
    macchie = morbido(rng, 5)
    col *= (1 + 0.10 * macchie[..., None] + 0.06 * fine[..., None])
    # un po' di muschio negli incavi
    musc = np.clip(morbido(rng, 7) * 0.5 - 0.7, 0, 1) * (1 - alt) ** 2
    col = col * (1 - musc[..., None]) + np.array([0.30, 0.36, 0.20]) * musc[..., None]
    return col, alt, 3.0


def intonaco(rng):
    """Calce: bianco sporco caldo, macchie larghe, grana fine, qualche crepa e qualche pietra che spunta."""
    grana = rumore(rng, 16, esponente=1.6)
    macchie = morbido(rng, 4)
    macchie2 = morbido(rng, 12)
    base = np.array([0.80, 0.76, 0.66])
    col = base[None, None, :] * (1 + 0.07 * macchie[..., None] + 0.04 * macchie2[..., None] + 0.025 * grana[..., None])
    col *= np.array([1.0, 0.99, 0.97]) + 0.03 * macchie2[..., None] * np.array([0.0, -0.3, -0.8])
    alt = 0.5 + 0.12 * grana + 0.05 * macchie2
    # crepe: linee spezzate sottili
    for _ in range(9):
        x, y = rng.uniform(0, N, 2)
        ang = rng.uniform(0, np.pi)
        for _ in range(int(rng.uniform(20, 70))):
            ang += rng.normal(0, 0.35)
            for _ in range(4):
                xi, yi = int(x) % N, int(y) % N
                col[yi, xi] *= 0.72
                alt[yi, xi] -= 0.25
                x += np.cos(ang)
                y += np.sin(ang)
    # dove l'intonaco è caduto: pietra sotto
    caduto = (morbido(rng, 9) + 0.4 * rumore(rng, 64) > 2.6)
    pietre = morbido(rng, 60)
    col[caduto] = np.array([0.52, 0.49, 0.44])[None, :] * (1 + 0.15 * pietre[caduto, None])
    alt[caduto] = 0.2 + 0.15 * pietre[caduto]
    return col, alt, 2.0


def assi(rng, scuro=False):
    """Assi verticali larghe 16-26 cm (2 m = 1024 px), con venature, nodi, fessure e due chiodi per trave."""
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    col = np.zeros((N, N, 3), np.float32)
    alt = np.zeros((N, N), np.float32)
    x = 0
    larghezze = []
    while x < N:
        w = int(rng.uniform(85, 130))
        if N - x - w < 70:
            w = N - x
        larghezze.append((x, w))
        x += w
    base = np.array([0.42, 0.31, 0.20]) if not scuro else np.array([0.27, 0.20, 0.14])
    for (x0, w) in larghezze:
        m = (xx >= x0) & (xx < x0 + w)
        u = (xx[m] - x0) / w
        fase = rng.uniform(0, 100)
        onda = np.sin((yy[m] * 0.012 + np.sin(xx[m] * 0.05 + fase) * 1.5) + fase)
        ven = np.sin(xx[m] * rng.uniform(0.35, 0.6) + onda * 3.0 + fase)
        tinta = base * rng.uniform(0.8, 1.2)
        col[m] = tinta * (0.88 + 0.10 * ven[:, None] + 0.04 * onda[:, None])
        bordo = np.minimum(u, 1 - u) * w
        alt[m] = 0.6 + 0.3 * np.clip(bordo / 8.0, 0, 1) + 0.04 * ven
        # nodo
        if rng.random() < 0.5:
            ny, nxx = rng.uniform(0, N), x0 + w * rng.uniform(0.3, 0.7)
            dy = np.minimum(np.abs(yy[m] - ny), N - np.abs(yy[m] - ny))
            d = np.sqrt(((xx[m] - nxx) * 1.6) ** 2 + dy ** 2)
            k = np.clip(1 - d / 18.0, 0, 1)
            col[m] *= (1 - 0.45 * k)[:, None]
        # chiodi a due altezze (le traverse dietro)
        for cy in (N * 0.18, N * 0.68):
            d = np.sqrt((xx[m] - (x0 + w / 2)) ** 2 + (yy[m] - cy) ** 2)
            k = d < 5
            col[m] = np.where(k[:, None], np.array([0.12, 0.11, 0.10]), col[m])
        # fessura tra le assi
        alt[(xx >= x0) & (xx < x0 + 3)] = 0.05
        col[(xx >= x0) & (xx < x0 + 3)] *= 0.25
    grana = rumore(rng, 32)
    col *= (1 + 0.05 * grana[..., None] + 0.07 * morbido(rng, 6)[..., None])
    return col, alt, 3.0


def trave(rng):
    """Legno di trave scuro: venatura lungo U (orizzontale nell'immagine), crepe da stagionatura."""
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    onda = np.sin(xx * 2 * np.pi / N * 2 + morbido(rng, 3) * 1.5)
    ven = np.sin(yy * 0.25 + onda * 4 + morbido(rng, 8) * 2.0)
    base = np.array([0.30, 0.22, 0.15])
    col = base * (0.85 + 0.10 * ven[..., None] + 0.08 * morbido(rng, 5)[..., None])
    alt = 0.6 + 0.05 * ven
    for _ in range(14):
        y = rng.uniform(0, N)
        x = rng.uniform(0, N)
        lung = rng.uniform(80, 300)
        for t in range(int(lung)):
            xi, yi = int(x + t) % N, int(y + 3 * np.sin(t * 0.05)) % N
            col[yi, xi] *= 0.4
            alt[yi, xi] = 0.2
    col *= (1 + 0.04 * rumore(rng, 32)[..., None])
    return col, alt, 2.5


def coppi(rng):
    """Coppi di cotto: file orizzontali (lungo U) di tegole curve; V scende lungo la falda. 2 m: 8 file, 12 colonne."""
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    righe, colonne = 8, 12
    ph = N / righe
    pw = N / colonne
    col = np.zeros((N, N, 3), np.float32)
    alt = np.zeros((N, N), np.float32)
    riga = np.floor(yy / ph).astype(int)
    sfasa = (riga % 2) * 0.5
    u = (xx / pw + sfasa) % 1.0
    v = (yy % ph) / ph
    colonna = np.floor(xx / pw + sfasa).astype(int) % colonne
    curva = np.sin(u * np.pi)                    # la tegola è curva
    # la parte bassa della tegola copre quella sotto: verso il basso (v -> 1) si alza e finisce con un bordo
    alt = 0.35 + 0.45 * curva + 0.25 * v
    gola = (u < 0.06) | (u > 0.94)
    alt[gola] = 0.1
    tinte = rng.uniform(0.75, 1.2, (righe, colonne))
    caldo = rng.uniform(-0.05, 0.05, (righe, colonne))
    t = tinte[riga % righe, colonna]
    c = caldo[riga % righe, colonna]
    base = np.stack([0.62 + c, 0.33 + c * 0.5, 0.22 + 0 * c], -1)
    col = base * t[..., None] * (0.80 + 0.25 * curva[..., None]) * (0.85 + 0.15 * (1 - v[..., None]))
    col[gola] *= 0.35
    # bordo inferiore in ombra (la tegola sopra)
    ombra = np.clip((v - 0.9) / 0.1, 0, 1)
    col *= (1 - 0.5 * ombra[..., None])
    licheni = np.clip(morbido(rng, 10) - 1.2, 0, 1)
    col = col * (1 - 0.6 * licheni[..., None]) + np.array([0.45, 0.45, 0.32]) * 0.6 * licheni[..., None]
    sporco = morbido(rng, 4)
    col *= (1 + 0.08 * sporco[..., None] + 0.05 * rumore(rng, 32)[..., None])
    return col, alt, 4.0


def paglia(rng):
    """Paglia a strati: fili lungo V, file sovrapposte ogni 25 cm (2 m = 8 file)."""
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    righe = 8
    ph = N / righe
    v = (yy % ph) / ph
    fili = rumore(rng, 4, esponente=0.6)
    # fili: rumore stirato lungo V
    fili = sfoca(fili, 0.8)
    f2 = np.zeros_like(fili)
    for k in range(-12, 13):
        f2 += np.roll(fili, k * 3, 0)
    fili = f2 / 25.0
    fili = (fili - fili.mean()) / (fili.std() + 1e-9)
    base = np.array([0.60, 0.50, 0.30])
    vecchio = morbido(rng, 5)
    col = base * (0.78 + 0.22 * fili[..., None]) * (0.75 + 0.35 * v[..., None])
    col = col * (1 - 0.25 * np.clip(vecchio, 0, 2)[..., None] / 2) + np.array([0.33, 0.30, 0.24]) * 0.25 * np.clip(vecchio, 0, 2)[..., None] / 2
    alt = 0.4 + 0.5 * v + 0.3 * fili
    return col, alt, 3.0


def ciottoli(rng):
    """Selciato: ciottoli tondeggianti di 10-18 cm (Voronoi), terra negli interstizi."""
    punti = rng.uniform(0, N, (520, 2))
    idx, bordo, centro = celle_voronoi(rng, punti)
    tinte = rng.uniform(0.75, 1.25, len(punti))
    caldo = rng.uniform(-0.04, 0.04, len(punti))
    base = np.array([0.50, 0.48, 0.44])
    t = tinte[idx]
    c = caldo[idx]
    sasso = np.clip(bordo / 9.0, 0, 1) ** 0.5
    col = (base[None, None, :] + np.stack([c, c * 0.5, -c * 0.5], -1)) * t[..., None]
    terra = np.array([0.33, 0.28, 0.22])
    col = col * sasso[..., None] + terra * (1 - sasso[..., None])
    alt = 0.2 + 0.7 * sasso + 0.05 * rumore(rng, 64)
    col *= (1 + 0.06 * morbido(rng, 5)[..., None] + 0.04 * rumore(rng, 32)[..., None])
    return col, alt, 4.0


def terra(rng):
    """Terra battuta (4 m): marrone con sassolini e solchi lievi; nell'alfa un rumore per i bordi sfrangiati."""
    m1 = morbido(rng, 6)
    m2 = morbido(rng, 20)
    grana = rumore(rng, 32)
    base = np.array([0.40, 0.33, 0.25])
    col = base * (1 + 0.10 * m1[..., None] + 0.06 * m2[..., None] + 0.05 * grana[..., None])
    alt = 0.5 + 0.1 * m2 + 0.05 * grana
    sassi = rng.uniform(0, N, (900, 2))
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    for (px, py) in sassi:
        r = rng.uniform(2, 6)
        x0, x1 = int(px - r - 1), int(px + r + 2)
        y0, y1 = int(py - r - 1), int(py + r + 2)
        ys = np.arange(y0, y1) % N
        xs = np.arange(x0, x1) % N
        gy, gx = np.meshgrid(np.arange(y0, y1), np.arange(x0, x1), indexing="ij")
        d = np.sqrt((gx - px) ** 2 + (gy - py) ** 2)
        k = np.clip(1 - d / r, 0, 1)
        tinta = rng.uniform(0.45, 0.65)
        blocco = col[np.ix_(ys, xs)]
        col[np.ix_(ys, xs)] = blocco * (1 - (k > 0)[..., None]) + tinta * np.array([1.0, 0.97, 0.92]) * (k > 0)[..., None]
        alt[np.ix_(ys, xs)] += k * 0.3
    bordi = morbido(rng, 14) * 0.6 + morbido(rng, 40) * 0.4
    alfa = np.clip(bordi * 0.25 + 0.5, 0, 1)
    return col, alt, 2.0, alfa


def roccia(rng):
    """(06/11, sera) Roccia viva (4 m): strati inclinati, fratture irregolari (creste del rumore, non una rete), venature,
    licheni e sporco negli incavi. Per la grotta est e le rupi (villaggio_grotta.py)."""
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    grosso = rumore(rng, 256, esponente=2.2)
    medio = rumore(rng, 64, esponente=2.0)
    fine = rumore(rng, 8, esponente=1.6)
    # gli strati: bande lungo y, deformate e di spessore diverso (periodiche: 5 per lato)
    fase = (yy / N * 5 + grosso * 0.22 + medio * 0.03) % 1.0
    gradino = np.clip(np.abs(fase - 0.5) * 2, 0, 1)
    strati = np.clip((gradino - 0.82) / 0.18, 0, 1)                 # il giunto tra uno strato e l'altro
    # le fratture: creste del rumore (|r| piccolo), solo dove un'altra macchia lo permette
    frattura = np.zeros((N, N), np.float32)
    for scala, peso in ((180, 1.0), (90, 0.7)):
        r = rumore(rng, scala, esponente=2.3)
        linea = np.clip(1 - np.abs(r) / 0.06, 0, 1) ** 2
        frattura = np.maximum(frattura, linea * np.clip(morbido(rng, 4) + 0.3, 0, 1) * peso)
    alt = 0.6 + grosso * 0.18 + medio * 0.10 + fine * 0.035 - strati * 0.25 - frattura * 0.45
    base = np.array([0.43, 0.41, 0.38])
    col = np.ones((N, N, 3), np.float32) * base
    col *= (1 + 0.14 * grosso + 0.08 * medio + 0.06 * fine)[..., None]
    banda = np.floor((yy / N * 5 + grosso * 0.22) % 5)
    col *= (1 + (np.sin(banda * 2.4) * 0.06))[..., None]           # ogni strato un poco diverso
    col *= (1 - strati * 0.35 - frattura * 0.5)[..., None]
    vena = np.clip(1 - np.abs(rumore(rng, 128, esponente=2.4)) / 0.04, 0, 1) * np.clip(morbido(rng, 3), 0, 1)
    col = col * (1 - vena[..., None] * 0.4) + np.array([0.68, 0.66, 0.61]) * vena[..., None] * 0.4
    ruggine = np.clip(morbido(rng, 6) - 1.3, 0, 1)
    col = col * (1 - ruggine[..., None] * 0.35) + np.array([0.42, 0.29, 0.18]) * ruggine[..., None] * 0.35
    lic = np.clip((morbido(rng, 16) * 0.7 + fine * 0.3 - 1.0) * 2.0, 0, 1) * np.clip(alt * 1.5 - 0.5, 0, 1)
    col = col * (1 - lic[..., None] * 0.45) + np.array([0.47, 0.50, 0.38]) * lic[..., None] * 0.45
    incavo = np.clip(0.40 - sfoca(alt, 8), 0, 1)
    col *= (1 - incavo[..., None] * 0.45)
    return col, alt, 3.0


TUTTE = {
    "Pietra": pietra,
    "Intonaco": intonaco,
    "Assi": assi,
    "Trave": trave,
    "Coppi": coppi,
    "Paglia": paglia,
    "Ciottoli": ciottoli,
    "Terra": terra,
    "Roccia": roccia,
}


def scrivi_tutte(cartella, seme=1105):
    os.makedirs(cartella, exist_ok=True)
    for i, (nome, funzione) in enumerate(TUTTE.items()):
        rng = np.random.default_rng(seme + i)
        risultato = funzione(rng)
        col, alt, forza = risultato[0], risultato[1], risultato[2]
        rgb = colore(col)
        if len(risultato) > 3:
            a = (np.clip(risultato[3], 0, 1) * 255).astype(np.uint8)
            Image.fromarray(np.dstack([rgb, a]), "RGBA").save(os.path.join(cartella, "T_%s.png" % nome))
        else:
            Image.fromarray(rgb, "RGB").save(os.path.join(cartella, "T_%s.png" % nome))
        alt = (alt - alt.min()) / (alt.max() - alt.min() + 1e-9) * 40.0
        Image.fromarray(normale(alt, forza / 10.0), "RGB").save(os.path.join(cartella, "T_%s_N.png" % nome))
        print("texture", nome)


if __name__ == "__main__":
    import sys
    scrivi_tutte(sys.argv[1] if len(sys.argv) > 1 else "uscita_villaggio")
