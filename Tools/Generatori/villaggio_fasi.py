# Valdorso - Il villaggio che cresce a fasi (v0.1.3). Scritto da Claude il 06/11/2026.
# Decisa da Fra il 05/11 ("ogni cosa con una fase"), approvata il 06/11 (A, B, C): il Design ("Insediamenti che
# crescono") chiede che spazi e stadi si preparino già nella v0.1.3, la crescita vera arriva con l'economia (v0.2).
#
#   Fase 1  il villaggio di oggi, con la palizzata stretta (che c'è solo nella fase 1).
#   Fase 2  un anello di case lungo le strade, fuori dalla palizzata di oggi; la palizzata più larga (fino a circa
#           200 m dalla piazza, con dentro le fattorie di sud-est), con le porte dove passano le strade, le lanterne
#           alle porte e la torre del nord.
#   Fase 3  il muro di pietra sulla stessa linea della palizzata larga (che c'è solo nella fase 2), con due torri
#           a ogni porta e le torri d'angolo.
#
# Ogni cosa del JSON ha "fase": [da, fino_a] (fino_a 0 = per sempre; senza "fase" = [1, 0]). crea_villaggio.py mette
# nel livello le cose di tutte le fasi con l'etichetta "ValdorsoFase da-fino_a"; AValdorsoFasiVillaggio (C++) le
# mostra o le nasconde secondo la fase decisa dal server.
# Coordinate: metri dal centro della piazza (x est, y sud), come L() di villaggio.py.

import math

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

import villaggio_edifici as ed
from villaggio_mesh import Mesh

# Le strade dipinte sul paesaggio (linee di mezzo misurate sullo strato Strade il 06/11, metri dalla piazza)
STRADA_NORD = [(-12.0, -150.0), (-16.0, -185.8), (-20.8, -221.5), (-26.7, -257.0)]
STRADA_NORDEST = [(94.6, -15.8), (148.3, -42.6), (223.4, -80.2)]
STRADA_SUDEST = [(95.2, 7.7), (149.3, 33.5), (225.4, 69.2)]

# La linea della palizzata larga (fase 2) e del muro (fase 3), senza i due capi nel fiume (li trova capo_nel_fiume)
# e senza le porte (le trova con_porte, dove la linea incrocia le strade). Gira come la palizzata di oggi: da nord,
# per est, fino a sud-ovest (così le traverse stanno dentro e i merli fuori).
LINEA_LARGA = [(-50, -196), (25, -194), (75, -188), (135, -158), (180, -112), (200, -69), (206, -5), (203, 57),
               (196, 112), (155, 160), (65, 190), (-25, 186), (-105, 152)]

LARGHEZZA_PORTA = 7.0      # m
RIS = 0.5                  # m per pixel della mappa dei posti occupati
MEZZO = 330.0              # la mappa va da -MEZZO a +MEZZO metri


# ------------------------------------------------------------------------------------------------ geometria

def _incrocio(a, b, c, d):
    """Dove il segmento a-b incrocia c-d: la frazione lungo a-b (o None)."""
    a, b, c, d = (np.asarray(p, np.float64) for p in (a, b, c, d))
    r, s = b - a, d - c
    den = r[0] * s[1] - r[1] * s[0]
    if abs(den) < 1e-9:
        return None
    q = c - a
    t = (q[0] * s[1] - q[1] * s[0]) / den
    u = (q[0] * r[1] - q[1] * r[0]) / den
    if 0.0 <= t <= 1.0 and 0.0 <= u <= 1.0:
        return t
    return None


def con_porte(percorso, strade, largo=LARGHEZZA_PORTA):
    """Mette le porte dove la linea incrocia le strade: ogni porta = due punti, larghi 'largo' metri."""
    fuori = [tuple(percorso[0])]
    porte = []
    for a, b in zip(percorso[:-1], percorso[1:]):
        A, B = np.asarray(a, np.float64), np.asarray(b, np.float64)
        lung = float(np.linalg.norm(B - A))
        colpi = []
        for s in strade:
            for c, d in zip(s[:-1], s[1:]):
                t = _incrocio(A, B, c, d)
                if t is not None:
                    colpi.append(t)
        for t in sorted(colpi):
            dt = largo / 2 / lung
            g0 = tuple((A + (B - A) * max(0.0, t - dt)).round(2))
            g1 = tuple((A + (B - A) * min(1.0, t + dt)).round(2))
            fuori += [g0, g1]
            porte.append((g0, g1))
        fuori.append(tuple(b))
    return fuori, porte


def pezzi_tra_porte(giro, porte):
    """Divide la linea in pezzi interrotti dalle porte (come la palizzata di oggi in villaggio.py)."""
    pezzi = [[]]
    for p in giro:
        pezzi[-1].append(p)
        if any(p == g[0] for g in porte):
            pezzi.append([])
        elif any(p == g[1] for g in porte):
            pezzi[-1] = [p]
    return [pz for pz in pezzi if len(pz) >= 2]


def dentro(p, poligono):
    """Punto dentro un poligono (pari e dispari)."""
    x, y = p
    n = len(poligono)
    c = False
    for i in range(n):
        x1, y1 = poligono[i]
        x2, y2 = poligono[(i + 1) % n]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1 + 1e-12) + x1:
            c = not c
    return c


def distanza_linea(p, linea):
    p = np.asarray(p, np.float64)
    best = 1e18
    for a, b in zip(linea[:-1], linea[1:]):
        a, b = np.asarray(a, np.float64), np.asarray(b, np.float64)
        d = b - a
        t = np.clip(np.dot(p - a, d) / max(1e-9, np.dot(d, d)), 0, 1)
        best = min(best, float(np.linalg.norm(p - (a + d * t))))
    return best


# ------------------------------------------------------------------------------------------------ il fiume

def capo_nel_fiume(t, L, p):
    """Da un punto (metri) verso il fiume più vicino, fino a 1,2 m sotto il pelo dell'acqua: lì finisce la cinta
    (come i capi della palizzata di oggi, 06/11). Restituisce il punto in metri."""
    A = np.array(L(*p), np.float64)
    q, d, _ = t.fiume_vicino(*A)
    q = np.asarray(q, np.float64)
    centro = q[:2] + d * float(np.dot(A - q[:2], d))
    verso = centro - A
    lung = float(np.linalg.norm(verso))
    verso /= max(lung, 1e-9)
    P0 = np.array(L(0, 0), np.float64)
    for s in np.arange(0, lung, 50.0):
        X = A + verso * s
        if t.quota(*X) < q[2] - 120.0:
            return tuple(((X - P0) / 100.0).round(1))
    return tuple(((centro - P0) / 100.0).round(1))


# ------------------------------------------------------------------------------------------------ i pezzi

def portale_legno(nome, largo):
    """Il portale di legno sopra una porta della palizzata (uguale a quelli di oggi), largo in cm."""
    m = Mesh(nome)
    for s in (-1, 1):
        m.cilindro("Trave", [s * (largo / 2 + 10), 0, -80], 26, 560, lati=8)
    m.trave("Trave", [-largo / 2 - 60, 0, 470], [largo / 2 + 60, 0, 470], 30, 30)
    m.trave("Trave", [-largo / 2 - 40, 0, 420], [largo / 2 + 40, 0, 420], 18, 18)
    ed.tetto_casa(m, largo + 120, 120, 490, "Coppi", 30, sporto=30, gronda=10, gable="Assi")
    for s in (-1, 1):
        m.scatola("Assi", [s * (largo / 2 + 10), -largo / 4 - 30, 210], [3 * s, 0, 0], [0, largo / 4, 0], [0, 0, 200])
    return m


def arco_pietra(nome, largo, spessore=200.0):
    """(fase 3) L'arco di pietra sopra una porta del muro, tra le due torri: architrave, merli e battenti aperti."""
    m = Mesh(nome)
    m.blocco("Pietra", -largo / 2 - 60, largo / 2 + 60, -spessore / 2, spessore / 2, 560, 800)
    m.blocco("Pietra", -largo / 2 - 75, largo / 2 + 75, -spessore / 2 - 12, spessore / 2 + 12, 800, 830)
    n = int((largo + 120) / 75)
    for k in range(0, n, 2):
        x = -largo / 2 - 60 + (k + 0.5) * (largo + 120) / n
        m.blocco("Pietra", x - 18, x + 18, -spessore / 2, -spessore / 2 + 50, 830, 905)
    for s in (-1, 1):
        m.scatola("Assi", [s * (largo / 2 - 8), largo / 4 + 20, 270], [3 * s, 0, 0], [0, largo / 4, 0], [0, 0, 265])
    return m


def _linea_terreno(t, L, pz, passo=300.0):
    """I punti di un pezzo di cinta (metri) in cm relativi al primo punto, fitti, con la quota del terreno."""
    A = L(*pz[0])
    qa = t.quota(*A)
    pts = [(L(*p)[0] - A[0], L(*p)[1] - A[1]) for p in pz]
    fitti = []
    for a, b in zip(pts[:-1], pts[1:]):
        n = max(1, int(math.hypot(b[0] - a[0], b[1] - a[1]) / passo))
        for j in range(n):
            xx = a[0] + (b[0] - a[0]) * j / n
            yy = a[1] + (b[1] - a[1]) * j / n
            fitti.append((xx, yy, t.quota(A[0] + xx, A[1] + yy) - qa))
    fitti.append((pts[-1][0], pts[-1][1], t.quota(*L(*pz[-1])) - qa))
    return A, qa, fitti


# ------------------------------------------------------------------------------------------------ i posti liberi

class Mappa:
    """I posti già occupati (case, strade, recinti, orti...), per mettere le case nuove senza toccare niente."""

    def __init__(self, pr, L):
        self.L = L
        self.n = int(2 * MEZZO / RIS)
        self.P0 = np.array(L(0, 0), np.float64)
        pieno = Image.new("L", (self.n, self.n), 0)
        dr = ImageDraw.Draw(pieno)
        strade = Image.new("L", (self.n, self.n), 0)
        ds = ImageDraw.Draw(strade)
        for sez, tipo, punti, nome in pr.disegni:
            pp = [self.px(self.m(p)) for p in punti]
            if tipo in ("edificio", "fab"):
                dr.polygon(pp, fill=255)
            elif tipo == "linea":
                dr.line(pp, fill=255, width=max(1, int(1.0 / RIS)))
                if len(punti) > 3 and punti[0] == punti[-1]:
                    dr.polygon(pp, fill=255)
            elif tipo == "cerchio":
                c = pp[0]
                r = nome / 100.0 / RIS
                dr.ellipse((c[0] - r, c[1] - r, c[0] + r, c[1] + r), fill=255)
            elif tipo == "strada":
                ds.line(pp, fill=255, width=max(1, int(nome / 100.0 / RIS)))
        # le strade dipinte sul paesaggio
        R = np.array(Image.open(pr.t.cartella + "/Strati/Strade.png").convert("L"))
        k = R.shape[0] / (pr.t.luoghi["lato_m"] * 100.0)
        ii, jj = np.meshgrid(np.arange(self.n), np.arange(self.n))
        X = self.P0[0] + (ii * RIS - MEZZO + RIS / 2) * 100.0
        Y = self.P0[1] + (jj * RIS - MEZZO + RIS / 2) * 100.0
        dipinte = R[np.clip((Y * k).astype(int), 0, R.shape[0] - 1), np.clip((X * k).astype(int), 0, R.shape[1] - 1)] > 100
        cose = ndimage.binary_dilation(np.array(pieno) > 0, iterations=int(3.0 / RIS))      # 3 m da case e recinti
        vie = ndimage.binary_dilation((np.array(strade) > 0) | dipinte, iterations=int(1.0 / RIS))
        self.occupato = cose | vie

    @staticmethod
    def m(p):
        """Da cm del paesaggio a metri dalla piazza (serve P0: lo mette la classe)."""
        return p

    def px(self, p_metri):
        return ((p_metri[0] + MEZZO) / RIS, (p_metri[1] + MEZZO) / RIS)

    def libero(self, poligono_m):
        im = Image.new("L", (self.n, self.n), 0)
        ImageDraw.Draw(im).polygon([self.px(p) for p in poligono_m], fill=255)
        dentro_ = np.array(im) > 0
        return not np.any(self.occupato & dentro_)

    def occupa(self, poligono_m, margine=2.0):
        im = Image.new("L", (self.n, self.n), 0)
        ImageDraw.Draw(im).polygon([self.px(p) for p in poligono_m], fill=255)
        self.occupato |= ndimage.binary_dilation(np.array(im) > 0, iterations=int(margine / RIS))

    def occupa_cerchio(self, c, r):
        im = Image.new("L", (self.n, self.n), 0)
        x, y = self.px(c)
        ImageDraw.Draw(im).ellipse((x - r / RIS, y - r / RIS, x + r / RIS, y + r / RIS), fill=255)
        self.occupato |= np.array(im) > 0


def _rettangolo_m(c, W, D, guarda):
    """Il rettangolo di una casa in metri (W lungo la facciata, D di profondità), girato come pr.edificio."""
    g = math.radians(guarda + 90.0)
    cs, sn = math.cos(g), math.sin(g)
    return [(c[0] + sx * W / 2 * cs - sy * D / 2 * sn, c[1] + sx * W / 2 * sn + sy * D / 2 * cs)
            for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]


# ------------------------------------------------------------------------------------------------ le fasi

def fasi_future(pr, S, L, giro_oggi, strade_villaggio, striscia):
    """striscia: la funzione di villaggio.py che fa una strada che segue il terreno."""
    t = pr.t
    rng = np.random.default_rng(206)          # un seme suo: le cose delle altre sezioni restano uguali
    P0 = np.array(L(0, 0), np.float64)

    def M(p_cm):
        return ((p_cm[0] - P0[0]) / 100.0, (p_cm[1] - P0[1]) / 100.0)

    # la mappa lavora in metri dalla piazza: le si insegna a convertire i disegni (che sono in cm)
    Mappa.m = staticmethod(M)

    # ---- la linea larga, con i capi nel fiume e le porte
    capo_n = capo_nel_fiume(t, L, LINEA_LARGA[0])
    capo_s = capo_nel_fiume(t, L, LINEA_LARGA[-1])
    strada_cimitero = [p for punti, _ in strade_villaggio for p in [punti] if punti[-1] == (-120, 195)]
    strade_porte = [STRADA_NORD, STRADA_NORDEST, STRADA_SUDEST] + strada_cimitero
    giro, porte = con_porte([capo_n] + LINEA_LARGA + [capo_s], strade_porte)
    print("fase 2: capi nel fiume %s e %s, %d porte" % (capo_n, capo_s, len(porte)))
    pezzi = pezzi_tra_porte(giro, porte)

    # la linea non deve passare dentro una casa o un recinto di oggi
    for sez, tipo, punti, nome in list(pr.disegni):
        if tipo in ("edificio", "fab"):
            poli = [M(p) for p in punti]
            for pz in pezzi:
                for a, b in zip(pz[:-1], pz[1:]):
                    for s in np.linspace(0, 1, 40):
                        q = (a[0] + (b[0] - a[0]) * s, a[1] + (b[1] - a[1]) * s)
                        if dentro(q, poli):
                            pr.avviso("fase 2: la cinta passa dentro %s (%.0f, %.0f)" % (nome, q[0], q[1]))
                            break

    # poligoni per dire "dentro": la palizzata di oggi e la cinta larga, chiusi lungo il fiume
    poli_oggi = [tuple(p) for p in giro_oggi]
    poli_largo = [tuple(p) for p in giro]

    # ================================================================ FASE 2
    pr.fase = (2, 2)
    for k, pz in enumerate(pezzi):
        A, qa, fitti = _linea_terreno(t, L, pz)
        pr.al_suolo(S, "Palizzata larga %d" % (k + 1), ed.palizzata("SM_Fase2_Palizzata_%d" % (k + 1), fitti, seme=k + 21),
                    A[0], A[1], qa, collisione=True)
        pr.disegni.append((S, "palizzata", [L(*p) for p in pz], ""))
    for k, (a, b) in enumerate(porte):
        Aw, Bw = np.array(L(*a)), np.array(L(*b))
        c = (Aw + Bw) / 2
        g = math.degrees(math.atan2(Bw[1] - Aw[1], Bw[0] - Aw[0]))
        pr.metti_mesh(S, "Portale largo %d" % (k + 1), portale_legno("SM_Fase2_Portale_%d" % (k + 1), float(np.linalg.norm(Bw - Aw))),
                      c[0], c[1], max(t.quota(*Aw), t.quota(*Bw)), g)

    pr.fase = (2, 0)
    mappa = Mappa(pr, L)
    oz = t.quota(*P0)
    centri_porte = [((a[0] + b[0]) / 2, (a[1] + b[1]) / 2) for a, b in porte]

    # la torre del nord, dentro la porta della strada del ponte, a ovest (prima delle zone libere intorno alle porte)
    porta_n = min(porte, key=lambda g: (g[0][1] + g[1][1]) / 2)
    A, B = np.array(porta_n[0], np.float64), np.array(porta_n[1], np.float64)
    u = (B - A) / np.linalg.norm(B - A)
    nrm = np.array([-u[1], u[0]])
    torre_ok = False
    for avanti, dentro_m in ((6.0, 8.0), (9.0, 8.0), (6.0, 12.0), (12.0, 10.0)):
        c = A - u * avanti + nrm * dentro_m
        guarda = math.degrees(math.atan2(nrm[1], nrm[0]))
        if mappa.libero(_rettangolo_m(c, 5.5, 5.5, guarda)):
            X, Y = L(*c)
            pr.edificio(S, "Torre_del_Nord", lambda nome, W, D, **kw: ed.torre(nome, lato=W, altezza=1150, fondazione=kw["fondazione"]),
                        X, Y, guarda, 550, 550, chiave=False)
            mappa.occupa(_rettangolo_m(c, 5.5, 5.5, guarda))
            torre_ok = True
            break
    if not torre_ok:
        pr.avviso("fase 2: la torre del nord non ha posto")

    # la cinta e le porte restano libere: 9 m dalla linea, 12 m intorno alle porte
    linea_px = Image.new("L", (mappa.n, mappa.n), 0)
    ImageDraw.Draw(linea_px).line([mappa.px(p) for p in giro], fill=255, width=2)
    mappa.occupato |= ndimage.binary_dilation(np.array(linea_px) > 0, iterations=int(9.0 / RIS))
    for c in centri_porte:
        mappa.occupa_cerchio(c, 12.0)

    # lanterne alle porte, dentro (3 m), una per parte
    lan = Mesh("SM_Fase2_Lanterne")
    luci = []
    for (a, b) in porte:
        A, B = np.array(a, np.float64), np.array(b, np.float64)
        u = (B - A) / np.linalg.norm(B - A)
        nrm = np.array([-u[1], u[0]])            # dentro (giro in senso orario)
        for e, s in ((A, -1), (B, 1)):
            p = e + u * s * 1.0 + nrm * 3.0
            X, Y = L(*p)
            luci.append(ed.lanterna(lan, X - P0[0], Y - P0[1], t.quota(X, Y) - oz))
    pr.al_suolo(S, "Lanterne delle porte larghe", lan, P0[0], P0[1], oz, collisione=True)
    for k, (lx, ly, lz) in enumerate(luci):
        pr.aggiungi(S, "Luce lanterna larga %d" % k, "luce", P0[0] + lx, P0[1] + ly, oz + lz, intensita=4.0, raggio=1000.0)

    # la via dell'anello: una strada di terra 24 m dentro la cinta larga, a pezzi dove c'è posto (le fattorie restano)
    via = []
    for i, p in enumerate(LINEA_LARGA):
        a = np.array(LINEA_LARGA[max(0, i - 1)], np.float64)
        b = np.array(LINEA_LARGA[min(len(LINEA_LARGA) - 1, i + 1)], np.float64)
        u = (b - a) / np.linalg.norm(b - a)
        via.append(np.array(p, np.float64) + np.array([-u[1], u[0]]) * 24.0)
    campioni = []
    for a, b in zip(via[:-1], via[1:]):
        n = max(1, int(np.linalg.norm(b - a)))
        for j in range(n):
            campioni.append(a + (b - a) * j / n)
    campioni.append(via[-1])
    pezzi_via, corrente = [], []
    for q in campioni:
        quadrato = [(q[0] - 3, q[1] - 3), (q[0] + 3, q[1] - 3), (q[0] + 3, q[1] + 3), (q[0] - 3, q[1] + 3)]
        X, Y = L(*q)
        ripido = abs(t.quota(X + 300, Y) - t.quota(X - 300, Y)) + abs(t.quota(X, Y + 300) - t.quota(X, Y - 300)) > 250
        libero = (mappa.libero(quadrato) or any(np.hypot(q[0] - c[0], q[1] - c[1]) < 12 for c in centri_porte)) and not ripido
        if libero and not dentro(tuple(q), poli_oggi):
            corrente.append(tuple(q.round(1)))
        else:
            if len(corrente) >= 30:
                pezzi_via.append(corrente[::3] + ([corrente[-1]] if (len(corrente) - 1) % 3 else []))
            corrente = []
    if len(corrente) >= 30:
        pezzi_via.append(corrente[::3] + ([corrente[-1]] if (len(corrente) - 1) % 3 else []))
    if pezzi_via:
        sm = Mesh("SM_Fase2_Strade")
        for punti in pezzi_via:
            striscia(sm, t, P0[0], P0[1], oz, [L(*p) for p in punti], 400)
            pr.disegni.append((S, "strada", [L(*p) for p in punti], 400))
            im = Image.new("L", (mappa.n, mappa.n), 0)
            ImageDraw.Draw(im).line([mappa.px(p) for p in punti], fill=255, width=int(6.0 / RIS))
            mappa.occupato |= np.array(im) > 0
        pr.al_suolo(S, "Via dell'anello", sm, P0[0], P0[1], oz)
    print("fase 2: via dell'anello in %d pezzi (%s m)" % (len(pezzi_via), ", ".join("%.0f" % (len(p) * 3) for p in pezzi_via)))

    # l'anello di case: lungo le strade, fuori dalla palizzata di oggi e dentro quella larga
    strade_case = [(STRADA_NORD, 4.5), (STRADA_NORDEST, 4.5), (STRADA_SUDEST, 4.5)] + [(sc, 3.5) for sc in strada_cimitero] \
        + [(pv, 3.5) for pv in pezzi_via]
    case = 0
    stili = [dict(base="Intonaco", sopra="Intonaco", tetto="Coppi", pendenza=32),
             dict(base="Pietra", sopra="Intonaco", tetto="Coppi", pendenza=32),
             dict(base="Pietra", sopra="Assi", tetto="Paglia", pendenza=45),
             dict(base="Intonaco", sopra="Intonaco", tetto="Paglia", pendenza=45),
             dict(base="Assi", sopra="Assi", tetto="Paglia", pendenza=45, graticcio=False)]
    for strada, mezza in strade_case:
        P = [np.array(p, np.float64) for p in strada]
        cum = [0.0]
        for p, q in zip(P[:-1], P[1:]):
            cum.append(cum[-1] + float(np.linalg.norm(q - p)))
        totale = cum[-1]

        def lungo(sv):
            """Il punto e la direzione della strada a sv metri dall'inizio."""
            i = max(0, min(len(P) - 2, int(np.searchsorted(cum, sv, side="right")) - 1))
            d = P[i + 1] - P[i]
            ll = max(1e-9, float(np.linalg.norm(d)))
            return P[i] + d * (sv - cum[i]) / ll, d / ll

        for lato in (1, -1):
            s = 0.0
            while s < totale:
                W = float(rng.choice([8.0, 9.0, 10.0]))
                D = float(rng.choice([7.0, 8.0]))
                if s + W > totale:
                    break
                p, u = lungo(s + W / 2)
                nrm = np.array([-u[1], u[0]]) * lato
                c = p + nrm * (mezza + D / 2)
                guarda = math.degrees(math.atan2(-nrm[1], -nrm[0]))      # la facciata verso la strada
                rett = _rettangolo_m(c, W, D, guarda)
                ok = dentro(tuple(c), poli_largo) and not dentro(tuple(c), poli_oggi) and distanza_linea(c, giro_oggi) > 7.0 \
                    and mappa.libero(rett)
                if ok:
                    X, Y = L(*c)
                    qmin, qmax = t.sotto(X, Y, W * 100 + 100, D * 100 + 100, guarda + 90.0)
                    ok = qmax - qmin < 220.0
                if not ok:
                    s += 1.0
                    continue
                if rng.random() < 0.35:         # un posto lasciato vuoto (un lotto per chi verrà)
                    s += W + 4.0
                    continue
                case += 1
                st = dict(stili[int(rng.integers(len(stili)))])
                piani = 1 + int(rng.random() < 0.4)
                pr.edificio(S, "Casa_Anello_%d" % case, ed.casa, X, Y, guarda, W * 100, D * 100, chiave=True,
                            seme=900 + case, piani=piani, **st)
                mappa.occupa(rett, margine=2.0)
                # tra una casa e l'altra un orto, un cortile o un vicolo: l'anello non è una fila di città
                s += W + float(rng.uniform(9.0, 18.0))
    print("fase 2: %d case nuove" % case)
    if case < 12:
        pr.avviso("fase 2: solo %d case nell'anello" % case)

    # ================================================================ FASE 3
    pr.fase = (3, 0)
    for k, pz in enumerate(pezzi):
        A, qa, fitti = _linea_terreno(t, L, pz)
        pr.al_suolo(S, "Muro di cinta %d" % (k + 1), ed.muro_cinta("SM_Fase3_Muro_%d" % (k + 1), fitti, seme=k + 31),
                    A[0], A[1], qa, collisione=True)
        pr.disegni.append((S, "muro", [L(*p) for p in pz], ""))
    # a ogni porta due torri e l'arco
    torri = 0
    for k, (a, b) in enumerate(porte):
        A, B = np.array(a, np.float64), np.array(b, np.float64)
        largo = float(np.linalg.norm(B - A))
        u = (B - A) / largo
        g = math.degrees(math.atan2(u[1], u[0]))
        Aw, Bw = np.array(L(*A)), np.array(L(*B))
        c = (Aw + Bw) / 2
        pr.metti_mesh(S, "Porta di pietra %d" % (k + 1), arco_pietra("SM_Fase3_Arco_%d" % (k + 1), largo * 100 - 20),
                      c[0], c[1], max(t.quota(*Aw), t.quota(*Bw)), g)
        for e, s in ((A, -1), (B, 1)):
            p = e + u * s * 2.6
            X, Y = L(*p)
            torri += 1
            pr.edificio(S, "Torre_della_porta_%d" % torri,
                        lambda nome, W, D, **kw: ed.torre(nome, lato=W, altezza=950, fondazione=kw["fondazione"], porta=False),
                        X, Y, g - 90.0, 480, 480, chiave=False)
    # torri d'angolo dove la linea gira di più di 30 gradi
    for i in range(1, len(LINEA_LARGA) - 1):
        a, b, c3 = (np.array(p, np.float64) for p in LINEA_LARGA[i - 1:i + 2])
        d1, d2 = b - a, c3 - b
        giro_g = abs(math.degrees(math.atan2(d1[0] * d2[1] - d1[1] * d2[0], np.dot(d1, d2))))
        if giro_g < 30 or min(distanza_linea(b, [p, p]) for p in centri_porte) < 15:
            continue
        X, Y = L(*b)
        torri += 1
        pr.edificio(S, "Torre_d_angolo_%d" % i,
                    lambda nome, W, D, **kw: ed.torre(nome, lato=W, altezza=850, fondazione=kw["fondazione"], porta=False),
                    X, Y, math.degrees(math.atan2(d1[1], d1[0])), 420, 420, chiave=False)
    print("fase 3: muro in %d pezzi, %d torri" % (len(pezzi), torri))
    pr.fase = (1, 0)
