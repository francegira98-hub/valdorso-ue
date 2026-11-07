# Valdorso - Il belvedere sopra la cascata (v0.1.3). Scritto da Claude il 06/11/2026 (sera).
# Dal piano (idee di Claude sulla cascata, approvate da Fra): "il belvedere sopra l'orlo, per l'immagine del
# Kickstarter". Accanto all'orlo della cascata, 18 m a sinistra del getto (guardando a valle): un terrazzino di
# lastre di pietra su un piccolo bastione, a un metro dal ciglio, con il parapetto basso di sassi a secco (davanti e sui lati), una panca per sedersi
# (ValdorsoPanca) che guarda la valle, l'ometto di sassi dei viandanti e un'iscrizione da leggere con E.
# Il terrazzino guarda fuori dal ciglio (la cascata a sinistra, la valle, il lago e il villaggio davanti); il ciglio si
# misura sul terreno (dove cade di 2 m) e il bordo davanti resta un metro dentro.

import json
import math
import os

import numpy as np

import villaggio_grotta as vg
from villaggio_mesh import Mesh, normalizza

S = "Belvedere"


def belvedere(pr, rng):
    t = pr.t
    with open(os.path.join(t.cartella, "Luoghi_3000.json"), encoding="utf-8") as f:
        c = json.load(f)["cascata"]
    lato_cm = float(t.luoghi["lato_m"]) * 100.0
    ox, oy = c["orlo_m"][0] * 100.0, lato_cm - c["orlo_m"][1] * 100.0
    av = normalizza(np.array([c["direzione"][0], -c["direzione"][1]]))          # verso dove va l'acqua (y a sud)
    lat = np.array([-av[1], av[0]])
    # (cercato su tutto il ciglio dei due lati: i pendii accanto all'orlo sono ripidi, il punto più piano è a 18 m a
    # sinistra del getto, guardando a valle; lì sotto i 4,5 x 3,5 m del terrazzino il terreno scende di circa 3 m,
    # e lo zoccolo diventa un piccolo bastione di pietra)
    fronte = av.copy()                                                            # guarda giù per la valle
    lungo = np.array([-fronte[1], fronte[0]])
    W, D = 450.0, 350.0                                                           # lungo il ciglio x verso fuori
    # il ciglio vero: dal ripiano si va verso valle finché il terreno non cade di 3 m
    dentro = np.array([ox, oy]) - lat * 1800.0 - av * 1500.0
    q_dentro = t.quota(*dentro)
    ciglio = dentro.copy()
    for k in range(150):
        p = dentro + fronte * (k * 20.0)
        if t.quota(*p) < q_dentro - 300.0:
            ciglio = p
            break
    C = ciglio - fronte * (D / 2 + 100.0)                                         # il bordo davanti un metro dentro

    def P(u, v):
        """Punto del mondo (cm) da u (lungo il ciglio) e v (verso fuori)."""
        return C + lungo * u + fronte * v

    q = [t.quota(*P(u, v)) for u in np.linspace(-W / 2, W / 2, 7) for v in np.linspace(-D / 2, D / 2, 5)]
    qmin, qmax = min(q), max(q)
    if qmax - qmin > 250.0:
        pr.avviso("Belvedere: terreno in pendenza (%.1f m sotto il terrazzino)" % ((qmax - qmin) / 100))
    z0 = qmax + 15.0                                                              # il piano delle lastre (quota vera)
    m = Mesh("SM_Belvedere")
    gradi = math.degrees(math.atan2(fronte[1], fronte[0]))

    def scatola(mat, u, v, z, hu, hv, hz, gira=0.0):
        a = math.radians(gira)
        du = lungo * math.cos(a) + fronte * math.sin(a)
        dv = -lungo * math.sin(a) + fronte * math.cos(a)
        p = P(u, v)
        m.scatola(mat, [p[0] - C[0], p[1] - C[1], z - z0], [du[0] * hu, du[1] * hu, 0], [dv[0] * hv, dv[1] * hv, 0], [0, 0, hz])

    # lo zoccolo sotto le lastre, fino a 50 cm sotto il punto più basso
    scatola("Pietra", 0, 0, (z0 - 12.0 + qmin - 50.0) / 2, W / 2 + 10, D / 2 + 10, (z0 - 12.0 - (qmin - 50.0)) / 2)
    # le lastre: file sfalsate, ognuna un poco diversa di quota e di giro
    v = -D / 2
    while v < D / 2 - 1:
        prof = min(rng.uniform(70, 110), D / 2 - v)
        u = -W / 2 + rng.uniform(0, 40)
        scatola("Pietra", (-W / 2 + u) / 2, v + prof / 2, z0 - 6.0, (u + W / 2) / 2 - 2, prof / 2 - 2, 6.0)
        while u < W / 2 - 1:
            lungh = min(rng.uniform(80, 150), W / 2 - u)
            scatola("Pietra", u + lungh / 2, v + prof / 2, z0 - 6.0 + rng.uniform(-1.5, 1.5), lungh / 2 - 2, prof / 2 - 2, 6.0,
                    rng.uniform(-1.5, 1.5))
            u += lungh
        v += prof
    # il parapetto di sassi a secco: davanti (con un varco al centro, dove si guarda giù) e sui due lati
    def muretto(u0, v0, u1, v1, alto=85.0, spesso=55.0):
        a = np.array([u0, v0])
        b = np.array([u1, v1])
        L = float(np.linalg.norm(b - a))
        d = (b - a) / L
        gira = math.degrees(math.atan2(d[1], d[0]))
        n = max(1, int(L / 45.0))
        for corso, h in enumerate((30.0, 28.0, 27.0)):
            sposta = 0.5 if corso % 2 else 0.0
            for k in range(-1 if sposta else 0, n):
                s0 = max(0.0, (k + sposta) / n)
                s1 = min(1.0, (k + 1 + sposta) / n)
                if s1 <= s0:
                    continue
                cen = a + d * (L * (s0 + s1) / 2)
                zb = z0 + sum((30.0, 28.0, 27.0)[:corso])
                scatola("Roccia", cen[0], cen[1], zb + h / 2, L * (s1 - s0) / 2 - 1.5 + rng.uniform(-2, 2),
                        spesso / 2 - corso * 3 + rng.uniform(-3, 3), h / 2 - 1.0, gira + rng.uniform(-4, 4))
        # la copertina: lastre piatte sopra
        for k in range(max(1, int(L / 70.0))):
            cen = a + d * (L * (k + 0.5) / max(1, int(L / 70.0)))
            scatola("Pietra", cen[0], cen[1], z0 + alto + 5.0, L / max(1, int(L / 70.0)) / 2 - 1.0, spesso / 2 + 4.0, 6.0,
                    gira + rng.uniform(-3, 3))

    fv = D / 2 - 30.0
    muretto(-W / 2 + 25, fv, -55.0, fv)
    muretto(55.0, fv, W / 2 - 25, fv)
    muretto(-W / 2 + 25, -D / 2 + 60, -W / 2 + 25, fv)
    muretto(W / 2 - 25, -D / 2 + 60, W / 2 - 25, fv)
    # nel varco del parapetto, un sasso piatto dove mettere i piedi (si guarda giù)
    scatola("Roccia", 0.0, fv + 10.0, z0 + 8.0, 50.0, 30.0, 8.0, rng.uniform(-5, 5))

    # l'ometto di sassi dei viandanti, nell'angolo dietro a destra
    om = P(W / 2 - 85.0, -D / 2 + 70.0)
    zc = z0
    for k, r in enumerate((32.0, 27.0, 23.0, 19.0, 15.0, 12.0, 9.0)):
        vg._sasso(m, [om[0] - C[0] + rng.uniform(-3, 3), om[1] - C[1] + rng.uniform(-3, 3), zc - z0 + r * 0.55], r, rng,
                  600 + k)
        zc += r * 0.95

    pr.metti_mesh(S, "Belvedere", m, C[0], C[1], z0, 0.0, collisione=True)
    pr.cose[-1]["suolo"] = True
    # la panca (ValdorsoPanca): dietro, al centro, guarda fuori (come le panche del villaggio: +X verso dove si guarda)
    pb = P(0.0, -D / 2 + 70.0)
    pr.aggiungi(S, "Panca del belvedere", "panca", pb[0], pb[1], z0, gradi, suolo=True)
    # l'iscrizione: sulla faccia interna del parapetto, a sinistra del varco; si legge stando sul terrazzino (+X = indietro)
    pi = P(-140.0, fv - 32.0)
    pr.aggiungi(S, "Iscrizione del belvedere", "iscrizione", pi[0], pi[1], z0 - 95.0, gradi + 180.0, suolo=True,
                titolo="Il belvedere",
                avvisi=["Da qui si vede tutta la valle: il fiume che corre al lago, il villaggio sulla riva, la collina dell'Orso",
                        "e il bosco grande a ovest, dove stanno le rovine. Nelle giornate chiare si vede la gola del passo.",
                        "Sotto, a sinistra, la Vena salta trenta metri nella pozza. Il rombo si sente nel petto.",
                        "L'ometto di sassi nell'angolo lo fanno i viandanti: chi sale fin qui ne aggiunge uno. Porta bene.",
                        "Il muretto è basso. Non sporgersi."])
    pr.zone_senza_erba.append((C[0], C[1], W / 2 + 60, W / 2 + 60))
    pr.disegni.append((S, "linea", [tuple(P(u, v)) for u, v in ((-W / 2, -D / 2), (W / 2, -D / 2), (W / 2, D / 2), (-W / 2, D / 2), (-W / 2, -D / 2))],
                       "Belvedere"))
    sentiero(pr, rng, C, z0, fronte, D)
    print("belvedere: terrazzino %d x %d m a %.1f m, dislivello sotto %.1f m, ciglio a %.1f m dal centro, guarda a %.0f gradi" %
          (W / 100, D / 100, z0 / 100, (qmax - qmin) / 100, float(np.linalg.norm(ciglio - C)) / 100, gradi))
    return {"C": C, "z0": z0, "fronte": fronte, "gradi": gradi}


def sentiero(pr, rng, C, z0, fronte, D):
    """(07/11) Il sentiero del belvedere (Fra: "ma come si arriva?"). Dal retro del terrazzino al prato in piano sotto
    la cascata (le strade dipinte sono lontane): il percorso più corto su una griglia di 1,5 m, che costa poco dove si cammina (pendenza sotto 0,75), molto
    dove serve un gradino (fino a 1,8) e niente dove c'è l'acqua o la parete. Sopra: una striscia di terra battuta
    larga 1,4 m (sulla mappa di carta e senza erba vera) e, dove il pendio è ripido, gradini di pietra con il piano dritto."""
    import heapq

    from PIL import Image

    import villaggio as vl
    t = pr.t
    # in cima: qualche gradino dal terreno al piano delle lastre, attaccati al retro dello zoccolo (alzata 22 cm al
    # massimo, pedata 35 cm); il sentiero arriva al piede del primo
    retro = C - fronte * (D / 2 + 10.0)
    z_piede = t.quota(*(retro - fronte * 120.0))
    n_alti = max(1, int(math.ceil((z0 - z_piede) / 22.0)) - 1)
    E = retro - fronte * (n_alti * 35.0 + 40.0)
    lato_cm = float(t.luoghi["lato_m"]) * 100.0
    strade = np.array(Image.open(os.path.join(t.cartella, "Strati", "Strade.png")).convert("L"))
    acqua = np.array(Image.open(os.path.join(t.cartella, "Strati", "Acqua.png")).convert("L"))
    k = strade.shape[0] / lato_cm

    # (la strada dipinta più vicina è a 700 m: la cascata sta nei prati. Il sentiero parte dal prato in piano più
    # vicino, almeno 30 m sotto il belvedere; se c'è una strada entro 150 m, parte dalla strada)
    PASSO = 150.0
    R = 13000.0
    xs = np.arange(E[0] - R, E[0] + R, PASSO)
    ys = np.arange(E[1] - R, E[1] + R, PASSO)
    H = np.array([[t.quota(x, y) for x in xs] for y in ys])
    gy, gx = np.gradient(H, PASSO)
    pend = np.hypot(gx, gy)
    ri = np.clip((ys * k).astype(int), 0, strade.shape[0] - 1)
    ci = np.clip((xs * k).astype(int), 0, strade.shape[1] - 1)
    in_acqua = acqua[np.ix_(ri, ci)] > 100
    dist_E = np.hypot(*np.meshgrid(xs - E[0], ys - E[1]))
    su_strada = (strade[np.ix_(ri, ci)] > 128) & (dist_E < 15000)
    su_strada |= (pend < 0.3) & (H < z0 - 3000) & ~in_acqua
    costo = np.where(pend < 0.75, 1.0, np.where(pend < 1.8, 6.0, 1e6))
    costo = np.where(in_acqua, 1e6, costo)
    ni, nj = H.shape
    s = (int(round((E[1] - ys[0]) / PASSO)), int(round((E[0] - xs[0]) / PASSO)))
    dist = np.full(H.shape, np.inf)
    prima = {}
    dist[s] = 0.0
    coda = [(0.0, s)]
    meta = None
    while coda:
        dd, (i, j) = heapq.heappop(coda)
        if dd > dist[i, j]:
            continue
        if su_strada[i, j]:
            meta = (i, j)
            break
        for di in (-1, 0, 1):
            for dj in (-1, 0, 1):
                if di == 0 and dj == 0:
                    continue
                a, b = i + di, j + dj
                if not (0 <= a < ni and 0 <= b < nj):
                    continue
                w = math.hypot(di, dj) * 0.5 * (costo[i, j] + costo[a, b])
                if dd + w < dist[a, b]:
                    dist[a, b] = dd + w
                    prima[(a, b)] = (i, j)
                    heapq.heappush(coda, (dd + w, (a, b)))
    if meta is None or dist[meta] >= 1e5:
        pr.avviso("Belvedere: nessun passaggio fino al prato o alla strada, niente sentiero")
        return
    celle = [meta]
    while celle[-1] != s:
        celle.append(prima[celle[-1]])
    punti = [np.array([xs[j], ys[i]]) for i, j in celle]               # dal prato al belvedere
    punti[-1] = E.copy()
    # lisciato due volte (Chaikin), poi un punto ogni 45 cm
    for _ in range(2):
        nuovi = [punti[0]]
        for a, b in zip(punti[:-1], punti[1:]):
            nuovi += [a * 0.75 + b * 0.25, a * 0.25 + b * 0.75]
        nuovi.append(punti[-1])
        punti = nuovi
    m = Mesh("SM_Belvedere_Sentiero")
    ox, oy = C[0], C[1]
    vl.striscia(m, t, ox, oy, z0, [tuple(p) for p in punti], 140.0, mat="Terra", alza=6.0, passo=80.0)
    # i gradini dove il pendio è ripido
    fitti = []
    for a, b in zip(punti[:-1], punti[1:]):
        L = float(np.linalg.norm(b - a))
        for q in range(max(1, int(L / 45.0))):
            fitti.append(a + (b - a) * q / max(1, int(L / 45.0)))
    gradini = 0
    for i in range(1, len(fitti) - 1):
        p = fitti[i]
        e = 60.0
        pendenza = math.hypot(t.quota(p[0] + e, p[1]) - t.quota(p[0] - e, p[1]),
                              t.quota(p[0], p[1] + e) - t.quota(p[0], p[1] - e)) / (2 * e)
        if pendenza < 0.65:
            continue
        d = normalizza(fitti[i + 1] - fitti[i - 1])
        nrm = np.array([-d[1], d[0]])
        q = [t.quota(*(p + nrm * u)) for u in (-80.0, 0.0, 80.0)]
        top = q[1] + 12.0 + 0.25 * (max(q) - min(q))
        fondo = min(q) - 40.0
        m.scatola("Pietra", [p[0] - ox, p[1] - oy, (top + fondo) / 2 - z0], [d[0] * 22.0, d[1] * 22.0, 0],
                  [nrm[0] * (75.0 + rng.uniform(-6, 6)), nrm[1] * (75.0 + rng.uniform(-6, 6)), 0], [0, 0, (top - fondo) / 2])
        gradini += 1
    lungo_r = np.array([-fronte[1], fronte[0]])
    for g in range(1, n_alti + 1):
        p = retro - fronte * (g * 35.0 - 17.5)
        top = z0 - (z0 - t.quota(*E)) * g / (n_alti + 1)
        fondo = min(t.quota(*(p + lungo_r * u)) for u in (-80.0, 80.0)) - 40.0
        m.scatola("Pietra", [p[0] - ox, p[1] - oy, (top + fondo) / 2 - z0], [fronte[0] * 17.0, fronte[1] * 17.0, 0],
                  [lungo_r[0] * 80.0, lungo_r[1] * 80.0, 0], [0, 0, (top - fondo) / 2])
    pr.metti_mesh(S, "Sentiero del belvedere", m, ox, oy, z0, 0.0, collisione=True)
    pr.cose[-1]["suolo"] = True
    pr.disegni.append((S, "strada", [tuple(p) for p in punti[::3]] + [tuple(punti[-1])], 140))
    lungo = sum(float(np.linalg.norm(b - a)) for a, b in zip(punti[:-1], punti[1:])) / 100
    print("sentiero del belvedere: %.0f m dal prato, %d gradini, parte a %.1f m" % (lungo, gradini, t.quota(*punti[0]) / 100))
