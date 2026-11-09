# Valdorso - Gli alberi del bosco, fatti da zero (v0.1.4, 09/11/2026, chiesto da Fra). Scritto da Claude.
# Ogni albero e' una Mesh (villaggio_mesh.py): il tronco e i rami di corteccia, le fronde su carte trasparenti (le
# texture di bosco_texture.py). Come gli alberi veri dei giochi: un abete e' un tronco con i palchi di rami, ogni ramo
# una carta piegata che pende; un faggio un tronco che si divide, con i ciuffi di foglie in cima ai rami.
# Le normali delle fronde guardano "fuori dalla chioma" (la luce la prende tutta la chioma, non ogni carta per conto
# suo) e il colore dei vertici fa l'ombra dentro la chioma (piu' scuro vicino al tronco e in basso).
# Quando arrivano gli alberi di Fab si cambia solo il modello: le posizioni del bosco restano (bosco.py).
#
# Gli alberi (Id del modello, altezza in cm):
#   SM_Bosco_Abete_1..4        abete rosso (18-30 m), l'albero del bosco grande
#   SM_Bosco_AbeteGiovane_1..2 abeti giovani (3-5 m) nelle radure e ai margini
#   SM_Bosco_Larice_1..2       larici (16-24 m), piu' chiari e radi, verso i monti
#   SM_Bosco_Faggio_1..3       faggi (15-22 m), nel boschetto e in basso nel bosco grande
#   SM_Bosco_Betulla_1..2      betulle (12-17 m), ai margini e vicino all'acqua
#   SM_Bosco_Melo_1..2         meli (4-5 m), negli orti del villaggio
#   SM_Bosco_Nocciolo_1..3     cespugli di nocciolo (2-3,5 m)
#   SM_Bosco_Ceppo_1..2        ceppi tagliati (vicino al villaggio: la legna)
#   SM_Bosco_Tronco_1..2       tronchi caduti nel bosco

import math

import numpy as np

from villaggio_mesh import Mesh, normalizza, v3


# ------------------------------------------------------------------------------------------------ pezzi

def _base_ortonormale(asse):
    a = normalizza(np.asarray(asse, np.float64))
    e1 = normalizza(np.cross(a, [0, 0, 1] if abs(a[2]) < 0.9 else [1, 0, 0]))
    return a, e1, np.cross(a, e1)


def ramo_cilindro(m, mat, punti, raggi, lati=8, tile_u=100.0, tile_v=100.0, colore=None, tappo=True):
    """Un ramo (o il tronco) lungo una linea spezzata: sezioni rotonde che si stringono (raggi per ogni punto).
    UV: u intorno (in metri di circonferenza / tile), v lungo (cm / tile_v). Il tappo chiude la punta."""
    P, N, UV, C, T = [], [], [], [], []
    lungo = 0.0
    e1_prec = None
    for i, (p, r) in enumerate(zip(punti, raggi)):
        p = np.asarray(p, np.float64)
        if i < len(punti) - 1:
            d = np.asarray(punti[i + 1], np.float64) - p
        else:
            d = p - np.asarray(punti[i - 1], np.float64)
        if i > 0 and i < len(punti) - 1:
            d = normalizza(d) + normalizza(p - np.asarray(punti[i - 1], np.float64))
        a, e1, e2 = _base_ortonormale(d)
        if e1_prec is not None:
            # stessa "cucitura" lungo tutto il ramo (niente torsioni della texture)
            e1 = normalizza(e1_prec - a * np.dot(e1_prec, a))
            e2 = np.cross(a, e1)
        e1_prec = e1
        if i > 0:
            lungo += np.linalg.norm(p - np.asarray(punti[i - 1], np.float64))
        circ = 2 * math.pi * max(r, 1.0)
        giri = max(1.0, round(circ / tile_u))
        for k in range(lati + 1):
            ang = 2 * math.pi * k / lati
            n = e1 * math.cos(ang) + e2 * math.sin(ang)
            P.append(p + n * r)
            N.append(n)
            UV.append([giri * k / lati, -lungo / tile_v])
            C.append(colore if colore is not None else [1, 1, 1, 1])
    for i in range(len(punti) - 1):
        for k in range(lati):
            a = i * (lati + 1) + k
            b = a + lati + 1
            T += [[a, b, a + 1], [a + 1, b, b + 1]]
    m.triangoli(mat, P, N, UV, T, C)
    if tappo and raggi[-1] > 1.5:
        cima = np.asarray(punti[-1], np.float64)
        a = normalizza(cima - np.asarray(punti[-2], np.float64))
        anello = P[-(lati + 1):-1]
        m.triangoli(mat, [cima] + anello, [a] * (lati + 1), [[0.5, 0.5]] + [[0.5, 0.5]] * lati,
                    [[0, k + 1, (k + 1) % lati + 1] for k in range(lati)], [colore or [1, 1, 1, 1]] * (lati + 1))


def linea_curva(base, direzione, lunghezza, piega_su, passi, rng=None, storto=0.0):
    """Punti lungo una linea che parte in 'direzione' e piega verso l'alto (piega_su > 0) o verso il basso."""
    p = np.asarray(base, np.float64)
    d = normalizza(np.asarray(direzione, np.float64))
    pts = [p.copy()]
    for _ in range(passi):
        d = normalizza(d + np.array([0, 0, piega_su / passi]))
        if rng is not None and storto > 0:
            d = normalizza(d + rng.normal(0, storto, 3))
        p = p + d * lunghezza / passi
        pts.append(p.copy())
    return pts


def carta_ramo(m, mat, punti, larghezza, su_locale, centro_chioma, rng, ao_base=1.0, peso_sfera=0.65, larghezza_punta=None):
    """Una carta lunga lungo una linea (un ramo d'abete): u da 0 (tronco) a 1 (punta), v attraverso.
    'su_locale' e' la direzione della faccia (in su di solito); le normali vanno verso fuori dalla chioma."""
    pts = [np.asarray(p, np.float64) for p in punti]
    n_pts = len(pts)
    lunghezze = [0.0]
    for i in range(1, n_pts):
        lunghezze.append(lunghezze[-1] + np.linalg.norm(pts[i] - pts[i - 1]))
    tot = max(lunghezze[-1], 1e-6)
    P, N, UV, C, T = [], [], [], [], []
    for i, p in enumerate(pts):
        d = normalizza(pts[min(i + 1, n_pts - 1)] - pts[max(i - 1, 0)])
        lato = normalizza(np.cross(su_locale, d))
        faccia = normalizza(np.cross(d, lato))
        s = lunghezze[i] / tot
        w = larghezza * (1.0 if larghezza_punta is None else (1 - s) + larghezza_punta * s) * 0.5
        fuori = p - centro_chioma(p)
        fuori[2] *= 0.6
        fuori = normalizza(fuori + np.array([0, 0, 0.35]))
        n = normalizza(faccia * (1 - peso_sfera) + fuori * peso_sfera)
        ao = ao_base * (0.55 + 0.45 * s)
        for j, v in enumerate((-1.0, 1.0)):
            P.append(p + lato * w * v)
            N.append(n)
            UV.append([s, 0.5 + 0.5 * v])
            C.append([ao, ao, ao, 1.0])
    for i in range(n_pts - 1):
        a = 2 * i
        T += [[a, a + 2, a + 1], [a + 1, a + 2, a + 3]]
    m.triangoli(mat, P, N, UV, T, C)


def carta_ciuffo(m, mat, attacco, verso, grandezza, centro, rng, ao=1.0, inclina=0.0):
    """Un ciuffo di foglie: una carta quadrata attaccata in basso al centro (la texture e' fatta cosi'),
    che cresce lungo 'verso'. Le normali guardano fuori dalla chioma (dal centro)."""
    a = np.asarray(attacco, np.float64)
    su = normalizza(np.asarray(verso, np.float64))
    lato = normalizza(np.cross(su, rng.normal(0, 1, 3)))
    if inclina:
        lato = normalizza(lato + np.array([0, 0, inclina]))
    faccia = normalizza(np.cross(lato, su))
    g = grandezza
    angoli = [a - lato * g * 0.5, a + lato * g * 0.5, a + lato * g * 0.5 + su * g, a - lato * g * 0.5 + su * g]
    UV = [[0, 1], [1, 1], [1, 0], [0, 0]]
    N, C = [], []
    for p in angoli:
        fuori = normalizza(p - centro + np.array([0, 0, 0.3 * np.linalg.norm(p - centro)]))
        N.append(normalizza(faccia * 0.3 + fuori * 0.7))
        alt = np.clip((p[2] - centro[2]) / (g * 2) + 0.5, 0, 1)
        dentro = np.clip(np.linalg.norm(p - centro) / (g * 1.6), 0, 1)
        k = ao * (0.45 + 0.35 * dentro + 0.2 * alt)
        C.append([k, k, k, 1])
    m.triangoli(mat, angoli, N, UV, [[0, 1, 2], [0, 2, 3]], C)


# ------------------------------------------------------------------------------------------------ conifere

def conifera(nome, rng, altezza, chioma_da, raggio_max, mat_ramo, palco=42.0, rami_palco=6, pende=-0.55,
             mat_corteccia="CortecciaAbete", larice=False):
    m = Mesh(nome)
    H = altezza
    r0 = max(6.0, H / 75.0)
    # il tronco: un poco storto, svasato alla base, fino alla punta
    passi = max(6, int(H / 220))
    pts, raggi = [], []
    off = np.zeros(3)
    for i in range(passi + 1):
        t = i / passi
        z = -40 + (H + 40) * t
        if 0 < i < passi:
            off = off + rng.normal(0, 4.0, 3) * np.array([1, 1, 0])
        pts.append(v3(0, 0, z) + off * min(1.0, t * 3))
        r = r0 * (1 - t) ** 0.9 + 1.2
        if z < 60:
            r *= 1 + 0.5 * (1 - max(0.0, z + 40) / 100.0)
        raggi.append(r)
    ramo_cilindro(m, mat_corteccia, pts, raggi, lati=10)
    # le radici che escono dal terreno
    for k in range(5):
        ang = 2 * math.pi * k / 5 + rng.uniform(-0.3, 0.3)
        d = np.array([math.cos(ang), math.sin(ang), -0.35])
        ramo_cilindro(m, mat_corteccia, linea_curva(v3(0, 0, 25), d, r0 * 3.2, -0.3, 3),
                      [r0 * 0.55, r0 * 0.4, r0 * 0.25, 2.0], lati=6)

    def asse(z):
        t = np.clip((z + 40) / (H + 40), 0, 1)
        i = min(int(t * passi), passi - 1)
        f = t * passi - i
        return pts[i] * (1 - f) + pts[i + 1] * f

    def centro_chioma(p):
        return asse(p[2])

    # i rami secchi sotto la chioma (spuntoni)
    z = 150.0
    while z < chioma_da:
        for _ in range(rng.integers(1, 4)):
            ang = rng.uniform(0, 2 * math.pi)
            d = np.array([math.cos(ang), math.sin(ang), rng.uniform(-0.4, 0.1)])
            base = asse(z)
            l = rng.uniform(25, 90)
            ramo_cilindro(m, mat_corteccia, [base, base + normalizza(d) * l], [2.2, 0.8], lati=4, tappo=False)
        z += rng.uniform(40, 80)
    # i palchi di rami
    z = chioma_da
    giro = rng.uniform(0, 2 * math.pi)
    while z < H - 25:
        t = (z - chioma_da) / max(1.0, H - chioma_da)
        R = raggio_max * (1 - t) ** 0.85 * rng.uniform(0.85, 1.12) + 30
        quanti = rami_palco if t < 0.85 else max(3, rami_palco - 2)
        giro += rng.uniform(0.4, 1.1)
        for k in range(quanti):
            ang = giro + 2 * math.pi * k / quanti + rng.uniform(-0.25, 0.25)
            d = np.array([math.cos(ang), math.sin(ang), 0.0])
            base = asse(z) + d * 3
            # i rami bassi pendono di piu', quelli in cima salgono
            sale = rng.uniform(0.05, 0.25) + 0.35 * t
            l = R * rng.uniform(0.9, 1.08)
            d0 = normalizza(d + np.array([0, 0, sale]))
            punti = linea_curva(base, d0, l, pende * (1 - 0.6 * t) * (1.3 if larice else 1.0), 4, rng, 0.05)
            su = normalizza(np.array([0, 0, 1]) + rng.normal(0, 0.35, 3))
            ao = 0.65 + 0.35 * min(1.0, t * 1.6)
            carta_ramo(m, mat_ramo, punti, l * rng.uniform(0.85, 1.0), su, centro_chioma, rng, ao_base=ao,
                       larghezza_punta=0.75)
            # un secondo strato quasi in piedi, per dare spessore al ramo (da lato non e' una lama)
            if rng.random() < (0.55 if not larice else 0.3):
                su2 = normalizza(np.cross(d0, np.array([0, 0, 1])) + rng.normal(0, 0.2, 3))
                carta_ramo(m, mat_ramo, punti[:-1], l * 0.6, su2, centro_chioma, rng, ao_base=ao * 0.9,
                           larghezza_punta=0.6)
            # il ramo di legno sotto le fronde (si vede da sotto)
            if t < 0.7:
                ramo_cilindro(m, mat_corteccia, punti[:3], [max(1.5, r0 * 0.18 * (1 - t)), 1.6, 0.8], lati=4, tappo=False)
        z += palco * rng.uniform(0.8, 1.2)
    # la cima: tre carte che salgono
    cima = asse(H)
    for k in range(3):
        ang = giro + 2 * math.pi * k / 3
        d = normalizza(np.array([math.cos(ang) * 0.25, math.sin(ang) * 0.25, 1.0]))
        carta_ramo(m, mat_ramo, linea_curva(asse(H - 70), d, 110, 0.0, 2), 55,
                   np.array([math.cos(ang + 1.57), math.sin(ang + 1.57), 0]), centro_chioma, rng, ao_base=1.0,
                   larghezza_punta=0.3)
    info = {"altezza": H, "raggio_tronco": raggi[1], "chioma": raggio_max, "collisione": "tronco"}
    return m, info


# ------------------------------------------------------------------------------------------------ latifoglie

def latifoglia(nome, rng, altezza, fusto, rami, livelli, mat_corteccia, mat_foglie, ciuffo, chioma_larga=0.32,
               apertura=0.55, raggio_tronco=None, ciuffi_per_ramo=4, pendono=False, polloni=0):
    """Un tronco fino a 'fusto' cm, poi 'rami' rami principali che si dividono 'livelli' volte; i ciuffi di foglie
    in cima e lungo gli ultimi rami. Con polloni > 0 e' un cespuglio: tanti fusti sottili dal terreno."""
    m = Mesh(nome)
    H = altezza
    r0 = raggio_tronco or max(5.0, H / 60.0)
    centro = v3(0, 0, fusto + (H - fusto) * 0.5)
    raggio_chioma = H * chioma_larga
    punte = []

    def cresci(base, d, lung, r, livello):
        punti = linea_curva(base, d, lung, 0.25 if not pendono else -0.1, 4, rng, 0.07)
        ramo_cilindro(m, mat_corteccia, punti, [r, r * 0.85, r * 0.7, r * 0.55, max(0.8, r * 0.4)], lati=max(4, 8 - livello * 2))
        if livello >= livelli:
            punte.append(punti)
            return
        figli = 2 if livello > 0 else 3
        for k in range(figli):
            s = rng.uniform(0.55, 0.95)
            i = int(s * (len(punti) - 1))
            pb = punti[i]
            dir_ramo = normalizza(punti[-1] - punti[0])
            lato = normalizza(np.cross(dir_ramo, rng.normal(0, 1, 3)))
            nd = normalizza(dir_ramo + lato * rng.uniform(0.5, 0.9) + np.array([0, 0, 0.2]))
            # resta dentro la forma della chioma
            dentro = centro - pb
            nd = normalizza(nd + dentro / (np.linalg.norm(dentro) + 1) * 0.15)
            cresci(pb, nd, lung * rng.uniform(0.55, 0.75), r * 0.55, livello + 1)

    if polloni:
        for k in range(polloni):
            ang = rng.uniform(0, 2 * math.pi)
            base = v3(math.cos(ang) * rng.uniform(0, 25), math.sin(ang) * rng.uniform(0, 25), -10)
            d = normalizza(np.array([math.cos(ang) * apertura, math.sin(ang) * apertura, 1.0]))
            cresci(base, d, H * rng.uniform(0.55, 0.8), r0 * rng.uniform(0.6, 1.0), 1)
        info_r = r0 * 2
    else:
        # il fusto
        passi = 4
        pts = [v3(0, 0, -40)]
        off = np.zeros(3)
        for i in range(1, passi + 1):
            off = off + rng.normal(0, 6.0, 3) * np.array([1, 1, 0])
            pts.append(v3(0, 0, fusto * i / passi) + off)
        raggi = [r0 * 1.45, r0 * 1.1, r0, r0 * 0.95, r0 * 0.9]
        ramo_cilindro(m, mat_corteccia, pts, raggi, lati=10, tappo=False)
        for k in range(4):
            ang = 2 * math.pi * k / 4 + rng.uniform(-0.3, 0.3)
            ramo_cilindro(m, mat_corteccia, linea_curva(v3(0, 0, 20), [math.cos(ang), math.sin(ang), -0.4], r0 * 3, -0.2, 3),
                          [r0 * 0.6, r0 * 0.4, r0 * 0.25, 2.0], lati=6)
        cima_fusto = pts[-1]
        giro = rng.uniform(0, 2 * math.pi)
        for k in range(rami):
            ang = giro + 2 * math.pi * k / rami + rng.uniform(-0.3, 0.3)
            d = normalizza(np.array([math.cos(ang) * apertura, math.sin(ang) * apertura, 1.0]))
            cresci(cima_fusto, d, (H - fusto) * rng.uniform(0.5, 0.65), r0 * 0.65, 0)
        # la cima che continua
        cresci(cima_fusto, normalizza(np.array([0.05, 0.0, 1.0])), (H - fusto) * 0.6, r0 * 0.6, 1)
        info_r = raggi[1]
    # i ciuffi di foglie
    for punti in punte:
        for k in range(ciuffi_per_ramo):
            s = 1.0 - k * 0.3
            i = max(1, int(s * (len(punti) - 1)))
            p = punti[i]
            fuori = p - centro
            fuori[2] = fuori[2] * 0.5 + (0.0 if pendono else np.linalg.norm(fuori) * 0.25)
            verso = normalizza(normalizza(fuori) + rng.normal(0, 0.45, 3))
            if pendono:
                verso = normalizza(verso * 0.4 + np.array([0, 0, -1.0]))
            g = ciuffo * rng.uniform(0.75, 1.15)
            carta_ciuffo(m, mat_foglie, p - verso * g * 0.12, verso, g, centro, rng, ao=1.0, inclina=0.3)
    # qualche ciuffo dentro la chioma, piu' scuro, cosi' non si vede attraverso
    for _ in range(int(len(punte) * 1.2)):
        u = rng.normal(0, 1, 3)
        u = normalizza(u) * raggio_chioma * rng.uniform(0.2, 0.55)
        u[2] *= 0.7
        p = centro + u
        verso = normalizza(u + rng.normal(0, 0.5, 3))
        carta_ciuffo(m, mat_foglie, p, verso, ciuffo * 0.9, centro, rng, ao=0.7)
    info = {"altezza": H, "raggio_tronco": info_r, "chioma": raggio_chioma, "collisione": "tronco" if not polloni else "cespuglio"}
    return m, info


def betulla(nome, rng, altezza, chioma_da):
    """La betulla: un fusto bianco sottile fino in cima, rametti sottili lungo il fusto che pendono, la chioma
    stretta e lunga (un ovale) fatta di ciuffi che ricadono."""
    m = Mesh(nome)
    H = altezza
    r0 = H / 85.0
    passi = 7
    pts, raggi = [], []
    off = np.zeros(3)
    for i in range(passi + 1):
        t = i / passi
        if 0 < i:
            off = off + rng.normal(0, 7.0, 3) * np.array([1, 1, 0])
        pts.append(v3(0, 0, -40 + (H + 40) * t) + off)
        raggi.append(r0 * (1 - t) ** 0.8 + 1.0 + (r0 * 0.4 if i == 0 else 0))
    ramo_cilindro(m, "CortecciaBetulla", pts, raggi, lati=9)

    def asse(z):
        t = np.clip((z + 40) / (H + 40), 0, 1)
        i = min(int(t * passi), passi - 1)
        f = t * passi - i
        return pts[i] * (1 - f) + pts[i + 1] * f

    centro = asse(chioma_da + (H - chioma_da) * 0.55)
    R = H * 0.19
    z = chioma_da
    giro = rng.uniform(0, 6.3)
    while z < H - 60:
        t = (z - chioma_da) / (H - chioma_da)
        largo = R * math.sin(math.pi * (0.12 + 0.85 * t)) ** 0.8 + 40
        for k in range(3):
            ang = giro + 2 * math.pi * k / 3 + rng.uniform(-0.4, 0.4)
            d = normalizza(np.array([math.cos(ang), math.sin(ang), 0.55 - 0.2 * t]))
            base = asse(z)
            punti = linea_curva(base, d, largo * rng.uniform(0.9, 1.15), -0.9, 4, rng, 0.06)
            ramo_cilindro(m, "CortecciaBetulla", punti, [max(1.5, r0 * 0.25 * (1 - t)), 1.6, 1.2, 0.9, 0.6], lati=4, tappo=False)
            for j in (2, 3, 4):
                p = punti[j]
                verso = normalizza(np.array([0, 0, -1.0]) + (p - asse(p[2])) / max(1.0, largo) * 0.6 + rng.normal(0, 0.25, 3))
                g = rng.uniform(200, 280) * (0.75 + 0.25 * (1 - t))
                carta_ciuffo(m, "FoglieBetulla", p - verso * g * 0.15, verso, g, centro, rng, ao=0.85 + 0.15 * t, inclina=0.2)
        giro += rng.uniform(0.6, 1.2)
        z += rng.uniform(70, 100)
    for k in range(4):
        d = normalizza(np.array([rng.normal(0, 0.25), rng.normal(0, 0.25), 1.0]))
        carta_ciuffo(m, "FoglieBetulla", asse(H - 120), d, 220, centro, rng, ao=1.0)
    return m, {"altezza": H, "raggio_tronco": raggi[1], "chioma": R, "collisione": "tronco"}


# ------------------------------------------------------------------------------------------------ legno a terra

def _faccia_anelli(m, centro, asse, raggio, lati=12):
    a, e1, e2 = _base_ortonormale(asse)
    P = [np.asarray(centro, np.float64)]
    UV = [[0.5, 0.5]]
    for k in range(lati):
        ang = 2 * math.pi * k / lati
        P.append(P[0] + (e1 * math.cos(ang) + e2 * math.sin(ang)) * raggio)
        UV.append([0.5 + 0.47 * math.cos(ang), 0.5 + 0.47 * math.sin(ang)])
    m.triangoli("CortecciaAnelli", P, [a] * len(P), UV, [[0, k + 1, (k + 1) % lati + 1] for k in range(lati)])


def ceppo(nome, rng, raggio, alto):
    m = Mesh(nome)
    pts = [v3(0, 0, -30), v3(0, 0, alto * 0.3), v3(0, 0, alto)]
    ramo_cilindro(m, "CortecciaAbete", pts, [raggio * 1.25, raggio * 1.02, raggio], lati=12, tappo=False)
    _faccia_anelli(m, v3(0, 0, alto), [0, 0, 1], raggio)
    for k in range(5):
        ang = 2 * math.pi * k / 5 + rng.uniform(-0.3, 0.3)
        ramo_cilindro(m, "CortecciaAbete", linea_curva(v3(0, 0, alto * 0.3), [math.cos(ang), math.sin(ang), -0.5], raggio * 2.2, -0.2, 3),
                      [raggio * 0.4, raggio * 0.3, raggio * 0.18, 2.0], lati=6)
    return m, {"altezza": alto, "raggio_tronco": raggio * 1.1, "chioma": raggio * 1.5, "collisione": "ceppo"}


def tronco_caduto(nome, rng, raggio, lungo):
    m = Mesh(nome)
    pts = [v3(-lungo / 2 + lungo * i / 4, rng.normal(0, 6), raggio * 0.7 + rng.normal(0, 3)) for i in range(5)]
    raggi = [raggio * (1.05 - 0.25 * i / 4) for i in range(5)]
    ramo_cilindro(m, "CortecciaAbete", pts, raggi, lati=10, tappo=False)
    _faccia_anelli(m, pts[0], pts[0] - pts[1], raggi[0])
    _faccia_anelli(m, pts[-1], pts[-1] - pts[-2], raggi[-1])
    for _ in range(6):
        s = rng.uniform(0.2, 0.9)
        base = pts[0] + (pts[-1] - pts[0]) * s
        d = normalizza(np.array([rng.uniform(-0.3, 0.3), rng.choice([-1, 1]), rng.uniform(0.0, 0.8)]))
        ramo_cilindro(m, "CortecciaAbete", [base, base + d * rng.uniform(30, 90)], [raggio * 0.15, 1.5], lati=4, tappo=False)
    return m, {"altezza": raggio * 2, "raggio_tronco": raggio, "chioma": lungo / 2, "collisione": "tronco_a_terra",
               "lungo": lungo}


# ------------------------------------------------------------------------------------------------ il catalogo

def tutti(seme=1109):
    """Tutti i modelli del bosco: {Id: (Mesh, info)}. Sempre uguali (semi fissi)."""
    fatti = {}

    def fai(nome, funzione, *args, **kw):
        rng = np.random.default_rng(seme + len(fatti) * 31)
        m, info = funzione(nome, rng, *args, **kw)
        fatti[nome] = (m, info)

    for i, (h, da) in enumerate(((2600, 520), (2200, 420), (1800, 300), (3000, 700))):
        fai("SM_Bosco_Abete_%d" % (i + 1), conifera, h, da, h * 0.19, "RamoAbete", palco=36.0, rami_palco=7)
    for i, h in enumerate((320, 480)):
        fai("SM_Bosco_AbeteGiovane_%d" % (i + 1), conifera, h, 15, h * 0.3, "RamoAbete", palco=26.0, rami_palco=5)
    for i, (h, da) in enumerate(((2200, 600), (1700, 450))):
        fai("SM_Bosco_Larice_%d" % (i + 1), conifera, h, da, h * 0.21, "RamoLarice", palco=40.0, rami_palco=7,
            pende=-0.85, larice=True)
    for i, (h, f) in enumerate(((2000, 650), (1700, 520), (2200, 800))):
        fai("SM_Bosco_Faggio_%d" % (i + 1), latifoglia, h, f, 4, 3, "CortecciaFaggio", "FoglieFaggio", 420,
            chioma_larga=0.3, ciuffi_per_ramo=5)
    for i, (h, f) in enumerate(((1600, 500), (1300, 380))):
        fai("SM_Bosco_Betulla_%d" % (i + 1), betulla, h, f)
    for i, (h, f) in enumerate(((480, 130), (420, 110))):
        fai("SM_Bosco_Melo_%d" % (i + 1), latifoglia, h, f, 5, 1, "CortecciaMelo", "FoglieMelo", 170,
            chioma_larga=0.45, apertura=1.2, raggio_tronco=11, ciuffi_per_ramo=3)
    for i, h in enumerate((300, 240, 340)):
        fai("SM_Bosco_Nocciolo_%d" % (i + 1), latifoglia, h, 0, 0, 2, "CortecciaMelo", "FoglieNocciolo", 120,
            chioma_larga=0.45, apertura=0.5, raggio_tronco=4, ciuffi_per_ramo=3, polloni=8)
    for i, (r, a) in enumerate(((38, 55), (30, 40))):
        fai("SM_Bosco_Ceppo_%d" % (i + 1), ceppo, r, a)
    for i, (r, l) in enumerate(((30, 820), (24, 560))):
        fai("SM_Bosco_Tronco_%d" % (i + 1), tronco_caduto, r, l)
    return fatti


if __name__ == "__main__":
    import sys
    import os
    uscita = sys.argv[1] if len(sys.argv) > 1 else "uscita_bosco"
    os.makedirs(uscita, exist_ok=True)
    for nome, (m, info) in tutti().items():
        m.scrivi(os.path.join(uscita, nome + ".glb"))
        print(nome, m.conta(), "triangoli", {k: round(v) if isinstance(v, float) else v for k, v in info.items()})
