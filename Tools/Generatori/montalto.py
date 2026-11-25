# Valdorso - Generatore delle Terre di Montalto, la seconda mappa (prova del 24/11/2026). Scritto da Claude.
# Deciso da Fra il 24/11: oltre il passo di sud-est ci sono le terre di confine del ducato di Montalto; mappa piu' grande
# della valle; ci si arriva passando dal passo; dentro terreno, natura, strade e un borgo.
# Fa le stesse cose di valle.py (di cui riusa gli attrezzi), nello stesso formato:
#   Altezze_<lato>.png   altezze a 16 bit per il Landscape di Unreal
#   Strati_<lato>/*.png  maschere a 8 bit (prato, roccia, terra, ghiaia, neve, bosco, strade, acqua, campi, villaggio...)
#   T_ValleMaschere*.png le maschere impacchettate come quelle della valle (cosi' vale lo stesso materiale)
#   Luoghi_<lato>.json   i luoghi in metri (x est, y nord, origine a sud-ovest) e in Unreal, i fiumi, le strade
#   Anteprima_<lato>.png la terra in rilievo vista dall'alto
# Uso: python montalto.py <cartella_di_uscita> [4000] [--vertici 1009|2017|4033]
#
# Com'e' fatta (4 x 4 km, 16 km quadrati; la valle e' 3 x 3):
#   - a nord-ovest i monti di Val d'Orso: da li' scende la GOLA DEL PASSO (si arriva dall'angolo in alto a sinistra);
#   - i monti chiudono tutto il nord e l'ovest; verso sud-est la terra scende a colline larghe e a una piana;
#   - il FIUME nasce nei monti a nord, scende a sud-est, fa un'ansa intorno al borgo ed esce a sud; un TORRENTE
#     scende dalla gola del passo e lo raggiunge a monte del borgo;
#   - la STRADA DEL RE scende dal passo a tornanti, passa la DOGANA ai piedi della gola, il PONTE di pietra, il BORGO,
#     ed esce a sud-est verso Aurelia; un ramo sale a est alla ROCCA del castellano del duca, su un colle di roccia;
#   - colline a sud con i CAMPI e le VIGNE (terra lavorata), bosco fitto sui versanti a nord e a est;
#   - un LAGHETTO del mulino a monte del borgo, una CAVA di pietra a ovest, una TORRE DIROCCATA sul crinale a nord-est,
#     un'EDICOLA al bivio, i PASCOLI alti sotto i monti.
# I nomi sono provvisori: quelli veri li decide la chat delle Cronache.

import json
import math
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import valle as V  # noqa: E402  (gli attrezzi: lisca, distanza_da_linea, curva, frattale, erosione_termica, anteprima...)

N = 4033
QUOTA_MAX = 800.0
SEME = 2411               # il giorno della prova

Q_PASSO = 300.0           # quota della strada all'uscita della gola (in alto a sinistra)
Q_PIANA = 70.0            # quota della piana del borgo
Q_ROCCA = 165.0           # la cima del colle della rocca
# l'ovale delle mura della citta' bassa (la linea degli spigoli di dentro): 0,97 del bordo della citta' (430 x 400 m)
MURA_A, MURA_B = 430.0 * 0.97, 400.0 * 0.97


def montalto(lato, uscita):
    rng = np.random.default_rng(SEME)
    n = N
    passo = lato / (n - 1)
    asse = (np.arange(n) * passo).astype(np.float32)
    X, Y = np.meshgrid(asse, asse[::-1])        # riga 0 = nord, Y cresce verso nord
    lisca = V.lisca

    def P(u, v):
        """Da coordinate della mappa (0..1; u = est, v = 0 a nord) a metri (x est, y nord)."""
        return (u * lato, (1 - v) * lato)

    # ------------------------------------------------------------------ i luoghi (frazioni della mappa)
    luoghi = {
        "Uscita del passo": P(0.085, 0.075),
        "Pietralta": P(0.235, 0.265),
        "Edicola del bivio": P(0.345, 0.385),
        "Ponte di pietra": P(0.445, 0.470),
        "Città": P(0.570, 0.520),
        "Porto": P(0.565, 0.617),
        "Granaio della Corona": P(0.610, 0.575),
        # (24/11) i posti di Montalto che esistono nelle Cronache
        "Porta nord": P(0.565, 0.425),
        "Riva dei magazzini della lana": P(0.470, 0.494),
        "Cimitero vecchio": P(0.578, 0.385),
        "Ponte del fiume minore": P(0.682, 0.592),
        "Mulino": P(0.470, 0.415),
        "Rocca": P(0.5525, 0.4825),
        "Cava": P(0.190, 0.560),
        "Torre diroccata": P(0.700, 0.165),
        "Vigne": P(0.735, 0.430),
        "Pascoli alti": P(0.380, 0.180),
        "Strada per Aurelia": P(0.940, 0.905),
    }

    # ------------------------------------------------------------------ le linee: fiume, torrente, strade
    fiume_pts = V.curva([P(0.560, 0.000), P(0.540, 0.090), P(0.500, 0.190), P(0.505, 0.290), P(0.470, 0.380),
                         P(0.440, 0.470), P(0.455, 0.560), P(0.530, 0.625), P(0.610, 0.640), P(0.650, 0.720),
                         P(0.625, 0.830), P(0.660, 0.930), P(0.650, 1.000)], 20)
    torrente_pts = V.curva([P(0.110, 0.060), P(0.160, 0.150), P(0.200, 0.215), P(0.270, 0.270), P(0.340, 0.330),
                            P(0.410, 0.365), P(0.468, 0.385)], 16)
    # (25/11) IL PONTE della chat 3D e' lungo 110 m e dritto: deve attraversare l'Aure in squadra. Il centro sta sull'asse
    # del fiume, l'asse del ponte e' perpendicolare alla corrente; la strada arriva e riparte in linea per 130 m.
    t_fp = V.frazione_lungo(fiume_pts, luoghi["Ponte di pietra"])
    PB = np.array(V.punto_a(fiume_pts, t_fp))
    corr = np.array(V.punto_a(fiume_pts, min(t_fp + 0.012, 1.0))) - np.array(V.punto_a(fiume_pts, max(t_fp - 0.012, 0.0)))
    corr = corr / np.linalg.norm(corr)                       # il verso della corrente (verso valle)
    asse_p = np.array([corr[1], -corr[0]])
    if asse_p[0] < 0:
        asse_p = -asse_p                                     # l'asse del ponte, da ovest (il passo) a est (la citta')
    luoghi["Ponte di pietra"] = (float(PB[0]), float(PB[1]))
    sul_ponte = [tuple(PB + asse_p * m_) for m_ in (-150.0, -75.0, 0.0, 75.0, 150.0)]
    # (25/11) IL PORTO della chat 3D e' una riva dritta di 200 m (Cronache: «il ponte» con la riva dei magazzini della lana).
    # Sta sulla riva della citta', a valle del ponte: da 40 a 240 m dal centro del ponte lungo il fiume, dove l'Aure e'
    # quasi dritto. Sistema del porto (quello della chat 3D): X lungo il fiume (verso monte), Y verso la citta', l'acqua a -Y.
    lung_f = sum(math.dist(fiume_pts[i], fiume_pts[i + 1]) for i in range(len(fiume_pts) - 1))
    pa = np.array(V.punto_a(fiume_pts, t_fp + 40.0 / lung_f))
    pb = np.array(V.punto_a(fiume_pts, t_fp + 240.0 / lung_f))
    t_porto = t_fp + 140.0 / lung_f
    px_ = (pa - pb) / np.linalg.norm(pa - pb)                # +X del porto: verso monte
    py_ = np.array([-px_[1], px_[0]])
    if py_[0] < 0:
        py_ = -py_                                           # +Y del porto: verso la citta' (est)
    # la faccia della banchina (Y -5,45) sta a 40 m dalla corda del fiume; X 140 e' l'estremo verso il ponte
    PO = pa + py_ * 45.45 - px_ * 140.0
    luoghi["Porto"] = (float(PO[0] + px_[0] * 40.0 + py_[0] * 10.0), float(PO[1] + px_[1] * 40.0 + py_[1] * 10.0))
    luoghi["Riva dei magazzini della lana"] = luoghi["Porto"]
    # (24/11) il secondo fiume: la citta' sta sul colle TRA i due fiumi (Cronache, cap. VII)
    minore_pts = V.curva([P(1.000, 0.400), P(0.900, 0.440), P(0.800, 0.490), P(0.725, 0.535), P(0.680, 0.590),
                          P(0.628, 0.642)], 16)
    strada_re = V.curva([P(0.060, 0.050), P(0.085, 0.075), P(0.120, 0.115), P(0.100, 0.150), P(0.150, 0.175),
                         P(0.135, 0.215), P(0.190, 0.235), P(0.235, 0.265), P(0.290, 0.330), P(0.345, 0.385),
                         P(0.385, 0.447)] + sul_ponte + [P(0.492, 0.505), P(0.515, 0.573), P(0.570, 0.595),
                         P(0.630, 0.578), P(0.682, 0.592), P(0.740, 0.690), P(0.820, 0.800), P(0.940, 0.905),
                         P(1.000, 0.960)], 16)
    # la salita arriva nel piazzale davanti alla porta della rocca (lato sud, 25 m a ovest del centro) e ci entra dritta
    strada_rocca = V.curva([P(0.570, 0.595), P(0.602, 0.562), P(0.600, 0.528), P(0.578, 0.512), P(0.556, 0.512),
                            P(0.54625, 0.5030), P(0.54625, 0.4930)], 16)
    strada_cava = V.curva([P(0.345, 0.385), P(0.300, 0.450), P(0.240, 0.510), P(0.190, 0.560)], 16)
    strada_vigne = V.curva([P(0.630, 0.578), P(0.650, 0.520), P(0.690, 0.480), P(0.735, 0.430)], 16)
    strada_torre = V.curva([P(0.690, 0.480), P(0.680, 0.380), P(0.690, 0.260), P(0.700, 0.165)], 16)
    # (25/11, mura della chat 3D) la Porta nord sta SULL'OVALE delle mura (0,97 del bordo della citta') e la Strada del nord
    # esce in squadra per 40 m, altrimenti non infila l'arco (5 m di luce)
    cx_m, cy_m = luoghi["Città"]
    f_pn = math.atan2((luoghi["Porta nord"][1] - cy_m) / MURA_B, (luoghi["Porta nord"][0] - cx_m) / MURA_A)
    PN = np.array([cx_m + MURA_A * math.cos(f_pn), cy_m + MURA_B * math.sin(f_pn)])
    n_pn = np.array([math.cos(f_pn) / MURA_A, math.sin(f_pn) / MURA_B])
    n_pn = n_pn / np.linalg.norm(n_pn)                       # verso fuori
    luoghi["Porta nord"] = (float(PN[0]), float(PN[1]))
    strada_nord = V.curva([tuple(PN), tuple(PN + n_pn * 20.0), tuple(PN + n_pn * 40.0), P(0.578, 0.385), P(0.620, 0.360),
                           P(0.680, 0.380)], 16)
    # (25/11) il cimitero vecchio della chat 3D (recinto 40 x 30 m, il cancello sul lato lungo verso la strada): sta DI
    # FIANCO alla Strada del nord, dalla parte di fuori della curva, con il centro a 27 m dalla strada
    pc_ = np.array(P(0.578, 0.385))
    i_c = int(np.argmin([math.dist(q_, pc_) for q_ in strada_nord]))
    t_c = np.array(strada_nord[min(i_c + 3, len(strada_nord) - 1)]) - np.array(strada_nord[max(i_c - 3, 0)])
    t_c = t_c / np.linalg.norm(t_c)
    n_c = np.array([-t_c[1], t_c[0]])
    if n_c[0] > 0:
        n_c = -n_c                                           # verso ovest
    CIM = np.array(strada_nord[i_c]) + n_c * 27.0
    luoghi["Cimitero vecchio"] = (float(CIM[0]), float(CIM[1]))
    # (25/11) due vie di citta' per le case della chat 3D: la via alta, un arco sul ripiano della citta' alta a sud della
    # rocca, e la via bassa, che gira sul ripiano basso da est a nord fino a ovest (dove non passa la Strada del Re)
    cx_c, cy_c = luoghi["Città"]

    def arco(cx_, cy_, rx_, ry_, da_gradi, a_gradi, passo_gradi=8.0):
        quanti = int(abs(a_gradi - da_gradi) / passo_gradi) + 1
        return [(cx_ + rx_ * math.cos(math.radians(g_)), cy_ + ry_ * math.sin(math.radians(g_)))
                for g_ in np.linspace(da_gradi, a_gradi, quanti)]
    via_alta = V.curva(arco(cx_c - 55.0, cy_c + 70.0, 190.0, 144.0, 200.0, 340.0), 6)
    via_bassa = V.curva(arco(cx_c, cy_c, 335.0, 312.0, -35.0, 205.0), 6)
    strade_pts = {"Strada del Re": strada_re, "Strada del nord": strada_nord, "Salita alla rocca": strada_rocca, "Via della cava": strada_cava,
                  "Via delle vigne": strada_vigne, "Sentiero della torre": strada_torre, "Via alta": via_alta,
                  "Via bassa": via_bassa}

    # ------------------------------------------------------------------ la forma grande
    u = X / lato
    v = 1 - Y / lato
    # la terra scende da nord-ovest (monti di Val d'Orso) a sud-est (la piana)
    discesa = np.clip(1.0 - (0.55 * u + 0.62 * v), -0.3, 1.0)
    base = Q_PIANA + 40.0 + 260.0 * lisca((discesa - 0.05) / 0.85) ** 1.25
    # i monti: una fascia alta a nord e a ovest, frastagliata
    rum_grande = V.frattale(rng, n, 5, 4, 0.55)
    orlo_n = lisca((0.20 + 0.05 * rum_grande - v) / 0.20)
    orlo_o = lisca((0.15 + 0.05 * rum_grande - u) / 0.17)
    monti = np.maximum(orlo_n, orlo_o * (1 - 0.55 * lisca((v - 0.55) / 0.35)))
    creste = np.abs(V.frattale(rng, n, 9, 6, 0.55))
    h = base + monti * (230.0 + 250.0 * creste)
    # colline larghe nella parte bassa
    colline = V.frattale(rng, n, 7, 5, 0.5)
    bx0, by0 = luoghi["Città"]
    piana = lisca(1.25 - np.hypot((X - bx0) / 1500.0, (Y - (by0 - 150.0)) / 1250.0))     # la conca lavorata intorno al borgo
    h += (1 - monti) * (1 - 0.72 * piana) * (48.0 * colline + 16.0 * V.frattale(rng, n, 22, 4, 0.5))
    h -= 22.0 * piana * (1 - monti)
    # i bordi est e sud si rialzano in colline boscose: la terra finisce senza un muro
    bordo_e = lisca((u - 0.90) / 0.10)
    bordo_s = lisca((v - 0.92) / 0.08)
    h += np.maximum(bordo_e, bordo_s) * (55.0 + 30.0 * np.abs(colline))
    del rum_grande, creste, orlo_n, orlo_o

    # il colle della rocca: un dosso di roccia, ripido a ovest (verso il fiume), dolce a est
    rx, ry = luoghi["Vigne"]                     # il colle delle vigne (nella seconda bozza era il colle della rocca)
    dr = np.hypot((X - rx) / 230.0, (Y - ry) / 190.0)
    colle = lisca(1 - dr) ** 1.4
    h = h * (1 - 0.85 * colle) + (128.0 - 6.0 * dr) * 0.85 * colle + 0.15 * colle * h
    # il crinale della torre diroccata
    tx, ty = luoghi["Torre diroccata"]
    h += 55.0 * lisca(1 - np.hypot((X - tx) / 260.0, (Y - ty) / 150.0))

    # ------------------------------------------------------------------ il fiume e il torrente scavano
    df, tf = V.distanza_da_linea(X, Y, fiume_pts)
    # quota dell'acqua lungo il fiume: dai monti (190 m) alla piana, poi quasi piatto fino all'uscita
    # (24/11 notte, chiesto da Fra) navigabile anche a NORD, tra i monti: un giorno le navi andranno a una mappa oltre
    # le montagne. Un fiume da navi non ha salti: il pelo scende di soli 6 m in tutta la mappa.
    def qf(t):
        return 58.0 - 6.0 * t
    q_fiume = qf(tf)
    # (24/11, chiesto da Fra) il fiume e' grande: e' quello che piu' a valle taglia la capitale, e ci passeranno le navi.
    # Mezza larghezza: 8 m nei monti, 40 m dalla citta' in giu' (80 m da riva a riva); profondo 7 m dove e' navigabile
    # (dalla citta' in giu'), 2,5 m a monte.
    navigabile = np.ones_like(tf)
    largo_f = 34.0 + 6.0 * lisca((tf - 0.15) / 0.25)        # 68 m tra i monti, 80 m dalla piana in giu'
    fondo_f = 12.0 * navigabile                             # (25/11, chiesto da Fra) piu' profondo: 12 m
    in_piano = lisca((tf - 0.22) / 0.3)
    valle_f = lisca(1 - df / (520.0 + 100.0 * in_piano))
    h = h * (1 - valle_f) + np.minimum(h, q_fiume + 2.0 + df * (0.50 - 0.43 * in_piano)) * valle_f
    alveo_f = lisca((largo_f + 6.0 - df) / 12.0)            # sponde che scendono in 12 m
    h = h * (1 - alveo_f) + (q_fiume - fondo_f) * alveo_f

    dt, tt = V.distanza_da_linea(X, Y, torrente_pts)
    fx_, fy_ = V.punto_a(torrente_pts, 1.0)
    q_arrivo = qf(0.36)                                       # la quota del fiume alla confluenza
    q_torr = (Q_PASSO - 14.0) * (1 - tt) ** 1.25 + q_arrivo * (1 - (1 - tt) ** 1.25)
    largo_t = 3.5 + 2.5 * tt
    # nella gola (in alto) il torrente incide stretto e profondo, in basso la valletta si apre
    gola = lisca(1 - tt / 0.35)
    valle_t = lisca(1 - dt / (300.0 - 60.0 * gola))
    h = h * (1 - valle_t) + np.minimum(h, q_torr + 2.5 + dt * (0.42 * gola + 0.13)) * valle_t
    alveo_t = lisca(1 - dt / largo_t)
    h = h * (1 - alveo_t) + (q_torr - 0.9) * alveo_t

    # il fiume minore, da est: basso e largo una ventina di metri, si getta nel grande sotto la citta'
    dm, tm = V.distanza_da_linea(X, Y, minore_pts)
    tm = np.clip(tm, 0.0, 1.0)
    q_min = 104.0 * (1 - tm) ** 1.6 + qf(0.66) * (1 - (1 - tm) ** 1.6)
    valle_m = lisca(1 - dm / 260.0)
    h = h * (1 - valle_m) + np.minimum(h, q_min + 2.0 + dm * 0.10) * valle_m
    largo_m = 7.0 + 4.0 * tm
    alveo_m = lisca((largo_m + 4.0 - dm) / 8.0)
    h = h * (1 - alveo_m) + (q_min - 1.8) * alveo_m

    # il laghetto del mulino: il fiume si allarga a monte del borgo
    mx, my = luoghi["Mulino"]
    rl = np.hypot((X - (mx + 55.0)) / 95.0, (Y - (my + 20.0)) / 60.0)
    pelo_laghetto = qf(0.30)
    laghetto = lisca((1.0 - rl) / 0.25) * 0.0              # (24/11) con il fiume grande il laghetto non serve piu'
    h = h * (1 - laghetto) + (pelo_laghetto - 2.2) * laghetto
    sponda = lisca((1.55 - rl) / 0.55) * 0.0
    h = h * (1 - sponda) + np.minimum(h, pelo_laghetto + 0.6 + (rl - 1.0) * 9.0) * sponda

    # ------------------------------------------------------------------ i piani dove si costruisce
    piano = np.zeros_like(h)

    def spiana(nome, rx_m, ry_m, quota=None, morbido=0.45):
        nonlocal h, piano
        cx, cy = luoghi[nome]
        d = np.hypot((X - cx) / rx_m, (Y - cy) / ry_m)
        m = lisca((1.0 - d) / morbido)
        if quota is None:
            dentro = d < 0.6
            quota = float(np.median(h[dentro])) if dentro.any() else float(h.mean())
        h = h * (1 - m) + quota * m
        piano = np.maximum(piano, m)
        return quota

    # il borgo sta su un terrazzo una decina di metri sopra il fiume (al riparo dalle piene), con i bordi morbidi
    # (24/11 notte) Cronache, cap. VII: «una citta' su un colle tra due fiumi, con la rocca in cima».
    # Tre ripiani: la citta' bassa (12 m sull'acqua, col granaio e le porte), la citta' alta (+20 m), la rocca del duca (+18 m).
    q_acqua_citta = qf(0.55)
    ccx, ccy = luoghi["Città"]
    # non un bersaglio a cerchi: i ripiani sono spostati verso nord-ovest, cosi' la rocca sta sul ciglio alto che guarda
    # il fiume grande e il ponte, e la citta' scende dolce verso sud (il porto) e verso est (il fiume minore)
    storto = 1.0 + 0.16 * colline + 0.10 * V.frattale(rng, n, 16, 3, 0.5)
    dc = np.hypot((X - ccx) / 430.0, (Y - ccy) / 400.0) * storto
    storto2 = 1.0 + 0.24 * V.frattale(rng, n, 11, 3, 0.5) - 0.10 * colline
    d_alta = np.hypot((X - (ccx - 55.0)) / 265.0, (Y - (ccy + 70.0)) / 200.0) * storto2
    d_rocca = np.hypot((X - (ccx - 70.0)) / 125.0, (Y - (ccy + 150.0)) / 100.0)
    gradoni = 12.0 + 22.0 * lisca((1.0 - d_alta) / 0.30) + 26.0 * lisca((1.0 - d_rocca) / 0.25)
    m_citta = lisca((1.0 - dc) / 0.12)
    intorno = lisca((2.1 - dc) / 0.5)
    h = h * (1 - intorno) + np.minimum(h, q_acqua_citta + 12.0 + np.maximum(dc - 1.0, 0) * 415.0 * 0.07) * intorno
    h = h * (1 - m_citta) + (q_acqua_citta + gradoni) * m_citta
    piano = np.maximum(piano, m_citta)
    q_borgo = q_acqua_citta + 12.0
    q_alta = q_acqua_citta + 34.0
    q_rocca = q_acqua_citta + 60.0
    q_porto = qf(t_porto) - 0.35 + 3.0   # (secondo controllo) 3 m sull'acqua VERA di Unreal, che sta 35 cm sotto qf
    q_porto_vecchio = qf(t_porto) + 3.0                              # la banchina: 3 m sull'acqua (il piano lo fa il blocco del porto)
    h = h * (1 - alveo_f) + (q_fiume - fondo_f) * alveo_f     # la citta' e il porto non riempiono i fiumi
    h = h * (1 - alveo_m) + (q_min - 1.8) * alveo_m
    piano = piano * (1 - alveo_f) * (1 - alveo_m)
    q_dogana = spiana("Pietralta", 150.0, 105.0, None, 0.5)      # il borgo sotto la gola: una ventina di case
    q_cava = spiana("Cava", 80.0, 60.0)
    spiana("Torre diroccata", 30.0, 30.0)
    spiana("Edicola del bivio", 18.0, 18.0)
    spiana("Cimitero vecchio", 45.0, 35.0)
    # la cava: una parete tagliata nel monte a ovest del piazzale
    cx, cy = luoghi["Cava"]
    taglio = lisca(1 - np.hypot((X - (cx - 95.0)) / 70.0, (Y - cy) / 110.0))
    h += 34.0 * taglio * lisca((cx - 40.0 - X) / 30.0)

    # la Strada del Re scende dal passo con una pendenza che un carro regge: la sua quota e' decisa lungo il percorso
    # (dal passo al ponte in discesa costante, poi segue la terra), e il terreno le si adatta per una fascia larga
    dsr, tsr = V.distanza_da_linea(X, Y, strada_re)
    # la rampa finisce alla testa ovest del ponte (150 m prima del centro): da li' la strada e' in piano fino in citta'
    t_ponte = V.frazione_lungo(strada_re, sul_ponte[0])
    q_ponte = qf(t_fp) + 12.0                                # il piano stradale del ponte: 12 m sull'acqua
    q_rampa = Q_PASSO + (q_ponte - Q_PASSO) * np.clip(tsr / t_ponte, 0, 1)      # (24/11) pendenza costante
    salita = lisca(1 - (tsr - t_ponte) / 0.06)             # solo nel tratto in discesa (oltre il ponte la strada segue la terra)
    fascia = lisca(1 - dsr / 60.0) * salita
    h = h * (1 - fascia) + (q_rampa + np.maximum(dsr - 7.0, 0) * 0.30 * np.sign(h - q_rampa)) * fascia
    del dsr, tsr, fascia, salita, q_rampa

    # ------------------------------------------------------------------ il dettaglio e l'erosione
    dett = V.frattale(rng, n, 60, 5, 0.5)
    gy_, gx_ = np.gradient(h, passo)
    pend0 = np.hypot(gx_, gy_)
    h += dett * (1.2 + 9.0 * lisca(pend0 / 0.6)) * (1 - piano) * (1 - alveo_f) * (1 - alveo_t) * (1 - alveo_m) * (1 - laghetto)
    del dett, gx_, gy_, pend0
    h = V.erosione_termica(h.astype(np.float32), passo, giri=40 if n > 2100 else 60, pendenza_max=0.95)

    # ------------------------------------------------------------------ le strade si adagiano
    strade = np.zeros_like(h)
    larghezze = {"Strada del Re": 4.5, "Strada del nord": 3.5, "Salita alla rocca": 3.5, "Via della cava": 3.5, "Via delle vigne": 3.0,
                 "Sentiero della torre": 2.0, "Via alta": 3.0, "Via bassa": 3.0}
    acqua_prima = np.maximum(np.maximum(alveo_f, alveo_t), alveo_m)
    for nome, pts in strade_pts.items():
        ds, ts = V.distanza_da_linea(X, Y, pts)
        s = lisca(1 - ds / larghezze[nome])
        # (24/11) il profilo: le quote lungo la strada, lisciate su ~70 m; sui ponti si tira dritto da riva a riva
        fitti = V.curva(pts, 1) if False else pts
        seg = [0.0]
        for i in range(len(fitti) - 1):
            seg.append(seg[-1] + math.dist(fitti[i], fitti[i + 1]))
        tot = seg[-1]
        m_camp = np.arange(0.0, tot + 2.0, 2.0)
        px = np.interp(m_camp, seg, [q[0] for q in fitti]); py = np.interp(m_camp, seg, [q[1] for q in fitti])
        rr = np.clip(np.round((lato - py) / passo).astype(int), 0, n - 1)
        cc = np.clip(np.round(px / passo).astype(int), 0, n - 1)
        qz = h[rr, cc].astype(np.float64)
        bagn = acqua_prima[rr, cc] > 0.15
        if bagn.any() and (~bagn).any():
            qz[bagn] = np.interp(m_camp[bagn], m_camp[~bagn], qz[~bagn])
        q_capo, q_coda = qz[0], qz[-1]
        qz = ndimage.gaussian_filter1d(qz, 35.0, mode="nearest")
        if nome == "Salita alla rocca":
            # (25/11) una rampa sola a pendenza costante dalla citta' bassa al piazzale della porta; gli ultimi 40 m
            # in piano alla quota del cortile (la chat 3D vuole il terreno a Z 0 davanti alla porta)
            # (secondo controllo) NON una rampa unica: farebbe un terrapieno di 15 m in mezzo alla citta'. La strada segue
            # i ripiani (lisciata su ~200 m) e solo negli ultimi 300 m sale al piazzale: circa il 10%, su una rampa murata.
            qz = ndimage.gaussian_filter1d(qz, 50.0, mode="nearest")
            w_r = np.clip((m_camp - (tot - 300.0)) / 260.0, 0.0, 1.0)
            w_r = w_r * w_r * (3 - 2 * w_r)
            qz = qz * (1 - w_r) + q_rocca * w_r
            q_coda = q_rocca
        # i capi restano alla quota che trovano (un bivio non fa gradino con la strada da cui parte)
        qz += (q_capo - qz[0]) * np.exp(-m_camp / 50.0) + (q_coda - qz[-1]) * np.exp(-(tot - m_camp) / 50.0)
        profilo = np.interp(ts * tot, m_camp, qz).astype(np.float32)
        letto_str = lisca(1 - ds / (larghezze[nome] * 1.6))
        scarpata = lisca(1 - (ds - larghezze[nome] * 1.6) / 14.0)
        nuovo = profilo + np.clip(h - profilo, -1, 1) * np.maximum(ds - larghezze[nome] * 1.6, 0) * 0.55
        fuori_acqua = 1 - lisca(acqua_prima / 0.3)
        peso = np.maximum(letto_str, scarpata) * fuori_acqua
        h = h * (1 - peso) + np.where(np.abs(h - profilo) > np.abs(nuovo - profilo), nuovo, h) * peso
        h = h * (1 - letto_str * fuori_acqua) + profilo * letto_str * fuori_acqua
        strade = np.maximum(strade, s)
    # (25/11, secondo controllo) LA FASCIA DELLE MURA: i pezzi della chat 3D stanno tutti alla quota della citta' e reggono
    # il terreno solo da -3 a +3 m. Lungo l'ovale, per 14 m dentro e fuori, il terreno va alla quota della citta'; verso
    # l'Aure (dove fuori scende alla riva) si spiana solo dentro, e li' vanno i pezzi di riva.
    d_ov = np.hypot((X - cx_m) / MURA_A, (Y - cy_m) / MURA_B)
    dist_ov = np.abs(d_ov - 1.0) * (MURA_A + MURA_B) / 2.0
    fascia_m = lisca((16.0 - dist_ov) / 6.0)
    fascia_m = np.where((d_ov > 1.0) & (h < q_borgo - 3.0), 0.0, fascia_m)
    h = h * (1 - fascia_m) + q_borgo * fascia_m
    # davanti alla Porta nord: la soglia e' a +1,6 m (7 m fuori dalla linea), poi la strada sale al 6%
    t_pn = np.array([-n_pn[1], n_pn[0]])
    gx_ = (X - PN[0]) * t_pn[0] + (Y - PN[1]) * t_pn[1]
    gy_ = (X - PN[0]) * n_pn[0] + (Y - PN[1]) * n_pn[1]
    m_pn = lisca((15.0 - np.abs(gx_)) / 5.0) * (gy_ > 6.0) * lisca((75.0 - gy_) / 25.0)
    h = h * (1 - m_pn) + (q_borgo + 1.6 + 0.06 * np.maximum(gy_ - 8.6, 0.0)) * m_pn
    piano = np.maximum(piano, np.maximum(fascia_m * 0.6, m_pn))
    del d_ov, dist_ov, fascia_m, gx_, gy_, m_pn
    # il porto: il piazzale della banchina in piano a +3 m, la citta' dietro il muro di sponda a +12 m, e davanti alla
    # banchina il fondo scavato fino al canale (un bacino: li' il fiume piega di qualche metro verso l'altra riva)
    xp = (X - PO[0]) * px_[0] + (Y - PO[1]) * px_[1]
    yp = (X - PO[0]) * py_[0] + (Y - PO[1]) * py_[1]
    fuori_x = np.maximum(np.maximum(-60.0 - xp, xp - 140.0), 0.0)
    w_x = lisca(1 - fuori_x / 15.0)
    # (25/11, chiusura della chat 3D) verso monte il porto finisce NETTO contro il muro di fianco (faccia a X 140, il
    # muro arriva a 142): il piazzale fino a X 141, poi subito quota citta'. Verso valle resta la sfumatura di 15 m.
    w_valle = lisca(1 - np.maximum(-60.0 - xp, 0.0) / 15.0)
    # (secondo controllo) la citta' sta 9 m GIUSTI sopra la banchina, come i pezzi della chat 3D
    z_ban, z_cit, z_fon = q_porto, q_porto + 9.0, qf(t_porto) - 12.0
    # Le soglie sono scelte perche' il lato della CITTA' resti pulito (niente buco dietro i parapetti): il gradino del
    # terreno e' largo una maglia (2 m) e non sta tutto dentro un muro di 2 m, quindi un po' di terra puo' appoggiarsi
    # al piede del muro dalla parte del porto. Si sistema davvero solo con il terreno a 4033 punti.
    in_cima_rampa = (xp >= 128.0) & (yp >= 19.8)             # dove arriva la rampa dei carri: gia' quota citta'
    # i confini cadono DENTRO i pezzi pieni (il modulo della banchina va da Y -5,45 a +5; il muro di sponda da 24,75 a
    # 27): cosi' il gradino del terreno, largo una maglia (2 m), resta nascosto nella pietra. Niente strisce lasciate fuori.
    piazzale = ((yp >= -3.0) & (yp < 24.8) & (xp < 139.8)).astype(np.float32) * w_valle
    oltre_fianco = ((yp >= -3.0) & (yp < 24.8) & (xp >= 139.8)).astype(np.float32) * lisca((175.0 - xp) / 25.0)
    # oltre il fianco la riva scende all'acqua come una sponda di terra (non una parete di 20 m a picco)
    sponda_terra = np.minimum(z_cit, qf(t_porto) - 1.5 + (yp + 3.0) * 1.3)
    h = h * (1 - piazzale) + np.where(in_cima_rampa, z_cit, z_ban) * piazzale
    dietro_muro = (yp >= 24.8).astype(np.float32) * lisca((80.0 - yp) / 25.0) * w_valle * lisca((175.0 - xp) / 25.0)
    h = h * (1 - dietro_muro) + z_cit * dietro_muro
    h = h * (1 - oltre_fianco) + sponda_terra * oltre_fianco
    dietro = np.maximum(dietro_muro, oltre_fianco)
    del dietro_muro, sponda_terra
    bacino = ((yp < -3.0) & (yp >= -62.0)).astype(np.float32) * w_x
    h = h * (1 - bacino) + np.minimum(h, z_fon) * bacino
    piano = np.maximum(piano, np.maximum(piazzale, dietro * 0.5))
    acqua_porto = bacino
    del xp, yp, fuori_x, piazzale, dietro, oltre_fianco
    # il piano del cimitero: nel suo sistema X lungo la strada, Y via dalla strada; 52 x 40 m in piano
    cim_x = np.array([n_c[1], -n_c[0]])
    lx = (X - CIM[0]) * cim_x[0] + (Y - CIM[1]) * cim_x[1]
    ly = (X - CIM[0]) * n_c[0] + (Y - CIM[1]) * n_c[1]
    q_cim = float(V.campiona(h, CIM[0], CIM[1], lato)) if hasattr(V, "campiona") else float(
        h[int(round((lato - CIM[1]) / passo)), int(round(CIM[0] / passo))])
    m_cim = lisca(np.minimum(np.minimum(lx + 31.0, 31.0 - lx), np.minimum(ly + 26.0, 24.0 - ly)) / 5.0)
    h = h * (1 - m_cim) + q_cim * m_cim
    piano = np.maximum(piano, m_cim * 0.5)
    del lx, ly, m_cim
    # (25/11) il Granaio della Corona della chat 3D: girato di 180 gradi in Unreal (il fronte e la strada a sud). Nel suo
    # sistema serve in piano da X -35 a +22 e da Y -17,6 (6 m per i carri davanti al piano di carico) a +8,6.
    gx0, gy0 = luoghi["Granaio della Corona"]
    lx = -(X - gx0)
    ly = (Y - gy0)
    m_gr = lisca(np.minimum(np.minimum(lx + 43.0, 30.0 - lx), np.minimum(ly + 26.0, 16.0 - ly)) / 6.0)
    h = h * (1 - m_gr) + q_borgo * m_gr
    piano = np.maximum(piano, m_gr)
    del lx, ly, m_gr
    s_p = (X - PB[0]) * asse_p[0] + (Y - PB[1]) * asse_p[1]          # lungo il ponte (m dal centro)
    a_p = (X - PB[0]) * corr[0] + (Y - PB[1]) * corr[1]              # lungo la corrente
    teste = lisca((24.0 - np.abs(a_p)) / 8.0) * lisca((np.abs(s_p) - 42.0) / 2.0) * lisca((170.0 - np.abs(s_p)) / 40.0)
    h = h * (1 - teste) + q_ponte * teste
    piano = np.maximum(piano, teste)                         # niente bosco ne' erba alta sui piazzali del ponte
    del s_p, a_p
    # il ponte: la strada non scende nel fiume (il ponte lo mette lo script di Unreal), l'alveo resta scavato
    acqua_linea = np.clip(np.maximum(np.maximum(np.maximum(alveo_f, alveo_t), alveo_m), acqua_porto * (h < q_fiume)), 0, 1)
    letto = np.where(alveo_f >= np.maximum(alveo_t, alveo_m), q_fiume - fondo_f,
                     np.where(alveo_m > alveo_t, q_min - 1.4, q_torr - 0.8))
    h = np.where(acqua_linea > 0.5, np.minimum(h, letto), h)

    h = np.clip(h, 0, QUOTA_MAX - 1).astype(np.float32)

    # ============================================================== uscite
    os.makedirs(uscita, exist_ok=True)
    nome_f = f"{lato}"
    Image.fromarray(np.round(h / QUOTA_MAX * 65535).astype(np.uint16)).save(os.path.join(uscita, f"Altezze_{nome_f}.png"))

    gy_, gx_ = np.gradient(h, passo)
    pend = np.degrees(np.arctan(np.hypot(gx_, gy_)))
    del gx_, gy_
    acqua = np.clip(np.maximum(acqua_linea, laghetto), 0, 1)
    # bosco: versanti a nord e a est, e le colline dei bordi; niente sulla piana lavorata
    macchie = V.frattale(rng, n, 12, 4, 0.5)
    bosco_n = lisca((0.52 - v + 0.10 * macchie) / 0.16) * lisca((u - 0.16) / 0.10)
    bosco_e = lisca((u - 0.72 + 0.08 * macchie) / 0.12)
    bosco_s = lisca((v - 0.86 + 0.06 * macchie) / 0.08)
    bosco = np.clip(np.maximum(np.maximum(bosco_n, bosco_e), bosco_s) + 0.35 * lisca(macchie - 0.35), 0, 1)
    bosco = bosco * (1 - lisca(pend / 38)) * (1 - acqua) * (1 - strade) * (1 - piano) * (h < 420) \
        * (1 - lisca(1 - df / (largo_f + 20.0))) * (1 - colle * 0.8) * (1 - 0.85 * piana)
    # campi e vigne: la piana a sud del borgo e i colli esposti a sud, dove non e' ripido
    bx, by = luoghi["Città"]
    vx, vy = luoghi["Vigne"]
    campi = np.maximum(lisca(1.3 - np.hypot((X - bx) / 1500.0, (Y - (by - 150.0)) / 1250.0)),
                       lisca(1.2 - np.hypot((X - vx) / 520.0, (Y - vy) / 420.0)))
    campi = campi * (1 - lisca((pend - 13) / 9)) * (1 - acqua) * (1 - strade) * (1 - piano) * (1 - bosco) \
        * (1 - lisca(1 - df / (largo_f + 14.0))) * (1 - lisca(1 - dm / 22.0))
    def appezzamenti(angolo_gradi, largo_m, lungo_m, seme_):
        ca, sa = math.cos(math.radians(angolo_gradi)), math.sin(math.radians(angolo_gradi))
        a_ = (X * ca + Y * sa) / largo_m
        b_ = (-X * sa + Y * ca) / lungo_m
        ia, ib = np.floor(a_).astype(np.int64), np.floor(b_).astype(np.int64)
        caso = ((ia * 73856093) ^ (ib * 19349663) ^ seme_) % 1000 / 1000.0
        tipo = np.where(caso < 0.38, 0.75, np.where(caso < 0.68, 0.30, 0.0)).astype(np.float32)   # arato, stoppie, prato
        fa, fb = a_ - ia, b_ - ib
        bordo = np.minimum(np.minimum(fa, 1 - fa) * largo_m, np.minimum(fb, 1 - fb) * lungo_m)
        return tipo * lisca((bordo - 1.5) / 2.0)
    zona_campi = lisca((V.frattale(rng, n, 5, 2, 0.5) + 0.1) / 0.2)
    lavorato = appezzamenti(24.0, 55.0, 150.0, 911) * zona_campi + appezzamenti(-52.0, 48.0, 170.0, 352) * (1 - zona_campi)
    roccia = lisca((pend - 30) / 12)
    neve = lisca((h - 560 - 40 * V.frattale(rng, n, 30, 3)) / 50)
    ghiaia = np.clip(lisca(1 - df / (largo_f + 7)) - alveo_f, 0, 1) * lisca((40 - pend) / 10) \
        + np.clip(lisca(1 - dt / (largo_t + 3)) - alveo_t, 0, 1) * lisca((45 - pend) / 10) \
        + np.clip(lisca((1.3 - rl) / 0.3) - laghetto, 0, 1)
    terra = np.clip(strade + 0.6 * piano + campi * lavorato, 0, 1)
    bagnato = np.maximum(0.45 * lisca(1 - df / (largo_f + 4)), 0.4 * lisca(1 - dt / (largo_t + 2)))
    strati = {
        "Prato": np.clip(1 - roccia - neve - terra - ghiaia, 0, 1),
        "Roccia": np.clip(roccia - neve, 0, 1),
        "Neve": neve,
        "Terra": terra,
        "Ghiaia": np.clip(ghiaia, 0, 1),
        "Bosco": np.clip(bosco, 0, 1),
        "Strade": strade,
        "Acqua": acqua,
        "Palude": np.zeros_like(h),
        "Villaggio": piano,
        "Bagnato": np.clip(bagnato, 0, 1),
        "Campi": np.clip(campi, 0, 1),
    }
    cartella_strati = os.path.join(uscita, f"Strati_{nome_f}")
    os.makedirs(cartella_strati, exist_ok=True)
    for k, m in strati.items():
        Image.fromarray(np.round(np.clip(m, 0, 1) * 255).astype(np.uint8)).save(os.path.join(cartella_strati, f"{k}.png"))
    try:
        V.impacchetta(cartella_strati, os.path.join(uscita, f"T_MontaltoMaschere_{nome_f}.png"))
        V.impacchetta2(cartella_strati, os.path.join(uscita, f"T_MontaltoMaschere2_{nome_f}.png"))
    except Exception as ex:  # noqa: BLE001
        print("maschere impacchettate: non fatte (%s)" % ex)

    def quota(x, y):
        c = min(max(int(round(x / passo)), 0), n - 1)
        r_ = min(max(int(round((lato - y) / passo)), 0), n - 1)
        return float(h[r_, c])

    dati = {"lato_m": lato, "vertici": n, "scala_xy_cm": round(passo * 100, 4),
            "scala_z": round(QUOTA_MAX / 512 * 100, 4), "posizione_z_cm": round(QUOTA_MAX / 2 * 100, 1),
            "pelo_del_laghetto_m": round(pelo_laghetto, 2), "luoghi": {},
            "fiume_m": [[round(x, 2), round(y, 2)] for x, y in fiume_pts],
            "torrente_m": [[round(x, 2), round(y, 2)] for x, y in torrente_pts],
            "fiume_minore_m": [[round(x, 2), round(y, 2)] for x, y in minore_pts],
            "strade_m": {k: [[round(x, 2), round(y, 2), round(quota(x, y), 2)] for x, y in pts]
                         for k, pts in strade_pts.items()},
            "fiume_minore": {"nome": "la Brina"},
            "fiume": {"nome": "Aure", "larghezza_alla_citta_m": 80, "larghezza_tra_i_monti_m": 68, "profondita_navigabile_m": 12.0,
                      "navigabile": "da nord a sud, tutta la mappa", "pelo_a_nord_m": 58.0, "pelo_a_sud_m": 52.0,
                      "pelo_alla_citta_m": round(q_acqua_citta, 2), "ponte_luce_sull_acqua_m": 12.0},
            "cimitero": {"centro_unreal_cm": [round(float(CIM[0]) * 100, 1), round((lato - float(CIM[1])) * 100, 1)],
                         "giro_gradi": round(math.degrees(math.atan2(-float(n_c[0]), -float(n_c[1]))), 3),
                         "suolo_m": round(q_cim, 3),
                         "nota": "il cancello (-Y del recinto) guarda la Strada del nord, che passa a 27 m dal centro"},
            "granaio": {"centro_unreal_cm": [round(luoghi["Granaio della Corona"][0] * 100, 1),
                                             round((lato - luoghi["Granaio della Corona"][1]) * 100, 1)],
                        "giro_gradi": 180.0, "suolo_m": round(q_borgo, 3),
                        "nota": "girato di 180 gradi: il fronte con il piano di carico guarda a sud, verso la Strada del Re"},
            "porto": {"origine_m": [round(float(PO[0]), 2), round(float(PO[1]), 2)],
                      "origine_unreal_cm": [round(float(PO[0]) * 100, 1), round((lato - float(PO[1])) * 100, 1)],
                      "asse_x_m": [round(float(px_[0]), 5), round(float(px_[1]), 5)],
                      "asse_y_m": [round(float(py_[0]), 5), round(float(py_[1]), 5)],
                      "giro_gradi": round(math.degrees(math.atan2(-float(px_[1]), float(px_[0]))), 3),
                      "banchina_m": round(q_porto, 3), "pelo_m": round(qf(t_porto) - 0.35, 3),
                      "nota": "sistema del porto della chat 3D: X lungo il fiume verso monte (da -60 a +140 m), Y verso la "
                              "citta', Z 0 = la banchina (3 m sull'acqua). giro_gradi = lo yaw in Unreal dell'asse X."},
            "rocca": {"centro_unreal_cm": [round(luoghi["Rocca"][0] * 100, 1), round((lato - luoghi["Rocca"][1]) * 100, 1)],
                      "giro_gradi": 180.0, "cortile_m": round(q_rocca, 3),
                      "nota": "il sistema della rocca (X, Y, yaw da X verso Y) girato di 180 gradi in Unreal: la porta "
                              "guarda a sud (verso la citta'), 25 m a ovest del centro"},
            "ponte": {"centro_m": [round(float(PB[0]), 2), round(float(PB[1]), 2)],
                      "centro_unreal_cm": [round(float(PB[0]) * 100, 1), round((lato - float(PB[1])) * 100, 1)],
                      "asse_unreal": [round(float(asse_p[0]), 5), round(float(-asse_p[1]), 5)],
                      "pelo_m": round(qf(t_fp), 3), "strada_m": round(q_ponte, 3),
                      "nota": "asse_unreal = il verso del ponte da ovest a est, nelle X e Y di Unreal (la Y va a sud); "
                              "centro_unreal_cm dall'angolo nord-ovest del paesaggio"},
            "quote_m": {"citta_bassa": round(q_borgo, 2), "citta_alta": round(q_alta, 2), "porto": round(q_porto, 2), "rocca": round(q_rocca, 2), "pietralta": round(q_dogana, 2),
                        "cava": round(q_cava, 2)},
            "nota": "nomi provvisori: quelli veri li decide la chat delle Cronache"}
    for k, (x, y) in luoghi.items():
        q = quota(x, y)
        dati["luoghi"][k] = {"x_m": round(x, 1), "y_m": round(y, 1), "quota_m": round(q, 2),
                             "unreal_cm": [round(x * 100, 1), round((lato - y) * 100, 1), round(q * 100, 1)]}
    with open(os.path.join(uscita, f"Luoghi_{nome_f}.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)

    V.anteprima(h, acqua, strati["Bosco"], neve, strati["Roccia"], np.clip(strade + 0.5 * campi, 0, 1), passo,
                os.path.join(uscita, f"Anteprima_{nome_f}.png"))
    # controlli da leggere
    def rc(x, y):
        return (min(max(int(round((lato - y) / passo)), 0), n - 1), min(max(int(round(x / passo)), 0), n - 1))
    lung = sum(math.dist(strada_re[i], strada_re[i + 1]) for i in range(len(strada_re) - 1))
    pend_strada = []
    for i in range(len(strada_re) - 1):
        d = math.dist(strada_re[i], strada_re[i + 1])
        if d > 0.5 and acqua[rc(*strada_re[i])] < 0.2 and acqua[rc(*strada_re[i + 1])] < 0.2:
            pend_strada.append(abs(quota(*strada_re[i + 1]) - quota(*strada_re[i])) / d * 100)
    print("lato %d m, %d vertici (%.2f m per punto); quote %.0f..%.0f m" % (lato, n, passo, h.min(), h.max()))
    print("Strada del Re: %.0f m, pendenza media %.1f%%, massima %.1f%%" % (lung, float(np.mean(pend_strada)), float(np.max(pend_strada))))
    print("acqua %.1f%%, bosco %.1f%%, campi %.1f%%, roccia %.1f%%, sopra i 25 gradi %.1f%%"
          % (100 * acqua.mean(), 100 * strati["Bosco"].mean(), 100 * campi.mean(), 100 * strati["Roccia"].mean(),
             100 * float((pend > 25).mean())))
    for k in ("Uscita del passo", "Pietralta", "Ponte di pietra", "Granaio della Corona", "Porto", "Rocca", "Vigne", "Strada per Aurelia"):
        print("  %-20s quota %.0f m" % (k, dati["luoghi"][k]["quota_m"]))


def acqua_e_senza_erba(cartella, lato):
    """(24/11) Dopo il terreno: Acqua_<lato>.json per i fiumi del plugin Water di Unreal (stesso formato di acqua.py della
    valle: punti in cm dall'angolo nord-ovest, quota del pelo, larghezza e profondita') e T_MontaltoSenzaErba.png
    (R = dove l'erba vera non va: strade e piani della citta', di Pietralta, del porto)."""
    with open(os.path.join(cartella, f"Luoghi_{lato}.json"), encoding="utf-8") as f:
        dati = json.load(f)
    lisca = V.lisca

    def lungo(pts, ogni):
        """Punti ogni 'ogni' metri lungo la linea, con la frazione di lunghezza (0..1)."""
        seg = [0.0]
        for i in range(len(pts) - 1):
            seg.append(seg[-1] + math.dist(pts[i][:2], pts[i + 1][:2]))
        tot = seg[-1]
        quanti = max(int(round(tot / ogni)), 2)
        fuori = []
        for k in range(quanti + 1):
            m = tot * k / quanti
            x = float(np.interp(m, seg, [q[0] for q in pts])); y = float(np.interp(m, seg, [q[1] for q in pts]))
            fuori.append((x, y, m / tot))
        return fuori

    def cm(x, y, z):
        return [round(x * 100, 1), round((lato - y) * 100, 1), round(z * 100, 1)]

    def corso(punti, pelo_di, larghezza_di, profondita_di):
        fuori = []
        for i, (x, y, t) in enumerate(punti):
            a, b = punti[max(i - 1, 0)], punti[min(i + 1, len(punti) - 1)]
            strada = math.dist(a[:2], b[:2]) or 1.0
            pend = max(pelo_di(a[2]) - pelo_di(b[2]), 0) / strada
            fuori.append({"posizione": cm(x, y, pelo_di(t)), "larghezza_cm": round(larghezza_di(t) * 100, 1),
                          "profondita_cm": round(profondita_di(t) * 100 + 100, 1),
                          "velocita": round(min(max(1 + pend * 12, 1), 5), 2)})
        return fuori

    def qf(t):
        return 58.0 - 6.0 * t
    q_arrivo = qf(0.36)
    acqua = {"lato_cm": lato * 100.0,
             "fiume": {"nome": "Aure", "punti": corso(
                 lungo(dati["fiume_m"], 110.0), lambda t: qf(t) - 0.35,
                 lambda t: 2.0 * (34.0 + 6.0 * float(lisca(np.float32((t - 0.15) / 0.25))) + 4.0), lambda t: 11.6)},
             "torrente": {"nome": "Torrente di Pietralta", "punti": corso(
                 lungo(dati["torrente_m"], 60.0),
                 lambda t: (Q_PASSO - 14.0) * (1 - t) ** 1.25 + q_arrivo * (1 - (1 - t) ** 1.25) - 0.3,
                 lambda t: 0.7 * (3.5 + 2.5 * t), lambda t: 0.6)},      # (secondo controllo) alveo a V: acqua stretta
             "brina": {"nome": "la Brina", "punti": corso(
                 lungo(dati["fiume_minore_m"], 70.0),
                 lambda t: 104.0 * (1 - t) ** 1.6 + qf(0.66) * (1 - (1 - t) ** 1.6) - 0.4,
                 lambda t: 2.0 * (7.0 + 4.0 * t + 1.5), lambda t: 1.4)}}
    # (25/11) davanti al porto l'acqua arriva fino alla banchina: li' il fiume di Unreal si allarga quanto serve
    if "porto" in dati:
        po_ = dati["porto"]
        for p_ in acqua["fiume"]["punti"]:
            xm, ym = p_["posizione"][0] / 100.0, lato - p_["posizione"][1] / 100.0
            xr = (xm - po_["origine_m"][0]) * po_["asse_x_m"][0] + (ym - po_["origine_m"][1]) * po_["asse_x_m"][1]
            yr = (xm - po_["origine_m"][0]) * po_["asse_y_m"][0] + (ym - po_["origine_m"][1]) * po_["asse_y_m"][1]
            if -110.0 <= xr <= 190.0:
                p_["larghezza_cm"] = max(p_["larghezza_cm"], round(2.0 * (abs(yr) + 2.0) * 100.0, 1))
    # (secondo controllo) gli affluenti finiscono alla riva dell'Aure: via i punti dentro il suo alveo
    aure_xy = [(p_["posizione"][0], p_["posizione"][1]) for p_ in acqua["fiume"]["punti"]]

    def dall_aure(pos):
        meglio = 1e18
        for i in range(len(aure_xy) - 1):
            (ax, ay), (bx, by) = aure_xy[i], aure_xy[i + 1]
            dx, dy = bx - ax, by - ay
            t_ = min(max(((pos[0] - ax) * dx + (pos[1] - ay) * dy) / (dx * dx + dy * dy), 0), 1)
            meglio = min(meglio, math.hypot(pos[0] - ax - t_ * dx, pos[1] - ay - t_ * dy))
        return meglio
    for k in ("torrente", "brina"):
        pts_ = acqua[k]["punti"]
        while len(pts_) > 3 and dall_aure(pts_[-1]["posizione"]) < 4600.0:
            pts_.pop()
    # alla foce i due affluenti arrivano al pelo dell'Aure, non sotto
    for k, tf_ in (("torrente", 0.36), ("brina", 0.66)):
        ultimo = acqua[k]["punti"][-1]
        ultimo["posizione"][2] = round((qf(tf_) - 0.35) * 100, 1)
    with open(os.path.join(cartella, f"Acqua_{lato}.json"), "w", encoding="utf-8") as f:
        json.dump(acqua, f, ensure_ascii=False, indent=1)
    strati = os.path.join(cartella, f"Strati_{lato}")
    def leggi(nm):
        return np.array(Image.open(os.path.join(strati, nm + ".png")).convert("L").resize((2048, 2048), Image.BILINEAR))
    senza = np.maximum(leggi("Strade"), leggi("Villaggio"))
    senza = ndimage.maximum_filter(senza, 3)
    vuoto = np.zeros_like(senza)
    Image.merge("RGBA", [Image.fromarray(senza), Image.fromarray(vuoto), Image.fromarray(vuoto),
                         Image.fromarray(np.full_like(senza, 255))]).save(
        os.path.join(cartella, "T_MontaltoSenzaErba.png"), optimize=True)
    print("acqua: Aure %d punti, torrente %d, Brina %d; T_MontaltoSenzaErba fatta"
          % tuple(len(acqua[k]["punti"]) for k in ("fiume", "torrente", "brina")))


def case_di_citta(cartella, lato, seme=2511):
    """(25/11) Dove vanno le case di citta' della chat 3D (modulo 6 x 10 m, fronte sulla strada): file Case_<lato>.json.
    File di case lungo la Strada del Re, la salita alla rocca e la via delle vigne, dentro la citta'; ogni casa in piano
    alla quota del terreno davanti alla SUA porta; in testa alle file la casa d'angolo. Non rifa' il terreno: legge
    Luoghi e Altezze gia' fatti. Lascia liberi ponte, porto, granaio, rocca, acqua, strade e i pendii troppo ripidi."""
    with open(os.path.join(cartella, f"Luoghi_{lato}.json"), encoding="utf-8") as f:
        dati = json.load(f)
    h = np.asarray(Image.open(os.path.join(cartella, f"Altezze_{lato}.png")), dtype=np.float64) / 65535.0 * QUOTA_MAX
    n = h.shape[0]
    passo = lato / (n - 1)
    rng = np.random.default_rng(seme)

    def quota(x, y):                                   # metri del generatore (x est, y nord)
        c = min(max(x / passo, 0.0), n - 1.001)
        r = min(max((lato - y) / passo, 0.0), n - 1.001)
        c0, r0 = int(c), int(r)
        fc, fr = c - c0, r - r0
        return float((h[r0, c0] * (1 - fc) + h[r0, c0 + 1] * fc) * (1 - fr)
                     + (h[r0 + 1, c0] * (1 - fc) + h[r0 + 1, c0 + 1] * fc) * fr)

    # tutto in «metri di Unreal dall'angolo nord-ovest»: X est, Y SUD (cosi' gli yaw sono quelli di Unreal)
    def ue(x, y):
        return np.array([x, lato - y])

    def quota_ue(p):
        return quota(p[0], lato - p[1])

    citta = ue(dati["luoghi"]["Città"]["x_m"], dati["luoghi"]["Città"]["y_m"])
    rocca = np.array(dati["rocca"]["centro_unreal_cm"]) / 100.0
    ponte = np.array(dati["ponte"]["centro_unreal_cm"]) / 100.0
    gran = np.array(dati["granaio"]["centro_unreal_cm"]) / 100.0
    po = dati["porto"]
    po_o = ue(*po["origine_m"])
    po_x = np.array([po["asse_x_m"][0], -po["asse_x_m"][1]])
    po_y = np.array([po["asse_y_m"][0], -po["asse_y_m"][1]])
    pelo = po["pelo_m"]
    strade = {k: np.array([ue(q_[0], q_[1]) for q_ in v]) for k, v in dati["strade_m"].items()}
    mezza = {"Strada del Re": 4.5, "Salita alla rocca": 3.5, "Via delle vigne": 3.0, "Strada del nord": 3.5,
             "Via della cava": 3.0, "Sentiero della torre": 2.0, "Via alta": 3.0, "Via bassa": 3.0}
    def fitta(pts_):                                   # (secondo controllo) un punto ogni 2 m, non solo i vertici
        seg_ = np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(pts_, axis=0), axis=1))]
        m_ = np.arange(0.0, seg_[-1] + 2.0, 2.0)
        return np.c_[np.interp(m_, seg_, pts_[:, 0]), np.interp(m_, seg_, pts_[:, 1])]
    punti_strade = [(k, fitta(v)) for k, v in strade.items()]

    def rett(c, yaw, x0, x1, y0=-6.3, y1=6.3):
        a = math.radians(yaw)
        ex, ey = np.array([math.cos(a), math.sin(a)]), np.array([-math.sin(a), math.cos(a)])
        return [c + ex * x + ey * y for x, y in ((x0, y0), (x1, y0), (x1, y1), (x0, y1))], ex, ey

    def si_toccano(A, B):
        for poli in (A, B):
            for i in range(4):
                e = poli[(i + 1) % 4] - poli[i]
                asse = np.array([-e[1], e[0]])
                a_ = [float(asse @ p_) for p_ in A]
                b_ = [float(asse @ p_) for p_ in B]
                if max(a_) <= min(b_) + 0.15 or max(b_) <= min(a_) + 0.15:
                    return False
        return True

    def libero(c, yaw, x0, x1, propria, mezza_y=6.3):
        ang, ex, ey = rett(c, yaw, x0, x1, -mezza_y, mezza_y)
        # dentro la citta', fuori da rocca, ponte, porto, granaio
        if np.hypot((c[0] - citta[0]) / 430.0, (c[1] - citta[1]) / 400.0) > 0.93:
            return None
        if np.hypot((c[0] - rocca[0]) / 138.0, (c[1] - rocca[1]) / 112.0) < 1.0:
            return None
        if np.linalg.norm(c - ponte) < 78.0:
            return None
        for p_ in ang + [c]:
            xp, yp = (p_ - po_o) @ po_x, (p_ - po_o) @ po_y
            if -82.0 < xp < 162.0 and -70.0 < yp < 40.0:
                return None
            gx, gy = -(p_[0] - gran[0]), -(p_[1] - gran[1])          # sistema del granaio (girato di 180)
            if -46.0 < gx < 33.0 and -30.0 < gy < 19.0:
                return None
        # (secondo controllo) la casa sta alla quota della strada davanti a lei: sotto la pianta il terreno non deve
        # salire piu' di 40 cm sopra il pavimento (casa sepolta) ne' scendere piu' di 3,5 m (lo zoccolo arriva a -4)
        qs = [quota_ue(p_) for p_ in ang + [c] + [(ang[i] + ang[(i + 1) % 4]) / 2 for i in range(4)]]
        z_davanti = quota_ue(c - ey * (mezza_y - 0.3))
        if min(qs) < pelo + 4.0 or max(qs) - z_davanti > 0.4 or min(qs) - z_davanti < -3.5:
            return None
        # nessuna strada sotto la casa (la propria strada sta davanti per costruzione, ma una curva puo' rientrare)
        for nome_s, pts_s in punti_strade:
            d_ = pts_s - c
            vicini = d_[(np.abs(d_[:, 0]) < 25.0) & (np.abs(d_[:, 1]) < 25.0)]
            if len(vicini) == 0:
                continue
            lx, ly = vicini @ ex, vicini @ ey
            m_ = mezza.get(nome_s, 3.0) + (0.2 if nome_s == propria else 0.8)
            if np.any((lx > x0 - m_) & (lx < x1 + m_) & (ly > -mezza_y - m_) & (ly < mezza_y + m_)):
                return None
        return ang

    case = []            # dict: pezzo, c, yaw, ang, fila, verso_x
    lotti = []           # (25/11 sera, chiesto da Fra) i lotti lasciati liberi per le sedi delle gilde: 30 m di fronte, 26 di fondo

    def lotto_gilda(nome, s_m, lato_):
        """Riserva un lotto sulla strada 'nome' a s_m metri dall'inizio, sul lato dato. Le case non ci vanno."""
        pts = strade[nome]
        seg = np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))]
        if s_m > seg[-1] - 20.0:
            return
        p0 = np.array([np.interp(s_m, seg, pts[:, 0]), np.interp(s_m, seg, pts[:, 1])])
        p1 = np.array([np.interp(s_m + 3.0, seg, pts[:, 0]), np.interp(s_m + 3.0, seg, pts[:, 1])])
        t_ = (p1 - p0) / (np.linalg.norm(p1 - p0) or 1.0)
        nrm = np.array([-t_[1], t_[0]]) * lato_
        c = p0 + nrm * (mezza[nome] + 2.0 + 13.0)
        yaw = math.degrees(math.atan2(-nrm[0], nrm[1]))
        a_ = math.radians(yaw)
        ex, ey = np.array([math.cos(a_), math.sin(a_)]), np.array([-math.sin(a_), math.cos(a_)])
        ang = [c + ex * x + ey * y for x, y in ((-15.0, -13.0), (15.0, -13.0), (15.0, 13.0), (-15.0, 13.0))]
        if libero(c, yaw, -15.0, 15.0, nome, 13.0) is None:
            return
        if any(si_toccano(ang, k["ang"]) for k in lotti):
            return
        lotti.append({"c": c, "yaw": yaw, "ang": ang, "strada": nome})

    def fila(nome, s_da, s_a, lati=(1.0, -1.0), pieno=0.7):
        """pieno = quanta strada ha case (il resto sono vicoli, orti e tratti liberi)."""
        pts = strade[nome]
        seg = np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))]
        tot = seg[-1]
        fuori = mezza[nome] + 2.0 + 5.0
        for lato_ in lati:
            corrente = []
            s_ = s_da + float(rng.uniform(0.0, 14.0))
            quante = int(rng.integers(4, 8))
            while s_ <= min(s_a, tot) - 3.0:
                p0 = np.array([np.interp(s_, seg, pts[:, 0]), np.interp(s_, seg, pts[:, 1])])
                p1 = np.array([np.interp(s_ + 2.0, seg, pts[:, 0]), np.interp(s_ + 2.0, seg, pts[:, 1])])
                p2 = np.array([np.interp(max(s_ - 2.0, 0), seg, pts[:, 0]), np.interp(max(s_ - 2.0, 0), seg, pts[:, 1])])
                t_ = (p1 - p2) / (np.linalg.norm(p1 - p2) or 1.0)
                nrm = np.array([-t_[1], t_[0]]) * lato_
                c = p0 + nrm * fuori
                r_ = -nrm
                yaw = math.degrees(math.atan2(r_[0], -r_[1]))
                ang = libero(c, yaw, -3.0, 3.0, nome)
                if ang is not None and any(np.linalg.norm(c - k["c"]) < 30.0 and si_toccano(ang, k["ang"]) for k in lotti):
                    ang = None                                       # e' un lotto di gilda
                if ang is not None and not any(np.linalg.norm(c - k["c"]) < 16.0 and si_toccano(ang, k["ang"]) for k in case):
                    a_ = math.radians(yaw)
                    k = {"pezzo": None, "c": c, "yaw": yaw, "ang": ang,
                         "verso_x": float(np.array([math.cos(a_), math.sin(a_)]) @ t_)}
                    case.append(k)
                    corrente.append(k)
                    s_ += 6.0
                    if len(corrente) >= quante:
                        # un vicolo (4 m) o, ogni tanto, un tratto libero piu' lungo: orti, cortili, una piazzetta
                        chiudi(corrente, nome)
                        corrente = []
                        quante = int(rng.integers(4, 8))
                        s_ += 4.0 if rng.random() < pieno else float(rng.uniform(18.0, 40.0))
                else:
                    if corrente:
                        chiudi(corrente, nome)
                        corrente = []
                    s_ += 2.0
            if corrente:
                chiudi(corrente, nome)

    def chiudi(fila_, nome_strada):
        # i tipi: A, C, D a caso; i portici B a gruppi di tre; in testa (dove il fianco -X e' libero) la casa d'angolo
        i = 0
        while i < len(fila_):
            t = rng.choice(["A", "C", "D", "B"], p=[0.36, 0.30, 0.24, 0.10])
            if t == "B":
                for _ in range(3):
                    if i < len(fila_):
                        fila_[i]["pezzo"] = "SM_Montalto_Casa_B"
                        i += 1
            else:
                fila_[i]["pezzo"] = "SM_Montalto_Casa_" + t
                i += 1
        if len(fila_) >= 3:
            testa = fila_[0] if fila_[0]["verso_x"] > 0 else fila_[-1]
            ang, _, _ = rett(testa["c"], testa["yaw"], -4.3, 3.0)
            largo, _, _ = rett(testa["c"], testa["yaw"], -9.0, 3.0)      # il fianco con la bottega deve restare libero
            if not any(k is not testa and np.linalg.norm(testa["c"] - k["c"]) < 22.0 and si_toccano(largo, k["ang"])
                       for k in case) and not any(si_toccano(largo, k["ang"]) for k in lotti) \
                    and libero(testa["c"], testa["yaw"], -4.3, 3.0, nome_strada) is not None:
                testa["pezzo"] = "SM_Montalto_Casa_Angolo"
                testa["ang"] = ang

    # i lotti delle gilde: sulle strade grandi, uno ogni tanto, alternando i lati
    for nome_, passi_ in (("Strada del Re", (2760.0, 2900.0, 3060.0, 3220.0, 3370.0)), ("Via bassa", (180.0, 520.0, 900.0, 1250.0)),
                          ("Via alta", (150.0, 330.0))):
        for i_, s_m in enumerate(passi_):
            lotto_gilda(nome_, s_m, 1.0 if i_ % 2 == 0 else -1.0)
    fila("Strada del Re", 0.0, 1e9, pieno=0.85)
    fila("Salita alla rocca", 12.0, 1e9, pieno=0.6)
    fila("Via delle vigne", 12.0, 1e9, lati=(1.0,), pieno=0.5)
    fila("Via alta", 0.0, 1e9, pieno=0.6)
    fila("Via bassa", 0.0, 1e9, lati=(-1.0, 1.0), pieno=0.45)
    porte = {"A": 1.55, "B": 0.0, "C": -1.75, "D": 0.0, "Angolo": 1.55}
    fuori = []
    for k in case:
        a_ = math.radians(k["yaw"])
        ex, ey = np.array([math.cos(a_), math.sin(a_)]), np.array([-math.sin(a_), math.cos(a_)])
        porta = k["c"] + ex * porte[k["pezzo"].rsplit("_", 1)[-1]] - ey * 6.0      # un metro davanti alla soglia
        z = quota_ue(porta)
        fuori.append({"pezzo": k["pezzo"], "unreal_cm": [round(k["c"][0] * 100, 1), round(k["c"][1] * 100, 1), round(z * 100, 1)],
                      "yaw": round(k["yaw"], 2), "tinta": int(rng.integers(0, 6))})
    lotti_fuori = []
    for i_, k in enumerate(lotti):
        a_ = math.radians(k["yaw"])
        ey = np.array([-math.sin(a_), math.cos(a_)])
        lotti_fuori.append({"nome": "Lotto per una gilda %d" % (i_ + 1), "strada": k["strada"],
                            "unreal_cm": [round(k["c"][0] * 100, 1), round(k["c"][1] * 100, 1),
                                          round(quota_ue(k["c"] - ey * 12.0) * 100, 1)],
                            "yaw": round(k["yaw"], 2), "fronte_m": 30, "fondo_m": 26})
    conti = {}
    for k in fuori:
        conti[k["pezzo"]] = conti.get(k["pezzo"], 0) + 1
    with open(os.path.join(cartella, f"Case_{lato}.json"), "w", encoding="utf-8") as f:
        json.dump({"_come": "case di citta' di Montalto: centro in cm dall'angolo nord-ovest del paesaggio (X est, Y sud), "
                            "quota vera in cm (il pavimento della bottega), yaw di Unreal; tinta = quale intonaco (0-5); "
                            "lotti_gilde = i posti lasciati liberi per le sedi delle gilde (il fronte, -Y, sulla strada)",
                   "conti": conti, "lotti_gilde": lotti_fuori, "case": fuori},
                  f, ensure_ascii=False, indent=1)
    print("lotti per le gilde: %d" % len(lotti_fuori))
    print("case di citta': %d (%s)" % (len(fuori), ", ".join("%s %d" % (k.rsplit("_", 1)[-1], v) for k, v in sorted(conti.items()))))
    return fuori


def mura_di_citta(cartella, lato):
    """(25/11) Il giro delle mura della citta' bassa con i pezzi della chat 3D: file Mura_<lato>.json.
    Un ovale sul bordo del ripiano basso. Moduli da 10 m agganciati sugli spigoli di dentro (girano meno di 3 gradi l'uno
    sull'altro, con un contrafforte su ogni giunto), una torre ogni 8 moduli, i pezzi «_Riva» dove fuori c'e' la riva
    dell'Aure, la Porta nord sulla Strada del nord (con la grata del passo), e quattro varchi chiusi dalle torri di
    testata: la Strada del Re verso il ponte, la rampa del porto, la Strada del Re verso la Brina, la via delle vigne.
    Sistema: metri di Unreal dall'angolo nord-ovest (X est, Y SUD); il +Y di ogni pezzo e' DENTRO, il +X il verso del giro."""
    with open(os.path.join(cartella, f"Luoghi_{lato}.json"), encoding="utf-8") as f:
        dati = json.load(f)
    h = np.asarray(Image.open(os.path.join(cartella, f"Altezze_{lato}.png")), dtype=np.float64) / 65535.0 * QUOTA_MAX
    n = h.shape[0]
    passo = lato / (n - 1)

    def quota_ue(p):
        c = min(max(p[0] / passo, 0.0), n - 1.001)
        r = min(max(p[1] / passo, 0.0), n - 1.001)
        c0, r0 = int(c), int(r)
        fc, fr = c - c0, r - r0
        return float((h[r0, c0] * (1 - fc) + h[r0, c0 + 1] * fc) * (1 - fr)
                     + (h[r0 + 1, c0] * (1 - fc) + h[r0 + 1, c0 + 1] * fc) * fr)

    C = np.array([dati["luoghi"]["Città"]["x_m"], lato - dati["luoghi"]["Città"]["y_m"]])
    A_, B_ = MURA_A, MURA_B                                  # la linea degli spigoli di dentro
    z_citta = dati["quote_m"]["citta_bassa"]

    def E(f_):
        return C + np.array([A_ * math.cos(f_), B_ * math.sin(f_)])   # f cresce: est -> sud -> ovest -> nord (dentro = +Y)

    def angolo_di(p):
        return math.atan2((p[1] - C[1]) / B_, (p[0] - C[0]) / A_) % (2 * math.pi)

    def avanti(f_, lungo, verso=1.0):
        """Il punto dell'ovale a 'lungo' metri di corda da E(f_), nel verso dato."""
        a, b = 0.0, 0.2
        for _ in range(40):
            m_ = (a + b) / 2
            if np.linalg.norm(E(f_ + verso * m_) - E(f_)) < lungo:
                a = m_
            else:
                b = m_
        return f_ + verso * (a + b) / 2

    def incrocio(nome):
        """Gli angoli dove una strada attraversa l'ovale."""
        pts = [np.array([q_[0], lato - q_[1]]) for q_ in dati["strade_m"][nome]]
        fuori = []
        for i in range(len(pts) - 1):
            d0 = np.hypot((pts[i][0] - C[0]) / A_, (pts[i][1] - C[1]) / B_) - 1
            d1 = np.hypot((pts[i + 1][0] - C[0]) / A_, (pts[i + 1][1] - C[1]) / B_) - 1
            if d0 * d1 < 0:
                t_ = d0 / (d0 - d1)
                fuori.append(angolo_di(pts[i] + (pts[i + 1] - pts[i]) * t_))
        return fuori

    po = dati["porto"]
    po_o = np.array([po["origine_m"][0], lato - po["origine_m"][1]])
    po_x = np.array([po["asse_x_m"][0], -po["asse_x_m"][1]])
    po_y = np.array([po["asse_y_m"][0], -po["asse_y_m"][1]])
    f_porto = angolo_di(po_o + po_x * 134.0 + po_y * 60.0)          # davanti alla cima della rampa dei carri
    f_porta = angolo_di(np.array([dati["luoghi"]["Porta nord"]["x_m"], lato - dati["luoghi"]["Porta nord"]["y_m"]]))
    re = incrocio("Strada del Re")
    vigne = incrocio("Via delle vigne")
    # i confini: (angolo al centro, mezza larghezza in metri di corda, tipo)
    confini = [(f_, 12.0, "varco: " + n_) for f_, n_ in zip(sorted(re), ("la Strada del Re", "la Strada del Re"))]
    confini += [(f_, 8.0, "varco: la via delle vigne") for f_ in vigne[:1]]
    confini += [(f_porto, 7.0, "varco: la rampa del porto"), (f_porta, 12.0, "porta")]
    confini.sort()
    # due varchi troppo vicini (meno di 45 m di muro in mezzo) diventano uno solo
    uniti = []
    for f_, mezzo, tipo in confini:
        da, a = avanti(f_, mezzo, -1.0), avanti(f_, mezzo, 1.0)
        if uniti and tipo != "porta" and uniti[-1][2] != "porta" and np.linalg.norm(E(da) - E(uniti[-1][1])) < 20.0:
            uniti[-1] = (uniti[-1][0], a, uniti[-1][2] + " + " + tipo.replace("varco: ", ""))
        else:
            uniti.append((da, a, tipo))
    confini = uniti

    pezzi = []            # dict: pezzo, c (centro), yaw, tipo ("muro", "torre", "testata", "porta"), riva, capi (H0, H1)

    def pezzo_tra(h0, h1, cosa):
        """Un pezzo con gli spigoli di dentro in h0 e h1 (h1 e' avanti nel verso del giro)."""
        d = (h1 - h0) / np.linalg.norm(h1 - h0)
        dentro = np.array([-d[1], d[0]])
        mezzo = (h0 + h1) / 2
        # (secondo controllo) i pezzi normali reggono il terreno fino a -3 m: sotto, il pezzo di riva
        riva = any(quota_ue(p_ - dentro * f_) < z_citta - 3.0 for p_ in (h0, mezzo, h1) for f_ in (3.5, 6.0, 9.5))
        y_in = 2.0 if (riva and cosa != "porta") else 1.5
        return {"cosa": cosa, "riva": riva and cosa != "porta", "c": mezzo - dentro * y_in,
                "yaw": math.degrees(math.atan2(d[1], d[0])), "h0": h0, "h1": h1, "dentro": dentro}

    def cammina(f_da, f_fino, verso, capo):
        """Mette pezzi da f_da verso f_fino (verso +1 avanti, -1 indietro); 'capo' = cosa c'e' in f_fino (varco o porta).
        Ritorna i pezzi nell'ordine del giro."""
        fila = []
        f_ = f_da
        k = 0
        def resta(f__):
            return ((f_fino - f__) * verso) % (2 * math.pi)
        while True:
            # quanto manca al confine, in metri di corda
            manca = np.linalg.norm(E(f_fino) - E(f_)) if resta(f_) < math.pi else 0.0
            k += 1
            torre = (k % 9 == 0)
            lungo = 9.0 if torre else 10.0
            if manca < lungo + 9.0 + 0.5:
                # ultimo pezzo prima del varco: la torre di testata (9 m), se ci sta
                if manca >= 9.0:
                    f2 = avanti(f_, 9.0, verso)
                    a_, b_ = (E(f_), E(f2)) if verso > 0 else (E(f2), E(f_))
                    fila.append(pezzo_tra(a_, b_, "testata_sx" if verso > 0 else "testata_dx"))
                elif fila:
                    # non ci sta: l'ultimo modulo messo diventa la testata (resta un po' piu' di varco)
                    ult = fila[-1]
                    mezzo = (ult["h0"] + ult["h1"]) / 2
                    tieni = ult["h0"] if verso > 0 else ult["h1"]
                    d = (ult["h1"] - ult["h0"]) / np.linalg.norm(ult["h1"] - ult["h0"])
                    altro = tieni + d * 9.0 * verso
                    a_, b_ = (tieni, altro) if verso > 0 else (altro, tieni)
                    fila[-1] = pezzo_tra(a_, b_, "testata_sx" if verso > 0 else "testata_dx")
                break
            f2 = avanti(f_, lungo, verso)
            a_, b_ = (E(f_), E(f2)) if verso > 0 else (E(f2), E(f_))
            fila.append(pezzo_tra(a_, b_, "torre" if torre else "muro"))
            f_ = f2
        return fila if verso > 0 else fila[::-1]

    tratti = []
    nc = len(confini)
    for i in range(nc):
        da, a, tipo = confini[i]
        _, a_prec, tipo_prec = confini[i - 1]
        da_succ, _, tipo_succ = confini[(i + 1) % nc]
        if tipo == "porta":
            f0, f1 = avanti(angolo_di((E(da) + E(a)) / 2), 12.0, -1.0), avanti(angolo_di((E(da) + E(a)) / 2), 12.0, 1.0)
            porta = pezzo_tra(E(f0), E(f1), "porta")
            # dalla porta si cammina nei due versi fino ai varchi vicini
            tratti.append(cammina(f0, a_prec, -1.0, tipo_prec) + [porta] + cammina(f1, da_succ, 1.0, tipo_succ))
        elif tipo_succ != "porta" and tipo != "porta":
            # da un varco al varco dopo: si parte con una testata_dx e si arriva con una testata_sx
            f2 = avanti(a, 9.0, 1.0)
            prima = pezzo_tra(E(a), E(f2), "testata_dx")
            tratti.append([prima] + cammina(f2, da_succ, 1.0, tipo_succ))
    nomi = {("muro", False): "SM_Montalto_Mura_Cortina", ("muro", True): "SM_Montalto_Mura_Riva",
            ("torre", False): "SM_Montalto_Mura_Torre", ("torre", True): "SM_Montalto_Mura_Torre_Riva",
            ("testata_sx", False): "SM_Montalto_Mura_Torre_Testata_Sx", ("testata_sx", True): "SM_Montalto_Mura_Torre_Testata_Sx_Riva",
            ("testata_dx", False): "SM_Montalto_Mura_Torre_Testata_Dx", ("testata_dx", True): "SM_Montalto_Mura_Torre_Testata_Dx_Riva",
            ("porta", False): "SM_Montalto_Porta_Nord"}
    fuori = []
    def metti(pezzo, c, yaw, z=0.0):
        fuori.append({"pezzo": pezzo, "unreal_cm": [round(c[0] * 100, 1), round(c[1] * 100, 1), round((z_citta + z) * 100, 1)],
                      "yaw": round(yaw, 3)})
    giunti = 0
    for tratto in tratti:
        for i, k in enumerate(tratto):
            metti(nomi[(k["cosa"], k["riva"])], k["c"], k["yaw"])
            if k["cosa"] == "porta":
                # la grata del passo nella scanalatura: chiusa a Z 125, a Y -392 del pezzo
                a_ = math.radians(k["yaw"])
                ey = np.array([-math.sin(a_), math.cos(a_)])
                metti("SM_Passo_Grata", k["c"] + ey * (-3.92), k["yaw"], 1.25)
            if i + 1 < len(tratto):
                s2 = tratto[i + 1]
                giro = (s2["yaw"] - k["yaw"] + 180.0) % 360.0 - 180.0
                if abs(giro) > 0.2:
                    riva = k["riva"] or s2["riva"]
                    dm = (k["dentro"] + s2["dentro"]) / 2
                    metti("SM_Montalto_Mura_Contrafforte_Riva" if riva else "SM_Montalto_Mura_Contrafforte",
                          k["h1"] - dm * (2.0 if riva else 1.5), k["yaw"] + giro / 2)
                    giunti += 1
    conti = {}
    for k in fuori:
        conti[k["pezzo"]] = conti.get(k["pezzo"], 0) + 1
    giri = [abs((t[i + 1]["yaw"] - t[i]["yaw"] + 180.0) % 360.0 - 180.0) for t in tratti for i in range(len(t) - 1)]
    with open(os.path.join(cartella, f"Mura_{lato}.json"), "w", encoding="utf-8") as f:
        json.dump({"_come": "mura della citta' bassa di Montalto: centro in cm dall'angolo nord-ovest del paesaggio (X est, Y sud), "
                            "quota vera in cm (la citta' dentro le mura), yaw di Unreal; il +Y di ogni pezzo e' dentro",
                   "varchi": [t for _, _, t in confini], "conti": conti, "pezzi": fuori}, f, ensure_ascii=False, indent=1)
    print("mura: %d pezzi (%s); giro massimo tra due moduli %.2f gradi; varchi: %s"
          % (len(fuori), ", ".join("%s %d" % (k.replace("SM_Montalto_", ""), v) for k, v in sorted(conti.items())),
             max(giri) if giri else 0.0, "; ".join(t for _, _, t in confini)))
    return fuori, tratti, confini


def sotto_gli_edifici(cartella, lato):
    """(25/11 sera, dalle foto di Fra: l'erba dentro le torri) Sotto le case, e le torri delle mura il
    terreno scende 35 cm sotto il pavimento, e li' l'erba vera non cresce (T_MontaltoSenzaErba). Va fatto DOPO aver
    deciso dove stanno case e mura; ritocca Altezze_<lato>.png e la maschera. Si puo' rifare: abbassa soltanto."""
    percorso = os.path.join(cartella, f"Altezze_{lato}.png")
    alt = np.asarray(Image.open(percorso)).astype(np.float64)
    prima = alt.copy()
    Image.open(percorso).save(os.path.join(cartella, f"Altezze_{lato}_base.png"))   # il terreno prima di questo passo
    n = alt.shape[0]
    passo = lato / (n - 1)
    senza = np.array(Image.open(os.path.join(cartella, "T_MontaltoSenzaErba.png")).convert("RGBA"))
    ns = senza.shape[0]
    cols, righe = np.meshgrid(np.arange(n) * passo, np.arange(n) * passo)          # metri di Unreal: X est, Y sud
    cs, rs = np.meshgrid((np.arange(ns) + 0.5) * lato / ns, (np.arange(ns) + 0.5) * lato / ns)

    def rettangolo(c, yaw, x0, x1, y0, y1, gx, gy):
        a_ = math.radians(yaw)
        lx = (gx - c[0]) * math.cos(a_) + (gy - c[1]) * math.sin(a_)
        ly = -(gx - c[0]) * math.sin(a_) + (gy - c[1]) * math.cos(a_)
        return (lx >= x0) & (lx <= x1) & (ly >= y0) & (ly <= y1)

    def abbassa(c, yaw, z_m, x0, x1, y0, y1, bordo=0.8):
        """Il terreno scende solo ben dentro i muri (bordo), cosi' fuori non si vede nessun fosso (la maglia e' di 2 m);
        l'erba invece sparisce su tutto il pavimento."""
        # una finestra piccola intorno al pezzo, per non passare tutta la mappa ogni volta
        r_ = 20.0
        i0, i1 = max(int((c[1] - r_) / passo), 0), min(int((c[1] + r_) / passo) + 2, n)
        j0, j1 = max(int((c[0] - r_) / passo), 0), min(int((c[0] + r_) / passo) + 2, n)
        dentro = rettangolo(c, yaw, x0 + bordo, x1 - bordo, y0 + bordo, y1 - bordo, cols[i0:i1, j0:j1], righe[i0:i1, j0:j1])
        tetto = (z_m - 0.35) / QUOTA_MAX * 65535.0
        pezzo = alt[i0:i1, j0:j1]
        pezzo[dentro] = np.minimum(pezzo[dentro], tetto)
        k0, k1 = max(int((c[1] - r_) / lato * ns), 0), min(int((c[1] + r_) / lato * ns) + 2, ns)
        l0, l1 = max(int((c[0] - r_) / lato * ns), 0), min(int((c[0] + r_) / lato * ns) + 2, ns)
        sopra = rettangolo(c, yaw, x0 - 0.5, x1 + 0.5, y0 - 0.5, y1 + 0.5, cs[k0:k1, l0:l1], rs[k0:k1, l0:l1])
        senza[k0:k1, l0:l1, 0][sopra] = 255

    quanti = 0
    with open(os.path.join(cartella, f"Case_{lato}.json"), encoding="utf-8") as f:
        for k in json.load(f)["case"]:
            x, y, z = [v / 100.0 for v in k["unreal_cm"]]
            # il pavimento della casa (lo zoccolo con la scarpa sta fuori da questo rettangolo e copre il gradino)
            abbassa((x, y), k["yaw"], z, -4.3 if k["pezzo"].endswith("Angolo") else -3.0, 3.0, -5.0, 5.0)
            quanti += 1
    with open(os.path.join(cartella, f"Mura_{lato}.json"), encoding="utf-8") as f:
        for k in json.load(f)["pezzi"]:
            x, y, z = [v / 100.0 for v in k["unreal_cm"]]
            nome = k["pezzo"]
            if "Torre" in nome:
                abbassa((x, y), k["yaw"], z, -4.3, 4.3, -7.2, 3.0)       # dentro la torre (9 x 9, sporge fuori: -Y)
                quanti += 1
            # la Porta nord no: li' il terreno e' gia' fatto apposta per la soglia e la strada che sale
    Image.fromarray(np.clip(np.round(alt), 0, 65535).astype(np.uint16)).save(percorso, optimize=True)
    Image.fromarray(senza, "RGBA").save(os.path.join(cartella, "T_MontaltoSenzaErba.png"), optimize=True)
    print("terreno abbassato e senza erba sotto %d edifici (case e torri delle mura)" % quanti)
    # i punti di prova: crea_montalto.py li misura in Unreal per capire se il Paesaggio importato e' questo o uno vecchio
    with open(os.path.join(cartella, f"Luoghi_{lato}.json"), encoding="utf-8") as f:
        luoghi_j = json.load(f)

    def quota_cm(x, y, alt=alt):
        fx, fy = min(max(x / passo, 0.0), n - 1.001), min(max(y / passo, 0.0), n - 1.001)
        j, i = int(fx), int(fy)
        a, b = fx - j, fy - i
        v = (alt[i, j] * (1 - a) * (1 - b) + alt[i, j + 1] * a * (1 - b) + alt[i + 1, j] * (1 - a) * b + alt[i + 1, j + 1] * a * b)
        return v / 65535.0 * QUOTA_MAX * 100.0

    def piatto(x, y):
        qs = [quota_cm(x + dx, y + dy) for dx in (-3.0, 0.0, 3.0) for dy in (-3.0, 0.0, 3.0)]
        return max(qs) - min(qs) < 40.0

    prove = []
    for cosa, blocco in (("il cortile della rocca", "rocca"), ("il granaio", "granaio"), ("il cimitero", "cimitero")):
        c = luoghi_j.get(blocco, {}).get("centro_unreal_cm")
        if c:
            prove.append((cosa, c[0] / 100.0, c[1] / 100.0))
    po = luoghi_j.get("porto")
    if po:
        ax, ay = po["asse_x_m"], po["asse_y_m"]
        for d, e in ((40.0, 12.0), (100.0, 12.0)):
            prove.append(("il piazzale del porto", po["origine_m"][0] + ax[0] * d + ay[0] * e,
                          lato - (po["origine_m"][1] + ax[1] * d + ay[1] * e)))
    with open(os.path.join(cartella, f"Case_{lato}.json"), encoding="utf-8") as f:
        cj = json.load(f)["case"]
    for k in cj[::max(len(cj) // 8, 1)][:8]:
        prove.append(("sotto una casa", k["unreal_cm"][0] / 100.0, k["unreal_cm"][1] / 100.0))
    # base_cm = la quota di prima di abbassare: se in Unreal si trova quella, il Paesaggio e' quello del giro prima
    luoghi_j["prove_terreno"] = [{"cosa": c, "unreal_cm": [round(x * 100, 1), round(y * 100, 1), round(quota_cm(x, y), 1)],
                                  "base_cm": round(quota_cm(x, y, prima), 1)}
                                 for c, x, y in prove if piatto(x, y)]
    with open(os.path.join(cartella, f"Luoghi_{lato}.json"), "w", encoding="utf-8") as f:
        json.dump(luoghi_j, f, ensure_ascii=False, indent=1)
    print("punti di prova del terreno: %d" % len(luoghi_j["prove_terreno"]))


if __name__ == "__main__":
    argomenti = list(sys.argv[1:])
    solo_acqua = "--solo-acqua" in argomenti
    if solo_acqua:
        argomenti.remove("--solo-acqua")
    if "--vertici" in argomenti:
        i = argomenti.index("--vertici"); N = int(argomenti[i + 1]); del argomenti[i:i + 2]
    uscita = argomenti[0] if argomenti else "."
    lati = [int(a) for a in argomenti[1:]] or [4000]
    for lato in lati:
        base_ = os.path.join(uscita, f"montalto_{lato}", f"Altezze_{lato}_base.png")
        if not solo_acqua:
            if os.path.exists(base_):
                os.remove(base_)
            montalto(lato, os.path.join(uscita, f"montalto_{lato}"))
        elif os.path.exists(base_):
            # si riparte dal terreno di prima di «sotto gli edifici», senno' case e mura scenderebbero a ogni giro
            Image.open(base_).save(os.path.join(uscita, f"montalto_{lato}", f"Altezze_{lato}.png"))
        acqua_e_senza_erba(os.path.join(uscita, f"montalto_{lato}"), lato)
        case_di_citta(os.path.join(uscita, f"montalto_{lato}"), lato)
        mura_di_citta(os.path.join(uscita, f"montalto_{lato}"), lato)
        sotto_gli_edifici(os.path.join(uscita, f"montalto_{lato}"), lato)
