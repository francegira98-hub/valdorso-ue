# Valdorso - Generatore della valle (v0.1.3, base del terreno). Scritto da Claude il 06/10/2026.
# Crea, senza licenze e sempre uguale (seme fisso), la valle approvata da Fra il 28/09 (mappa) e il 02/10 (2,5 km):
#   Altezze_<lato>.png   altezze a 16 bit per il Landscape di Unreal (Modalita Paesaggio -> Importa da file)
#   Strati/*.png         maschere a 8 bit per i materiali e la vegetazione (prato, roccia, terra, ghiaia, neve,
#                        bosco, strade, acqua, palude, villaggio)
#   Luoghi_<lato>.json   coordinate dei luoghi in metri (x est, y nord, origine nell'angolo sud-ovest) e in Unreal
#   Anteprima_<lato>.png la valle in rilievo vista dall'alto, per controllare
#   Mappa_<lato>.png     la mappa di carta disegnata a mano (pergamena, inchiostro, nomi), come la vedra il colono
# Uso: python valle.py <cartella_di_uscita> [3000 ...] [--vertici 2017|4033] [--fonts cartella]   (numpy, scipy, Pillow)
# Decisa da Fra il 06/10: valle di 3 x 3 km (9 km quadrati), con 4033 vertici per lato (un punto ogni 74 cm).
#
# Come e fatta (dalla mappa del 28/09, in proporzione al lato):
#   monti tutto intorno, piu alti a nord; un solo passo a sud-est verso Aurelia (gola con il posto di guardia);
#   fiume da nord-est in diagonale fino al lago a sud-ovest, e dal lago l'emissario verso ovest tra i monti; villaggio al centro-sud sulla riva est, mulino a ovest
#   del villaggio, ponte a nord; collina dell'Orso al centro-nord (la piu alta della conca); bosco grande a ovest e
#   nord-ovest con le rovine degli Antichi; boschetto a est; grotta a est nei monti, seconda grotta a nord oltre la
#   collina; pascoli a sud-est del villaggio; palude di Nonna Edda vicino al lago; cimitero a sud-ovest del villaggio.

import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from scipy import ndimage

N = 2017                  # vertici per lato: misura consigliata da Unreal (63 quadrati x 32 componenti + 1)
QUOTA_MAX = 800.0         # metri rappresentati dai 16 bit (0 .. 800 m)
SEME = 2809               # il giorno della mappa approvata
# (06/10) La cascata (decisa da Fra: un salto solo, alto e spettacolare, con la pozza)
T_ORLO = 0.13             # a che punto del fiume è l'orlo (0 = sorgente nei monti, 1 = lago): circa 320 m dalla sorgente
Q_ORLO = 112.0            # quota dell'orlo, in metri
SALTO = 30.0              # altezza del salto
RAGGIO_POZZA = 18.0       # la pozza dove cade l'acqua
PROFONDITA_POZZA = 7.0
POZZA_AVANTI = 19.0       # il centro della pozza è 19 m oltre l'orlo: la pozza comincia ai piedi della parete, il getto cade a 6-7 m da lei


# ------------------------------------------------------------------------------------------------ rumore

def lisca(t):
    t = np.clip(t, 0, 1)
    return t * t * (3 - 2 * t)


def distanza_da_linea(X, Y, punti, param=None):
    """Distanza (m) di ogni punto dalla spezzata, e posizione lungo la spezzata (0..1).
    (07/10) Con 'param' (un valore per punto) la posizione è quel valore interpolato, non la frazione di lunghezza:
    così il fiume può serpeggiare senza spostare le quote lungo il suo corso."""
    best = np.full(X.shape, np.inf, dtype=X.dtype)
    lungo = np.zeros(X.shape, dtype=X.dtype)
    lunghezze = [math.dist(punti[i], punti[i + 1]) for i in range(len(punti) - 1)]
    totale = sum(lunghezze)
    fatto = 0.0
    for i in range(len(punti) - 1):
        (ax, ay), (bx, by) = punti[i], punti[i + 1]
        dx, dy = bx - ax, by - ay
        L2 = dx * dx + dy * dy
        t = np.clip(((X - ax) * dx + (Y - ay) * dy) / L2, 0, 1)
        d = np.hypot(X - (ax + t * dx), Y - (ay + t * dy))
        meglio = d < best
        best = np.where(meglio, d, best)
        if param is None:
            lungo = np.where(meglio, (fatto + t * lunghezze[i]) / totale, lungo)
        else:
            lungo = np.where(meglio, param[i] + t * (param[i + 1] - param[i]), lungo)
        fatto += lunghezze[i]
    return best, lungo


def frazione_lungo(punti, punto):
    """(07/10) A che frazione della lunghezza della spezzata sta il suo punto più vicino a 'punto'."""
    migliore, frazione, fatto = 1e18, 0.0, 0.0
    lunghezze = [math.dist(punti[i], punti[i + 1]) for i in range(len(punti) - 1)]
    totale = sum(lunghezze)
    for i in range(len(punti) - 1):
        (ax, ay), (bx, by) = punti[i], punti[i + 1]
        dx, dy = bx - ax, by - ay
        L2 = dx * dx + dy * dy or 1e-12
        u = min(max(((punto[0] - ax) * dx + (punto[1] - ay) * dy) / L2, 0), 1)
        d = math.hypot(punto[0] - ax - u * dx, punto[1] - ay - u * dy)
        if d < migliore:
            migliore, frazione = d, (fatto + u * lunghezze[i]) / totale
        fatto += lunghezze[i]
    return frazione


def punto_param(punti, param, t):
    """(07/10) Il punto della spezzata dove il parametro (un valore per punto, crescente) vale t."""
    i = int(np.searchsorted(param, t)) - 1
    i = min(max(i, 0), len(punti) - 2)
    f = (t - param[i]) / max(param[i + 1] - param[i], 1e-12)
    return (punti[i][0] + f * (punti[i + 1][0] - punti[i][0]), punti[i][1] + f * (punti[i + 1][1] - punti[i][1]))


def serpeggia(punti, param, lunghezza, t_orlo):
    """(07/10) Il torrente nei monti fa curve: ogni punto si sposta di lato (perpendicolare al corso) con due onde
    di 95 e 41 m. Ampiezza 9 m sopra la cascata, 6 m nelle rapide sotto; zero vicino all'orlo e alla pozza
    (da 25 m prima dell'orlo a 60 m dopo) e dove il fiume entra nella conca, così il resto non cambia."""
    nuovi = []
    for i, (x, y) in enumerate(punti):
        t = param[i]
        s = t * lunghezza
        sopra = lisca(t / 0.02) * lisca((t_orlo - 25.0 / lunghezza - t) / (15.0 / lunghezza))
        sotto = lisca((t - t_orlo - 60.0 / lunghezza) / (25.0 / lunghezza)) * lisca((0.25 - t) / 0.03)
        ampiezza = 9.0 * sopra + 6.0 * sotto
        if ampiezza <= 0 or i == 0 or i == len(punti) - 1:
            nuovi.append((x, y))
            continue
        ax, ay = punti[i - 1]
        bx, by = punti[i + 1]
        dx, dy = bx - ax, by - ay
        l = math.hypot(dx, dy) or 1.0
        nx, ny = -dy / l, dx / l
        onda = math.sin(2 * math.pi * s / 95.0 + 0.7) + 0.45 * math.sin(2 * math.pi * s / 41.0 + 2.1)
        nuovi.append((x + nx * ampiezza * onda / 1.45, y + ny * ampiezza * onda / 1.45))
    return nuovi


def punto_a(punti, t):
    """Il punto della spezzata a una frazione t (0..1) della sua lunghezza."""
    lunghezze = [math.dist(punti[i], punti[i + 1]) for i in range(len(punti) - 1)]
    resto = t * sum(lunghezze)
    for i, l in enumerate(lunghezze):
        if resto <= l:
            f = resto / l if l > 0 else 0
            return (punti[i][0] + f * (punti[i + 1][0] - punti[i][0]), punti[i][1] + f * (punti[i + 1][1] - punti[i][1]))
        resto -= l
    return tuple(punti[-1])


def curva(punti, passi=24):
    """Spezzata arrotondata (Catmull-Rom) per fiume e strade."""
    p = [punti[0]] + list(punti) + [punti[-1]]
    out = []
    for i in range(1, len(p) - 2):
        p0, p1, p2, p3 = map(np.array, (p[i - 1], p[i], p[i + 1], p[i + 2]))
        for k in range(passi):
            t = k / passi
            out.append(tuple(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t
                                    + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3)))
    out.append(tuple(punti[-1]))
    return out


# ------------------------------------------------------------------------------------------------ la valle

def campo(rng, n, celle):
    """Campo casuale morbido n x n da una griglia di celle x celle (interpolazione cubica)."""
    base = rng.standard_normal((celle + 4, celle + 4))
    z = ndimage.zoom(base, n / celle, order=3)
    o = int(2 * n / celle)
    return z[o:o + n, o:o + n]


def frattale(rng, n, celle, ottave=6, persistenza=0.5, deforma=None):
    """Rumore frattale; con 'deforma' (dx, dy in pixel) i campi sono letti spostati: forme meno regolari."""
    tot = np.zeros((n, n), dtype=np.float32); amp = 1.0; somma = 0.0; c = celle
    righe, colonne = np.mgrid[0:n, 0:n].astype(float)
    for _ in range(ottave):
        f = campo(rng, n, max(2, int(c)))
        if deforma is not None:
            f = ndimage.map_coordinates(f, [np.clip(righe + deforma[1], 0, n - 1), np.clip(colonne + deforma[0], 0, n - 1)],
                                        order=1)
        tot += (amp * f).astype(np.float32); somma += amp; amp *= persistenza; c *= 2
        if c > n / 2:
            break
    tot /= somma
    return tot / (np.abs(tot).max() + 1e-9)


def erosione_termica(h, passo, giri=60, pendenza_max=0.85, quanto=0.25):
    """Il materiale scivola dove il pendio e troppo ripido: monti piu naturali, ghiaioni ai piedi."""
    # Otto direzioni (anche le diagonali, piu lontane) e bordi chiusi: niente righe dritte e niente travasi da un lato all'altro.
    direzioni = [(1, 0, 1.0), (-1, 0, 1.0), (0, 1, 1.0), (0, -1, 1.0),
                 (1, 1, 1.414), (1, -1, 1.414), (-1, 1, 1.414), (-1, -1, 1.414)]
    for _ in range(giri):
        p = np.pad(h, 1, mode="edge")
        nuovo = h.copy()
        for dr, dc, lung in direzioni:
            vicino = p[1 + dr:1 + dr + h.shape[0], 1 + dc:1 + dc + h.shape[1]]
            diff = h - vicino - pendenza_max * passo * lung
            sposta = np.where(diff > 0, diff * quanto / 8, 0)
            nuovo -= sposta
            arrivo = np.zeros_like(h)
            # il materiale tolto qui arriva al vicino (dr, dc)
            r0, r1 = max(dr, 0), h.shape[0] + min(dr, 0)
            c0, c1 = max(dc, 0), h.shape[1] + min(dc, 0)
            arrivo[r0:r1, c0:c1] = sposta[r0 - dr:r1 - dr, c0 - dc:c1 - dc]
            nuovo += arrivo
        h = nuovo
    return h


def valle(lato, uscita, cartella_font):
    rng = np.random.default_rng(SEME)
    passo = lato / (N - 1)
    asse = (np.arange(N) * passo).astype(np.float32)   # float32: con 4033 vertici la memoria non basta in float64
    X, Y = np.meshgrid(asse, asse[::-1])        # riga 0 = nord (in alto nell'immagine), Y cresce verso nord

    # La conca giocabile e un quadrato morbido al centro; fuori i monti. Con 3 km i monti sono piu larghi e piu alti
    # (la conca cresce meno del lato: la valle resta raccolta, i monti diventano maestosi).
    margine = 0.14 * lato if lato <= 2600 else 0.16 * lato
    conca = lato - 2 * margine

    def P(u, v):
        """Da coordinate della mappa (0..1, v=0 nord) a metri del mondo (x est, y nord)."""
        return (margine + u * conca, margine + (1 - v) * conca)

    luoghi = {
        "Piazza del villaggio": P(0.575, 0.707),
        "Tempio del Cuore": P(0.575, 0.690),
        "Mulino": P(0.49, 0.715),
        "Ponte": P(0.555, 0.565),
        "Collina dell'Orso": P(0.48, 0.30),
        "Rovine degli Antichi": P(0.17, 0.36),
        "Grotta est": P(1.04, 0.52),
        "Grotta nord": P(0.50, -0.03),
        "Lago": P(0.20, 0.79),
        "Palude": P(0.11, 0.90),
        "Pascoli": P(0.72, 0.82),
        "Cimitero": P(0.50, 0.81),
        "Boschetto": P(0.80, 0.47),
        "Posto di guardia del passo": P(1.05, 0.99),
    }

    # Si modella a meta risoluzione (piu veloce, e l'erosione lavora meglio), poi si raddoppia e si aggiungono i dettagli.
    M = (N + 1) // 2
    pm = lato / (M - 1)
    am = np.arange(M) * pm
    Xm, Ym = np.meshgrid(am, am[::-1])
    um = (Xm - margine) / conca
    vm = 1 - (Ym - margine) / conca

    # --- il contorno della conca: un quadrato arrotondato con il bordo mosso (niente pieghe agli angoli)
    cx0 = cy0 = lato / 2
    dx, dy = Xm - cx0, Ym - cy0
    rho = np.hypot(dx, dy)
    teta = np.arctan2(dy, dx)
    quadro = (np.abs(np.cos(teta)) ** 4 + np.abs(np.sin(teta)) ** 4) ** -0.25
    ondula = sum(a * np.sin(k * teta + f) for a, k, f in ((0.05, 3, 1.1), (0.035, 5, 2.3), (0.02, 9, 0.4), (0.012, 17, 4.0)))
    raggio = conca / 2 * quadro * (1 + ondula)
    fuori = rho - raggio                                  # metri oltre il bordo della conca (negativo = dentro)

    # --- i monti: creste deformate, piu alti a nord (seno di teta = 1 a nord)
    deforma = (campo(rng, M, 6) * M * 0.035, campo(rng, M, 6) * M * 0.035)
    cresta = 1 - np.abs(frattale(rng, M, 10, 6, 0.55, deforma))
    cresta = cresta ** 2.2
    massa = frattale(rng, M, 5, 3, 0.5)
    nord = 0.5 + 0.5 * np.sin(teta)
    alto = 230 + 300 * nord + (100 if lato > 2600 else 0)
    salita = lisca((fuori + 0.06 * conca) / (margine * 1.15 + 0.06 * conca))
    monti = salita ** 1.25 * alto * (0.45 + 0.55 * cresta + 0.25 * massa)

    # --- il fondo della conca: colline dolci che scendono verso il lago
    colline = frattale(rng, M, 7, 5, 0.45)
    fondo = 46 + 16 * (um - 0.2) + 12 * (0.75 - vm) + 9 * colline
    h = fondo + monti

    # --- la collina dell'Orso e il crinale verso i monti del nord (sentiero della grotta nord)
    ox, oy = luoghi["Collina dell'Orso"]
    r = np.hypot(Xm - ox, (Ym - oy) * 1.25)
    h += 72 * np.exp(-(r / (0.085 * conca)) ** 2) + 22 * np.exp(-(r / (0.17 * conca)) ** 2)
    nx, ny = luoghi["Grotta nord"]
    dcr, _ = distanza_da_linea(Xm, Ym, [(ox, oy), (nx, ny)])
    h += 26 * np.exp(-(dcr / (0.05 * conca)) ** 2) * lisca(1 - fuori / margine)

    h = erosione_termica(h, pm, 50)

    # --- dettagli a piena risoluzione
    h = ndimage.zoom(h, N / M, order=3)[:N, :N].astype(np.float32)
    del Xm, Ym, um, vm, dx, dy, rho, teta, quadro, ondula, raggio, fuori, cresta, massa, nord, alto, salita, monti, colline, fondo, r, dcr
    u = (X - margine) / conca
    v = 1 - (Y - margine) / conca
    dett = frattale(rng, N, 120, 4, 0.5)
    pend0 = np.hypot(*np.gradient(h, passo))
    h += dett * (0.6 + 7 * np.clip(pend0, 0, 1.2))     # sassi e rughe sui pendii, quasi niente sui prati

    # --- il passo sbarrato a sud-est: una gola tortuosa fino al bordo del mondo
    gx, gy = luoghi["Posto di guardia del passo"]
    gola_pts = curva([P(0.78, 0.80), P(0.90, 0.86), P(0.97, 0.94), (gx, gy), (gx + 0.05 * lato, gy - 0.07 * lato),
                      (lato, 0.03 * lato)])
    dg, tg = distanza_da_linea(X, Y, gola_pts)
    letto_gola = 52 + 26 * lisca(tg)
    largo = 70 + 30 * tg
    gola = lisca(1 - dg / (largo * 2.2))
    h = h * (1 - gola) + np.minimum(h, letto_gola + np.maximum(dg - 18, 0) * 1.1) * gola

    # --- il villaggio: un ripiano quasi piano (si costruisce meglio), raccordato.
    # (06/10) Prima del fiume: se viene dopo, lo spianamento riempie l'alveo vicino al mulino e il fiume sparisce.
    vx, vy = luoghi["Piazza del villaggio"]
    rv = np.hypot(X - vx, Y - vy) / (0.12 * conca)
    piano = lisca((1.3 - rv) / 0.6)
    liscio = ndimage.gaussian_filter(h, 50 / passo)
    h = h * (1 - piano) + (liscio * 0.9 + h * 0.1) * piano

    # --- il fiume: da nord-est nei monti, in diagonale con qualche ansa, a ovest del villaggio, fino al lago
    base_fiume = [P(0.90, -0.16), P(0.85, -0.05), P(0.80, 0.05), P(0.765, 0.13), P(0.725, 0.21), P(0.71, 0.29),
                  P(0.655, 0.36), P(0.625, 0.44), P(0.585, 0.50),
                  P(0.545, 0.585), P(0.53, 0.66), P(0.495, 0.735), P(0.43, 0.765), P(0.35, 0.80), P(0.28, 0.795)]
    fiume_pts = curva(base_fiume, 30)
    # (07/10, approvato da Fra: "mi fido di te, vai pure") Il torrente nei monti non è più dritto come un righello:
    # serpeggia tra le pareti (sopra la cascata e nelle rapide sotto), ma passa sempre dall'orlo, che resta dov'era.
    # Ogni punto tiene la sua posizione "di prima" lungo il corso (param), così le quote del fiume non cambiano.
    fiume_dritto = fiume_pts
    lunghezze_f = [math.dist(fiume_pts[i], fiume_pts[i + 1]) for i in range(len(fiume_pts) - 1)]
    L_fiume = sum(lunghezze_f)
    param_f = list(np.concatenate([[0.0], np.cumsum(lunghezze_f)]) / L_fiume)
    fiume_pts = serpeggia(fiume_pts, param_f, L_fiume, T_ORLO)
    # La distanza dal letto si misura dal fiume che serpeggia; la posizione lungo il corso (per le quote) dal corso
    # dritto: così i fianchi della gola restano lisci, senza pieghe dove le curve si avvicinano.
    df, _ = distanza_da_linea(X, Y, fiume_pts, param_f)
    _, tf = distanza_da_linea(X, Y, fiume_dritto)
    del _
    larghezza = 6 + 10 * tf
    quota_fiume = 64 - 34 * tf + 120 * lisca((0.22 - tf) / 0.22)
    # (06/10, decisa da Fra: "A") La cascata: dove il fiume esce dai monti di nord-est non scende più lungo uno scivolo,
    # ma salta un gradino di roccia di 30 m e cade in una pozza rotonda. Sopra l'orlo una cengia quasi piana, sotto le rapide.
    orlo_x, orlo_y = (float(v) for v in punto_a(fiume_dritto, T_ORLO))
    avanti = np.subtract(punto_a(fiume_dritto, T_ORLO + 0.004), (orlo_x, orlo_y)).astype(float)
    avanti = avanti / np.linalg.norm(avanti)                 # la direzione in cui l'acqua salta
    ax_, ay_ = float(avanti[0]), float(avanti[1])
    pozza_x, pozza_y = orlo_x + ax_ * POZZA_AVANTI, orlo_y + ay_ * POZZA_AVANTI
    # Tutto quello che riguarda la cascata si calcola in una finestra di 560 m intorno all'orlo (poca memoria).
    mezza = int(280 / passo)
    rc, cc = int(round((lato - orlo_y) / passo)), int(round(orlo_x / passo))
    fr, fc = slice(max(rc - mezza, 0), min(rc + mezza + 1, N)), slice(max(cc - mezza, 0), min(cc + mezza + 1, N))
    Xf, Yf = X[fr, fc], Y[fr, fc]
    # La parete non è una riga dritta: è un ferro di cavallo intorno alla pozza (i lati girano verso valle),
    # con il bordo frastagliato. lungo = metri verso valle dall'orlo, lato = metri di traverso.
    lungo = (Xf - orlo_x) * ax_ + (Yf - orlo_y) * ay_
    lato_ = -(Xf - orlo_x) * ay_ + (Yf - orlo_y) * ax_

    def frastaglio(l):
        return 2.2 * np.sin(l / 6.3 + 1.0) + 1.4 * np.sin(l / 2.9 + 2.0) + 0.8 * np.sin(l / 1.7 + 0.3)
    # Al centro, per 6 m di qua e di là, l'orlo è dritto e proprio al punto dell'orlo: lì cascata.py appoggia il getto.
    bordo = (np.minimum(0.006 * lato_ * lato_, 45.0)
             + (frastaglio(lato_) - frastaglio(0.0)) * lisca((np.abs(lato_) - 6) / 6))
    vicino = lisca(1 - (np.abs(lungo) - 80) / 40) * lisca(1 - (np.abs(lato_) - 160) / 40)
    gradino = lisca((tf - T_ORLO) / 0.002)                  # lontano dalla cascata: il salto di traverso al fiume
    gradino[fr, fc] = lisca((lungo - bordo) / 2.0) * vicino + gradino[fr, fc] * (1 - vicino)   # il salto in 2 m
    del lungo, lato_, bordo, vicino
    quota_fiume = np.maximum(quota_fiume, Q_ORLO) * (1 - gradino) + np.minimum(quota_fiume, Q_ORLO - SALTO) * gradino
    del gradino
    sponde = lisca(1 - df / (60 + 90 * lisca((0.25 - tf) / 0.25)))
    prima_del_fiume = h.copy()
    # (07/10) Nei monti la gola è più stretta (fianchi più ripidi: 1,3 invece di 0,9) e irregolare: i fianchi entrano
    # ed escono (speroni di roccia), leggendo il rumore dei dettagli già fatto (nessun dado nuovo).
    monti_f = lisca((0.25 - tf) / 0.05)
    df_mosso = df * (1 + 0.35 * dett * monti_f)
    h = h * (1 - sponde) + np.minimum(h, quota_fiume + 1.5 + df_mosso * (0.10 + 1.3 * lisca((0.25 - tf) / 0.25))) * sponde
    del df_mosso
    alveo = lisca(1 - df / larghezza)
    h -= alveo * (3.0 + 2.5 * tf)
    # (07/10) Il letto del torrente sopra la cascata scende a scalini: tratti quasi piani (le pozze) e piccoli salti di
    # 3-4 m ogni 22 m, come i torrenti veri. Solo nel letto: i fianchi della gola restano quelli di sopra.
    scalini_f = []
    t0, dt = 0.004, 22.0 / L_fiume
    fine_scalini = T_ORLO - 0.010
    nel_tratto = (tf > t0) & (tf < fine_scalini)
    if nel_tratto.any():
        def q_liscia(t):
            return 64 - 34 * t + 120 * lisca((0.22 - t) / 0.22)
        k = np.floor((tf - t0) / dt)
        tk = t0 + k * dt
        f = (tf - tk) / dt
        qa, qb = q_liscia(tk), q_liscia(np.minimum(tk + dt, fine_scalini))
        q_scalino = qa + (qb - qa) * np.where(f < 0.8, 0.25 * f / 0.8, 0.25 + 0.75 * lisca((f - 0.8) / 0.2))
        alza = (q_scalino - quota_fiume) * nel_tratto * lisca(1 - df / (larghezza + 3))
        h += np.maximum(alza, 0)
        del k, tk, f, qa, qb, q_scalino, alza
        tt = t0 + dt * 0.9
        while tt < fine_scalini - dt * 0.1:
            x_s, y_s = punto_param(fiume_pts, param_f, tt)
            x_d, y_d = punto_param(fiume_pts, param_f, tt + 0.002)
            scalini_f.append({"x_m": round(x_s, 2), "y_m": round(y_s, 2), "t": round(tt, 5),
                              "direzione": [round((x_d - x_s) / max(math.hypot(x_d - x_s, y_d - y_s), 1e-6), 4),
                                            round((y_d - y_s) / max(math.hypot(x_d - x_s, y_d - y_s), 1e-6), 4)]})
            tt += dt
    del nel_tratto, monti_f

    # --- (06/10) la pozza della cascata: una conca rotonda scavata dove cade l'acqua, con le pareti dritte verso monte
    # (un anfiteatro di roccia intorno al getto) e il fondo a 7 m sotto il pelo; verso valle la riva è bassa e l'acqua esce.
    # (07/10) Non più un cerchio perfetto (sembrava un pozzo): il bordo va e viene, e fuori dalla pozza la riva sale
    # in pendio dolce (ghiaia e sassi); dritta come una parete resta solo verso monte, sotto la cascata.
    def forma_pozza(x, y):
        teta = np.arctan2(y - pozza_y, x - pozza_x)
        return 1 + 0.10 * np.sin(3 * teta + 1.0) + 0.06 * np.sin(5 * teta + 2.0) + 0.03 * np.sin(9 * teta + 0.5)
    forma = forma_pozza(Xf, Yf)
    rz_f = np.hypot(Xf - pozza_x, Yf - pozza_y) / (RAGGIO_POZZA * forma)
    verso_monte = lisca((8.0 - ((Xf - orlo_x) * ax_ + (Yf - orlo_y) * ay_)) / 6.0)   # 1 vicino alla parete
    pelo_pozza = Q_ORLO - SALTO + 0.5
    hf = h[fr, fc]
    fondo_pozza = pelo_pozza - 0.4 - PROFONDITA_POZZA * np.clip(1 - rz_f * rz_f, 0, 1) ** 0.7
    hf = np.where(rz_f < 1, np.minimum(hf, fondo_pozza), hf)
    # La riva: dal pelo sale di 0,45 m per metro (circa 24 gradi) e poi sempre più ripida, finché incontra il monte
    # (nessuna sfumatura: dove il pendio arriva al terreno vero, finisce). Verso monte niente: lì resta la parete e l'orlo.
    oltre = np.maximum(rz_f - 1, 0) * RAGGIO_POZZA * forma            # metri veri oltre il bordo della pozza
    riva = pelo_pozza + 0.6 + 0.45 * oltre + 0.04 * np.maximum(oltre - 10.0, 0) ** 2
    lontano_dalla_parete = (1 - verso_monte) * (rz_f >= 1)
    hf = hf * (1 - lontano_dalla_parete) + np.minimum(hf, riva) * lontano_dalla_parete
    del forma, verso_monte, riva, oltre, lontano_dalla_parete
    # Massi e ghiaione ai piedi della parete, appena fuori dalla pozza: solo in basso (non sulla cengia né nel letto).
    # Con un dado suo: il dado della valle resta lo stesso, così il resto del terreno non cambia.
    lato_f = max(hf.shape)
    sassi = frattale(np.random.default_rng(SEME + 1), lato_f, max(4, int(160 * lato_f / N)), 2)[:hf.shape[0], :hf.shape[1]]
    massi = lisca(1 - np.abs(rz_f - 1.25) / 0.3) * lisca((sassi + 0.2) / 0.5)
    hf = hf + massi * 1.2 * (hf > pelo_pozza) * (hf < pelo_pozza + 6) * (1 - alveo[fr, fc])
    h[fr, fc] = hf
    acqua_pozza = np.zeros_like(h)
    acqua_pozza[fr, fc] = lisca((1.0 - rz_f) / 0.1)
    # (07/10) dove la cascata bagna: la nebbia degli spruzzi intorno all'orlo e alla pozza (per lo strato "bagnato")
    spruzzi_f = lisca(1 - np.hypot(Xf - (orlo_x + ax_ * 7), Yf - (orlo_y + ay_ * 7)) / 55.0) ** 0.7
    del fondo_pozza, sassi, massi, hf, Xf, Yf

    # --- (06/10) la gola della cascata: dove il fiume ha tagliato il monte, i fianchi erano piani lisci (si vedevano i
    # triangoli). Ora sono roccia viva: rughe, sporgenze e cenge, più forti dove il taglio è profondo. Solo nei monti
    # (finisce dove il fiume entra nella conca); alveo e pozza restano lisci.
    taglio = lisca((prima_del_fiume - h - 1.0) / 6.0) * lisca((0.21 - tf) / 0.03)
    del prima_del_fiume
    righe_g, colonne_g = np.nonzero(taglio > 0.01)
    if len(righe_g):
        r0, c0 = righe_g.min(), colonne_g.min()
        lato_g = int(max(righe_g.max() - r0, colonne_g.max() - c0)) + 1
        r1, c1 = min(r0 + lato_g, N), min(c0 + lato_g, N)
        dado = np.random.default_rng(SEME + 2)
        celle = max(8, int(260 * lato_g / N))
        rughe = frattale(dado, lato_g, celle, 4, 0.55)[:r1 - r0, :c1 - c0]
        mosse = frattale(dado, lato_g, max(4, celle // 3), 3, 0.5)[:r1 - r0, :c1 - c0]
        hg = h[r0:r1, c0:c1]
        livello = hg + 3.0 * mosse
        cenge = np.round(livello / 4.0) * 4.0 - livello    # gradini ogni 4 m circa, storti: gli strati della roccia
        Xg, Yg = X[r0:r1, c0:c1], Y[r0:r1, c0:c1]
        rz_g = np.hypot(Xg - pozza_x, Yg - pozza_y) / (RAGGIO_POZZA * forma_pozza(Xg, Yg))
        # (07/10) niente rughe sulla riva nuova della pozza (fino a 2,2 raggi) né nell'acqua
        lontano = (lisca((df[r0:r1, c0:c1] - larghezza[r0:r1, c0:c1] - 2) / 6) * lisca((rz_g - 2.2) / 0.3))
        h[r0:r1, c0:c1] = hg + taglio[r0:r1, c0:c1] * lontano * (3.5 * rughe + 0.3 * cenge)
        del rughe, mosse, hg, livello, cenge, lontano, dado, rz_g, Xg, Yg
    del taglio, righe_g, colonne_g

    # --- il lago a sud-ovest: due anse, fondale profondo, riva morbida.
    # (06/10, chiesto da Fra) Piu profondo (circa 27 m al centro invece di 17) e con la riva piu ripida:
    # la fascia di acqua bassa e la spiaggia di ghiaia sono piu strette.
    lx, ly = luoghi["Lago"]
    mosso = 0.22 * frattale(rng, N, 9, 3, 0.5)
    r1 = np.hypot((X - lx) / (0.10 * conca), (Y - ly) / (0.072 * conca))
    r2 = np.hypot((X - (lx - 0.05 * conca)) / (0.065 * conca), (Y - (ly - 0.03 * conca)) / (0.055 * conca))
    rl = np.minimum(r1, r2) * 0.85 + 0.15 * (r1 + r2) / 2 + mosso
    pelo_lago = 30.0
    intorno = lisca(1.8 - rl)
    h = h * (1 - intorno) + np.minimum(h, pelo_lago + 1.5 + 18 * np.clip(rl - 1, 0, 1)) * intorno
    lago = lisca((1.04 - rl) / 0.14)
    h -= lago * (3 + 24 * lisca((1 - rl) / 0.55))

    # --- la palude: piatta, appena sopra il lago, con pozze
    px, py = luoghi["Palude"]
    rp = np.hypot(X - px, (Y - py) * 1.3) / (0.075 * conca) + 0.3 * frattale(rng, N, 12, 3)
    palude = lisca((1 - rp) / 0.7)
    h = h * (1 - palude) + (pelo_lago + 0.5 + 0.7 * frattale(rng, N, 90, 2)) * palude

    # --- (06/10) l'emissario: il torrente che esce dal lago verso ovest e lascia la valle in una gola tra i monti
    # (un giorno porterà alle altre terre della baronia). Dopo la palude, perché la palude non lo riempia.
    base_emissario = [P(0.12, 0.82), P(0.04, 0.825), P(-0.04, 0.845), P(-0.12, 0.86), P(-0.19, 0.85),
                      (-0.02 * lato, P(0, 0.85)[1])]
    emissario_pts = curva(base_emissario, 30)
    de, te = distanza_da_linea(X, Y, emissario_pts)
    quota_em = 29.2 - 17 * te                              # dal lago (30 m) al bordo del mondo
    montagna = lisca((te - 0.25) / 0.25)                  # dopo un quarto entra nei monti
    largo_em = 40 + 140 * montagna
    gola_em = lisca(1 - de / largo_em)
    h = h * (1 - gola_em) + np.minimum(h, quota_em + 0.8 + np.maximum(de - 5, 0) * (0.10 + 0.8 * montagna)) * gola_em
    larghezza_em = 3.5 + 2 * te
    alveo_em = lisca(1 - de / larghezza_em)
    h -= alveo_em * 2.0
    del te, quota_em, montagna, largo_em, gola_em

    # --- le strade (maschera, e un leggero spianamento)
    strade_pts = {
        "dal passo al villaggio": curva([luoghi["Posto di guardia del passo"], P(0.93, 0.90), P(0.80, 0.80), P(0.62, 0.71)]),
        "al ponte e alla collina": curva([P(0.575, 0.69), P(0.565, 0.60), luoghi["Ponte"], P(0.50, 0.45), P(0.48, 0.33)]),
        "alle rovine": curva([P(0.53, 0.70), P(0.45, 0.62), P(0.33, 0.50), P(0.17, 0.38)]),
        "alla grotta est": curva([P(0.62, 0.70), P(0.78, 0.62), P(0.92, 0.55), luoghi["Grotta est"]]),
        "alla grotta nord": curva([P(0.48, 0.33), P(0.50, 0.15), luoghi["Grotta nord"]]),
    }
    strade = np.zeros_like(h)
    for pts in strade_pts.values():
        ds, _ = distanza_da_linea(X, Y, pts)
        strade = np.maximum(strade, lisca(1 - ds / 3.5))
    h = h * (1 - 0.6 * strade) + ndimage.gaussian_filter(h, 6 / passo) * 0.6 * strade

    h = np.clip(h, 0, QUOTA_MAX - 1)
    # (06/10) Con 4033 vertici e l'emissario la memoria non bastava: via i campi che non servono più alle maschere.
    del tf, quota_fiume, sponde, mosso, r1, r2, intorno, rp, liscio, dett, pend0, dg, tg, letto_gola, largo, gola, u, v

    # ============================================================== uscite
    os.makedirs(uscita, exist_ok=True)
    nome = f"{lato}"
    alt16 = np.round(h / QUOTA_MAX * 65535).astype(np.uint16)
    Image.fromarray(alt16).save(os.path.join(uscita, f"Altezze_{nome}.png"))

    # pendenza in gradi
    gy_, gx_ = np.gradient(h, passo)
    pend = np.degrees(np.arctan(np.hypot(gx_, gy_)))

    acqua = np.clip(np.maximum(np.maximum(np.maximum(alveo, lago), alveo_em), acqua_pozza), 0, 1)
    bosco_grande = lisca(1.2 - np.hypot((X - P(0.17, 0.30)[0]) / (0.30 * conca), (Y - P(0.17, 0.30)[1]) / (0.38 * conca))
                         + 0.25 * frattale(rng, N, 10, 3))
    bx, by = luoghi["Boschetto"]
    boschetto = lisca(1.1 - np.hypot(X - bx, Y - by) / (0.11 * conca) + 0.2 * frattale(rng, N, 14, 3))
    bosco = np.clip(np.maximum(bosco_grande, boschetto) * (1 - lisca(pend / 35)) * (1 - acqua) * (1 - strade)
                    * (1 - piano) * (1 - palude) * (h < 330), 0, 1)
    roccia = lisca((pend - 28) / 12)
    neve = lisca((h - (500 if lato > 2600 else 440) - 40 * frattale(rng, N, 30, 3)) / 50)
    # (07/10) la ghiaia delle rive solo dove non è ripido: sui fianchi della gola è roccia (prima erano strisce chiare)
    ghiaia = (np.clip(lisca(1 - df / (larghezza + 6)) - alveo, 0, 1) * lisca((40 - pend) / 10)
              + np.clip(lisca((1.12 - rl) / 0.15) - lago, 0, 1)
              + np.clip(lisca(1 - de / (larghezza_em + 3)) - alveo_em, 0, 1))
    # (06/10) la riva della pozza, solo nella finestra della cascata
    ghiaia[fr, fc] += np.clip(lisca((1.5 - rz_f) / 0.5) - acqua_pozza[fr, fc], 0, 1) * (pend[fr, fc] < 40)
    terra = np.clip(strade + 0.6 * piano * lisca(1 - rv / 0.6), 0, 1)
    # (07/10) bagnato: rocce scure e lucide sotto gli spruzzi della cascata, un filo lungo i fiumi e la riva del lago
    bagnato = np.maximum(0.45 * lisca(1 - df / (larghezza + 4)), 0.35 * lisca((1.15 - rl) / 0.15))
    bagnato = np.maximum(bagnato, 0.35 * lisca(1 - de / (larghezza_em + 3)))
    bagnato[fr, fc] = np.maximum(bagnato[fr, fc], spruzzi_f)
    strati = {
        "Prato": np.clip(1 - roccia - neve - terra - ghiaia, 0, 1),
        "Roccia": np.clip(roccia - neve, 0, 1),
        "Neve": neve,
        "Terra": terra,
        "Ghiaia": np.clip(ghiaia, 0, 1),
        "Bosco": bosco,
        "Strade": strade,
        "Acqua": acqua,
        "Palude": palude,
        "Villaggio": piano,
        "Bagnato": np.clip(bagnato, 0, 1),
    }
    cartella_strati = os.path.join(uscita, f"Strati_{nome}")
    os.makedirs(cartella_strati, exist_ok=True)
    for k, m in strati.items():
        Image.fromarray(np.round(np.clip(m, 0, 1) * 255).astype(np.uint8)).save(os.path.join(cartella_strati, f"{k}.png"))
    impacchetta(cartella_strati, os.path.join(uscita, f"T_ValleMaschere_{nome}.png"))
    impacchetta2(cartella_strati, os.path.join(uscita, f"T_ValleMaschere2_{nome}.png"))

    # Luoghi: metri, e posizione in Unreal (cm) con il Landscape importato con l'angolo nord-ovest in (0,0,0):
    # Unreal X = est, Unreal Y = sud (righe dell'immagine), Z = quota.
    def quota(x, y):
        c = int(round(x / passo)); r_ = int(round((lato - y) / passo))
        c = min(max(c, 0), N - 1); r_ = min(max(r_, 0), N - 1)
        return float(h[r_, c])
    dati = {"lato_m": lato, "vertici": N, "scala_xy_cm": round(passo * 100, 4),
            "scala_z": round(QUOTA_MAX / 512 * 100, 4), "posizione_z_cm": round(QUOTA_MAX / 2 * 100, 1),
            "pelo_del_lago_m": pelo_lago, "luoghi": {},
            # (06/10) i corsi d'acqua in metri (x est, y nord): li legge acqua.py per il lago, il fiume e l'emissario.
            "fiume_m": [[round(x, 2), round(y, 2)] for x, y in fiume_pts],
            "emissario_m": [[round(x, 2), round(y, 2)] for x, y in emissario_pts],
            # (06/10) la cascata: orlo del salto, direzione, pozza; li leggono acqua.py e cascata.py.
            "cascata": {"t_orlo": T_ORLO, "orlo_m": [round(orlo_x, 2), round(orlo_y, 2)], "quota_orlo_m": Q_ORLO,
                        "salto_m": SALTO, "direzione": [round(float(avanti[0]), 4), round(float(avanti[1]), 4)],
                        "pozza_m": [round(pozza_x, 2), round(pozza_y, 2)], "raggio_pozza_m": RAGGIO_POZZA,
                        "pelo_pozza_m": pelo_pozza, "profondita_pozza_m": PROFONDITA_POZZA,
                        "larghezza_orlo_m": round(float(6 + 10 * T_ORLO), 2),
                        # (07/10) l'orlo misurato lungo il fiume che serpeggia (lo usa acqua.py) e gli scalini del torrente
                        "t_orlo_linea": round(frazione_lungo(fiume_pts, (orlo_x, orlo_y)), 6),
                        "scalini": scalini_f}}
    luoghi["Cascata"] = (pozza_x, pozza_y)
    for k, (x, y) in luoghi.items():
        dati["luoghi"][k] = {"x_m": round(x, 1), "y_m": round(y, 1), "quota_m": round(quota(x, y), 1),
                             "unreal_cm": [round(x * 100), round((lato - y) * 100), round(quota(x, y) * 100)]}
    with open(os.path.join(uscita, f"Luoghi_{nome}.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=2)

    anteprima(h, acqua, bosco, neve, roccia, strade, passo, os.path.join(uscita, f"Anteprima_{nome}.png"))
    mappa(h, acqua, bosco, strade, palude, luoghi, lato, passo, fiume_pts, emissario_pts, cartella_font,
          os.path.join(uscita, f"Mappa_{nome}.png"))
    print(f"Fatto: valle {lato} m (quota {h.min():.0f}-{h.max():.0f} m), {uscita}")
    return dati


def impacchetta(cartella_strati, percorso, lato_px=4096):
    """(06/10) Le maschere che usa il materiale M_Valle, in un'immagine sola RGBA (lineare, non sRGB):
    R = roccia, G = neve, B = terra (strade e villaggio), A = ghiaia (rive, alveo e fondale).
    Copre tutta la valle: in Unreal UV = posizione del mondo X, Y / lato in cm."""
    def leggi(n):
        return Image.open(os.path.join(cartella_strati, f"{n}.png")).convert("L").resize((lato_px, lato_px), Image.BILINEAR)
    ghiaia = np.maximum(np.array(leggi("Ghiaia")), np.array(leggi("Acqua")))
    # Le sponde del fiume sono ripide ma non sono roccia: dove c'è ghiaia o acqua, niente roccia.
    roccia = (np.array(leggi("Roccia")).astype(np.float32) * (1 - ghiaia / 255.0)).astype(np.uint8)
    canali = [Image.fromarray(roccia), leggi("Neve"), leggi("Terra"), Image.fromarray(ghiaia)]
    Image.merge("RGBA", canali).save(percorso, optimize=True)


def impacchetta2(cartella_strati, percorso, lato_px=4096):
    """(07/10) La seconda immagine di maschere per M_Valle: R = bagnato (spruzzi della cascata, rive); G, B liberi
    (muschio e felci arriveranno con la vegetazione); A pieno."""
    bagnato = Image.open(os.path.join(cartella_strati, "Bagnato.png")).convert("L").resize((lato_px, lato_px), Image.BILINEAR)
    vuoto = Image.new("L", (lato_px, lato_px), 0)
    Image.merge("RGBA", [bagnato, vuoto, vuoto, Image.new("L", (lato_px, lato_px), 255)]).save(percorso, optimize=True)


def ombra(h, passo, forza=1.0):
    gy, gx = np.gradient(h, passo)
    nrm = np.dstack([-gx * forza, gy * forza, np.ones_like(h)])
    nrm /= np.linalg.norm(nrm, axis=2, keepdims=True)
    luce = np.array([-0.6, 0.6, 0.55]); luce /= np.linalg.norm(luce)
    return np.clip((nrm * luce).sum(axis=2), 0, 1)


def anteprima(h, acqua, bosco, neve, roccia, strade, passo, percorso):
    L = 1024
    def giu(a): return np.array(Image.fromarray(a.astype(np.float32)).resize((L, L), Image.BILINEAR))
    hh = giu(h); sh = ombra(hh, passo * h.shape[0] / L, 1.6)
    prato = np.array([96, 128, 62.0]); bosc = np.array([38, 70, 36.0]); rocc = np.array([120, 112, 100.0])
    nev = np.array([236, 238, 242.0]); terr = np.array([150, 120, 82.0]); acq = np.array([52, 88, 120.0])
    c = prato * np.ones((L, L, 1))
    for m, col in ((giu(bosco), bosc), (giu(roccia), rocc), (giu(neve), nev), (giu(strade), terr)):
        c = c * (1 - m[..., None]) + col * m[..., None]
    c = c * (0.5 + 0.7 * sh[..., None])
    a = giu(acqua)[..., None]
    c = c * (1 - a) + acq * a * (0.8 + 0.3 * sh[..., None])
    Image.fromarray(np.clip(c, 0, 255).astype(np.uint8)).save(percorso)


def mappa(h, acqua, bosco, strade, palude, luoghi, lato, passo, fiume_pts, emissario_pts, cartella_font, percorso):
    """La mappa di carta: pergamena, rilievo a tratteggio leggero, fiume a inchiostro, alberelli, nomi a mano."""
    L = 1600
    k = L / lato
    rng = np.random.default_rng(7)
    def giu(a): return np.array(Image.fromarray(a.astype(np.float32)).resize((L, L), Image.BILINEAR))
    hh = giu(h); sh = ombra(hh, lato / L, 2.2)
    # pergamena
    fibra = ndimage.gaussian_filter(rng.standard_normal((L, L)), 2) * 6 + ndimage.gaussian_filter(rng.standard_normal((L, L)), 40) * 60
    base = np.array([226, 208, 168.0]) + fibra[..., None] * np.array([1, 0.9, 0.7])
    yy, xx = np.mgrid[0:L, 0:L]
    bordo = np.minimum.reduce([xx, yy, L - 1 - xx, L - 1 - yy]) / L
    base *= (0.72 + 0.28 * lisca(bordo / 0.12))[..., None]
    inchiostro = np.array([58, 40, 26.0])
    # rilievo: ombre leggere dei monti e curve di livello ogni 40 m
    rilievo = (1 - sh) * lisca((hh - 80) / 200)
    c = base * (1 - 0.45 * rilievo[..., None])
    livelli = np.abs(((hh + 20) % 40) - 20) < 0.9 * (1 + np.hypot(*np.gradient(hh)))
    c = np.where((livelli & (hh > 60))[..., None], c * 0.86 + inchiostro * 0.14, c)
    # acqua: velatura azzurrina con bordo a inchiostro
    a = giu(acqua)
    c = c * (1 - 0.35 * a[..., None]) + np.array([120, 150, 160.0]) * 0.35 * a[..., None]
    riva = (a > 0.35) & (ndimage.binary_dilation(a <= 0.35, iterations=2))
    c[riva] = c[riva] * 0.3 + inchiostro * 0.7
    # palude: trattini orizzontali
    pm = giu(palude) > 0.4
    tr = pm & ((yy % 9) < 1) & ((xx // 14 + yy // 9) % 2 == 0)
    c[tr] = c[tr] * 0.4 + inchiostro * 0.6
    img = Image.fromarray(np.clip(c, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(img)
    # strade a tratteggio
    sm = giu(strade) > 0.5
    sy, sx = np.nonzero(sm & (((xx + yy) // 6) % 2 == 0))
    for x0, y0 in zip(sx[::2], sy[::2]):
        d.point((int(x0), int(y0)), fill=(90, 62, 40))
    # alberelli nel bosco
    bm = giu(bosco)
    for _ in range(16000):
        x0, y0 = rng.integers(10, L - 10, 2)
        if bm[y0, x0] > 0.3 and rng.random() < bm[y0, x0] * 0.8:
            s = rng.integers(5, 9)
            d.ellipse((x0 - s, y0 - s * 1.2, x0 + s, y0 + s * 0.4), outline=(64, 70, 40), width=1, fill=(150, 150, 100))
            d.line((x0, y0 + s * 0.4, x0, y0 + s), fill=(64, 50, 30))
    # fiume: linea a inchiostro piu spessa verso il lago
    pts = [(x * k, (lato - y) * k) for x, y in fiume_pts]
    for i in range(len(pts) - 1):
        d.line((pts[i], pts[i + 1]), fill=(46, 62, 78), width=int(1 + 4 * i / len(pts)))
    pts = [(x * k, (lato - y) * k) for x, y in emissario_pts]
    d.line(pts, fill=(46, 62, 78), width=2)
    # simboli e nomi
    def font(n, s):
        for p in n:
            f = os.path.join(cartella_font, p)
            if os.path.exists(f):
                return ImageFont.truetype(f, s)
        return ImageFont.load_default()
    titolo = font(["fontsource-cinzel-5.3.0/files/cinzel-latin-700-normal.woff", "Cinzel-Bold.ttf"], 56)
    nomef = font(["fontsource-eb-garamond-5.3.0/files/eb-garamond-latin-400-italic.woff", "EBGaramond-Italic.ttf"], 26)
    piccolo = font(["fontsource-eb-garamond-5.3.0/files/eb-garamond-latin-400-italic.woff", "EBGaramond-Italic.ttf"], 21)
    def scrivi(testo, x, y, f=nomef, dx=0, dy=0):
        X0, Y0 = x * k + dx, (lato - y) * k + dy
        w = d.textlength(testo, font=f)
        d.text((X0 - w / 2, Y0), testo, font=f, fill=(52, 34, 22), stroke_width=3, stroke_fill=(228, 212, 174))
    def croce(x, y, s=7):
        X0, Y0 = x * k, (lato - y) * k
        d.line((X0 - s, Y0 - s, X0 + s, Y0 + s), fill=(120, 30, 24), width=2)
        d.line((X0 - s, Y0 + s, X0 + s, Y0 - s), fill=(120, 30, 24), width=2)
    vx, vy = luoghi["Piazza del villaggio"]
    for i in range(16):   # casette del villaggio
        ang = rng.random() * 6.28; r_ = rng.random() ** 0.7 * 110
        X0, Y0 = (vx + math.cos(ang) * r_) * k, (lato - vy - math.sin(ang) * r_) * k
        d.rectangle((X0 - 4, Y0 - 3, X0 + 4, Y0 + 3), fill=(120, 80, 50), outline=(52, 34, 22))
    tx, ty = luoghi["Tempio del Cuore"]
    X0, Y0 = tx * k, (lato - ty) * k
    d.polygon([(X0, Y0 - 12), (X0 - 8, Y0), (X0 + 8, Y0)], fill=(140, 40, 30), outline=(52, 34, 22))
    scrivi("Val d'Orso", lato / 2, lato * 0.97, titolo)
    scrivi("Villaggio", vx, vy, nomef, dy=24)
    scrivi("Collina dell'Orso", *luoghi["Collina dell'Orso"], nomef, dy=-34)
    ox, oy = luoghi["Collina dell'Orso"]; d.ellipse((ox * k - 5, (lato - oy) * k - 5, ox * k + 5, (lato - oy) * k + 5), outline=(52, 34, 22), width=2)
    scrivi("Lago", *luoghi["Lago"], nomef)
    scrivi("Palude", *luoghi["Palude"], piccolo, dy=-6)
    scrivi("Rovine degli Antichi", *luoghi["Rovine degli Antichi"], piccolo, dy=12)
    croce(*luoghi["Rovine degli Antichi"])
    scrivi("Grotta", *luoghi["Grotta est"], piccolo, dy=10); croce(*luoghi["Grotta est"])
    scrivi("Grotta (frana)", *luoghi["Grotta nord"], piccolo, dy=10); croce(*luoghi["Grotta nord"])
    scrivi("Ponte", *luoghi["Ponte"], piccolo, dx=34)
    if "Cascata" in luoghi:
        scrivi("Cascata", *luoghi["Cascata"], piccolo, dx=-50, dy=-8)
    scrivi("Mulino", *luoghi["Mulino"], piccolo, dx=-40)
    scrivi("Pascoli", *luoghi["Pascoli"], piccolo)
    scrivi("Cimitero", *luoghi["Cimitero"], piccolo, dy=6)
    scrivi("Bosco Grande", *P_bosco(lato), nomef)
    scrivi("Boschetto", *luoghi["Boschetto"], piccolo)
    scrivi("Passo per Aurelia", *luoghi["Posto di guardia del passo"], piccolo, dx=-90, dy=-24)
    # rosa dei venti (solo il nord): sulla mappa non c'e "tu sei qui"
    rx, ry = L - 120, 150
    d.polygon([(rx, ry - 50), (rx - 10, ry), (rx, ry - 8), (rx + 10, ry)], fill=(52, 34, 22))
    d.text((rx - 7, ry - 82), "N", font=nomef, fill=(52, 34, 22))
    # scala in metri
    m500 = 500 * k
    d.line((80, L - 90, 80 + m500, L - 90), fill=(52, 34, 22), width=3)
    for t in (0, 0.5, 1):
        d.line((80 + m500 * t, L - 98, 80 + m500 * t, L - 82), fill=(52, 34, 22), width=2)
    d.text((80, L - 76), "500 passi" if False else "500 metri", font=piccolo, fill=(52, 34, 22))
    img = img.filter(ImageFilter.UnsharpMask(1, 60, 2))
    img.save(percorso)


def P_bosco(lato):
    margine = 0.14 * lato if lato <= 2600 else 0.16 * lato
    conca = lato - 2 * margine
    return (margine + 0.17 * conca, margine + (1 - 0.22) * conca)


if __name__ == "__main__":
    argomenti = [a for a in sys.argv[1:]]
    if argomenti[:1] == ["--solo-maschere"]:
        # python valle.py --solo-maschere <cartella Strati> <file di uscita>: rifà solo l'immagine delle maschere.
        # (07/10) con un terzo argomento rifà anche la seconda (bagnato): ... <file di uscita> <file di uscita 2>
        impacchetta(argomenti[1], argomenti[2])
        if len(argomenti) > 3 and os.path.exists(os.path.join(argomenti[1], "Bagnato.png")):
            impacchetta2(argomenti[1], argomenti[3])
        sys.exit(0)
    cartella_font = "."
    if "--vertici" in argomenti:
        i = argomenti.index("--vertici"); N = int(argomenti[i + 1]); del argomenti[i:i + 2]
    if "--fonts" in argomenti:
        i = argomenti.index("--fonts"); cartella_font = argomenti[i + 1]; del argomenti[i:i + 2]
    uscita = argomenti[0] if argomenti else "."
    lati = [int(a) for a in argomenti[1:]] or [3000]
    for lato in lati:
        valle(lato, os.path.join(uscita, f"valle_{lato}"), cartella_font)
