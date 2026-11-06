# Valdorso - Gli edifici del villaggio, fatti a pezzi (v0.1.3). Scritto da Claude il 05/11/2026.
# Ogni funzione restituisce (Mesh, info): la mesh ha l'origine al centro della pianta, all'altezza del pavimento;
# la facciata con la porta guarda verso -Y. info["porte"] = le porte vere (AValdorsoPorta) da mettere: (x, y, gradi,
# a chiave); info["cose"] = altre cose da mettere dentro (letti, panche, tavoli...), sempre in coordinate locali.

import math

import numpy as np

from villaggio_mesh import Mesh, normalizza

SPESSORE = 40.0
PORTA_L, PORTA_H = 112.0, 222.0       # il buco: l'anta di AValdorsoPorta è 100 x 210


def _finestre(lunghezza, n, larghezza, evita=()):
    """Posizioni (centri, cm dall'inizio del muro) di n finestre distribuite, lontane dalle 'evita' (u0, u1)."""
    if n <= 0:
        return []
    passo = lunghezza / (n + 1)
    centri = [passo * (i + 1) for i in range(n)]
    buoni = []
    for c in centri:
        if all(c + larghezza / 2 + 40 < e0 or c - larghezza / 2 - 40 > e1 for e0, e1 in evita):
            buoni.append(c)
    return buoni


def casa(nome, W, D, piani=1, base="Pietra", sopra="Intonaco", tetto="Coppi", pendenza=32.0, fondazione=100.0,
         porta_x=0.0, porta_larga=PORTA_L, porta_alta=PORTA_H, finestre_aperte=False, camino=True, graticcio=True,
         h1=300.0, h2=280.0, finestre_lato=True, solaio_aperto=None, porta_retro=False, seme=0, porte_vere=True,
         finestra_l=80.0, finestra_h=100.0, sporto=50.0):
    """Casa rettangolare: W lungo la facciata (X), D di profondità (Y). La porta è sulla facciata (-Y)."""
    rng = np.random.default_rng(seme)
    m = Mesh(nome)
    T = SPESSORE
    info = {"porte": [], "cose": [], "W": W, "D": D}
    altezze = [h1] + [h2] * (piani - 1)
    # zoccolo di pietra: dal fondo (sotto terra) al pavimento
    m.blocco({"altri": "Pietra", "+z": "Assi"}, -W / 2 - 10, W / 2 + 10, -D / 2 - 10, D / 2 + 10, -fondazione, 0)
    # gradino davanti alla porta
    m.blocco("Pietra", porta_x - porta_larga / 2 - 20, porta_x + porta_larga / 2 + 20, -D / 2 - 60, -D / 2 - 10, -fondazione, -2)
    z = 0.0
    muri = [
        # (origine, gradi, lunghezza, verso fuori, lato)
        ((-W / 2, -D / 2 + T / 2), 0.0, W, np.array([0, -1.0, 0]), "fronte"),
        ((-W / 2, D / 2 - T / 2), 0.0, W, np.array([0, 1.0, 0]), "retro"),
        ((-W / 2 + T / 2, -D / 2 + T), 90.0, D - 2 * T, np.array([-1.0, 0, 0]), "sinistra"),
        ((W / 2 - T / 2, -D / 2 + T), 90.0, D - 2 * T, np.array([1.0, 0, 0]), "destra"),
    ]
    for k, h in enumerate(altezze):
        mat = base if k == 0 else sopra
        for (ox, oy), gradi, L, fuori, lato in muri:
            buchi = []
            if lato in ("sinistra", "destra") and not finestre_lato and k == 0:
                n = 0
            else:
                n = max(0, int((L - 60) / 260))
            evita = []
            if k == 0 and lato == "fronte":
                u = porta_x + W / 2
                buchi.append((u - porta_larga / 2, u + porta_larga / 2, 0.0, porta_alta))
                evita.append((u - porta_larga / 2, u + porta_larga / 2))
            if k == 0 and lato == "retro" and porta_retro:
                u = W / 2 - 150
                buchi.append((u - PORTA_L / 2, u + PORTA_L / 2, 0.0, PORTA_H))
                evita.append((u - PORTA_L / 2, u + PORTA_L / 2))
            fl, fh = finestra_l, finestra_h
            davanzale = 100.0 if k == 0 else 80.0
            for c in _finestre(L, n, fl, evita):
                buchi.append((c - fl / 2, c + fl / 2, davanzale, min(h - 30, davanzale + fh)))
            m.muro(mat, (ox, oy, z), gradi, L, h, T, buchi)
            a = math.radians(gradi)
            u = np.array([math.cos(a), math.sin(a), 0.0])
            o = np.array([ox, oy, z])
            faccia = o + fuori * (T / 2)
            for (u0, u1, z0, z1) in buchi:
                porta = z0 == 0.0
                # architrave
                m.trave("Trave", faccia + u * (u0 - 15) + [0, 0, z1 + 9] + fuori * 3, faccia + u * (u1 + 15) + [0, 0, z1 + 9] + fuori * 3, 10, 18, su=fuori)
                if porta:
                    for uu in (u0 - 7, u1 + 7):
                        m.trave("Trave", faccia + u * uu + fuori * 3, faccia + u * uu + [0, 0, z1] + fuori * 3, 14, 8)
                    if porte_vere:
                        centro = o + u * ((u0 + u1) / 2)
                        info["porte"].append((float(centro[0]), float(centro[1]), float(gradi), lato))
                    continue
                # davanzale
                m.trave("Pietra" if mat == "Pietra" else "Trave", faccia + u * (u0 - 8) + [0, 0, z0 - 5] + fuori * 4,
                        faccia + u * (u1 + 8) + [0, 0, z0 - 5] + fuori * 4, 10, 10, su=fuori)
                if not finestre_aperte:
                    # vetro scuro a metà muro (non si passa) e imposte aperte contro il muro
                    m.scatola("Vetro", o + u * ((u0 + u1) / 2) + [0, 0, (z0 + z1) / 2], u * (u1 - u0) / 2, fuori * 3, [0, 0, (z1 - z0) / 2])
                    # croce dell'infisso
                    m.trave("Trave", o + u * ((u0 + u1) / 2) + [0, 0, z0] + fuori * 4, o + u * ((u0 + u1) / 2) + [0, 0, z1] + fuori * 4, 5, 5)
                    m.trave("Trave", o + u * u0 + [0, 0, (z0 + z1) / 2] + fuori * 4, o + u * u1 + [0, 0, (z0 + z1) / 2] + fuori * 4, 5, 5)
                lw = (u1 - u0) / 2
                for lato_imposta, uu in ((-1, u0 - lw / 2 - 4), (1, u1 + lw / 2 + 4)):
                    m.scatola("Assi", faccia + u * uu + [0, 0, (z0 + z1) / 2] + fuori * 3, u * lw / 2, fuori * 2, [0, 0, (z1 - z0) / 2])
            # graticcio: travi sulla faccia esterna dei piani alti (o di tutta la casa, se è di intonaco)
            if graticcio and mat == "Intonaco":
                f3 = faccia + fuori * 3
                m.trave("Trave", f3 + [0, 0, 9], f3 + u * L + [0, 0, 9], 18, 10, su=fuori)
                m.trave("Trave", f3 + [0, 0, h - 9], f3 + u * L + [0, 0, h - 9], 18, 10, su=fuori)
                montanti = [9.0, L - 9.0]
                for (u0, u1, z0, z1) in buchi:
                    montanti += [u0 - 9, u1 + 9]
                montanti = sorted(set(round(x, 1) for x in montanti if 0 < x < L))
                for x in montanti:
                    m.trave("Trave", f3 + u * x + [0, 0, 18], f3 + u * x + [0, 0, h - 18], 16, 10)
                # diagonali nei campi vuoti larghi
                for a0, a1 in zip(montanti[:-1], montanti[1:]):
                    libero = all(not (b[0] < (a0 + a1) / 2 < b[1]) for b in buchi)
                    if libero and a1 - a0 > 110:
                        if rng.random() < 0.5:
                            m.trave("Trave", f3 + u * (a0 + 8) + [0, 0, 18], f3 + u * (a1 - 8) + [0, 0, h - 18], 14, 8, su=fuori)
                        else:
                            m.trave("Trave", f3 + u * (a1 - 8) + [0, 0, 18], f3 + u * (a0 + 8) + [0, 0, h - 18], 14, 8, su=fuori)
        # solaio tra un piano e l'altro
        if k < len(altezze) - 1:
            buchi_solaio = [solaio_aperto] if solaio_aperto else []
            m.lastra({"altri": "Trave", "+z": "Assi", "-z": "Assi"}, -W / 2 + T, W / 2 - T, -D / 2 + T, D / 2 - T, z + h - 22, z + h, buchi_solaio)
            # trave di bordo che sporge (il solaio si vede da fuori)
            for (ox, oy), gradi, L, fuori, lato in muri[:2]:
                a = math.radians(gradi)
                u = np.array([math.cos(a), math.sin(a), 0.0])
                o = np.array([ox, oy, z + h])
                m.trave("Trave", o + fuori * (T / 2 + 6) - u * 10, o + fuori * (T / 2 + 6) + u * (L + 10), 22, 20, su=fuori)
        z += h
    cima = z
    info["cima"] = cima
    # soffitto dell'ultimo piano (assi), così da dentro non si vede il tetto vuoto
    m.blocco({"altri": "Trave", "-z": "Assi", "+z": "Assi"}, -W / 2 + T, W / 2 - T, -D / 2 + T, D / 2 - T, cima - 15, cima)
    # angolari di pietra (cantonali) al piano terra
    if base == "Pietra":
        for sx in (-1, 1):
            for sy in (-1, 1):
                for i in range(int(h1 / 40)):
                    lx = 34 if i % 2 == 0 else 22
                    ly = 22 if i % 2 == 0 else 34
                    x0, x1 = (W / 2 - lx, W / 2 + 2) if sx > 0 else (-W / 2 - 2, -W / 2 + lx)
                    y0, y1 = (D / 2 - ly, D / 2 + 2) if sy > 0 else (-D / 2 - 2, -D / 2 + ly)
                    m.blocco("Pietra", x0, x1, y0, y1, i * 40 + 1, i * 40 + 39)
    # tetto a capanna, colmo lungo X
    tetto_casa(m, W, D, cima, tetto, pendenza, sporto=sporto, gable=sopra if piani > 1 else (sopra if base != "Pietra" else "Pietra"))
    if camino:
        cx = W / 2 - 110 if rng.random() < 0.5 else -W / 2 + 110
        s = D / 2
        alto = cima + s * math.tan(math.radians(pendenza)) + 90
        m.blocco({"altri": "Pietra", "+z": "Ferro"}, cx - 40, cx + 40, D / 4 - 40, D / 4 + 40, cima - 15, alto)
        m.blocco("Pietra", cx - 48, cx + 48, D / 4 - 48, D / 4 + 48, alto, alto + 12)
    return m, info


def tetto_casa(m, W, D, cima, tetto, pendenza, sporto=50.0, gronda=40.0, gable="Intonaco", spessore=None):
    t = math.tan(math.radians(pendenza))
    s = D / 2
    spessore = spessore or (35.0 if tetto == "Paglia" else 16.0)
    hx = W / 2 + gronda
    colmo = cima + s * t
    for lato in (-1, 1):
        y_gronda = lato * (s + sporto)
        z_gronda = cima - sporto * t
        a = np.array([0, y_gronda, z_gronda])
        b = np.array([0, 0.0, colmo])
        su = normalizza(b - a)
        n = normalizza(np.cross([1.0, 0, 0], su))
        if n[2] < 0:
            n = -n
        lung = np.linalg.norm(b - a)
        centro = (a + b) / 2 + n * spessore / 2
        m.scatola({"+z": tetto, "-z": "Assi", "altri": "Trave"}, centro, [hx, 0, 0], su * lung / 2, n * spessore / 2)
    # colmo
    m.trave(tetto, [-hx, 0, colmo + spessore * 0.9], [hx, 0, colmo + spessore * 0.9], 30 if tetto != "Paglia" else 50, 16 if tetto != "Paglia" else 30)
    # timpani (triangoli) sui due lati corti, spessi come il muro
    for sx in (-1, 1):
        for x in (sx * W / 2, sx * (W / 2 - SPESSORE)):
            m.poligono(gable, [[x, -s, cima], [x, s, cima], [x, 0, colmo]], asse_u=[0, 1, 0])


def torre(nome, lato=500.0, altezza=1200.0, fondazione=150.0, tetto="Coppi", base="Pietra", porta=True, merli=False):
    """Torre quadrata di pietra con una porta, feritoie, un ballatoio di legno in cima e il tetto a piramide."""
    m = Mesh(nome)
    T = 60.0
    info = {"porte": [], "cose": []}
    m.blocco({"altri": "Pietra", "+z": "Assi"}, -lato / 2 - 15, lato / 2 + 15, -lato / 2 - 15, lato / 2 + 15, -fondazione, 0)
    muri = [((-lato / 2, -lato / 2 + T / 2), 0.0, lato), ((-lato / 2, lato / 2 - T / 2), 0.0, lato),
            ((-lato / 2 + T / 2, -lato / 2 + T), 90.0, lato - 2 * T), ((lato / 2 - T / 2, -lato / 2 + T), 90.0, lato - 2 * T)]
    for i, ((ox, oy), g, L) in enumerate(muri):
        buchi = []
        if i == 0 and porta:
            buchi.append((L / 2 - PORTA_L / 2, L / 2 + PORTA_L / 2, 0, PORTA_H))
        for zz in (altezza * 0.35, altezza * 0.62):
            buchi.append((L / 2 - 12, L / 2 + 12, zz, zz + 90))
        m.muro(base, (ox, oy, 0), g, L, altezza, T, buchi)
    if porta:
        info["porte"].append((0.0, -lato / 2 + T / 2, 0.0, "fronte"))
        m.trave("Trave", [-PORTA_L / 2 - 15, -lato / 2 - 3, PORTA_H + 9], [PORTA_L / 2 + 15, -lato / 2 - 3, PORTA_H + 9], 10, 18, su=[0, -1, 0])
    # (06/11) la scala a pioli: dentro, contro il muro di fondo, dal pavimento fino al ballatoio, che ha una botola.
    # Il colono sale con la faccia al muro, 50 cm davanti ai montanti (vedi AValdorsoScala).
    buchi = []
    if porta:
        y_scala = lato / 2 - T - 8                     # i montanti, 8 cm davanti al muro di fondo
        buchi = [(-60.0, 60.0, y_scala - 110.0, y_scala + 5.0)]
        for sx in (-1, 1):
            m.blocco("Trave", sx * 25 - 4, sx * 25 + 4, y_scala - 4, y_scala + 4, 0, altezza + 20)
        z = 30.0
        while z < altezza + 10:
            m.cilindro("Trave", [-25, y_scala, z], 2.5, 50, lati=6, asse=(1, 0, 0))
            z += 30.0
        # (06/11) intorno alla botola, sul ballatoio, una ringhiera di legno su tre lati: resta aperto solo il lato
        # da cui si esce (verso -Y), così non ci si cade dentro camminando
        zb = altezza + 20
        y0, y1 = y_scala - 110.0, y_scala + 5.0
        for x in (-64.0, 64.0):
            m.blocco("Trave", x - 4, x + 4, y0 - 4, y1 + 4, zb, zb + 100)
        m.blocco("Trave", -68, 68, y1, y1 + 8, zb, zb + 100)
        for x in (-64.0, 64.0):
            m.blocco("Trave", x - 5, x + 5, y0 - 5, y0 + 5, zb, zb + 110)
        # la scala guarda verso -Y (verso il colono): in gradi locali -90; l'uscita in cima, 170 cm davanti ai montanti
        info["cose"].append(("scala", 0.0, y_scala, -90.0, 0.0, {"altezza": round(altezza + 20.0, 1), "uscita_cima": 170.0}))
    # cornice di pietra e ballatoio di legno sporgente (con la botola sopra la scala)
    m.lastra("Pietra", -lato / 2 - 25, lato / 2 + 25, -lato / 2 - 25, lato / 2 + 25, altezza - 30, altezza, buchi)
    sb = lato / 2 + 90
    m.lastra({"altri": "Trave", "+z": "Assi", "-z": "Assi"}, -sb, sb, -sb, sb, altezza, altezza + 20, buchi)
    for sx in (-1, 1):
        for sy in (-1, 1):
            # mensole sotto il ballatoio
            m.trave("Trave", [sx * lato / 2, sy * lato / 2, altezza - 120], [sx * sb, sy * sb, altezza], 16, 16)
            m.cilindro("Trave", [sx * (sb - 15), sy * (sb - 15), altezza + 20], 12, 260, lati=6)
    for k in range(4):
        a = [(-sb, -sb), (sb, -sb), (sb, sb), (-sb, sb)]
        p0, p1 = a[k], a[(k + 1) % 4]
        for zz in (altezza + 60, altezza + 115):
            m.trave("Trave", [p0[0] * 0.97, p0[1] * 0.97, zz], [p1[0] * 0.97, p1[1] * 0.97, zz], 10, 12)
        # assi del parapetto
        m.scatola("Assi", [(p0[0] + p1[0]) / 2 * 0.98, (p0[1] + p1[1]) / 2 * 0.98, altezza + 20 + 45],
                  [(p1[0] - p0[0]) / 2, (p1[1] - p0[1]) / 2, 0], normalizza(np.array([-(p1[1] - p0[1]), p1[0] - p0[0], 0])) * 2.5, [0, 0, 45])
    m.piramide(tetto, [0, 0, altezza + 280], sb, sb, sb * 0.9, sporgenza=40)
    if merli:
        pass
    info["cima"] = altezza
    return m, info


def palizzata(nome, punti, raggio=13.0, altezza=380.0, affondo=60.0, varchi=(), seme=1):
    """Pali appuntiti uno accanto all'altro lungo una spezzata di punti (x, y, z_terreno), con due traverse dietro.
    varchi: lista di (indice_tratto, t0, t1) dove non ci sono pali (le porte)."""
    rng = np.random.default_rng(seme)
    m = Mesh(nome)
    for i in range(len(punti) - 1):
        a = np.asarray(punti[i], np.float64)
        b = np.asarray(punti[i + 1], np.float64)
        d = b[:2] - a[:2]
        L = np.linalg.norm(d)
        if L < 1:
            continue
        u = np.array([d[0] / L, d[1] / L, 0])
        nrm = np.array([-u[1], u[0], 0])
        n = int(L / (raggio * 2.05))
        tratto = []
        for k in range(n + 1):
            t = k / max(1, n)
            if any(v[0] == i and v[1] <= t <= v[2] for v in varchi):
                tratto.append(None)
                continue
            p = a + (b - a) * t
            h = altezza * rng.uniform(0.9, 1.08)
            r = raggio * rng.uniform(0.85, 1.15)
            m.cilindro("Trave", [p[0], p[1], p[2] - affondo], r, h + affondo, lati=6, punta=r * 2.6)
            tratto.append((p, h))
        # traverse dietro (dalla parte +nrm, dentro il villaggio se i punti girano in senso orario)
        for zz in (90.0, altezza - 90.0):
            pezzo = []
            for el in tratto + [None]:
                if el is not None:
                    pezzo.append(el[0])
                elif len(pezzo) >= 2:
                    m.trave("Trave", pezzo[0] + nrm * (raggio + 7) + [0, 0, zz], pezzo[-1] + nrm * (raggio + 7) + [0, 0, zz], 12, 12)
                    pezzo = []
                else:
                    pezzo = []
    return m


def muro_cinta(nome, punti, altezza=620.0, spessore=180.0, affondo=150.0, passo_merli=150.0, seme=1):
    """(06/11, fase 3) Muro di cinta di pietra lungo una spezzata di punti (x, y, z_terreno), a gradini che seguono il
    terreno, con i merli sul lato di fuori (-nrm: fuori dal villaggio se i punti girano in senso orario, come la
    palizzata) e una cornice. Un blocco ogni tratto: i tratti corti (3 m) fanno i gradini."""
    rng = np.random.default_rng(seme)
    m = Mesh(nome)
    for i in range(len(punti) - 1):
        a = np.asarray(punti[i], np.float64)
        b = np.asarray(punti[i + 1], np.float64)
        d = b[:2] - a[:2]
        L = float(np.linalg.norm(d))
        if L < 1:
            continue
        u = np.array([d[0] / L, d[1] / L, 0.0])
        nrm = np.array([-u[1], u[0], 0.0])
        z0 = min(a[2], b[2]) - affondo
        z1 = max(a[2], b[2]) + altezza
        c = (a + b) / 2
        # il corpo del muro (un poco più lungo, così i tratti si toccano senza fessure)
        m.scatola("Pietra", [c[0], c[1], (z0 + z1) / 2], u * (L / 2 + 8), nrm * spessore / 2, [0, 0, (z1 - z0) / 2])
        # la cornice sporgente in cima
        m.scatola("Pietra", [c[0], c[1], z1 + 15], u * (L / 2 + 8), nrm * (spessore / 2 + 12), [0, 0, 15])
        # i merli, dalla parte di fuori
        n = max(1, int(L / passo_merli))
        for k in range(n):
            if k % 2:
                continue
            p = a[:2] + d * (k + 0.5) / n
            h = 75.0 * rng.uniform(0.95, 1.05)
            m.scatola("Pietra", [p[0] - nrm[0] * spessore * 0.3, p[1] - nrm[1] * spessore * 0.3, z1 + 30 + h / 2],
                      u * min(40.0, L / n / 2), nrm * spessore * 0.2, [0, 0, h / 2])
    return m


def recinto(nome, punti, altezza=110.0, passo=220.0, mat="Trave", chiuso=False):
    """Staccionata: paletti e due correnti, lungo punti (x, y, z_terreno)."""
    m = Mesh(nome)
    P = list(punti) + ([punti[0]] if chiuso else [])
    for i in range(len(P) - 1):
        a = np.asarray(P[i], np.float64)
        b = np.asarray(P[i + 1], np.float64)
        L = np.linalg.norm(b[:2] - a[:2])
        n = max(1, int(round(L / passo)))
        pali = [a + (b - a) * k / n for k in range(n + 1)]
        for p in pali:
            m.cilindro(mat, [p[0], p[1], p[2] - 40], 7, altezza + 40, lati=5)
        for k in range(n):
            for zz in (altezza * 0.45, altezza * 0.9):
                m.trave(mat, pali[k] + [0, 0, zz], pali[k + 1] + [0, 0, zz], 7, 9)
    return m


def pozzo(nome):
    m = Mesh(nome)
    lati = 14
    r_fuori, r_dentro, h = 95.0, 70.0, 90.0
    m.cilindro("Pietra", [0, 0, -40], r_fuori, h + 40, lati=lati, tappo=False)
    m.cilindro("Pietra", [0, 0, -300], r_dentro, h + 300, lati=lati, tappo=False)
    # bordo superiore (anello)
    P, T = [], []
    for i in range(lati + 1):
        a = 2 * math.pi * i / lati
        P += [[math.cos(a) * r_dentro, math.sin(a) * r_dentro, h], [math.cos(a) * (r_fuori + 6), math.sin(a) * (r_fuori + 6), h]]
    for i in range(lati):
        k = 2 * i
        T += [[k, k + 1, k + 2], [k + 1, k + 3, k + 2]]
    m.triangoli("Pietra", P, [[0, 0, 1]] * len(P), [[p[0] / 200, p[1] / 200] for p in P], T)
    m.cilindro("Acqua", [0, 0, -120], r_dentro - 1, 1, lati=lati)
    for sx in (-1, 1):
        m.blocco("Trave", sx * 110 - 9, sx * 110 + 9, -9, 9, 0, 260)
    m.trave("Trave", [-125, 0, 230], [125, 0, 230], 14, 14)
    m.cilindro("Trave", [-80, 0, 230], 12, 160, lati=8, asse=(1, 0, 0))
    # secchio appeso
    m.cilindro("Assi", [0, 0, 120], 18, 30, lati=8, raggio_alto=21)
    m.trave("Ferro", [0, 0, 150], [0, 0, 218], 1.5, 1.5)
    # tettuccio
    for lato in (-1, 1):
        a = np.array([0, lato * 120.0, 255.0])
        b = np.array([0, 0.0, 320.0])
        su = normalizza(b - a)
        n = normalizza(np.cross([1.0, 0, 0], su))
        if n[2] < 0:
            n = -n
        m.scatola({"+z": "Coppi", "-z": "Assi", "altri": "Trave"}, (a + b) / 2 + n * 6, [150, 0, 0], su * np.linalg.norm(b - a) / 2, n * 6)
    return m


def ponte(nome, lunghezza, larghezza, quote_appoggi, profondita_acqua=None):
    """Ponte di legno lungo X da -L/2 a L/2, piano a z=0, su cavalletti; quote_appoggi: lista (x, z_terreno) dove
    mettere i pali (scendono fino al terreno)."""
    m = Mesh(nome)
    L, B = lunghezza, larghezza
    m.blocco({"altri": "Trave", "+z": "Assi", "-z": "Assi"}, -L / 2, L / 2, -B / 2, B / 2, -18, 0)
    for sy in (-1, 1):
        m.trave("Trave", [-L / 2, sy * (B / 2 - 20), -40], [L / 2, sy * (B / 2 - 20), -40], 26, 40)
    # parapetti
    passo = 200.0
    n = int(L / passo)
    for sy in (-1, 1):
        y = sy * (B / 2 - 6)
        for k in range(n + 1):
            x = -L / 2 + L * k / n
            m.blocco("Trave", x - 6, x + 6, y - 6, y + 6, 0, 110)
        m.trave("Trave", [-L / 2, y, 105], [L / 2, y, 105], 12, 10)
        m.trave("Trave", [-L / 2, y, 55], [L / 2, y, 55], 8, 8)
    for x, zt in quote_appoggi:
        for sy in (-1, 1):
            m.cilindro("Trave", [x, sy * (B / 2 - 25), zt - 80], 16, -zt + 80 - 40, lati=8)
        m.trave("Trave", [x, -B / 2 - 10, -50], [x, B / 2 + 10, -50], 24, 24)
        # croce di controvento
        m.trave("Trave", [x, -B / 2 + 25, zt + 20], [x, B / 2 - 25, -70], 12, 12, su=[1, 0, 0])
    return m


def pontile(nome, lunghezza, larghezza, quota_terreno_fine, alto=60.0):
    """Pontile di assi lungo X (da 0 a lunghezza), piano a z=0, pali fino al fondo."""
    m = Mesh(nome)
    m.blocco({"altri": "Trave", "+z": "Assi", "-z": "Assi"}, 0, lunghezza, -larghezza / 2, larghezza / 2, -10, 0)
    n = max(1, int(lunghezza / 180))
    for k in range(n + 1):
        x = lunghezza * k / n
        zt = quota_terreno_fine(x)
        for sy in (-1, 1):
            m.cilindro("Trave", [x, sy * (larghezza / 2 - 10), zt - 60], 10, -zt + 60 + 15, lati=6)
    return m


def capanna_palafitta(nome, W=480.0, D=420.0, alto=140.0, quote_pali=None):
    """La capanna di Nonna Edda: su pali sopra il fango, pareti di assi, tetto di paglia ripido, scaletta."""
    m = Mesh(nome)
    info = {"porte": [], "cose": []}
    PW, PD = W + 160, D + 120
    # piattaforma
    m.blocco({"altri": "Trave", "+z": "Assi", "-z": "Assi"}, -PW / 2, PW / 2, -PD / 2 - 60, PD / 2, alto - 20, alto)
    for x in (-PW / 2 + 15, 0, PW / 2 - 15):
        for y in (-PD / 2 - 45, 0, PD / 2 - 15):
            zt = quote_pali(x, y) if quote_pali else 0
            m.cilindro("Trave", [x, y, zt - 80], 13, alto - zt + 80 - 20, lati=6)
    # pareti di assi con una porta e due finestrelle
    T = 14.0
    base = alto
    muri = [((-W / 2, -D / 2 + T / 2), 0.0, W, "fronte"), ((-W / 2, D / 2 - T / 2), 0.0, W, "retro"),
            ((-W / 2 + T / 2, -D / 2 + T), 90.0, D - 2 * T, "s"), ((W / 2 - T / 2, -D / 2 + T), 90.0, D - 2 * T, "d")]
    for (ox, oy), g, L, lato in muri:
        buchi = []
        if lato == "fronte":
            buchi.append((L / 2 - PORTA_L / 2, L / 2 + PORTA_L / 2, 0, PORTA_H - 10))
        else:
            buchi.append((L / 2 - 30, L / 2 + 30, 110, 160))
        m.muro("Assi", (ox, oy, base), g, L, 250, T, buchi)
    info["porte"].append((0.0, -D / 2 + T / 2, 0.0, "fronte", base))
    for sx in (-1, 1):
        for sy in (-1, 1):
            m.blocco("Trave", sx * W / 2 - 10, sx * W / 2 + 10, sy * D / 2 - 10, sy * D / 2 + 10, base, base + 250)
    m.blocco({"altri": "Trave", "-z": "Assi", "+z": "Assi"}, -W / 2, W / 2, -D / 2, D / 2, base + 240, base + 252)
    tetto_casa(m, W, D, base + 250, "Paglia", 50.0, sporto=70, gronda=50, gable="Assi")
    # scaletta (gradini) davanti
    for k in range(5):
        zz = alto - (k + 1) * alto / 6
        m.blocco("Assi", -60, 60, -PD / 2 - 60 - (k + 1) * 30, -PD / 2 - 60 - k * 30, zz - 6, zz)
    for sx in (-1, 1):
        m.trave("Trave", [sx * 65, -PD / 2 - 60, alto], [sx * 65, -PD / 2 - 60 - 180, -10], 8, 14)
    # comignolo di pietra e fango per il fuoco delle erbe
    m.blocco({"altri": "Pietra", "+z": "Ferro"}, W / 2 - 100, W / 2 - 40, 30, 90, base + 200, base + 250 + D / 2 * 1.19 + 60)
    return m, info


def tettoia(nome, W, D, altezza=260.0, tetto="Paglia", pendenza=25.0):
    """Tettoia a una falda su pali (la fucina, la conceria, il mercato coperto)."""
    m = Mesh(nome)
    t = math.tan(math.radians(pendenza))
    for sx in (-1, 1):
        for sy in (-1, 1):
            h = altezza + (D / 2 - sy * D / 2) * t * 0.0 + (0 if sy > 0 else D * t)
            m.cilindro("Trave", [sx * (W / 2 - 15), sy * (D / 2 - 15), -30], 12, h + 30, lati=6)
    a = np.array([0, D / 2 + 50, altezza - 50 * t])
    b = np.array([0, -D / 2 - 30, altezza + (D + 30) * t])
    su = normalizza(b - a)
    n = normalizza(np.cross([1.0, 0, 0], su))
    if n[2] < 0:
        n = -n
    sp = 30.0 if tetto == "Paglia" else 14.0
    m.scatola({"+z": tetto, "-z": "Assi", "altri": "Trave"}, (a + b) / 2 + n * sp / 2, [W / 2 + 40, 0, 0], su * np.linalg.norm(b - a) / 2, n * sp / 2)
    for sy in (-1, 1):
        y = sy * (D / 2 - 15)
        z = altezza + (D * t if sy < 0 else 0)
        m.trave("Trave", [-W / 2 - 20, y, z], [W / 2 + 20, y, z], 18, 18)
    return m


def fienile(nome, W=1400.0, D=800.0, fondazione=60.0):
    """Stalla-fienile: pareti basse di pietra, sopra assi, grande apertura sulla facciata, tetto di paglia."""
    m, info = casa(nome, W, D, piani=1, base="Assi", sopra="Assi", tetto="Paglia", pendenza=45, fondazione=fondazione,
                   porta_larga=360, porta_alta=330, h1=420, camino=False, graticcio=False, finestre_lato=False,
                   porte_vere=False, finestra_l=60, finestra_h=50)
    return m, info


def tempio(nome, W=1200.0, D=2200.0, alto=820.0, fondazione=120.0):
    """Il Tempio del Cuore: una navata di pietra, ingresso grande sul lato corto (-Y), finestre alte e strette,
    tetto di coppi, un campanile quadrato sul retro a destra. Dentro: panche (vere) e l'altare."""
    m = Mesh(nome)
    T = 70.0
    info = {"porte": [], "cose": []}
    m.blocco({"altri": "Pietra", "+z": "Ciottoli"}, -W / 2 - 20, W / 2 + 20, -D / 2 - 20, D / 2 + 20, -fondazione, 0)
    # scalinata davanti
    for k in range(3):
        m.blocco("Pietra", -300 + k * 30, 300 - k * 30, -D / 2 - 20 - (3 - k) * 45, -D / 2 - 20, -fondazione, -15 * (2 - k) - 1)
    muri = [((-W / 2, -D / 2 + T / 2), 0.0, W, "fronte"), ((-W / 2, D / 2 - T / 2), 0.0, W, "retro"),
            ((-W / 2 + T / 2, -D / 2 + T), 90.0, D - 2 * T, "sinistra"), ((W / 2 - T / 2, -D / 2 + T), 90.0, D - 2 * T, "destra")]
    for (ox, oy), g, L, lato in muri:
        buchi = []
        if lato == "fronte":
            buchi.append((L / 2 - 120, L / 2 + 120, 0, 380))
            buchi.append((L / 2 - 60, L / 2 + 60, 520, 680))   # finestra sopra la porta
        elif lato == "retro":
            buchi.append((L / 2 - 50, L / 2 + 50, 380, 680))
        else:
            for c in _finestre(L, 5, 70):
                buchi.append((c - 35, c + 35, 380, 640))
        m.muro("Pietra", (ox, oy, 0), g, L, alto, T, buchi)
        a = math.radians(g)
        u = np.array([math.cos(a), math.sin(a), 0.0])
        o = np.array([ox, oy, 0])
        for (u0, u1, z0, z1) in buchi:
            if z0 > 0:
                # vetrata colorata: vetro scuro con l'architrave
                m.scatola("Vetro", o + u * ((u0 + u1) / 2) + [0, 0, (z0 + z1) / 2], u * (u1 - u0) / 2, np.cross([0, 0, 1], u) * 3, [0, 0, (z1 - z0) / 2])
    # contrafforti sui fianchi
    for sx in (-1, 1):
        for y in np.linspace(-D / 2 + 150, D / 2 - 150, 5):
            x0, x1 = (W / 2 - 5, W / 2 + 90) if sx > 0 else (-W / 2 - 90, -W / 2 + 5)
            m.blocco("Pietra", x0, x1, y - 50, y + 50, -fondazione, alto - 150)
    # portale: arco simulato con un architrave grande e due stipiti sporgenti
    m.blocco("Pietra", -170, 170, -D / 2 - 35, -D / 2 + 5, 380, 450)
    for sx in (-1, 1):
        x0, x1 = (120, 170) if sx > 0 else (-170, -120)
        m.blocco("Pietra", x0, x1, -D / 2 - 35, -D / 2 + 5, 0, 380)
    m.blocco({"altri": "Trave", "-z": "Assi"}, -W / 2 + T, W / 2 - T, -D / 2 + T, D / 2 - T, alto - 20, alto)
    tetto_casa_y(m, W, D, alto, "Coppi", 35.0, sporto=60, gronda=50)
    # campanile
    cl = 450.0
    cx, cy = W / 2 + cl / 2 - 20, D / 2 - cl / 2 - 100
    camp, _ = torre("x", lato=cl, altezza=1550, fondazione=fondazione, porta=False)
    m.unisci(camp, [cx, cy, 0])
    # campana sotto il tetto del campanile
    m.cilindro("Ferro", [cx, cy, 1550 + 120], 45, 70, lati=12, raggio_alto=22)
    # altare
    m.blocco("Pietra", -140, 140, D / 2 - T - 260, D / 2 - T - 160, 0, 105)
    m.blocco("Pietra", -160, 160, D / 2 - T - 280, D / 2 - T - 140, 105, 118)
    m.blocco("Pietra", -300, 300, D / 2 - T - 360, D / 2 - T, 0, 20)
    info["porte"] = []
    info["altare"] = (0.0, D / 2 - T - 210, 118.0)
    # panche vere, due file
    for k in range(6):
        y = -D / 2 + 380 + k * 230
        for sx in (-1, 1):
            info["cose"].append(("panca", sx * 260.0, y, 90.0))
    return m, info


def tetto_casa_y(m, W, D, cima, tetto, pendenza, sporto=50.0, gronda=40.0):
    """Tetto a capanna con il colmo lungo Y (per gli edifici con l'ingresso sul lato corto)."""
    t = math.tan(math.radians(pendenza))
    s = W / 2
    sp = 16.0
    hy = D / 2 + gronda
    colmo = cima + s * t
    for lato in (-1, 1):
        a = np.array([lato * (s + sporto), 0, cima - sporto * t])
        b = np.array([0.0, 0, colmo])
        su = normalizza(b - a)
        n = normalizza(np.cross(su, [0, 1.0, 0]))
        if n[2] < 0:
            n = -n
        m.scatola({"+z": tetto, "-z": "Assi", "altri": "Trave"}, (a + b) / 2 + n * sp / 2, [0, hy, 0], su * np.linalg.norm(b - a) / 2, n * sp / 2)
    m.trave(tetto, [0, -hy, colmo + 14], [0, hy, colmo + 14], 30, 16)
    for sy in (-1, 1):
        for y in (sy * D / 2, sy * (D / 2 - 70)):
            m.poligono("Pietra", [[-s, y, cima], [s, y, cima], [0, y, colmo]], asse_u=[1, 0, 0])


def bancone(m, x0, x1, y, h=110.0):
    """Bancone di assi lungo X (locanda, gilda)."""
    m.blocco({"altri": "Assi", "+z": "Trave"}, x0, x1, y - 35, y + 35, 0, h)
    m.blocco("Trave", x0 - 5, x1 + 5, y - 45, y + 45, h, h + 8)


def scala(m, x, y0, y1, z0, z1, larga=100.0):
    """Scala di legno lungo +Y da (x, y0, z0) a (x, y1, z1), gradini pieni."""
    n = max(2, int((z1 - z0) / 20))
    for k in range(n):
        ya = y0 + (y1 - y0) * k / n
        yb = y0 + (y1 - y0) * (k + 1) / n
        m.blocco("Assi", x - larga / 2, x + larga / 2, min(ya, yb), max(ya, yb), z0, z0 + (z1 - z0) * (k + 1) / n)


def lapide(m, x, y, z, gradi, tipo, rng):
    if tipo == 0:
        h = rng.uniform(60, 95)
        c = np.array([x, y, z + h / 2 - 15])
        a = math.radians(gradi + rng.uniform(-6, 6))
        u = np.array([math.cos(a), math.sin(a), 0])
        n = np.array([-u[1], u[0], 0])
        m.scatola("Pietra", c, u * rng.uniform(22, 30), n * 7, [0, 0, h / 2 + 15])
    else:
        a = math.radians(gradi)
        u = np.array([math.cos(a), math.sin(a), 0])
        m.trave("Trave", [x, y, z - 30], [x, y, z + 110], 9, 6)
        m.trave("Trave", np.array([x, y, z + 75]) - u * 30, np.array([x, y, z + 75]) + u * 30, 8, 6)
    # tumulo
    a = math.radians(gradi)
    u = np.array([math.cos(a), math.sin(a), 0])
    n = np.array([-u[1], u[0], 0])
    m.scatola("Terra", np.array([x, y, z]) + n * 110, u * 40, n * 95, [0, 0, 12])


def lanterna(m, x, y, z, alto=280.0):
    """Palo con la lanterna (la luce vera la mette crea_villaggio.py: info 'luce')."""
    m.cilindro("Trave", [x, y, z - 50], 9, alto + 50, lati=6)
    m.trave("Trave", [x, y, z + alto - 10], [x + 55, y, z + alto - 10], 7, 7)
    m.blocco("Ferro", x + 40, x + 70, y - 15, y + 15, z + alto - 55, z + alto - 50)
    m.blocco("Fiamma", x + 45, x + 65, y - 10, y + 10, z + alto - 50, z + alto - 25)
    m.blocco("Ferro", x + 40, x + 70, y - 15, y + 15, z + alto - 25, z + alto - 18)
    return (x + 55, y, z + alto - 40)


def covone(m, x, y, z, r=180.0, h=260.0):
    m.cilindro("Paglia", [x, y, z - 20], r, h * 0.45 + 20, lati=10, raggio_alto=r * 0.95, tappo=False)
    m.cilindro("Paglia", [x, y, z + h * 0.45], r * 0.95, h * 0.45, lati=10, raggio_alto=r * 0.25, tappo=False)
    m.cilindro("Paglia", [x, y, z + h * 0.9], r * 0.25, h * 0.15, lati=10, raggio_alto=4, tappo=False)


def catasta(m, x, y, z, gradi=0.0, L=200.0, file=4):
    a = math.radians(gradi)
    u = np.array([math.cos(a), math.sin(a), 0.0])
    n = np.array([-u[1], u[0], 0.0])
    for r in range(file):
        for k in range(file - r + 2):
            c = np.array([x, y, z]) + n * ((k - (file - r + 1) / 2) * 24) + [0, 0, 12 + r * 21]
            m.cilindro("Trave", c - u * L / 2, 11, L, lati=7, asse=u)


def ruota_mulino(m, cx, cy, cz, raggio=260.0, larga=120.0, asse_gradi=0.0):
    """Ruota del mulino nel piano XZ (asse lungo Y girato di asse_gradi)."""
    a = math.radians(asse_gradi)
    ay = np.array([-math.sin(a), math.cos(a), 0])
    ax = np.array([math.cos(a), math.sin(a), 0])
    c = np.array([cx, cy, cz])
    m.cilindro("Trave", c - ay * (larga / 2 + 80), 18, larga + 160, lati=8, asse=ay)
    for lato in (-1, 1):
        anello = []
        for k in range(16):
            t = 2 * math.pi * k / 16
            anello.append(c + ay * lato * larga / 2 + (ax * math.cos(t) + np.array([0, 0, 1]) * math.sin(t)) * raggio)
        for k in range(16):
            m.trave("Trave", anello[k], anello[(k + 1) % 16], 14, 14, su=ay)
        for k in range(0, 16, 2):
            m.trave("Trave", c + ay * lato * larga / 2, anello[k], 10, 10, su=ay)
    for k in range(16):
        t = 2 * math.pi * (k + 0.5) / 16
        d = ax * math.cos(t) + np.array([0, 0, 1]) * math.sin(t)
        p = c + d * (raggio - 35)
        m.scatola("Assi", p, ay * larga / 2, d * 40, np.cross(ay, d) * 3)


def botte(nome):
    """Botte di doghe con due cerchi di ferro (alta 95 cm)."""
    m = Mesh(nome)
    m.cilindro("Assi", [0, 0, 0], 30, 47, lati=12, raggio_alto=36, tappo=False)
    m.cilindro("Assi", [0, 0, 47], 36, 48, lati=12, raggio_alto=30, tappo=False)
    m.cilindro("Trave", [0, 0, 2], 29.5, 1, lati=12)
    m.cilindro("Trave", [0, 0, 92], 29.5, 1, lati=12)
    for z0, r0, r1 in ((8, 31.2, 32.6), (78, 32.6, 31.2)):
        m.cilindro("Ferro", [0, 0, z0], r0 + 0.8, 7, lati=12, raggio_alto=r1 + 0.8, tappo=False)
    return m
