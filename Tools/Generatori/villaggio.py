# Valdorso - Il villaggio, la palude di Nonna Edda e il posto di guardia del passo (v0.1.3). Scritto da Claude il 05/11/2026.
# Decisi da Fra il 05/11: prima la palude, poi tutto il villaggio e il posto di guardia ("voglio un signor villaggio").
# Pianta: il Design "Il nuovo villaggio" (piazza col Tempio del Cuore a nord, locanda di Bruno, Gilda, casa del Balivo,
# via maestra fino alla porta est con la torre della milizia, quartiere degli artigiani verso il mulino, strada del
# ponte a nord, orti a sud, cimitero a sud-ovest, fattorie a sud-est; Nonna Edda nella palude; la Guardia della
# Corona al passo).
#
# Legge le altezze vere della valle e scrive in una cartella (di solito V:/Texture/valdorso/villaggio):
#   T_*.png             le texture (villaggio_texture.py)
#   SM_*.glb            gli edifici e i pezzi (villaggio_edifici.py), già con le fondamenta giuste per il terreno
#   Villaggio_3000.json dove va ogni cosa: lo legge Content/Python/nuovi/crea_villaggio.py nell'editor
#   Anteprima_Villaggio.png, Anteprima_Palude.png, Anteprima_Guardia.png   le piante, per controllare
# Uso: python villaggio.py <cartella valle (Altezze_3000.png, Luoghi_3000.json, Acqua_3000.json, Strati/)> <uscita>
# Sempre uguale (semi fissi). Coordinate: cm dall'angolo nord-ovest del paesaggio (Unreal X = est, Y = sud), Z = quota
# vera in cm (come Rocce_3000.json); i gradi come lo yaw di Unreal (0 = +X, 90 = +Y).

import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw

import sigillo
import villaggio_edifici as ed
import villaggio_fasi
import villaggio_milizia
import villaggio_texture
from villaggio_mesh import Mesh, normalizza

QUOTA_MAX = 80000.0

# --------------------------------------------------------------------------------------------------- il terreno


class Terreno:
    def __init__(self, cartella):
        self.cartella = cartella
        with open(os.path.join(cartella, "Luoghi_3000.json"), encoding="utf-8") as f:
            self.luoghi = json.load(f)
        with open(os.path.join(cartella, "Acqua_3000.json"), encoding="utf-8") as f:
            self.acqua = json.load(f)
        self.h = np.array(Image.open(os.path.join(cartella, "Altezze_3000.png"))).astype(np.float64) / 65535.0 * QUOTA_MAX
        self.S = float(self.luoghi["lato_m"]) * 100.0 / (self.h.shape[0] - 1)
        self.fiume = [p["posizione"] for p in self.acqua["fiume"]["punti"]]

    def quota(self, x, y):
        c, r = x / self.S, y / self.S
        c0, r0 = int(math.floor(c)), int(math.floor(r))
        fc, fr = c - c0, r - r0
        h = self.h
        return (h[r0, c0] * (1 - fc) * (1 - fr) + h[r0, c0 + 1] * fc * (1 - fr) + h[r0 + 1, c0] * (1 - fc) * fr
                + h[r0 + 1, c0 + 1] * fc * fr)

    def luogo(self, nome):
        return self.luoghi["luoghi"][nome]["unreal_cm"]

    def sotto(self, cx, cy, W, D, gradi, passo=100.0):
        """Quota minima e massima sotto un rettangolo W x D (centrato, girato)."""
        a = math.radians(gradi)
        c, s = math.cos(a), math.sin(a)
        q = []
        for u in np.linspace(-W / 2, W / 2, max(2, int(W / passo) + 1)):
            for v in np.linspace(-D / 2, D / 2, max(2, int(D / passo) + 1)):
                q.append(self.quota(cx + u * c - v * s, cy + u * s + v * c))
        return min(q), max(q)

    def fiume_vicino(self, x, y):
        """Il punto del fiume più vicino: (punto, direzione della corrente, normale a sinistra)."""
        P = self.fiume
        i = min(range(len(P) - 1), key=lambda k: (P[k][0] - x) ** 2 + (P[k][1] - y) ** 2)
        p, q = np.array(P[i]), np.array(P[i + 1])
        d = normalizza(q[:2] - p[:2])
        return p, d, np.array([-d[1], d[0]])


# --------------------------------------------------------------------------------------------------- il progetto


class Disegni(list):
    """(06/11) L'elenco dei disegni dell'anteprima: per ognuno ricorda anche la fase del progetto in quel momento."""

    def __init__(self, progetto):
        super().__init__()
        self.progetto = progetto
        self.fasi = []

    def append(self, cosa):
        super().append(cosa)
        self.fasi.append(tuple(self.progetto.fase))


def nella_fase(fase, f):
    """Vero se una cosa con fase (da, fino_a) c'è nella fase f (fino_a 0 = per sempre)."""
    return fase[0] <= f and (fase[1] == 0 or f <= fase[1])


class Progetto:
    def __init__(self, terreno, uscita):
        self.t = terreno
        self.uscita = uscita
        self.cose = []
        self.mesh_scritte = set()
        # (06/11) il villaggio cresce a fasi: ogni cosa nasce in una fase e, se viene sostituita, finisce in un'altra.
        # (1, 0) = c'è dalla fase 1 per sempre; (1, 1) = solo nella fase 1 (la palizzata di oggi); (2, 0) = dalla fase 2.
        self.fase = (1, 0)
        self.disegni = Disegni(self)        # per l'anteprima: (sezione, tipo, punti in cm)
        self.avvisi = []
        self.zone_senza_erba = []   # (cx, cy, mezzo_x, mezzo_y) in cm

    def avviso(self, testo):
        self.avvisi.append(testo)
        print("AVVISO", testo)

    def mesh(self, m):
        if m.nome in self.mesh_scritte:
            return m.nome
        m.scrivi(os.path.join(self.uscita, m.nome + ".glb"))
        self.mesh_scritte.add(m.nome)
        return m.nome

    def aggiungi(self, sezione, nome, tipo, x, y, z, gradi=0.0, **altro):
        d = {"sezione": sezione, "nome": nome, "tipo": tipo, "pos": [round(float(x), 1), round(float(y), 1), round(float(z), 1)],
             "gradi": round(float(gradi), 2), "terreno": round(float(self.t.quota(x, y)), 1)}
        if tuple(self.fase) != (1, 0):
            d["fase"] = [int(self.fase[0]), int(self.fase[1])]
        d.update(altro)
        self.cose.append(d)
        return d

    def metti_mesh(self, sezione, nome, m, x, y, z, gradi=0.0, collisione=True):
        self.aggiungi(sezione, nome, "mesh", x, y, z, gradi, asset=self.mesh(m), collisione=collisione)

    # --- trasformazioni locali -> mondo
    @staticmethod
    def al_mondo(x, y, gradi, lx, ly):
        a = math.radians(gradi)
        return x + lx * math.cos(a) - ly * math.sin(a), y + lx * math.sin(a) + ly * math.cos(a)

    def edificio(self, sezione, nome, costruttore, x, y, guarda, W, D, chiave=True, **kw):
        """Mette un edificio fatto da villaggio_edifici: 'guarda' = verso dove guarda la facciata (gradi).
        Le fondamenta arrivano 60 cm sotto il punto più basso; il pavimento 15 cm sopra il più alto."""
        gradi = guarda + 90.0
        qmin, qmax = self.t.sotto(x, y, W + 40, D + 40, gradi)
        if qmax - qmin > 300:
            self.avviso("%s: terreno in pendenza (%.1f m sotto la pianta)" % (nome, (qmax - qmin) / 100))
        pavimento = qmax + 15.0
        kw["fondazione"] = pavimento - qmin + 60.0
        m, info = costruttore("SM_" + nome, W, D, **kw)
        self.metti_mesh(sezione, nome, m, x, y, pavimento, gradi)
        for porta in info.get("porte", []):
            px, py = self.al_mondo(x, y, gradi, porta[0], porta[1])
            dz = porta[4] if len(porta) > 4 else 0.0
            self.aggiungi(sezione, nome + " porta", "porta", px, py, pavimento + dz, gradi + porta[2] - 90.0, a_chiave=chiave)
        for cosa in info.get("cose", []):
            tipo, lx, ly, lg = cosa[:4]
            px, py = self.al_mondo(x, y, gradi, lx, ly)
            # (06/11) cosa[5]: dati in più per il JSON (per esempio l'altezza di una scala a pioli)
            altro = cosa[5] if len(cosa) > 5 else {}
            self.aggiungi(sezione, "%s %s" % (nome, tipo), tipo, px, py, pavimento + (cosa[4] if len(cosa) > 4 else 0.0), gradi + lg,
                          **altro)
        self.disegni.append((sezione, "edificio", self.rettangolo(x, y, W, D, gradi), nome))
        return pavimento, gradi, info

    @staticmethod
    def rettangolo(x, y, W, D, gradi):
        return [Progetto.al_mondo(x, y, gradi, sx * W / 2, sy * D / 2) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]

    def fab(self, sezione, nome, percorso, x, y, gradi, W, D, modo="perno", zoccolo=True, scala=1.0):
        """Un modello di Fab (Blueprint o mesh). modo 'perno': l'origine è già al centro, a terra (i BP di crea_bp_fab);
        'scatola': crea_villaggio lo mette con il centro della sua scatola qui e il fondo a terra."""
        qmin, qmax = self.t.sotto(x, y, W, D, gradi)
        z = qmax + (10.0 if zoccolo else 0.0)
        if zoccolo and qmax - qmin > 20:
            m = Mesh("SM_Zoccolo_%d_%d_%d" % (int(W), int(D), int(qmax - qmin + 80)))
            m.blocco({"altri": "Pietra", "+z": "Pietra"}, -W / 2 - 15, W / 2 + 15, -D / 2 - 15, D / 2 + 15, -(qmax - qmin + 80), 0)
            self.metti_mesh(sezione, nome + " zoccolo", m, x, y, z, gradi)
        self.aggiungi(sezione, nome, "fab", x, y, z, gradi, asset=percorso, modo=modo, scala=scala)
        self.disegni.append((sezione, "fab", self.rettangolo(x, y, W, D, gradi), nome))
        return z

    def oggetto(self, sezione, nome, percorso, x, y, gradi=0.0, scala=1.0, dz=0.0, z=None):
        """Un oggetto piccolo di Fab (botte, cassa, sgabello...), messo con la sua scatola a terra."""
        if z is None:
            z = self.t.quota(x, y)
        if percorso.endswith("wooden_barrel"):
            # (05/11) le botti di Fab sono un pezzo solo con la tenda: la botte la facciamo noi
            self.metti_mesh(sezione, nome, ed.botte("SM_Villaggio_Botte"), x, y, z + dz, gradi)
            return
        self.aggiungi(sezione, nome, "fab", x, y, z + dz, gradi, asset=percorso, modo="scatola", scala=scala)

    # --- cose che seguono il terreno (strade, piazza, palizzate): mesh in coordinate del mondo, attorno a un'origine
    def al_suolo(self, sezione, nome, m, ox, oy, oz, collisione=False):
        self.metti_mesh(sezione, nome, m, ox, oy, oz, 0.0, collisione=collisione)
        self.cose[-1]["suolo"] = True


# --------------------------------------------------------------------------------------------------- strade e suolo

def striscia(m, t, ox, oy, oz, punti, larga, mat="Terra", alza=6.0, passo=120.0, bordo=0.35):
    """Una strada che segue il terreno: punti (x, y) in cm del mondo; la mesh è relativa a (ox, oy, oz).
    Il colore dei vertici (alfa) va a 0 sui bordi: il materiale li sfrangia."""
    pts = [np.array(p, np.float64) for p in punti]
    lungo = []
    for a, b in zip(pts[:-1], pts[1:]):
        L = np.linalg.norm(b - a)
        n = max(1, int(L / passo))
        for k in range(n):
            lungo.append(a + (b - a) * k / n)
    lungo.append(pts[-1])
    across = [-1.0, -0.55, 0.0, 0.55, 1.0]
    P, N, UV, C, T = [], [], [], [], []
    s = 0.0
    for i, p in enumerate(lungo):
        if i > 0:
            s += np.linalg.norm(p - lungo[i - 1])
        d = lungo[min(i + 1, len(lungo) - 1)] - lungo[max(i - 1, 0)]
        d = d / (np.linalg.norm(d) + 1e-9)
        nrm = np.array([-d[1], d[0]])
        for k, a in enumerate(across):
            q = p + nrm * a * larga / 2
            z = t.quota(q[0], q[1]) + alza
            P.append([q[0] - ox, q[1] - oy, z - oz])
            N.append([0, 0, 1])
            UV.append([q[0] / 400.0, q[1] / 400.0])
            alfa = 1.0 if abs(a) < 0.6 else (0.0 if abs(a) == 1.0 else 0.6)
            if i == 0 or i == len(lungo) - 1:
                alfa = min(alfa, 0.3)
            C.append([1, 1, 1, alfa])
    na = len(across)
    for i in range(len(lungo) - 1):
        for k in range(na - 1):
            a0 = i * na + k
            T += [[a0, a0 + na, a0 + 1], [a0 + 1, a0 + na, a0 + na + 1]]
    m.triangoli(mat, P, N, UV, T, C)


def lastricato(m, t, ox, oy, oz, cx, cy, mezzo_x, mezzo_y, mat="Ciottoli", passo=100.0, sfuma=250.0, alza=8.0, angoli=500.0):
    """Una superficie (la piazza) che segue il terreno, con gli angoli arrotondati e il bordo sfumato."""
    xs = np.arange(-mezzo_x - sfuma, mezzo_x + sfuma + 1, passo)
    ys = np.arange(-mezzo_y - sfuma, mezzo_y + sfuma + 1, passo)
    P, N, UV, C, T = [], [], [], [], []
    for y in ys:
        for x in xs:
            # distanza dal rettangolo arrotondato
            qx = max(abs(x) - (mezzo_x - angoli), 0)
            qy = max(abs(y) - (mezzo_y - angoli), 0)
            d = math.hypot(qx, qy) - angoli
            alfa = float(np.clip(1 - d / sfuma, 0, 1)) if d > 0 else 1.0
            X, Y = cx + x, cy + y
            P.append([X - ox, Y - oy, t.quota(X, Y) + alza - oz])
            N.append([0, 0, 1])
            UV.append([X / 200.0, Y / 200.0])
            C.append([1, 1, 1, alfa])
    nx = len(xs)
    for j in range(len(ys) - 1):
        for i in range(nx - 1):
            a = j * nx + i
            T += [[a, a + nx, a + 1], [a + 1, a + nx, a + nx + 1]]
    m.triangoli(mat, P, N, UV, T, C)


# --------------------------------------------------------------------------------------------------- la palude

def palude(pr, rng):
    t = pr.t
    cx, cy, cz = t.luogo("Palude")
    S = "Palude"
    # 1. la capanna di Nonna Edda, su pali, un po' a nord del centro
    hx, hy = cx + 600, cy - 900
    q0 = t.quota(hx, hy)
    capanna, info = ed.capanna_palafitta("SM_Palude_CapannaEdda", quote_pali=lambda x, y: t.quota(hx + x, hy + y) - q0)
    gradi = 20.0
    pr.metti_mesh(S, "Capanna di Nonna Edda", capanna, hx, hy, q0, gradi)
    for porta in info["porte"]:
        px, py = pr.al_mondo(hx, hy, gradi, porta[0], porta[1])
        pr.aggiungi(S, "Capanna di Nonna Edda porta", "porta", px, py, q0 + porta[4], gradi + porta[2] - 90, a_chiave=False)
    # dentro: il letto di Edda e un tavolo con le erbe
    lx, ly = pr.al_mondo(hx, hy, gradi, 150, 60)
    pr.aggiungi(S, "Letto di Nonna Edda", "letto", lx, ly, q0 + 140, gradi + 180)
    tx, ty = pr.al_mondo(hx, hy, gradi, -120, 80)
    pr.oggetto(S, "Tavolo di Edda", "/Game/Fab/Mercato_Tenda1/table_002", tx, ty, gradi, z=q0 + 140)
    for k, (dx, dy) in enumerate(((-150, -40), (-90, 130), (60, -80))):
        bx, by = pr.al_mondo(hx, hy, gradi, dx, dy)
        pr.oggetto(S, "Vaso di Edda %d" % k, ["/Game/Fab/Mercato_Tenda1/Pot_01_006", "/Game/Fab/Mercato_Tenda1/Pot_02_013"][k % 2], bx, by, rng.uniform(0, 360), z=q0 + 140)
    pr.disegni.append((S, "edificio", pr.rettangolo(hx, hy, 640, 540, gradi), "Capanna di Edda"))

    # 2. il fuoco con il calderone, davanti alla scaletta, su un rialzo di pietre
    fx, fy = pr.al_mondo(hx, hy, gradi, -300, -520)
    qf = t.quota(fx, fy)
    m = Mesh("SM_Palude_Fuoco")
    m.blocco("Terra", -170, 170, -170, 170, -60, 15)
    for k in range(10):
        a = 2 * math.pi * k / 10
        m.scatola("Pietra", [math.cos(a) * 75, math.sin(a) * 75, 20], [18 * math.cos(a), 18 * math.sin(a), 0], [-14 * math.sin(a), 14 * math.cos(a), 0], [0, 0, 12])
    m.blocco("Fiamma", -30, 30, -30, 30, 15, 45)
    for k in range(3):
        a = 2 * math.pi * k / 3
        m.trave("Trave", [math.cos(a) * 95, math.sin(a) * 95, 15], [0, 0, 170], 7, 7)
    m.trave("Ferro", [0, 0, 170], [0, 0, 115], 1.5, 1.5)
    m.cilindro("Ferro", [0, 0, 60], 32, 50, lati=10, raggio_alto=26)
    pr.metti_mesh(S, "Fuoco del calderone", m, fx, fy, qf + 5)
    pr.aggiungi(S, "Luce del fuoco", "luce", fx, fy, qf + 90, intensita=10.0, raggio=900.0)
    # la panca accanto al fuoco
    bx, by = pr.al_mondo(hx, hy, gradi, -300, -300)
    pr.aggiungi(S, "Panca di Nonna Edda", "panca", bx, by, t.quota(bx, by), gradi + 90)

    # 3. la passerella: dal bordo est della palude (dove arriva il sentiero dal lago) alla scaletta
    sx0, sy0 = pr.al_mondo(hx, hy, gradi, 0, -560)
    percorso = [(sx0, sy0), (cx + 2600, cy - 2200), (cx + 4600, cy - 1200), (cx + 6200, cy - 1800)]
    m = Mesh("SM_Palude_Passerella")
    q0p = t.quota(*percorso[0])
    pts = []
    for a, b in zip(percorso[:-1], percorso[1:]):
        a, b = np.array(a), np.array(b)
        n = max(1, int(np.linalg.norm(b - a) / 150))
        for k in range(n):
            pts.append(a + (b - a) * k / n)
    pts.append(np.array(percorso[-1]))
    for i in range(len(pts) - 1):
        a, b = pts[i], pts[i + 1]
        za = max(t.quota(*a), q0p - 200) + 45
        zb = max(t.quota(*b), q0p - 200) + 45
        d = normalizza(np.array([b[0] - a[0], b[1] - a[1], 0]))
        nrm = np.array([-d[1], d[0], 0])
        A = np.array([a[0] - sx0, a[1] - sy0, za - q0p])
        B = np.array([b[0] - sx0, b[1] - sy0, zb - q0p])
        c = (A + B) / 2
        L = np.linalg.norm(B - A)
        u = (B - A) / L
        m.scatola({"+z": "Assi", "-z": "Assi", "altri": "Trave"}, c + [0, 0, -5], u * (L / 2 + 2), nrm * 75, np.cross(u, nrm) * 5 * np.sign(np.cross(u, nrm)[2] or 1))
        for sl in (-1, 1):
            p = A + nrm * sl * 65
            m.cilindro("Trave", [p[0], p[1], t.quota(a[0] + nrm[0] * sl * 65, a[1] + nrm[1] * sl * 65) - q0p - 60], 9, A[2] - (t.quota(a[0], a[1]) - q0p) + 60 - 10, lati=6)
    pr.al_suolo(S, "Passerella della palude", m, sx0, sy0, q0p, collisione=True)
    pr.disegni.append((S, "linea", percorso, "Passerella"))

    # 4. le pozze d'acqua ferma: dove il terreno è più basso, a macchie
    m = Mesh("SM_Palude_Pozze")
    R = 7500.0
    pelo = None
    quote = []
    for gy in np.arange(-R, R, 100):
        for gx in np.arange(-R, R, 100):
            quote.append(t.quota(cx + gx, cy + gy))
    pelo = float(np.percentile(quote, 35)) + 6.0
    macchie = villaggio_texture.morbido(np.random.default_rng(77), 9, n=256)
    passo = 100.0
    P, N, UV, T = [], [], [], []
    for gy in np.arange(-R, R, passo):
        for gx in np.arange(-R, R, passo):
            if gx * gx + gy * gy > R * R:
                continue
            X, Y = cx + gx, cy + gy
            if math.hypot(X - hx, Y - hy) < 700:
                continue
            mi = macchie[int((gy + R) / (2 * R) * 255), int((gx + R) / (2 * R) * 255)]
            if mi < -0.2:
                continue
            q = [t.quota(X, Y), t.quota(X + passo, Y), t.quota(X, Y + passo), t.quota(X + passo, Y + passo)]
            if min(q) > pelo - 4:
                continue
            b = len(P)
            for dx, dy in ((0, 0), (passo, 0), (passo, passo), (0, passo)):
                P.append([gx + dx, gy + dy, pelo - cz])
                N.append([0, 0, 1])
                UV.append([(X + dx) / 400, (Y + dy) / 400])
            T += [[b, b + 1, b + 2], [b, b + 2, b + 3]]
    if P:
        m.triangoli("Acqua", P, N, UV, T)
        pr.al_suolo(S, "Pozze della palude", m, cx, cy, cz)

    # 5. i canneti: ciuffi densi intorno alle pozze e lungo il bordo, non sulla passerella né davanti alla capanna
    def vicino_passerella(X, Y):
        for p in pts:
            if (p[0] - X) ** 2 + (p[1] - Y) ** 2 < 260 ** 2:
                return True
        return math.hypot(X - hx, Y - hy) < 900 or math.hypot(X - fx, Y - fy) < 400

    canne = rng.uniform(-R * 1.05, R * 1.05, (90, 2))
    gruppo = 0
    for (gx, gy) in canne:
        if gx * gx + gy * gy > (R * 1.05) ** 2:
            continue
        X, Y = cx + gx, cy + gy
        if vicino_passerella(X, Y):
            continue
        q = t.quota(X, Y)
        if q > pelo + 60:
            continue
        m = Mesh("SM_Palude_Canneto_%02d" % gruppo)
        raggio = rng.uniform(250, 600)
        n = int(raggio * raggio / 900)
        Pp, Np, UVp, Cp, Tp = [], [], [], [], []
        for k in range(n):
            r = raggio * math.sqrt(rng.random())
            a = rng.uniform(0, 2 * math.pi)
            px, py = r * math.cos(a), r * math.sin(a)
            if vicino_passerella(X + px, Y + py):
                continue
            zb = t.quota(X + px, Y + py) - q - 10
            alt = rng.uniform(170, 290) * (1 - 0.35 * r / raggio)
            curva = rng.uniform(-30, 30, 2)
            larg = rng.uniform(3, 5)
            verde = rng.uniform(0.0, 1.0)
            base_col = np.array([0.30, 0.36, 0.16]) * (1 - verde) + np.array([0.42, 0.40, 0.22]) * verde
            for piano in (0, 1):
                ang = rng.uniform(0, math.pi) + piano * math.pi / 2
                e = np.array([math.cos(ang), math.sin(ang), 0]) * larg
                b0 = len(Pp)
                for s in range(4):
                    f = s / 3
                    c = np.array([px + curva[0] * f * f, py + curva[1] * f * f, zb + alt * f])
                    w = 1 - 0.8 * f
                    Pp += [c - e * w, c + e * w]
                    nn = normalizza(np.cross(e, [0, 0, 1]))
                    Np += [nn, nn]
                    UVp += [[0, f], [1, f]]
                    col = base_col * (0.7 + 0.5 * f)
                    Cp += [list(col) + [1], list(col) + [1]]
                for s in range(3):
                    k0 = b0 + 2 * s
                    Tp += [[k0, k0 + 1, k0 + 2], [k0 + 1, k0 + 3, k0 + 2]]
            # pennacchio scuro in cima (metà delle canne)
            if rng.random() < 0.5:
                c = np.array([px + curva[0], py + curva[1], zb + alt])
                b0 = len(Pp)
                Pp += [c + [-6, 0, -30], c + [6, 0, -30], c + [6, 0, 5], c + [-6, 0, 5]]
                Np += [[0, 1, 0]] * 4
                UVp += [[0, 0], [1, 0], [1, 1], [0, 1]]
                Cp += [[0.28, 0.20, 0.12, 1]] * 4
                Tp += [[b0, b0 + 1, b0 + 2], [b0, b0 + 2, b0 + 3]]
        if Pp:
            m.triangoli("Canna", Pp, Np, UVp, Tp, Cp)
            pr.al_suolo(S, "Canneto %d" % gruppo, m, X, Y, q)
            pr.disegni.append((S, "canneto", [(X, Y)], raggio))
            gruppo += 1

    # 6. tronchi morti mezzo affondati e ceppi
    m = Mesh("SM_Palude_Tronchi")
    for k in range(14):
        gx, gy = rng.uniform(-R, R, 2)
        if gx * gx + gy * gy > R * R or vicino_passerella(cx + gx, cy + gy):
            continue
        z = t.quota(cx + gx, cy + gy) - cz
        if rng.random() < 0.5:
            a = rng.uniform(0, 2 * math.pi)
            L = rng.uniform(300, 700)
            d = np.array([math.cos(a), math.sin(a), rng.uniform(-0.08, 0.08)])
            m.cilindro("Trave", np.array([gx, gy, z - 10]) - d * L / 2, rng.uniform(18, 30), L, lati=7, asse=d, raggio_alto=rng.uniform(10, 18))
        else:
            m.cilindro("Trave", [gx, gy, z - 30], rng.uniform(22, 35), rng.uniform(60, 160), lati=7, raggio_alto=rng.uniform(15, 25))
    pr.al_suolo(S, "Tronchi della palude", m, cx, cy, cz, collisione=True)

    # 7. felci e rocce muschiose (modelli già nel progetto)
    for k in range(40):
        a = rng.uniform(0, 2 * math.pi)
        r = R * rng.uniform(0.85, 1.25)
        X, Y = cx + math.cos(a) * r, cy + math.sin(a) * r
        pr.oggetto(S, "Felce %d" % k, "/Game/KiteDemo/Environments/Foliage/Ferns/SM_Fern_0%d" % (1 + k % 3), X, Y, rng.uniform(0, 360), scala=rng.uniform(0.8, 1.4), dz=-5)
    for k in range(8):
        a = rng.uniform(0, 2 * math.pi)
        r = R * rng.uniform(0.9, 1.2)
        X, Y = cx + math.cos(a) * r, cy + math.sin(a) * r
        pr.oggetto(S, "Roccia muschiosa %d" % k, "/Game/Fab/PolyHaven/rock_moss_set_0%d" % (1 + k % 2), X, Y, rng.uniform(0, 360), scala=rng.uniform(1.0, 2.0), dz=-30)
    pr.aggiungi(S, "Luce della capanna", "luce", *pr.al_mondo(hx, hy, gradi, 0, -330), q0 + 380, intensita=6.0, raggio=700.0)
    pr.disegni.append((S, "cerchio", [(cx, cy)], R))
    print("palude: pelo delle pozze %.1f m, %d canneti" % (pelo / 100, gruppo))


# --------------------------------------------------------------------------------------------------- il villaggio

def ponte_su_strada(pr, sezione, nome, mesh_nome, cx, cy, u, larga):
    """(05/11) Un ponte dove una strada dipinta sul paesaggio passa il fiume, girato come la strada (non di traverso al
    fiume): così la strada arriva dritta sulle due teste del ponte. Restituisce le due teste (cm)."""
    t = pr.t
    u = np.asarray(u, np.float64) / np.linalg.norm(u)
    p, _, _ = t.fiume_vicino(cx, cy)
    pelo = p[2]
    prof = [(tt, t.quota(cx + u[0] * tt * 100, cy + u[1] * tt * 100)) for tt in range(-30, 31)]
    estremo = max(abs(tt) for tt, q in prof if q < pelo + 420)
    lung = (estremo + 4) * 2 * 100
    a = (cx - u[0] * lung / 2, cy - u[1] * lung / 2)
    b = (cx + u[0] * lung / 2, cy + u[1] * lung / 2)
    piano = max(t.quota(*a), t.quota(*b)) + 5
    appoggi = [(xx, t.quota(cx + u[0] * xx, cy + u[1] * xx) - piano) for xx in np.linspace(-lung / 2 + 300, lung / 2 - 300, 5)]
    pr.metti_mesh(sezione, nome, ed.ponte(mesh_nome, lung, larga, appoggi), cx, cy, piano, math.degrees(math.atan2(u[1], u[0])))
    pr.disegni.append((sezione, "linea", [a, b], nome))
    print("%s: %.0f m" % (nome, lung / 100))
    return a, b


def villaggio(pr, rng):
    t = pr.t
    P0x, P0y, _ = t.luogo("Piazza del villaggio")
    S = "Villaggio"

    def L(x, y):
        """Da metri locali (x est, y sud, dal centro della piazza) a cm del paesaggio."""
        return P0x + x * 100.0, P0y + y * 100.0

    def casa(nome, x, y, guarda, W, D, chiave=True, **kw):
        X, Y = L(x, y)
        return pr.edificio(S, nome, ed.casa, X, Y, guarda, W * 100, D * 100, chiave=chiave, seme=len(pr.cose), **kw)

    oz = t.quota(P0x, P0y)

    # ---- la piazza
    m = Mesh("SM_Villaggio_Piazza")
    lastricato(m, t, P0x, P0y, oz, P0x, P0y - 150, 2300, 2000)
    pr.zone_senza_erba.append((P0x, P0y - 150, 2300, 2000))
    pr.al_suolo(S, "Piazza", m, P0x, P0y, oz)

    # ---- (1) il Tempio del Cuore, a nord, con l'ingresso verso la piazza
    X, Y = L(0, -33.5)
    pav, gr, info = pr.edificio(S, "Tempio_del_Cuore", lambda nome, W, D, **kw: ed.tempio(nome, W, D, fondazione=kw["fondazione"]),
                                X, Y, 90.0, 1200, 2200)
    # l'edificio ha l'ingresso a -Y: guarda = 90 lo gira verso sud (la piazza)
    ax_, ay_ = pr.al_mondo(X, Y, gr, *info["altare"][:2])
    pr.aggiungi(S, "Altare del Cuore (luce)", "luce", ax_, ay_, pav + 260, intensita=8.0, raggio=1200.0, colore=[1.0, 0.45, 0.35])
    for k in range(2):
        lx, ly = pr.al_mondo(X, Y, gr, (-1) ** k * 160, -1100 + 60)
        pr.aggiungi(S, "Luce del portale %d" % k, "luce", lx, ly, pav + 330, intensita=5.0, raggio=800.0)

    # (2) la canonica di Padre Anselmo, a ovest del Tempio
    casa("Canonica", -27, -38, 90, 8, 7, piani=2, base="Pietra", sopra="Intonaco", chiave=True)
    # (3) la casa del Balivo (il Blueprint grande con il camino) dentro un cortile murato
    bx, by = L(41, -19)
    pr.fab(S, "Casa del Balivo", "/Game/Fab/Casa_Camino/BP_Casa_Camino", bx, by, 0.0, 714, 1253)
    cort = [(28, -4), (28, -35), (55, -35), (55, -4), (35, -4)]
    m = Mesh("SM_Villaggio_CortileBalivo")
    cX, cY = L(28, -4)
    cz = t.quota(cX, cY)
    for (a, b) in zip(cort[:-1], cort[1:]):
        A, B = np.array(L(*a)), np.array(L(*b))
        n = max(1, int(np.linalg.norm(B - A) / 150))
        for k in range(n):
            p = A + (B - A) * (k + 0.5) / n
            z = t.quota(*p) - cz
            d = normalizza(np.array([B[0] - A[0], B[1] - A[1], 0]))
            m.scatola({"altri": "Pietra", "+z": "Coppi"}, [p[0] - cX, p[1] - cY, z + 40], d * (np.linalg.norm(B - A) / n / 2 + 2), [-d[1] * 30, d[0] * 30, 0], [0, 0, 130])
    # pilastri del cancello
    for gx in (28.0, 35.0):
        p = L(gx, -4)
        m.blocco("Pietra", p[0] - cX - 35, p[0] - cX + 35, p[1] - cY - 35, p[1] - cY + 35, t.quota(*p) - cz - 50, t.quota(*p) - cz + 240)
    pr.al_suolo(S, "Muro del cortile del Balivo", m, cX, cY, cz, collisione=True)
    pr.disegni.append((S, "linea", [L(*p) for p in cort], "Cortile"))
    for k, (x, y) in enumerate(((31, -32), (32, -30.5), (52, -32))):
        pr.oggetto(S, "Botte del Balivo %d" % k, "/Game/Fab/Mercato_Tenda2/wooden_barrel", *L(x, y), rng.uniform(0, 360))
    pr.aggiungi(S, "Panca del cortile", "panca", *L(46, -8), t.quota(*L(46, -8)), -90.0)

    # (4) la locanda di Bruno, a ovest della piazza: due piani, si entra, dentro il bancone e i tavoli, sopra i letti
    def locanda(nome, W, D, fondazione):
        m, info = ed.casa(nome, W, D, piani=2, base="Pietra", sopra="Intonaco", tetto="Coppi", fondazione=fondazione,
                          porta_x=-150, finestre_aperte=False, solaio_aperto=(W / 2 - 190, W / 2 - 60, -D / 2 + 60, D / 2 - 340), seme=4)
        ed.bancone(m, -W / 2 + 120, -W / 2 + 520, D / 2 - 180)
        ed.scala(m, W / 2 - 125, -D / 2 + 70, D / 2 - 340, 0, 300 - 22, larga=120)
        # tramezzo di assi sopra, per le stanze
        m.muro("Assi", (-60, -D / 2 + 40, 300), 90.0, D - 80 - 220, 250, 10, [(D - 80 - 220 - 120, D - 80 - 220, 0, 220)])
        info["cose"] = [
            ("letto", -W / 2 + 150, -D / 2 + 150, 0.0, 300.0), ("letto", -W / 2 + 150, D / 2 - 150, 0.0, 300.0),
            ("letto", -W / 2 + 420, -D / 2 + 150, 0.0, 300.0), ("letto", 160, D / 2 - 150, 90.0, 300.0),
            ("panca", -150, -60, 0.0), ("panca", 150, -60, 180.0), ("panca", 250, 160, 0.0),
        ]
        info["tavoli"] = [(0, -60), (380, 160)]
        return m, info

    X, Y = L(-31, 2)
    pav, gr, info = pr.edificio(S, "Locanda_di_Bruno", lambda nome, W, D, **kw: locanda(nome, W, D, kw["fondazione"]),
                                X, Y, 0.0, 1400, 1000, chiave=False)
    for k, (lx, ly) in enumerate(info["tavoli"]):
        px, py = pr.al_mondo(X, Y, gr, lx, ly)
        pr.oggetto(S, "Tavolo della locanda %d" % k, "/Game/Fab/Mercato_Tenda1/table_002", px, py, gr, z=pav)
    for k, (lx, ly) in enumerate(((-500, 380), (-420, 400), (-560, 300))):
        px, py = pr.al_mondo(X, Y, gr, lx, ly)
        pr.oggetto(S, "Botte della locanda %d" % k, "/Game/Fab/Mercato_Tenda2/wooden_barrel", px, py, rng.uniform(0, 360), z=pav)
    px, py = pr.al_mondo(X, Y, gr, 0, 0)
    pr.aggiungi(S, "Luce della locanda", "luce", px, py, pav + 250, intensita=6.0, raggio=900.0)
    px, py = pr.al_mondo(X, Y, gr, -150, -700)
    pr.aggiungi(S, "Insegna della locanda (luce)", "luce", px, py, pav + 300, intensita=4.0, raggio=700.0)

    # (5) la Gilda, a sud della piazza: si entra, il bancone di Lucia
    def gilda(nome, W, D, fondazione):
        m, info = ed.casa(nome, W, D, piani=2, base="Pietra", sopra="Intonaco", fondazione=fondazione, seme=5)
        ed.bancone(m, -200, 200, 120)
        m.blocco("Assi", -W / 2 + 60, -W / 2 + 100, -150, 250, 0, 220)   # scaffale degli incarichi
        return m, info

    X, Y = L(-6, 26.5)
    pav, gr, info = pr.edificio(S, "Gilda", lambda nome, W, D, **kw: gilda(nome, W, D, kw["fondazione"]), X, Y, -90.0, 1200, 900, chiave=False)
    px, py = pr.al_mondo(X, Y, gr, 0, 0)
    pr.aggiungi(S, "Luce della Gilda", "luce", px, py, pav + 250, intensita=5.0, raggio=800.0)
    px, py = pr.al_mondo(X, Y, gr, 350, -560)
    pr.aggiungi(S, "Bacheca della Gilda", "bacheca", px, py, t.quota(px, py), gr - 90.0,
                titolo="Gilda di Valdorso", avvisi=["Cercasi braccia per il raccolto agli orti: chiedere a Lucia.",
                                                     "La palude non è un posto per gli sprovveduti. Chi va da Nonna Edda porti il dovuto."])

    # (6) il pozzo e (7) il mercato
    X, Y = L(-7, -6)
    pr.metti_mesh(S, "Pozzo", ed.pozzo("SM_Villaggio_Pozzo"), X, Y, t.quota(X, Y))
    pr.disegni.append((S, "cerchio", [(X, Y)], 100.0))
    for k, (x, y, g) in enumerate(((-7, -2.5, -90.0), (-2.5, -6, 180.0))):
        pr.aggiungi(S, "Panca del pozzo %d" % k, "panca", *L(x, y), t.quota(*L(x, y)), g)
    pr.fab(S, "Mercato (tenda grande)", "/Game/Fab/Mercato_Tenda1/BP_Mercato_Tenda1", *L(12, 8.5), 0.0, 1165, 996, zoccolo=False)
    pr.fab(S, "Mercato (tenda piccola)", "/Game/Fab/Mercato_Tenda2/tent_prop_02_Sfab", *L(-11, 10), 90.0, 1000, 1000, modo="scatola", zoccolo=False)
    for k in range(6):
        x, y = rng.uniform(-16, -6), rng.uniform(5, 15)
        pr.oggetto(S, "Merce %d" % k, ["/Game/Fab/Mercato_Tenda1/Crate_003", "/Game/Fab/Mercato_Tenda1/vegetable_box_01_005",
                                         "/Game/Fab/Mercato_Tenda1/vegetable_box_03_006", "/Game/Fab/Mercato_Tenda1/wooden_bucket_01_005"][k % 4],
                   *L(x, y), rng.uniform(0, 360))
    pr.aggiungi(S, "Bacheca del villaggio", "bacheca", *L(16, -14), t.quota(*L(16, -14)), 180.0,
                titolo="Bacheca di Valdorso", avvisi=["Per ordine del Balivo: il mercato si tiene ogni giorno dall'alba al vespro.",
                                                      "La locanda di Bruno ha letti liberi."])

    # lanterne della piazza
    lan = Mesh("SM_Villaggio_Lanterne")
    luci = []
    punti_lanterna = [(-20, -15), (20, -15), (-20, 16), (20, 16), (24, 2), (60, 2), (96, 2), (-0.1 + (-60 + 35) * 0.1033 + 4, -60), (-0.1 + (-110 + 35) * 0.1033 + 4, -110),
                      (-24, -14), (8, 40), (8, 80), (-50, -14)]
    for (x, y) in punti_lanterna:
        X, Y = L(x, y)
        luci.append(ed.lanterna(lan, X - P0x, Y - P0y, t.quota(X, Y) - oz))
    pr.al_suolo(S, "Lanterne", lan, P0x, P0y, oz, collisione=True)
    for k, (lx, ly, lz) in enumerate(luci):
        pr.aggiungi(S, "Luce lanterna %d" % k, "luce", P0x + lx, P0y + ly, oz + lz, intensita=4.0, raggio=1000.0)

    # ---- via maestra, verso la porta est
    pr.fab(S, "Casa di Donna Beatrice", "/Game/Fab/Case_Medievali/BP_Case_Medievali", *L(62, -10.5), 180.0, 804, 741)
    casa("Casa_Nord_1", 74, -9.5, 90, 10, 8, piani=2, base="Pietra", sopra="Intonaco")
    casa("Casa_Nord_2", 83.5, -9, 90, 9, 7, piani=1, base="Intonaco", sopra="Intonaco", tetto="Coppi")
    casa("Casa_Milizia", 103, -4, 180, 10, 8, piani=2, base="Pietra", sopra="Assi", chiave=False)
    for k, (x, W, D, piani, base, tetto) in enumerate(((30, 10, 8, 2, "Pietra", "Coppi"), (42, 9, 8, 1, "Intonaco", "Paglia"),
                                                        (66, 10, 8, 2, "Intonaco", "Coppi"), (78, 9, 7, 1, "Pietra", "Paglia"))):
        casa("Casa_Sud_%d" % (k + 1), x, 7 + D / 2, -90, W, D, piani=piani, base=base, sopra="Intonaco", tetto=tetto,
             pendenza=45 if tetto == "Paglia" else 32)
    pr.fab(S, "Casa alta della via maestra", "/Game/Fab/Case_Medievali/BP_Case_Medievali", *L(54, 11), 0.0, 804, 741)

    # ---- (14) porta est con la torre della milizia
    X, Y = L(114, -18.2)
    pr.edificio(S, "Torre_della_Milizia", lambda nome, W, D, **kw: ed.torre(nome, lato=W, altezza=1300, fondazione=kw["fondazione"]),
                X, Y, 180.0, 550, 550, chiave=False)
    # (06/11) il cortile della milizia: due fantocci e una rastrelliera tra la casa della milizia e la palizzata
    villaggio_milizia.cortile_fase1(pr, S, L, t)
    # ---- strada del ponte (a nord) e il lavatoio
    def x_strada_nord(y):
        # (05/11) la linea di mezzo vera della strada dipinta (misurata sullo strato Strade, da y -35 a -165)
        return -0.1 + (y + 35) * 0.1033
    for k, y in enumerate((-70, -86, -102, -118, -134)):
        casa("Casa_Ponte_O%d" % (k + 1), x_strada_nord(y) - 11, y, 0, 9, 7, piani=1 + (k % 2), base=["Pietra", "Intonaco"][k % 2],
             tetto=["Coppi", "Paglia"][(k // 2) % 2], pendenza=[32, 45][(k // 2) % 2])
    for k, y in enumerate((-64, -80, -96, -126)):
        casa("Casa_Ponte_E%d" % (k + 1), x_strada_nord(y) + 11, y, 180, 9, 7, piani=1 + ((k + 1) % 2), base=["Intonaco", "Pietra"][k % 2],
             tetto=["Paglia", "Coppi"][k % 2], pendenza=[45, 32][k % 2])
    # lavatoio: vasca di pietra con l'acqua, tettoia sopra
    X, Y = L(x_strada_nord(-111) + 10, -111)
    q = t.quota(X, Y)
    m = ed.tettoia("SM_Villaggio_Lavatoio", 700, 450, altezza=260, tetto="Coppi")
    m.blocco("Pietra", -280, 280, -130, 130, -30, 70)
    m.blocco("Acqua", -250, 250, -100, 100, 50, 62)
    pr.metti_mesh(S, "Lavatoio", m, X, Y, q, 90.0)
    pr.disegni.append((S, "edificio", pr.rettangolo(X, Y, 700, 450, 90.0), "Lavatoio"))

    # ---- quartiere nord-est: vicolo e case, tre lotti vuoti
    for k, y in enumerate((-46, -61, -76)):
        casa("Casa_Vicolo_E%d" % (k + 1), 67, y, 180, 9, 7, piani=1 + (k % 2), base=["Intonaco", "Pietra"][k % 2], tetto="Coppi")
    for k, y in enumerate((-52, -68)):
        casa("Casa_Vicolo_O%d" % (k + 1), 46, y, 0, 9, 7, piani=1, base="Intonaco", tetto="Paglia", pendenza=45)
    for k, x in enumerate((20, 34)):
        casa("Casa_Vicolo_N%d" % (k + 1), x, -104, 90, 9, 7, piani=1 + k, base="Pietra", tetto="Coppi")
    lotti = [(84, -50), (84, -70), (84, -90), (40, 98), (60, 98)]
    # case lungo la palizzata est
    for k, (x, y) in enumerate(((101, -36), (101, -52), (99, -84), (100, 26), (100, 42), (96, 70))):
        casa("Casa_Est_%d" % (k + 1), x, y, 180, 8 + k % 2, 7, piani=1 + (k % 3 == 1), base=["Intonaco", "Pietra", "Assi"][k % 3],
             sopra="Intonaco", tetto=["Paglia", "Coppi"][k % 2], pendenza=[45, 32][k % 2], graticcio=k % 3 != 2)

    # ---- (9) quartiere degli artigiani: fucina, falegname, conceria
    def fucina(nome, W, D, fondazione):
        m, info = ed.casa(nome, W, D, piani=1, base="Pietra", tetto="Coppi", fondazione=fondazione, seme=9)
        t2 = ed.tettoia("x", W - 100, 500, altezza=280, tetto="Coppi")
        m.unisci(t2, [0, D / 2 + 250, 0], 180.0)
        m.blocco({"altri": "Pietra", "+z": "Ferro"}, -150, 50, D / 2 + 150, D / 2 + 300, 0, 90)   # forgia
        m.blocco("Fiamma", -110, 10, D / 2 + 180, D / 2 + 270, 90, 110)
        m.blocco("Pietra", -110, 10, D / 2 + 300, D / 2 + 340, 0, 420)                            # cappa
        m.blocco("Ferro", 150, 230, D / 2 + 200, D / 2 + 240, 60, 85)                             # incudine
        m.blocco("Trave", 165, 215, D / 2 + 195, D / 2 + 245, 0, 60)
        return m, info

    X, Y = L(-28, 31)
    pav, gr, _ = pr.edificio(S, "Fucina_di_Marta", lambda nome, W, D, **kw: fucina(nome, W, D, kw["fondazione"]), X, Y, -90.0, 1000, 800, chiave=False)
    fx_, fy_ = pr.al_mondo(X, Y, gr, -50, 400 + 225)
    pr.aggiungi(S, "Luce della forgia", "luce", fx_, fy_, pav + 150, intensita=8.0, raggio=900.0, colore=[1.0, 0.5, 0.2])
    casa("Falegname", -14, 46, -90, 9, 7, piani=1, base="Assi", sopra="Assi", tetto="Paglia", pendenza=45, graticcio=False)
    cat = Mesh("SM_Villaggio_Cataste")
    X, Y = L(-20, 52)
    qc = t.quota(X, Y)
    ed.catasta(cat, 0, 0, 0, 0.0, 300, 5)
    ed.catasta(cat, 0, 260, 0, 0.0, 250, 4)
    pr.metti_mesh(S, "Cataste del falegname", cat, X, Y, qc - 5)
    casa("Conceria", -46, 47, 0, 11, 7, piani=1, base="Pietra", sopra="Assi", tetto="Paglia", pendenza=45)
    tel = Mesh("SM_Villaggio_Telai")
    for k in range(3):
        y0 = k * 220
        tel.cilindro("Trave", [-150, y0, -40], 7, 260, lati=5)
        tel.cilindro("Trave", [150, y0, -40], 7, 260, lati=5)
        tel.trave("Trave", [-160, y0, 200], [160, y0, 200], 8, 8)
        tel.scatola("Stoffa", [0, y0, 140], [120, 0, 0], [0, 2, 0], [0, 0, 55])
    X, Y = L(-36, 56)
    pr.metti_mesh(S, "Telai della conceria", tel, X, Y, t.quota(X, Y))
    casa("Casa_Artigiani_1", -3, 44, 0, 8, 7, piani=1, base="Intonaco", tetto="Coppi")
    casa("Casa_Artigiani_2", -3, 58, 0, 9, 7, piani=2, base="Pietra", tetto="Coppi")
    casa("Casa_Artigiani_3", -3, 72, 0, 8, 7, piani=1, base="Intonaco", tetto="Paglia", pendenza=45)
    for k, y in enumerate((36, 50, 64)):
        casa("Casa_Sud_Vicolo_%d" % (k + 1), 20, y, 180, 8, 7, piani=1 + (k % 2), base=["Pietra", "Intonaco"][k % 2], tetto="Coppi")

    # ---- (12) il mulino di Teodoro e Agnese e il ponte del mulino, (13) la capanna di Orlando il Muto
    # (05/11) il ponte va dove la strada dipinta di nord-ovest passa il fiume (misurato sullo strato Strade)
    ponte_su_strada(pr, S, "Ponte del mulino", "SM_Villaggio_PonteMulino", *L(-112.8, -35.4), (0.707, 0.707), 400)
    p, d, n = t.fiume_vicino(*L(-119, -19))
    # n punta a sinistra della corrente; il villaggio è a est: si sceglie il verso che va verso il villaggio
    verso_villaggio = np.array([P0x - p[0], P0y - p[1]])
    if np.dot(n, verso_villaggio) < 0:
        n = -n
    # il mulino: 25 m a valle, sulla sponda del villaggio, con la ruota nell'acqua
    pm = np.array(p[:2]) + d * 2500
    pm3, dm, nm = t.fiume_vicino(*pm)
    if np.dot(nm, verso_villaggio) < 0:
        nm = -nm
    centro = np.array(pm3[:2]) + nm * 1700
    guarda = math.degrees(math.atan2(-nm[1], -nm[0]))   # la facciata verso il fiume
    pav_m, gr_m, _ = pr.edificio(S, "Mulino", ed.casa, centro[0], centro[1], guarda, 1000, 900, chiave=True, piani=2, base="Pietra",
                                  sopra="Assi", tetto="Coppi", graticcio=False, seme=12)
    ruota = Mesh("SM_Villaggio_RuotaMulino")
    rc = np.array(pm3[:2]) + nm * 450
    ed.ruota_mulino(ruota, 0, 0, 0, raggio=280, larga=120, asse_gradi=math.degrees(math.atan2(dm[1], dm[0])))
    asse_l = np.linalg.norm(centro - rc) - 450
    ruota.cilindro("Trave", [0, 0, 0], 16, max(100.0, asse_l), lati=8, asse=(nm[0], nm[1], 0))
    pr.metti_mesh(S, "Ruota del mulino", ruota, rc[0], rc[1], pm3[2] + 170, collisione=False)
    # Orlando il Muto: 45 m più a valle, capanna sulla sponda con il pontile nel fiume
    po = np.array(p[:2]) + d * 6000
    po3, do, no = t.fiume_vicino(*po)
    if np.dot(no, verso_villaggio) < 0:
        no = -no
    hc = np.array(po3[:2]) + no * 1500
    q0 = t.quota(*hc)
    cap, info = ed.capanna_palafitta("SM_Villaggio_CapannaOrlando", W=420, D=380, alto=100,
                                     quote_pali=lambda x, y: 0.0)
    # i pali seguono la sponda: la capanna si rifà con le quote vere intorno
    gr_o = math.degrees(math.atan2(-no[1], -no[0])) + 90
    cap, info = ed.capanna_palafitta("SM_Villaggio_CapannaOrlando", W=420, D=380, alto=100 + max(0, q0 - min(t.quota(*(hc + no * s)) for s in (-300, 300))),
                                     quote_pali=lambda x, y: t.quota(*pr.al_mondo(hc[0], hc[1], gr_o, x, y)) - q0)
    pr.metti_mesh(S, "Capanna di Orlando il Muto", cap, hc[0], hc[1], q0, gr_o)
    for porta in info["porte"]:
        px, py = pr.al_mondo(hc[0], hc[1], gr_o, porta[0], porta[1])
        pr.aggiungi(S, "Capanna di Orlando porta", "porta", px, py, q0 + porta[4], gr_o + porta[2] - 90, a_chiave=True)
    pp = np.array(po3[:2]) + no * 900
    zp = po3[2] + 60
    pont = ed.pontile("SM_Villaggio_PontileOrlando", 700, 160,
                      lambda x: t.quota(pp[0] - no[0] * x, pp[1] - no[1] * x) - zp)
    pr.metti_mesh(S, "Pontile di Orlando", pont, pp[0], pp[1], zp, math.degrees(math.atan2(-no[1], -no[0])))
    pr.disegni.append((S, "edificio", pr.rettangolo(hc[0], hc[1], 580, 500, gr_o), "Orlando"))

    # ---- (19) il cimitero con la cappella
    CX, CY = L(-150, 205)
    cq = t.quota(CX, CY)
    m = Mesh("SM_Villaggio_Cimitero")
    W2, D2 = 2600, 2000
    angoli = [(-W2 / 2, -D2 / 2), (W2 / 2, -D2 / 2), (W2 / 2, D2 / 2), (-W2 / 2, D2 / 2), (-W2 / 2, -D2 / 2)]
    for (a, b) in zip(angoli[:-1], angoli[1:]):
        A, B = np.array(a), np.array(b)
        n = max(1, int(np.linalg.norm(B - A) / 150))
        for k in range(n):
            pmid = A + (B - A) * (k + 0.5) / n
            if abs(pmid[1] + D2 / 2) < 1 and abs(pmid[0] - 600) < 160:     # cancello a nord-est (verso il villaggio)
                continue
            z = t.quota(CX + pmid[0], CY + pmid[1]) - cq
            dd = normalizza(np.array([B[0] - A[0], B[1] - A[1], 0]))
            m.scatola({"altri": "Pietra", "+z": "Pietra"}, [pmid[0], pmid[1], z + 25], dd * (np.linalg.norm(B - A) / n / 2 + 3), [-dd[1] * 30, dd[0] * 30, 0], [0, 0, 85])
    r2 = np.random.default_rng(19)
    for i in range(6):
        for j in range(5):
            if r2.random() < 0.18:
                continue
            gx = -W2 / 2 + 300 + i * 330 + r2.uniform(-30, 30)
            gy = -D2 / 2 + 300 + j * 300 + r2.uniform(-20, 20)
            if gx > W2 / 2 - 800 and gy > D2 / 2 - 900:
                continue
            ed.lapide(m, gx, gy, t.quota(CX + gx, CY + gy) - cq, 0.0, int(r2.random() < 0.4), r2)
    pr.al_suolo(S, "Cimitero", m, CX, CY, cq, collisione=True)
    pr.disegni.append((S, "linea", [(CX + a, CY + b) for a, b in angoli], "Cimitero"))
    X, Y = CX + W2 / 2 - 450, CY + D2 / 2 - 500
    pr.edificio(S, "Cappella_del_cimitero", ed.casa, X, Y, -90.0, 600, 800, chiave=False, piani=1, base="Pietra", sopra="Pietra",
                tetto="Coppi", camino=False, h1=420, pendenza=40, finestra_l=50, finestra_h=120)
    pr.aggiungi(S, "Panca del cimitero", "panca", CX - 300, CY - D2 / 2 + 250, t.quota(CX - 300, CY - D2 / 2 + 250), 90.0)

    # ---- orti a sud (dentro la palizzata): aiuole con i solchi e i cavoli, staccionate
    orti = Mesh("SM_Villaggio_Orti")
    ox_, oy_ = L(30, 40)
    oq = t.quota(ox_, oy_)
    r3 = np.random.default_rng(31)
    for (x0, y0, w, h) in ((30, 40, 22, 16), (56, 40, 20, 16), (30, 60, 22, 18), (56, 60, 20, 18)):
        A = L(x0, y0)
        pts = [L(x0, y0), L(x0 + w, y0), L(x0 + w, y0 + h), L(x0, y0 + h)]
        rec = ed.recinto("x", [(p[0] - ox_, p[1] - oy_, t.quota(*p) - oq) for p in pts], chiuso=True)
        orti.unisci(rec)
        for rr in range(int(h / 1.2) - 1):
            yy = y0 + 1.0 + rr * 1.2
            a, b = L(x0 + 1, yy), L(x0 + w - 1, yy)
            za, zb = t.quota(*a) - oq, t.quota(*b) - oq
            orti.trave("Terra", [a[0] - ox_, a[1] - oy_, za], [b[0] - ox_, b[1] - oy_, zb], 50, 22)
            if r3.random() < 0.8:
                for k in range(int((w - 2) / 0.6)):
                    c = (a[0] - ox_ + (b[0] - a[0]) * (k + 0.5) / ((w - 2) / 0.6), a[1] - oy_, za + (zb - za) * (k + 0.5) / ((w - 2) / 0.6) + 12)
                    orti.cilindro("Verde", c, r3.uniform(12, 20), r3.uniform(14, 26), lati=6, raggio_alto=r3.uniform(4, 10))
        pr.disegni.append((S, "linea", pts + [pts[0]], "Orto"))
    pr.al_suolo(S, "Orti", orti, ox_, oy_, oq, collisione=True)

    # lotti vuoti: quattro paletti con la corda (per le case dei giocatori)
    lm = Mesh("SM_Villaggio_Lotti")
    for (x, y) in lotti:
        pts = [L(x - 6, y - 5), L(x + 6, y - 5), L(x + 6, y + 5), L(x - 6, y + 5)]
        for k in range(4):
            a, b = pts[k], pts[(k + 1) % 4]
            for p in (a,):
                lm.cilindro("Trave", [p[0] - P0x, p[1] - P0y, t.quota(*p) - oz - 30], 6, 120, lati=5)
            lm.trave("Stoffa", [a[0] - P0x, a[1] - P0y, t.quota(*a) - oz + 80], [b[0] - P0x, b[1] - P0y, t.quota(*b) - oz + 80], 2, 2)
        pr.disegni.append((S, "linea", pts + [pts[0]], "Lotto"))
    pr.al_suolo(S, "Lotti vuoti", lm, P0x, P0y, oz, collisione=False)

    # ---- fuori: (15) piazzale delle carovane, (16) stalle e fienili, fattorie a sud-est
    X, Y = L(132, -4)
    m = Mesh("SM_Villaggio_Carovane")
    lastricato(m, t, X, Y, t.quota(X, Y), X, Y, 1300, 900, mat="Terra", sfuma=300, alza=5, angoli=600)
    pr.zone_senza_erba.append((X, Y, 1300, 900))
    pr.al_suolo(S, "Piazzale delle carovane", m, X, Y, t.quota(X, Y))
    for k in range(8):
        x, y = 132 + rng.uniform(-11, 11), -4 + rng.uniform(-7, 7)
        pr.oggetto(S, "Carico %d" % k, ["/Game/Fab/Mercato_Tenda1/Crate_003", "/Game/Fab/Mercato_Tenda1/Crate_03_007",
                                          "/Game/Fab/Mercato_Tenda2/wooden_barrel", "/Game/Fab/Mercato_Tenda1/vegetable_box_01_005"][k % 4],
                   *L(x, y), rng.uniform(0, 360))
    pr.edificio(S, "Stalla_della_porta", lambda nome, W, D, **kw: ed.fienile(nome, W, D, kw["fondazione"]), *L(134, 44), -90.0, 1400, 800)
    pas = Mesh("SM_Villaggio_Recinti")
    rq = t.quota(*L(134, 44))
    rx, ry = L(134, 44)
    for poly in ([(124, 50), (150, 50), (150, 64), (118, 64), (118, 50)], [(150, 60), (190, 60), (195, 100), (150, 104), (146, 76)]):
        pts = [L(*p) for p in poly]
        pas.unisci(ed.recinto("x", [(p[0] - rx, p[1] - ry, t.quota(*p) - rq) for p in pts]))
        pr.disegni.append((S, "linea", pts, "Recinto"))
    for (x, y) in ((140, 58), (128, 58)):
        p = L(x, y)
        ed.covone(pas, p[0] - rx, p[1] - ry, t.quota(*p) - rq)
    pr.al_suolo(S, "Recinti e covoni", pas, rx, ry, rq, collisione=True)
    pr.fab(S, "Fattoria di sud-est", "/Game/Fab/Casa_Camino/BP_Casa_Camino", *L(165, 52), 90.0, 714, 1253)
    pr.edificio(S, "Fienile_fattoria", lambda nome, W, D, **kw: ed.fienile(nome, W, D, kw["fondazione"]), *L(170, 80), 180.0, 1200, 700)
    casa("Fattoria_Est", 178, 18, 180, 11, 8, piani=1, base="Pietra", sopra="Intonaco", tetto="Paglia", pendenza=45)
    pr.edificio(S, "Fienile_Est", lambda nome, W, D, **kw: ed.fienile(nome, W, D, kw["fondazione"]), *L(180, 34), 180.0, 1000, 600)

    # ---- la palizzata: dal fiume a nord, intorno, fino al fiume a sud-ovest; quattro porte
    # (05/11) le porte stanno dove passano le strade dipinte: a nord la strada del ponte, a est le due strade
    # (grotta e passo) che partono dalla fine della via maestra, a sud il viottolo del cimitero.
    # (06/11) visto da Fra in Play: i due capi a ovest si fermavano a metà della sponda e lasciavano un varco.
    # Ora scendono fino all'acqua: a nord fino a ~1,5 m sotto il pelo del fiume (-75.3, -152.6); a sud-ovest passano
    # a valle della capanna e del pontile di Orlando (che restano dentro, a ~20 m) e finiscono nel fiume (-151.8, 53.2).
    # Misurati sulle altezze vere (Altezze_3000.png) e sul fiume di Acqua_3000.json; nessuna strada dipinta attraversata.
    # (06/11, visto da Fra in Play) la palizzata passava dentro la torre della milizia (centro (114, -18.2), lato 5,5 m
    # più lo zoccolo): dopo la porta est di nord-est ripartiva da (114, -22.0). Ora riparte dall'altro lato della torre
    # (114, -15.2): la torre chiude lei il varco tra il portale e la palizzata.
    giro = [(-75.3, -152.6), (-57, -150), (-16.0, -155.0), (-9.2, -155.8), (40, -160), (100, -122), (114, -60), (114, -28.6), (114, -15.2),
            (114, 13.8), (114, 20.3), (114, 60),
            (82, 106), (11.3, 117), (4.7, 117), (-30, 112), (-70, 95), (-104, 62), (-140.6, 61.6), (-151.8, 53.2)]
    porte = [((-16.0, -155.0), (-9.2, -155.8)), ((114, -28.6), (114, -22.0)), ((114, 13.8), (114, 20.3)), ((11.3, 117), (4.7, 117))]
    pezzi = [[]]
    for p in giro:
        pezzi[-1].append(p)
        if any(p == g[0] for g in porte):
            pezzi.append([])
        elif any(p == g[1] for g in porte):
            pezzi[-1] = [p]
    pezzi = [pz for pz in pezzi if len(pz) >= 2]
    # (06/11) la palizzata di oggi e i suoi portali ci sono solo nella fase 1: dalla fase 2 c'è quella più larga
    pr.fase = (1, 1)
    for k, pz in enumerate(pezzi):
        A = L(*pz[0])
        qa = t.quota(*A)
        pts = [(L(*p)[0] - A[0], L(*p)[1] - A[1], t.quota(*L(*p)) - qa) for p in pz]
        # punti fitti, così i pali seguono il terreno
        fitti = []
        for a, b in zip(pts[:-1], pts[1:]):
            n = max(1, int(math.hypot(b[0] - a[0], b[1] - a[1]) / 300))
            for j in range(n):
                xx = a[0] + (b[0] - a[0]) * j / n
                yy = a[1] + (b[1] - a[1]) * j / n
                fitti.append((xx, yy, t.quota(A[0] + xx, A[1] + yy) - qa))
        fitti.append(pts[-1])
        pr.al_suolo(S, "Palizzata %d" % (k + 1), ed.palizzata("SM_Villaggio_Palizzata_%d" % (k + 1), fitti, seme=k + 3), A[0], A[1], qa, collisione=True)
        pr.disegni.append((S, "palizzata", [L(*p) for p in pz], ""))
    # i portali sulle porte
    for k, (a, b) in enumerate(porte):
        A, B = np.array(L(*a)), np.array(L(*b))
        c = (A + B) / 2
        largo = np.linalg.norm(B - A)
        g = math.degrees(math.atan2(B[1] - A[1], B[0] - A[0]))
        m = Mesh("SM_Villaggio_Portale_%d" % (k + 1))
        for s in (-1, 1):
            m.cilindro("Trave", [s * (largo / 2 + 10), 0, -80], 26, 560, lati=8)
        m.trave("Trave", [-largo / 2 - 60, 0, 470], [largo / 2 + 60, 0, 470], 30, 30)
        m.trave("Trave", [-largo / 2 - 40, 0, 420], [largo / 2 + 40, 0, 420], 18, 18)
        ed.tetto_casa(m, largo + 120, 120, 490, "Coppi", 30, sporto=30, gronda=10, gable="Assi")
        # i battenti aperti contro la palizzata
        for s in (-1, 1):
            m.scatola("Assi", [s * (largo / 2 + 10), -largo / 4 - 30, 210], [3 * s, 0, 0], [0, largo / 4, 0], [0, 0, 200])
        pr.metti_mesh(S, "Portale %d" % (k + 1), m, c[0], c[1], max(t.quota(*A), t.quota(*B)), g)
    pr.fase = (1, 0)

    # ---- le strade dentro il villaggio (terra battuta; quelle grandi sono già dipinte sul paesaggio)
    strade = [
        ([(22, 1), (60, 1.5), (80, 0), (91, -4)], 600),                         # via maestra, fino al bivio delle strade dipinte
        ([(-2, -18), (-14, -24), (-14, -50), (-3, -66)], 400),                   # dalla piazza alla strada del ponte
        ([(30, -1), (30, -4)], 300),
        ([(56, -2), (56, -40), (56, -95), (-5, -95)], 400),                        # vicolo nord-est
        ([(8, 18), (8, 60), (8, 117), (8, 130), (-30, 152), (-120, 195)], 400),             # verso sud, porta sud e cimitero
        ([(-22, -12), (-45, -14), (-70, -14), (-90.5, -13.1)], 400),             # fino alla strada dipinta del mulino
        ([(-20, 18), (-28, 22), (-40, 40), (-46, 52)], 350),                      # artigiani
        ([(8, 50), (30, 50)], 300),
        ([(128, 22.5), (150, 40), (165, 50)], 350),                              # fattorie, dalla strada del passo
    ]
    sm = Mesh("SM_Villaggio_Strade")
    for punti, larga in strade:
        striscia(sm, t, P0x, P0y, oz, [L(*p) for p in punti], larga)
        pr.disegni.append((S, "strada", [L(*p) for p in punti], larga))
    pr.al_suolo(S, "Strade del villaggio", sm, P0x, P0y, oz)

    # il ponte a nord (sulla strada del ponte)
    ponte_su_strada(pr, S, "Ponte a nord", "SM_Villaggio_PonteNord", *L(-42.9, -294.1), (0.416, 0.909), 450)

    # (06/11) le fasi future: fase 2 (anello di case, palizzata più larga) e fase 3 (il muro di pietra)
    villaggio_fasi.fasi_future(pr, S, L, giro, strade, striscia, lastricato)
    return [L(x, y) for (x, y) in lotti]


# --------------------------------------------------------------------------------------------------- il posto di guardia

def guardia(pr, rng):
    t = pr.t
    G = t.luogo("Posto di guardia del passo")
    gx, gy = G[0], G[1]
    S = "Guardia"
    d = np.array([-0.80105, -0.59860])        # la strada, verso la valle (misurata sullo strato Strade)
    n = np.array([-d[1], d[0]])               # di traverso al passo
    gq = t.quota(gx, gy)

    def P(along, across):
        """along: metri verso la valle (+) o verso il passo (-); across: metri di traverso."""
        return gx + (d[0] * along + n[0] * across) * 100, gy + (d[1] * along + n[1] * across) * 100

    gr_traverso = math.degrees(math.atan2(n[1], n[0]))
    # la palizzata di traverso, dalle pareti alla porta e dalla porta alle pareti
    for k, (a, b) in enumerate(((-30, -3), (3, 30))):
        A = P(0, a)
        qa = t.quota(*A)
        pts = []
        for s in np.linspace(a, b, int(abs(b - a) / 2.5) + 1):
            p = P(0, s)
            pts.append((p[0] - A[0], p[1] - A[1], t.quota(*p) - qa))
        pr.al_suolo(S, "Palizzata del passo %d" % (k + 1), ed.palizzata("SM_Guardia_Palizzata_%d" % (k + 1), pts, altezza=450, seme=40 + k), A[0], A[1], qa, collisione=True)
        pr.disegni.append((S, "palizzata", [P(0, a), P(0, b)], ""))
    # il portale e la sbarra
    m = Mesh("SM_Guardia_Portale")
    largo = 600.0
    for s in (-1, 1):
        m.cilindro("Trave", [s * (largo / 2 + 10), 0, -80], 30, 640, lati=8)
    m.trave("Trave", [-largo / 2 - 70, 0, 540], [largo / 2 + 70, 0, 540], 34, 34)
    ed.tetto_casa(m, largo + 160, 140, 560, "Coppi", 30, sporto=30, gronda=10, gable="Assi")
    # battenti aperti
    for s in (-1, 1):
        m.scatola("Assi", [s * (largo / 2 + 10), largo / 4 + 30, 220], [3 * s, 0, 0], [0, largo / 4, 0], [0, 0, 215])
    # sbarra (verso la valle, 3 m davanti): due cavalletti, il palo a strisce
    for s in (-1, 1):
        m.blocco("Trave", s * (largo / 2 + 40) - 12, s * (largo / 2 + 40) + 12, -300 - 12, -300 + 12, -40, 110)
    m.cilindro("Stoffa", [-largo / 2 - 60, -300, 100], 9, largo + 120, lati=8, asse=(1, 0, 0))
    m.blocco("Pietra", -largo / 2 - 90, -largo / 2 - 30, -330, -270, 80, 140)    # contrappeso
    pr.metti_mesh(S, "Porta del passo", m, *P(0, 0), gq, gr_traverso)
    # (06/11) il sigillo della Corona: il velo di rune 6 m dietro la sbarra, verso il passo, da parete a parete.
    # L'attore (AValdorsoSigillo) guarda verso la valle: la sua X è la strada che scende, la Y è di traverso.
    vm, dv = sigillo.velo(t, P, -6.0, gq)
    pr.mesh(vm)
    X, Y = P(-6.0, 0.0)
    pr.aggiungi(S, "Sigillo della Corona", "sigillo", X, Y, gq, math.degrees(math.atan2(d[1], d[0])), asset=vm.nome,
                suolo=True, **dv)
    pr.disegni.append((S, "linea", [P(-6.0, dv["traverso_da"] / 100.0), P(-6.0, dv["traverso_a"] / 100.0)], "Sigillo"))
    print("sigillo: largo %.0f m, alto %.0f m" % (dv["larghezza"] / 100, dv["altezza"] / 100))
    # la torre, dalla parte della valle, a sinistra della strada
    X, Y = P(5, -10)
    pr.edificio(S, "Torre_del_passo", lambda nome, W, D, **kw: ed.torre(nome, lato=W, altezza=1400, fondazione=kw["fondazione"]),
                X, Y, math.degrees(math.atan2(n[1], n[0])), 600, 600, chiave=False)
    # la casermetta, dall'altra parte, con la facciata verso la strada
    X, Y = P(14, 9.5)
    pr.edificio(S, "Casermetta_della_Guardia", ed.casa, X, Y, math.degrees(math.atan2(-n[1], -n[0])), 1100, 700, chiave=False,
                piani=1, base="Pietra", sopra="Intonaco", tetto="Coppi", pendenza=35)
    # dentro la casermetta: due letti
    # (letti messi accanto alla casa, dentro: coordinate locali)
    a = math.radians(math.degrees(math.atan2(-n[1], -n[0])) + 90)
    for k, lx in enumerate((-350, 350)):
        px, py = pr.al_mondo(X, Y, math.degrees(a), lx, 150)
        pr.aggiungi(S, "Branda della Guardia %d" % k, "letto", px, py, t.sotto(X, Y, 1140, 740, math.degrees(a))[1] + 15, math.degrees(a))
    # rastrelliera, fuoco, botti, stendardo
    m = Mesh("SM_Guardia_Campo")
    for s in range(5):
        m.trave("Trave", [-200 + s * 100, 0, 0], [-200 + s * 100, 30, 190], 6, 6)
    m.trave("Trave", [-250, 0, 150], [250, 0, 150], 8, 8)
    m.cilindro("Trave", [600, 0, -60], 9, 760, lati=6)
    m.scatola("Stoffa", [600, 90, 600], [1.5, 0, 0], [0, 85, 0], [0, 0, 110])
    for k in range(8):
        a2 = 2 * math.pi * k / 8
        m.scatola("Pietra", [math.cos(a2) * 70 - 300, 350 + math.sin(a2) * 70, 10], [15, 0, 0], [0, 15, 0], [0, 0, 12])
    m.blocco("Fiamma", -330, -270, 320, 380, 0, 30)
    X, Y = P(14, -2)
    pr.metti_mesh(S, "Campo della Guardia", m, X, Y, t.quota(X, Y), gr_traverso)
    lx, ly = pr.al_mondo(X, Y, gr_traverso, -300, 350)
    pr.aggiungi(S, "Luce del fuoco della Guardia", "luce", lx, ly, t.quota(lx, ly) + 80, intensita=8.0, raggio=900.0, colore=[1.0, 0.5, 0.2])
    for k in range(5):
        x, y = P(rng.uniform(8, 18), rng.uniform(4, 8))
        pr.oggetto(S, "Botte della Guardia %d" % k, ["/Game/Fab/Mercato_Tenda2/wooden_barrel", "/Game/Fab/Mercato_Tenda1/Crate_003"][k % 2], x, y, rng.uniform(0, 360))
    pr.aggiungi(S, "Panca della Guardia", "panca", *P(16, -5), t.quota(*P(16, -5)), gr_traverso + 90)
    pr.aggiungi(S, "Bacheca del passo", "bacheca", *P(6, 4), t.quota(*P(6, 4)), gr_traverso,
                titolo="Guardia della Corona - Posto del passo",
                avvisi=["Per ordine della capitana Livia: chi passa dichiari nome e merci.", "Il passo per Aurelia è aperto dall'alba al tramonto."])
    lan = Mesh("SM_Guardia_Lanterne")
    luci = []
    for (al, ac) in ((-2, -4.5), (-2, 4.5), (10, 6)):
        X, Y = P(al, ac)
        luci.append(ed.lanterna(lan, X - gx, Y - gy, t.quota(X, Y) - gq))
    pr.al_suolo(S, "Lanterne del passo", lan, gx, gy, gq, collisione=True)
    for k, (lx, ly, lz) in enumerate(luci):
        pr.aggiungi(S, "Luce lanterna del passo %d" % k, "luce", gx + lx, gy + ly, gq + lz, intensita=4.0, raggio=1000.0)


# --------------------------------------------------------------------------------------------------- prova degli assi

def prova_assi(pr):
    """Una mesh storta apposta: crea_villaggio.py guarda come l'ha importata Unreal (assi scambiati o specchiati)."""
    m = Mesh("SM_Villaggio_ProvaAssi")
    m.blocco("Pietra", 0, 100, -10, 10, -10, 10)       # +X fino a 100
    m.blocco("Pietra", -10, 10, 0, 300, -10, 10)       # +Y fino a 300
    m.blocco("Pietra", -10, 10, -10, 10, 0, 50)        # +Z fino a 50
    pr.mesh(m)


# --------------------------------------------------------------------------------------------------- anteprime

def anteprima(pr, sezione, cx, cy, mezzo, file, px=1400, fase=1):
    """La pianta di una zona, con le cose che ci sono nella fase indicata (06/11: di serie la fase 1, quella di oggi)."""
    t = pr.t
    S = t.S
    x0, y0 = cx - mezzo, cy - mezzo
    c0, r0 = int(x0 / S), int(y0 / S)
    n = int(2 * mezzo / S)
    h = t.h[r0:r0 + n, c0:c0 + n]
    gy, gx = np.gradient(h)
    sh = np.clip(0.8 + (-gx - gy) / S * 1.2, 0, 1)
    rgb = np.stack([sh * 225, sh * 215, sh * 175], -1)
    im = Image.fromarray(rgb.astype(np.uint8)).resize((px, px))
    dr = ImageDraw.Draw(im)

    def f(x, y):
        return ((x - x0) / (2 * mezzo) * px, (y - y0) / (2 * mezzo) * px)

    pts = [f(p[0], p[1]) for p in t.fiume]
    dr.line(pts, fill=(70, 100, 200), width=5)
    scala = px / (2 * mezzo)
    for (sez, tipo, punti, nome), fd in zip(pr.disegni, pr.disegni.fasi):
        if not nella_fase(fd, fase):
            continue
        nuova = fd[0] == fase and fase > 1          # le cose nuove di questa fase, in blu
        if (tipo == "edificio" or tipo == "fab") and nuova:
            dr.polygon([f(*p) for p in punti], fill=(70, 110, 190), outline=(20, 30, 80))
            continue
        if tipo == "muro":
            dr.line([f(*p) for p in punti], fill=(110, 110, 120), width=7)
            continue
        if tipo == "edificio" or tipo == "fab":
            dr.polygon([f(*p) for p in punti], fill=(150, 80, 60) if tipo == "edificio" else (120, 60, 120), outline=(40, 20, 10))
            c = np.mean(punti, 0)
            dr.text(f(*c), nome.replace("_", " ")[:18], fill=(0, 0, 0))
        elif tipo == "strada":
            dr.line([f(*p) for p in punti], fill=(160, 120, 80), width=max(1, int(nome * scala)))
        elif tipo == "palizzata":
            dr.line([f(*p) for p in punti], fill=(70, 40, 20), width=3)
        elif tipo == "linea":
            dr.line([f(*p) for p in punti], fill=(90, 60, 30), width=2)
        elif tipo == "canneto":
            X, Y = f(*punti[0])
            r = nome * scala
            dr.ellipse((X - r, Y - r, X + r, Y + r), outline=(60, 110, 50), fill=(110, 150, 80))
        elif tipo == "cerchio":
            X, Y = f(*punti[0])
            r = nome * scala
            dr.ellipse((X - r, Y - r, X + r, Y + r), outline=(40, 40, 120))
    for c in pr.cose:
        if not nella_fase(c.get("fase", (1, 0)), fase):
            continue
        X, Y = f(c["pos"][0], c["pos"][1])
        if c["tipo"] == "porta":
            dr.rectangle((X - 3, Y - 3, X + 3, Y + 3), fill=(0, 160, 0) if not c.get("a_chiave") else (200, 0, 0))
        elif c["tipo"] == "luce":
            dr.ellipse((X - 3, Y - 3, X + 3, Y + 3), fill=(255, 200, 0))
        elif c["tipo"] in ("panca", "letto", "bacheca"):
            dr.ellipse((X - 3, Y - 3, X + 3, Y + 3), fill=(0, 0, 200))
    dr.line([(20, px - 20), (20 + 50 * 100 * scala, px - 20)], fill=(0, 0, 0), width=3)
    dr.text((20, px - 40), "50 m", fill=(0, 0, 0))
    if fase > 1:
        dr.text((20, 20), "Fase %d (in blu le cose nuove)" % fase, fill=(0, 0, 0))
    im.save(file)


def senza_erba(pr, file, lato_px=4096):
    """(05/11) T_SenzaErba.png: dove il paesaggio NON deve seminare l'erba vera (strade, piazza, sotto le case, orti).
    La legge crea_materiale_valle.py (se c'è) e spegne lì l'erba: prima spuntava dal selciato."""
    lato_cm = float(pr.t.luoghi["lato_m"]) * 100.0
    k = lato_px / lato_cm
    im = Image.new("L", (lato_px, lato_px), 0)
    dr = ImageDraw.Draw(im)
    for sez, tipo, punti, nome in pr.disegni:
        if tipo in ("edificio", "fab"):
            c = np.mean(punti, 0)
            grandi = [(c[0] + (p[0] - c[0]) * 1.08, c[1] + (p[1] - c[1]) * 1.08) for p in punti]
            dr.polygon([(x * k, y * k) for x, y in grandi], fill=255)
        elif tipo == "strada":
            dr.line([(x * k, y * k) for x, y in punti], fill=255, width=max(1, int(nome * 0.8 * k)))
        elif tipo == "linea" and nome in ("Orto",):
            dr.polygon([(x * k, y * k) for x, y in punti], fill=255)
    for (cx, cy, mx, my) in pr.zone_senza_erba:
        dr.rectangle(((cx - mx) * k, (cy - my) * k, (cx + mx) * k, (cy + my) * k), fill=255)
    from PIL import ImageFilter
    im = im.filter(ImageFilter.GaussianBlur(1.2))
    im.save(file)


def main(cartella_valle, uscita):
    os.makedirs(uscita, exist_ok=True)
    t = Terreno(cartella_valle)
    pr = Progetto(t, uscita)
    villaggio_texture.scrivi_tutte(uscita)
    sigillo.file_sigillo(uscita)
    rng = np.random.default_rng(2611)
    prova_assi(pr)
    palude(pr, rng)
    villaggio(pr, rng)
    guardia(pr, rng)
    senza_erba(pr, os.path.join(uscita, "T_SenzaErba.png"))
    dati = {"nota": "Fatto da Tools/Generatori/villaggio.py: cm dall'angolo nord-ovest del paesaggio, Z = quota vera in cm.",
            "cose": pr.cose, "mesh": sorted(pr.mesh_scritte), "avvisi": pr.avvisi}
    with open(os.path.join(uscita, "Villaggio_3000.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)
    # (06/11) la pianta (case, strade, cinte) per la mappa di carta: la disegna mappa.py
    pianta = [{"sezione": sez, "tipo": tipo, "punti": [[round(float(p[0]), 1), round(float(p[1]), 1)] for p in punti],
               "nome": nome if isinstance(nome, str) else "", "larga": nome if isinstance(nome, (int, float)) else 0,
               "fase": list(fd)} for (sez, tipo, punti, nome), fd in zip(pr.disegni, pr.disegni.fasi) if tipo != "cerchio"]
    with open(os.path.join(uscita, "Pianta_3000.json"), "w", encoding="utf-8") as f:
        json.dump(pianta, f, ensure_ascii=False)
    P0 = t.luogo("Piazza del villaggio")
    anteprima(pr, "Villaggio", P0[0] + 2000, P0[1], 26000, os.path.join(uscita, "Anteprima_Villaggio.png"))
    for fase in (2, 3):
        anteprima(pr, "Villaggio", P0[0] + 2000, P0[1], 26000, os.path.join(uscita, "Anteprima_Villaggio_Fase%d.png" % fase), fase=fase)
    pa = t.luogo("Palude")
    anteprima(pr, "Palude", pa[0] + 2000, pa[1] - 800, 10000, os.path.join(uscita, "Anteprima_Palude.png"))
    g = t.luogo("Posto di guardia del passo")
    anteprima(pr, "Guardia", g[0], g[1], 5000, os.path.join(uscita, "Anteprima_Guardia.png"))
    conta = {}
    for c in pr.cose:
        k = tuple(c.get("fase", (1, 0)))
        conta[k] = conta.get(k, 0) + 1
    print("fasi: " + ", ".join("%s: %d" % (("dalla %d" % a) if b == 0 else ("%d-%d" % (a, b)), n) for (a, b), n in sorted(conta.items())))
    print("fatto: %d cose, %d mesh, %d avvisi" % (len(pr.cose), len(pr.mesh_scritte), len(pr.avvisi)))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
