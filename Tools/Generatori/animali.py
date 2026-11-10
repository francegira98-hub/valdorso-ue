# Valdorso - Gli animali della valle fatti da zero, provvisori (v0.1.4, 10/11/2026, chiesto da Fra: "inserisci altri
# pericoli ed animali, tanto poi cambiamo solo le figure"). Scritto da Claude.
#
# Sei specie a quattro zampe, tutte con gli stessi pezzi (come il lupo del 09/11): SM_<Specie>_Corpo, _Testa (l'origine
# al collo), _Zampa (l'origine alla spalla o all'anca, giu' fino a terra: la stessa per le quattro), _Coda (l'origine
# all'attacco). Guardano +X, cm, terra a z = 0. Il C++ (ValdorsoLupi.cpp, la tabella in ValdorsoAnimaliForme.inl) li
# monta e li muove: le misure di FORME qui sotto e quelle della tabella devono essere le stesse (python animali.py
# --inl scrive la tabella).
#   Lupo (predatore, di notte e con la nebbia), Cinghiale (difende il suo posto: carica), Orso (raro, sulla Collina
#   dell'Orso: carica, fortissimo), Cervo (fugge, con i palchi), Lepre (fugge, piccola), Volpe (fugge, di notte).
# I colori: ogni triangolo prende il materiale del colore piu' vicino della tavolozza (MV_<Colore>); crea_pericoli.py
# fa MI_Pericoli_<Colore> con quella tinta. Il colore dei vertici resta (come sfumatura, 0..1 rispetto alla tinta):
# se Unreal non lo importa, gli animali escono comunque del colore giusto (il 09/11 i lupi erano bianchi).

import math
import os
import sys

import numpy as np

from villaggio_mesh import Mesh, normalizza

# la tavolozza (nomi senza trattini bassi: sono le chiavi dei materiali)
TAVOLOZZA = {
    "Grigio": (0.40, 0.38, 0.35), "GrigioScuro": (0.20, 0.19, 0.18), "Chiaro": (0.70, 0.66, 0.58),
    "Nero": (0.04, 0.04, 0.04), "Bruno": (0.24, 0.16, 0.10), "BrunoScuro": (0.12, 0.08, 0.05),
    "Fulvo": (0.50, 0.32, 0.17), "Rossiccio": (0.62, 0.30, 0.10), "Bianco": (0.85, 0.83, 0.78),
    "Corno": (0.80, 0.74, 0.62), "Avorio": (0.88, 0.84, 0.72), "Lepre": (0.48, 0.40, 0.30),
}

# le forme: corpo da xr (groppa) a xf (petto) all'altezza h, mezzo largo ry e mezzo alto rz; le zampe davanti a xa,
# dietro a xp, di lato y, lunghe (= alte all'attacco) lz, grosse rl; la testa (collo) e la coda dove si attaccano
FORME = {
    "Lupo":      dict(xr=-55, xf=48, h=58, ry=15, rz=20, xa=32, xp=-40, y=9, lz=56, rl=6.5, testa=(44, 0, 64), coda=(-54, 0, 66),
                      colori=("Grigio", "GrigioScuro", "Chiaro")),
    "Cinghiale": dict(xr=-52, xf=46, h=52, ry=21, rz=27, xa=30, xp=-36, y=11, lz=42, rl=7.0, testa=(36, 0, 54), coda=(-52, 0, 60),
                      colori=("Bruno", "BrunoScuro", "Bruno")),
    "Orso":      dict(xr=-78, xf=70, h=86, ry=36, rz=40, xa=46, xp=-56, y=19, lz=72, rl=11.0, testa=(58, 0, 98), coda=(-78, 0, 92),
                      colori=("Bruno", "BrunoScuro", "Fulvo")),
    "Cervo":     dict(xr=-50, xf=46, h=96, ry=16, rz=22, xa=32, xp=-38, y=10, lz=92, rl=4.5, testa=(38, 0, 106), coda=(-50, 0, 102),
                      colori=("Fulvo", "Bruno", "Chiaro")),
    "Lepre":     dict(xr=-15, xf=12, h=19, ry=6.5, rz=8, xa=8, xp=-9, y=4, lz=16, rl=1.8, testa=(12, 0, 24), coda=(-15, 0, 21),
                      colori=("Lepre", "Bruno", "Bianco")),
    "Volpe":     dict(xr=-35, xf=30, h=36, ry=9, rz=12, xa=20, xp=-25, y=6, lz=34, rl=3.2, testa=(28, 0, 42), coda=(-34, 0, 40),
                      colori=("Rossiccio", "Bruno", "Bianco")),
}


def tubo(m, centri, raggi_y, raggi_z, colore, lati=12):
    P, N, UV, C, T = [], [], [], [], []
    for i, c in enumerate(centri):
        c = np.asarray(c, np.float64)
        d = np.asarray(centri[min(i + 1, len(centri) - 1)], np.float64) - np.asarray(centri[max(i - 1, 0)], np.float64)
        a = normalizza(d)
        e1 = normalizza(np.cross(a, [0, 0, 1]) if abs(a[2]) < 0.95 else np.cross(a, [1, 0, 0]))
        e2 = np.cross(e1, a)
        for k in range(lati + 1):
            ang = 2 * math.pi * k / lati
            p = c + e1 * math.cos(ang) * raggi_y[i] + e2 * math.sin(ang) * raggi_z[i]
            nn = normalizza(e1 * math.cos(ang) / max(raggi_y[i], 0.1) + e2 * math.sin(ang) / max(raggi_z[i], 0.1))
            P.append(p)
            N.append(nn)
            UV.append([k / lati, i / max(1, len(centri) - 1)])
            C.append(np.asarray(colore(p, nn), np.float64))
    for i in range(len(centri) - 1):
        for k in range(lati):
            a0 = i * (lati + 1) + k
            b0 = a0 + lati + 1
            T += [[a0, b0, a0 + 1], [a0 + 1, b0, b0 + 1]]
    # i tappi
    for fine, idx in ((0, 0), (1, len(centri) - 1)):
        c = np.asarray(centri[idx], np.float64)
        d = normalizza(np.asarray(centri[-1], np.float64) - np.asarray(centri[0], np.float64)) * (1 if fine else -1)
        base = len(P)
        P.append(c)
        N.append(d)
        UV.append([0.5, 0.5])
        C.append(np.asarray(colore(c, d), np.float64))
        for k in range(lati):
            i0 = idx * (lati + 1) + k
            T.append([base, i0, i0 + 1] if fine else [base, i0 + 1, i0])
    m.triangoli("Pelo", P, N, UV, T, [list(c) + [1.0] for c in C])


def triangolo(m, a, b, c, colore):
    a, b, c = (np.asarray(x, np.float64) for x in (a, b, c))
    n = normalizza(np.cross(b - a, c - a))
    col = list(colore) + [1.0]
    m.triangoli("Pelo", [a, b, c], [n] * 3, [[0, 0], [1, 0], [0.5, 1]], [[0, 1, 2]], [col] * 3)
    m.triangoli("Pelo", [a, c, b], [-n] * 3, [[0, 0], [1, 0], [0.5, 1]], [[0, 1, 2]], [col] * 3)


def colore_pelo(base, dorso, pancia):
    B, D, P = (np.array(TAVOLOZZA[x]) for x in (base, dorso, pancia))

    def f(p, n):
        su = n[2]
        c = B.copy()
        if su > 0.25:
            c = c * (1 - (su - 0.25)) + D * (su - 0.25)
        elif su < -0.3:
            c = c * (1 + (su + 0.3)) + P * (-(su + 0.3))
        rumore = 0.93 + 0.14 * (math.sin(p[0] * 0.37 + p[1] * 0.71) * 0.5 + 0.5)
        return np.clip(c * rumore, 0, 1)
    return f


def fisso(nome):
    c = np.array(TAVOLOZZA[nome])
    return lambda p, n: c


def tavolozza(m, ammessi):
    """Divide i triangoli del materiale Pelo tra i colori della tavolozza (il piu' vicino alla media dei tre vertici);
    il colore dei vertici diventa la sfumatura rispetto alla tinta (0..1)."""
    if "Pelo" not in m.parti:
        return
    parte = m.parti.pop("Pelo")
    P = np.concatenate(parte[0])
    N = np.concatenate(parte[1])
    UV = np.concatenate(parte[2])
    C = np.concatenate(parte[3])
    T = np.concatenate(parte[4])
    nomi = list(ammessi)
    tinte = np.array([TAVOLOZZA[x] for x in nomi])
    medie = C[T][:, :, :3].mean(1)
    scelta = np.argmin(((medie[:, None, :] - tinte[None, :, :]) ** 2).sum(2), 1)
    for j, nome in enumerate(nomi):
        tri = T[scelta == j]
        if len(tri) == 0:
            continue
        usati = np.unique(tri)
        mappa = {int(v): i for i, v in enumerate(usati)}
        nuovi = np.vectorize(lambda v: mappa[int(v)])(tri)
        sfum = np.clip(C[usati][:, :3] / np.maximum(tinte[j], 0.02), 0.0, 1.0)
        m.triangoli(nome, P[usati], N[usati], UV[usati], nuovi, np.hstack([sfum, np.ones((len(usati), 1))]))


def animale(nome):
    F = FORME[nome]
    base, dorso, pancia = F["colori"]
    pelo = colore_pelo(base, dorso, pancia)
    pezzi = {}

    # --- il corpo: un fuso dalla groppa al petto, piu' alto alle spalle (l'orso e il cinghiale con la gobba)
    m = Mesh("SM_%s_Corpo" % nome)
    xs = np.linspace(F["xr"], F["xf"], 14)
    gobba = {"Cinghiale": 0.35, "Orso": 0.25}.get(nome, 0.12)
    centri, ry, rz = [], [], []
    for x in xs:
        t = (x - F["xr"]) / (F["xf"] - F["xr"])
        forma = math.sin(math.pi * min(1.0, 0.08 + t * 0.95)) ** 0.55
        spalle = 1 + gobba * math.exp(-((t - 0.72) / 0.18) ** 2)
        centri.append([x, 0, gobba * F["rz"] * 0.4 * math.exp(-((t - 0.72) / 0.2) ** 2)])
        ry.append(max(1.0, F["ry"] * forma))
        rz.append(max(1.0, F["rz"] * forma * spalle))
    tubo(m, centri, ry, rz, pelo, lati=14)
    pezzi[m.nome] = m

    # --- la testa: dal collo (origine) in avanti; muso, orecchie, occhi e quello che ha in piu' ogni specie
    m = Mesh("SM_%s_Testa" % nome)
    s = F["rz"] / 20.0          # la misura della testa va con quella del corpo
    if nome == "Cervo":
        # il collo lungo che sale, poi la testa
        centri = [[0, 0, 0], [6, 0, 10], [12, 0, 20], [18, 0, 26], [26, 0, 28], [34, 0, 26], [40, 0, 23], [44, 0, 21]]
        raggi = [9, 8, 7, 6.5, 6, 5, 3.5, 2.5]
    elif nome == "Cinghiale":
        centri = [[0, 0, 0], [10, 0, -2], [20, 0, -5], [30, 0, -9], [38, 0, -12], [42, 0, -13]]
        raggi = [14, 13, 10, 7, 5.5, 5]
    elif nome == "Orso":
        centri = [[0, 0, 0], [10, 0, 2], [20, 0, 3], [28, 0, 1], [36, 0, -3], [42, 0, -5]]
        raggi = [20, 19, 17, 12, 8, 6]
    elif nome == "Lepre":
        centri = [[0, 0, 0], [3, 0, 2], [6, 0, 3], [8.5, 0, 2.5], [10, 0, 1.5]]
        raggi = [4, 4.2, 3.8, 2.6, 1.6]
    else:   # lupo e volpe: il muso lungo e appuntito
        k = 1.0 if nome == "Lupo" else 0.62
        centri = [[0, 0, 0], [10 * k, 0, 6 * k], [20 * k, 0, 11 * k], [28 * k, 0, 13 * k], [36 * k, 0, 12 * k], [44 * k, 0, 9 * k],
                  [52 * k, 0, 7 * k], [58 * k, 0, 6 * k]]
        raggi = [10 * k, 9.5 * k, 8.5 * k, 8.5 * k, 7.5 * k, 5 * k, 3.8 * k, 2.8 * k]
    ry = list(raggi)
    rz = [r * 1.05 for r in raggi]

    def col_testa(p, n, punta=centri[-1][0]):
        c = pelo(p, n)
        if p[0] > punta - 4:
            c = np.array(TAVOLOZZA["Nero"])        # il naso (o il grugno)
        elif nome in ("Lupo", "Volpe") and p[0] > punta * 0.7 and n[2] < 0.2:
            c = np.array(TAVOLOZZA["Bianco" if nome == "Volpe" else "Chiaro"])
        return c
    tubo(m, centri, ry, rz, col_testa, lati=12)
    # gli occhi
    cx = centri[len(centri) // 2]
    for lato in (-1, 1):
        r = raggi[len(raggi) // 2]
        m.triangoli("Pelo", *_puntino(np.array([cx[0], lato * r * 0.85, cx[2] + r * 0.45]), lato, max(0.6, r * 0.13)))
    # le orecchie
    testa_c = centri[1] if nome != "Cervo" else centri[5]
    r0 = raggi[1] if nome != "Cervo" else raggi[5]
    for lato in (-1, 1):
        b = np.array([testa_c[0], lato * r0 * 0.55, testa_c[2] + r0 * 0.85])
        if nome == "Lepre":
            cima = b + np.array([-3, lato * 1.5, 11])
            larg = 1.6
        elif nome == "Orso":
            cima = b + np.array([0, lato * 2, 6])
            larg = 4.5
        else:
            cima = b + np.array([-3 * s, lato * 2.5 * s, 11 * s])
            larg = 4 * s
        triangolo(m, b + np.array([larg, 0, 0]), b - np.array([larg, 0, 0]), cima, TAVOLOZZA[dorso])
    if nome == "Cervo":
        # i palchi: due rami che salgono e si aprono, con le punte
        for lato in (-1, 1):
            radice = np.array([testa_c[0] - 2, lato * 4, testa_c[2] + 6])
            fusto = [radice, radice + [-6, lato * 8, 18], radice + [-10, lato * 16, 36], radice + [-8, lato * 22, 52]]
            for i in range(3):
                tubo(m, [fusto[i], fusto[i + 1]], [1.6 - i * 0.3] * 2, [1.6 - i * 0.3] * 2, fisso("Corno"), lati=6)
            for i, alto in ((1, 14), (2, 12)):
                p0 = fusto[i]
                tubo(m, [p0, p0 + [8, lato * 3, alto]], [1.0, 0.6], [1.0, 0.6], fisso("Corno"), lati=6)
    if nome == "Cinghiale":
        # le zanne che escono dal grugno e curvano in su
        for lato in (-1, 1):
            p0 = np.array([centri[-2][0], lato * 5, centri[-2][2] - 2])
            tubo(m, [p0, p0 + [4, lato * 2, 3], p0 + [6, lato * 2.5, 8]], [1.2, 0.9, 0.4], [1.2, 0.9, 0.4], fisso("Avorio"), lati=6)
    pezzi[m.nome] = m

    # --- la zampa: dall'attacco giu' fino a terra (il gomito un po' indietro, il piede in avanti)
    m = Mesh("SM_%s_Zampa" % nome)
    L, r = F["lz"], F["rl"]
    centri = [[0, 0, 0], [0.02 * L, 0, -0.22 * L], [-0.04 * L, 0, -0.46 * L], [0, 0, -0.72 * L], [0.03 * L, 0, -0.92 * L],
              [0.08 * L, 0, -L + r * 0.3]]
    ry = [r, r * 0.85, r * 0.62, r * 0.5, r * 0.48, r * (0.9 if nome == "Orso" else 0.62)]
    rz = [r, r * 0.85, r * 0.62, r * 0.5, r * 0.48, r * 0.3]

    def col_zampa(p, n):
        c = pelo(p, n)
        if p[2] < -0.93 * L:
            c = np.array(TAVOLOZZA["Nero" if nome in ("Cervo", "Cinghiale") else dorso])     # zoccoli o piedi scuri
        return c
    tubo(m, centri, ry, rz, col_zampa, lati=8)
    pezzi[m.nome] = m

    # --- la coda
    m = Mesh("SM_%s_Coda" % nome)
    if nome in ("Lupo", "Volpe"):
        k = 1.0 if nome == "Lupo" else 0.85
        centri = [[0, 0, 0], [-8 * k, 0, -4 * k], [-18 * k, 0, -12 * k], [-28 * k, 0, -20 * k], [-38 * k, 0, -26 * k], [-46 * k, 0, -29 * k]]
        rr = [3.5 * k, 5 * k, 7 * k, 7.5 * k, 6 * k, 2 * k]

        def col_coda(p, n):
            if nome == "Volpe" and p[0] < -34 * k:
                return np.array(TAVOLOZZA["Bianco"])
            return pelo(p, n) if p[0] > -40 * k else np.array(TAVOLOZZA[dorso])
        tubo(m, centri, rr, rr, col_coda, lati=10)
    elif nome == "Lepre":
        tubo(m, [[0, 0, 0], [-2, 0, 1], [-4, 0, 0]], [2.5, 3, 2], [2.5, 3, 2], fisso("Bianco"), lati=8)
    elif nome == "Cervo":
        tubo(m, [[0, 0, 0], [-4, 0, -3], [-7, 0, -8]], [3, 3, 1.5], [2, 2, 1], fisso("Bianco"), lati=8)
    elif nome == "Cinghiale":
        tubo(m, [[0, 0, 0], [-4, 0, -6], [-5, 0, -14], [-4, 0, -18]], [1.5, 1.2, 1, 1.6], [1.5, 1.2, 1, 1.6], fisso(dorso), lati=6)
    else:   # orso: un mozzicone
        tubo(m, [[0, 0, 0], [-5, 0, -2], [-8, 0, -6]], [5, 5, 3], [5, 5, 3], pelo, lati=8)
    pezzi[m.nome] = m

    extra = {"Cervo": ["Corno", "Bianco"], "Cinghiale": ["Avorio"], "Volpe": ["Bianco"], "Lepre": ["Bianco"], "Lupo": ["Chiaro"]}
    ammessi = sorted(set([base, dorso, pancia, "Nero"] + extra.get(nome, [])))
    for m in pezzi.values():
        tavolozza(m, ammessi)
    return pezzi


def _puntino(c, lato, r):
    P = [c + np.array([math.cos(a) * r, 0, math.sin(a) * r]) for a in np.linspace(0, 2 * math.pi, 7)[:-1]]
    n = np.array([0, lato * 1.0, 0])
    col = list(TAVOLOZZA["Nero"]) + [1.0]
    T = [[0, i, i + 1] if lato > 0 else [0, i + 1, i] for i in range(1, 5)]
    return [P, [n] * 6, [[0, 0]] * 6, T, [col] * 6]


def scrivi_inl(percorso):
    """La tabella delle forme per il C++ (ValdorsoAnimaliForme.inl): dove va ogni pezzo."""
    righe = ["// Valdorso - Le forme degli animali: dove si attaccano i pezzi (cm, da terra, l'animale guarda +X).",
             "// Scritto da Tools/Generatori/animali.py --inl (non a mano): le misure devono restare quelle dei modelli.",
             "// { Specie, corpo z, testa x/z, coda x/z, zampe davanti x, dietro x, di lato y, lunghezza zampa, mezzo largo, mezzo lungo }"]
    for nome, F in FORME.items():
        meta_lungo = (F["xf"] - F["xr"]) / 2
        righe.append('{ TEXT("%s"), %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff },' % (
            nome, F["h"], F["testa"][0], F["testa"][2], F["coda"][0], F["coda"][2], F["xa"], F["xp"], F["y"], F["lz"],
            F["ry"], meta_lungo))
    with open(percorso, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(righe) + "\n")


def main(uscita):
    os.makedirs(uscita, exist_ok=True)
    for nome in FORME:
        for pezzo, m in animale(nome).items():
            m.scrivi(os.path.join(uscita, pezzo + ".glb"))
            lo, hi = m.scatola_limiti()
            print(pezzo, m.conta(), "triangoli", sorted(m.parti), lo.round(), hi.round())


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--inl":
        scrivi_inl(sys.argv[2])
    else:
        main(sys.argv[1] if len(sys.argv) > 1 else "uscita_animali")
