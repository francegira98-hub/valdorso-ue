# Valdorso - Dentro la grotta del nord: le gallerie dei nani, gli scheletri, il bottino (v0.1.7). Scritto da Claude
# l'11/11/2026. Dal Punto di ripresa del 10/11 ("gallerie e sale fatte con il generatore come la grotta est") e dalla scelta
# di Fra del 10/11 ("generata da Claude").
#
# Le gallerie stanno SOTTO la montagna del nord (il paesaggio non si buca): si entra dal portale (AValdorsoFrana, con E)
# e si arriva qui, sessanta metri sotto il luogo "Grotta nord". Tutto e' fatto dal punto d'arrivo (0, 0, 0): x verso est,
# y verso sud, z in su, in cm (come Unreal). Dal sud verso il nord, scendendo:
#   1. La galleria della frana (30 m, scende di 3 m): squadrata dai nani, con le travi della spedizione del 309, e dietro
#      l'arrivo il retro della frana (sassi fino al soffitto).
#   2. La sala delle colonne (20 x 16 m, alta 7,5): sei colonne con i segni dei nani ("non sono nomi: sono un conto"),
#      tre scheletri addormentati.
#   3. A ovest, il campo della spedizione: giacigli, casse, il fuoco spento, il taccuino di Gisella.
#   4. A nord, la scala dei nani (12 m, scende di 6).
#   5. La sala delle ossa (rotonda, 20 m): mucchi d'ossa, sarcofagi, il capitano e due scheletri con lo scudo, il forziere
#      dei nani, la lanterna di Cecco ai piedi della scala; in fondo la porta dei dormienti, chiusa (v0.6), con i martelli.
# E i pezzi degli scheletri (teschio, torace, bacino, braccio, gamba, spada, scudo) e tre suoni (la frana, lo scheletro
# che si sveglia, le ossa che crollano).
#
# Scrive in una cartella (di solito V:/Texture/valdorso/grotta_nord):
#   SM_*.glb          i pezzi (materiali MV_<nome>: quelli del villaggio, piu' "Ossa")
#   S_*.wav           i suoni nuovi
#   GrottaNord.json   dove va ogni cosa (dal punto d'arrivo): lo legge Content/Python/nuovi/crea_grotta_nord.py
#   Anteprima_GrottaNord.png   la pianta, per controllare
# Uso: python grotta_nord.py <uscita>     (sempre uguale: semi fissi; non servono le altezze della valle)

import json
import math
import os
import sys
import wave

import numpy as np
from PIL import Image, ImageDraw

import villaggio_mesh
from villaggio_mesh import Mesh, normalizza
from villaggio_grotta import _sasso

# le ossa: piastrelle piccole (il materiale e' un colore, MI_Grotta_Ossa)
villaggio_mesh.TILE.setdefault("Ossa", 50.0)

GIU = 6000.0                     # quanto sotto il luogo "Grotta nord" sta l'arrivo (cm)
LARGO_GALLERIA = 420.0
ALTO_GALLERIA = 420.0
SPESSORE = 60.0


class Grotta:
    def __init__(self, uscita):
        self.uscita = uscita
        self.cose = []
        self.scritte = set()
        self.pianta = []            # (tipo, punti) in cm, per l'anteprima

    def mesh(self, m):
        if m.nome not in self.scritte:
            m.scrivi(os.path.join(self.uscita, m.nome + ".glb"))
            self.scritte.add(m.nome)
        return m.nome

    def metti(self, nome, m, x=0.0, y=0.0, z=0.0, gradi=0.0, collisione=True):
        self.cose.append({"tipo": "mesh", "nome": nome, "asset": self.mesh(m), "pos": [round(x, 1), round(y, 1), round(z, 1)],
                          "gradi": round(gradi, 2), "collisione": bool(collisione)})

    def aggiungi(self, tipo, nome, x, y, z, gradi=0.0, **altro):
        d = {"tipo": tipo, "nome": nome, "pos": [round(float(x), 1), round(float(y), 1), round(float(z), 1)], "gradi": round(float(gradi), 2)}
        d.update(altro)
        self.cose.append(d)

    def luce(self, nome, x, y, z, colore=(1.0, 0.55, 0.25), intensita=6.0, raggio=900.0):
        self.aggiungi("luce", nome, x, y, z, colore=list(colore), intensita=intensita, raggio=raggio)


# --------------------------------------------------------------------------------------------------- i mattoni

def pareti_dritte(m, a, b, za, zb, largo, alto, mat_muro="Pietra", mat_volta="Roccia", mat_terra="Pietra", passo=200.0, chiudi=(),
                  terra=True):
    """Una galleria dritta da a a b (x, y in cm) con il pavimento da za a zb: pavimento, due pareti e il soffitto, a pezzi
    di 'passo' (cosi' segue la pendenza). chiudi: 'inizio' e/o 'fine' per murare un capo."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    d = b - a
    L = float(np.linalg.norm(d))
    u = d / L
    n = np.array([-u[1], u[0]])
    k = max(1, int(math.ceil(L / passo)))
    h = largo / 2.0
    for i in range(k):
        t0, t1 = i / k, (i + 1) / k
        p0, p1 = a + d * t0, a + d * t1
        z0, z1 = za + (zb - za) * t0, za + (zb - za) * t1
        sx0, dx0 = p0 + n * h, p0 - n * h
        sx1, dx1 = p1 + n * h, p1 - n * h
        P = lambda q, z: [q[0], q[1], z]  # noqa: E731
        # il pavimento (se c'e': sopra la botola no) e il soffitto
        if terra:
            m.quad(mat_terra, P(sx0, z0), P(dx0, z0), P(dx1, z1), P(sx1, z1), normale=normalizza(np.array([0.0, 0.0, 1.0])))
        m.quad(mat_volta, P(sx0, z0 + alto), P(sx1, z1 + alto), P(dx1, z1 + alto), P(dx0, z0 + alto), normale=np.array([0.0, 0.0, -1.0]))
        # le pareti (verso dentro)
        m.quad(mat_muro, P(sx0, z0), P(sx1, z1), P(sx1, z1 + alto), P(sx0, z0 + alto), normale=np.array([-n[0], -n[1], 0.0]))
        m.quad(mat_muro, P(dx0, z0), P(dx0, z0 + alto), P(dx1, z1 + alto), P(dx1, z1), normale=np.array([n[0], n[1], 0.0]))
    if "inizio" in chiudi:
        sx, dx = a + n * h, a - n * h
        m.quad(mat_muro, [sx[0], sx[1], za], [sx[0], sx[1], za + alto], [dx[0], dx[1], za + alto], [dx[0], dx[1], za],
               normale=np.array([u[0], u[1], 0.0]))
    if "fine" in chiudi:
        sx, dx = b + n * h, b - n * h
        m.quad(mat_muro, [sx[0], sx[1], zb], [dx[0], dx[1], zb], [dx[0], dx[1], zb + alto], [sx[0], sx[1], zb + alto],
               normale=np.array([-u[0], -u[1], 0.0]))


def sala(m, x0, x1, y0, y1, z, alto, buchi_s=(), buchi_n=(), buchi_o=(), buchi_e=(), mat_terra="Pietra", mat_muro="Pietra",
         mat_volta="Roccia"):
    """Una sala rettangolare (pavimento, soffitto, quattro muri con i buchi (u0, u1, z0, z1) per le gallerie)."""
    m.lastra(mat_terra, x0 - SPESSORE, x1 + SPESSORE, y0 - SPESSORE, y1 + SPESSORE, z - SPESSORE, z)
    m.lastra(mat_volta, x0 - SPESSORE, x1 + SPESSORE, y0 - SPESSORE, y1 + SPESSORE, z + alto, z + alto + SPESSORE)
    # muro sud (y1), da ovest verso est; nord (y0); ovest (x0), da nord verso sud; est (x1)
    m.muro(mat_muro, [x0, y1 + SPESSORE / 2, z], 0.0, x1 - x0, alto, SPESSORE, buchi=buchi_s)
    m.muro(mat_muro, [x0, y0 - SPESSORE / 2, z], 0.0, x1 - x0, alto, SPESSORE, buchi=buchi_n)
    m.muro(mat_muro, [x0 - SPESSORE / 2, y0, z], 90.0, y1 - y0, alto, SPESSORE, buchi=buchi_o)
    m.muro(mat_muro, [x1 + SPESSORE / 2, y0, z], 90.0, y1 - y0, alto, SPESSORE, buchi=buchi_e)


def segni_dei_nani(m, centro, avanti, quanti, seme):
    """I segni squadrati dei nani (tratti di ferro) su una faccia: 'avanti' e' la normale della faccia."""
    r = np.random.default_rng(abs(int(seme)))
    av = normalizza(np.asarray(avanti, float))
    lato = normalizza(np.cross([0.0, 0.0, 1.0], av))
    c0 = np.asarray(centro, float)
    for k in range(quanti):
        c = c0 + lato * (k - quanti / 2.0) * 26.0 + av * 2.0
        for _ in range(3):
            a = c + lato * r.uniform(-8, 8) + np.array([0, 0, r.uniform(-14, 10)])
            b = a + (lato if r.random() < 0.5 else np.array([0, 0, 1.0])) * r.choice([-14.0, 14.0])
            m.trave("Ferro", a, b, 3.0, 3.0, su=av)


def torcia():
    """Una torcia da muro: il bastone, il ferro, la fiamma (la luce la mette lo script)."""
    m = Mesh("SM_GrottaNord_Torcia")
    m.trave("Trave", [0, 0, 0], [18, 0, 32], 6.0, 6.0)
    m.cilindro("Ferro", [18, 0, 30], 6.0, 10.0, lati=8)
    m.cilindro("Fiamma", [18, 0, 40], 6.0, 22.0, lati=8, raggio_alto=1.0)
    m.blocco("Ferro", -4, 4, -10, 10, -6, 6)
    return m


def giaciglio(m, x, y, z, gradi, colore_stoffa="Stoffa"):
    a = math.radians(gradi)
    u = np.array([math.cos(a), math.sin(a), 0.0])
    v = np.array([-u[1], u[0], 0.0])
    m.scatola(colore_stoffa, [x, y, z + 7], u * 95, v * 38, [0, 0, 7])
    m.scatola("Stoffa", [x, y, z + 18] + u * 70, u * 18, v * 30, [0, 0, 8])


def cassa(m, x, y, z, gradi, l=90.0, p=60.0, h=60.0):
    a = math.radians(gradi)
    u = np.array([math.cos(a), math.sin(a), 0.0])
    v = np.array([-u[1], u[0], 0.0])
    m.scatola("Assi", [x, y, z + h / 2], u * l / 2, v * p / 2, [0, 0, h / 2])
    for s in (-0.38, 0.38):
        m.scatola("Ferro", np.array([x, y, z + h / 2]) + u * l * s, u * 3, v * (p / 2 + 1), [0, 0, h / 2 + 1])


def mucchio_ossa(m, x, y, z, rng, quante=14, largo=90.0):
    for _ in range(quante):
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(0, largo)
        cx, cy = x + math.cos(a) * d, y + math.sin(a) * d
        g = rng.uniform(0, 360)
        lung = rng.uniform(25, 48)
        b = math.radians(g)
        p0 = np.array([cx, cy, z + rng.uniform(2, 14)])
        p1 = p0 + np.array([math.cos(b) * lung, math.sin(b) * lung, rng.uniform(-6, 10)])
        m.trave("Ossa", p0, p1, 4.5, 4.5)
        for q in (p0, p1):
            m.cilindro("Ossa", q - np.array([0, 0, 3.0]), 4.0, 6.0, lati=6)
    # un teschio in cima
    m.cilindro("Ossa", [x, y, z + 14], 9.0, 14.0, lati=10, raggio_alto=6.0)


# --------------------------------------------------------------------------------------------------- le parti

def galleria_della_frana(g, rng):
    m = Mesh("SM_GrottaNord_Galleria")
    # da poco dietro l'arrivo (y +300) fino alla sala delle colonne (y -3000), scendendo di 3 m
    pareti_dritte(m, (0.0, 300.0), (0.0, -3000.0), 18.0, -300.0, LARGO_GALLERIA, ALTO_GALLERIA, chiudi=("inizio",))
    # il retro della frana: sassi fino al soffitto, dietro l'arrivo
    for k in range(26):
        x = rng.uniform(-190, 190)
        y = rng.uniform(220, 300)
        zz = rng.uniform(0, 330)
        _sasso(m, [x, y, zz], rng.uniform(45, 90), rng, 600 + k)
    # le travi della spedizione: ogni 4 metri due pali e l'architrave (qualcuna caduta)
    for k in range(7):
        y = -200.0 - 400.0 * k
        z = 18.0 + (-318.0) * ((300.0 - y) / 3300.0)
        if k == 4:
            m.trave("Trave", [-170, y, z + 5], [140, y - 60, z + 22], 16.0, 16.0)       # caduta di traverso
            continue
        for s in (-1, 1):
            m.trave("Trave", [s * 180, y, z], [s * 180, y, z + ALTO_GALLERIA - 25], 18.0, 18.0)
        m.trave("Trave", [-195, y, z + ALTO_GALLERIA - 30], [195, y, z + ALTO_GALLERIA - 30], 20.0, 20.0)
    # sassi caduti lungo la galleria
    for k in range(12):
        y = rng.uniform(-2800, -300)
        z = 18.0 + (-318.0) * ((300.0 - y) / 3300.0)
        _sasso(m, [rng.choice([-1, 1]) * rng.uniform(120, 190), y, z + 10], rng.uniform(18, 40), rng, 700 + k)
    g.metti("Galleria della frana", m)
    g.pianta.append(("galleria", [(-210, 300), (210, 300), (210, -3000), (-210, -3000)]))
    # le torce (spente quelle della spedizione; queste le hanno lasciate accese i nani? no: le accendono le rune)
    for k, y in enumerate((-500.0, -1400.0, -2300.0)):
        z = 18.0 + (-318.0) * ((300.0 - y) / 3300.0)
        lato = -1 if k % 2 == 0 else 1
        g.aggiungi("torcia", "Torcia della galleria %d" % (k + 1), lato * 200.0, y, z + 210.0, 0.0 if lato < 0 else 180.0)
        g.luce("Luce della galleria %d" % (k + 1), lato * 150.0, y, z + 250.0, intensita=5.0, raggio=1000.0)


def sala_delle_colonne(g, rng):
    m = Mesh("SM_GrottaNord_Colonne")
    segni = Mesh("SM_GrottaNord_Colonne_Segni")
    z = -300.0
    alto = 750.0
    x0, x1, y0, y1 = -1000.0, 1000.0, -4600.0, -3000.0
    sala(m, x0, x1, y0, y1, z, alto,
         buchi_s=[(1000.0 - 210.0, 1000.0 + 210.0, 0.0, ALTO_GALLERIA)],        # la galleria della frana (al centro)
         buchi_n=[(1000.0 - 250.0, 1000.0 + 250.0, 0.0, 450.0)],                # la scala dei nani
         buchi_o=[(600.0, 900.0, 0.0, 350.0)],                                    # il corridoio del campo (y -4000..-3700)
         buchi_e=[(600.0, 900.0, 0.0, 360.0)])                                    # (11/11) l'ala segreta, dietro la porta segreta
    for cx in (-500.0, 500.0):
        for cy in (-3400.0, -3800.0, -4200.0):
            m.cilindro("Pietra", [cx, cy, z], 75.0, alto, lati=12)
            m.blocco("Pietra", cx - 105, cx + 105, cy - 105, cy + 105, z, z + 40)
            m.blocco("Pietra", cx - 100, cx + 100, cy - 100, cy + 100, z + alto - 50, z + alto)
            for k, av in enumerate(((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0))):
                c = np.array([cx, cy, z + 190 + 40 * k]) + np.asarray(av, float) * 76.0
                segni_dei_nani(segni, c, av, 4, int(cx + cy) + k)
    # pietre cadute e un mucchio d'ossa nel mezzo
    for k in range(9):
        _sasso(m, [rng.uniform(-900, 900), rng.uniform(-4500, -3100), z + 10], rng.uniform(20, 55), rng, 800 + k)
    mucchio_ossa(m, 0.0, -3800.0, z, rng, 20, 120.0)
    g.metti("Sala delle colonne", m)
    g.metti("Segni dei nani sulle colonne", segni, collisione=False)
    g.pianta.append(("sala", [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]))
    for cx in (-500.0, 500.0):
        for cy in (-3400.0, -3800.0, -4200.0):
            g.pianta.append(("colonna", [(cx, cy)]))
    g.aggiungi("ossario", "Scheletri della sala delle colonne", 0.0, -3800.0, z + 20.0, scheletro="Scheletro", quanti=3, raggio=500.0, minuti=30.0)
    for k, (x, y, gr) in enumerate(((-980.0, -3300.0, 0.0), (980.0, -3300.0, 180.0), (-980.0, -4400.0, 0.0), (980.0, -4400.0, 180.0))):
        g.aggiungi("torcia", "Torcia delle colonne %d" % (k + 1), x, y, z + 230.0, gr)
        g.luce("Luce delle colonne %d" % (k + 1), x * 0.9, y, z + 270.0, intensita=6.0, raggio=1300.0)
    g.aggiungi("iscrizione", "I segni sulle colonne", 500.0 + 90.0, -3400.0, z, 0.0, titolo="I segni sulle colonne",
               avvisi=["Sulle sei colonne, a altezza d'uomo, file di segni squadrati, gli stessi dell'arco della grotta di est.",
                       "Non sono parole: si ripetono, a gruppi di cinque, e ogni colonna ne ha di piu' di quella prima.",
                       "Un conto. I nani contavano qualcosa, quaggiu'. Sull'ultima colonna il conto si ferma a meta' di un gruppo.",
                       "Guardando meglio, la meta' che manca c'e': incisa piccola sul muro a est, vicino al pavimento, tra la seconda e la terza colonna."])


def campo_della_spedizione(g, rng):
    m = Mesh("SM_GrottaNord_Campo")
    z = -300.0
    # il corridoio dalla sala delle colonne (x -1000) al campo (x -2200), al centro y -3850
    pareti_dritte(m, (-1030.0, -3850.0), (-2230.0, -3850.0), z, z, 300.0, 350.0, mat_volta="Roccia")
    x0, x1, y0, y1 = -3200.0, -2200.0, -4400.0, -3300.0
    sala(m, x0, x1, y0, y1, z, 420.0, mat_muro="Roccia", buchi_e=[(400.0, 700.0, 0.0, 350.0)],
         buchi_o=[(400.0, 700.0, 0.0, 350.0)])                                    # (11/11) verso le miniere allagate
    # i giacigli, le casse, il fuoco spento
    giaciglio(m, -2950.0, -3500.0, z, 10.0)
    giaciglio(m, -2950.0, -3700.0, z, -5.0)
    giaciglio(m, -2950.0, -3900.0, z, 3.0)
    cassa(m, -2400.0, -4250.0, z, 20.0)
    cassa(m, -2520.0, -4280.0, z, -10.0, 70.0, 50.0, 45.0)
    for k in range(10):
        a = 2 * math.pi * k / 10
        _sasso(m, [-2650.0 + math.cos(a) * 55, -3700.0 + math.sin(a) * 55, z + 4], 14.0, rng, 900 + k)
    m.cilindro("Ferro", [-2650.0, -3700.0, z], 45.0, 2.0, lati=16)               # la cenere fredda
    m.cilindro("Assi", [-2380.0, -3450.0, z], 30.0, 14.0, lati=12)                # il rotolo di corda
    m.trave("Trave", [-3150.0, -4350.0, z], [-3150.0, -4350.0, z + 260.0], 12.0, 12.0)   # un palo con il segno della Gilda
    m.scatola("Stoffa", [-3150.0, -4330.0, z + 220.0], [0, 18, 0], [0.5, 0, 0], [0, 0, 26])
    g.metti("Campo della spedizione", m)
    g.pianta.append(("sala", [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]))
    g.pianta.append(("galleria", [(-1000, -3700), (-2200, -3700), (-2200, -4000), (-1000, -4000)]))
    g.aggiungi("traccia", "Il taccuino di Gisella", -2960.0, -3880.0, z + 20.0, 30.0, oggetto="Taccuino_Gisella", asset="SM_GrottaNord_Taccuino")
    g.aggiungi("iscrizione", "Il campo della spedizione", -2650.0, -3550.0, z, 90.0, titolo="Il campo della spedizione",
               avvisi=["Tre giacigli, due casse della Gilda con il sigillo rosso, un fuoco spento da anni.",
                       "Sul palo, il fazzoletto con la lanterna della Gilda. Qualcuno l'ha legato stretto, come un segnale.",
                       "Le casse sono vuote. Le hanno aperte dall'interno, con calma."])
    g.luce("Luce del campo (dalla fessura del soffitto)", -2700.0, -3800.0, z + 380.0, colore=(0.55, 0.65, 0.9), intensita=3.0, raggio=900.0)


def scala_dei_nani(g, rng):
    m = Mesh("SM_GrottaNord_Scala")
    largo = 500.0
    y_in, y_fuori = -4600.0, -5950.0
    z_in, z_fuori = -300.0, -900.0
    gradini = 12
    for k in range(gradini):
        ya = y_in - (y_in - y_fuori) * k / gradini
        yb = y_in - (y_in - y_fuori) * (k + 1) / gradini
        zz = z_in - (z_in - z_fuori) * (k + 1) / gradini
        m.blocco("Pietra", -largo / 2, largo / 2, yb, ya, zz - 40.0, zz)
    # le pareti e la volta, in pendenza
    pareti_dritte(m, (0.0, y_in), (0.0, y_fuori), z_in - 60.0, z_fuori, largo, 520.0, mat_terra="Pietra")
    # i corrimano di pietra dei nani
    for s in (-1, 1):
        m.trave("Pietra", [s * 220.0, y_in, z_in + 90.0], [s * 220.0, y_fuori, z_fuori + 90.0], 20.0, 14.0)
    g.metti("Scala dei nani", m)
    g.pianta.append(("scala", [(-250, y_in), (250, y_in), (250, y_fuori), (-250, y_fuori)]))
    g.aggiungi("torcia", "Torcia della scala", 245.0, -5200.0, -600.0 + 200.0, 180.0)
    g.luce("Luce della scala", 180.0, -5200.0, -600.0 + 240.0, intensita=4.0, raggio=900.0)


def sala_delle_ossa(g, rng):
    m = Mesh("SM_GrottaNord_Ossa")
    porta = Mesh("SM_GrottaNord_PortaDormienti")
    z = -900.0
    cx, cy, r = 0.0, -6900.0, 1000.0
    alto = 800.0
    lati = 28
    # il pavimento (un poligono) e la cupola (un cono basso)
    P = [[cx + r * 1.06 * math.cos(2 * math.pi * k / lati), cy + r * 1.06 * math.sin(2 * math.pi * k / lati), z] for k in range(lati)]
    m.poligono("Pietra", P[::-1])
    m.lastra("Pietra", cx - r - 80, cx + r + 80, cy - r - 80, cy + r + 80, z - 60, z - 1)
    cima = [cx, cy, z + alto + 380.0]
    for k in range(lati):
        a0, a1 = 2 * math.pi * k / lati, 2 * math.pi * (k + 1) / lati
        p0 = np.array([cx + r * math.cos(a0), cy + r * math.sin(a0), z])
        p1 = np.array([cx + r * math.cos(a1), cy + r * math.sin(a1), z])
        dentro = normalizza(np.array([cx, cy, 0.0]) - np.array([(p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2, 0.0]))
        medio = (a0 + a1) / 2
        # il buco a sud per la scala (verso +y: angolo 90 gradi) e a nord per la porta (270 gradi)
        verso_sud = abs(((math.degrees(medio) - 90.0) + 180.0) % 360.0 - 180.0) < 15.0
        verso_nord = abs(((math.degrees(medio) - 270.0) + 180.0) % 360.0 - 180.0) < 15.0
        # (11/11) a ovest la galleria che scende alla forgia dei nani (180 gradi)
        verso_ovest = abs(((math.degrees(medio) - 180.0) + 180.0) % 360.0 - 180.0) < 15.0
        z_buco = 520.0 if verso_sud else (720.0 if verso_nord else (450.0 if verso_ovest else 0.0))
        if z_buco < alto:
            m.quad("Roccia", p0 + [0, 0, z_buco], p0 + [0, 0, alto], p1 + [0, 0, alto], p1 + [0, 0, z_buco], normale=np.array([dentro[0], dentro[1], 0.0]))
        if z_buco > 0 and verso_nord:
            pass  # la porta chiude il buco a nord
        m.triangoli("Roccia", [p0 + [0, 0, alto], cima, p1 + [0, 0, alto]], [np.array([0.0, 0.0, -1.0])] * 3,
                    [[0, 0], [0.5, 1], [1, 0]], [[0, 1, 2]])
    # la porta dei dormienti: una lastra enorme nel buco a nord, con la cornice e i segni (le rune azzurre, spente)
    py = cy - r + 10.0
    porta.blocco("Pietra", -380.0, 380.0, py - 80.0, py, z, z + 760.0)
    porta.blocco("Pietra", -460.0, -380.0, py - 90.0, py + 20.0, z, z + 800.0)
    porta.blocco("Pietra", 380.0, 460.0, py - 90.0, py + 20.0, z, z + 800.0)
    porta.blocco("Pietra", -470.0, 470.0, py - 95.0, py + 25.0, z + 760.0, z + 860.0)
    for k in range(5):
        a = 2 * math.pi * k / 5 - math.pi / 2
        c = np.array([math.cos(a) * 170.0, py + 2.0, z + 420.0 + math.sin(a) * 170.0])
        porta.cilindro("Runa", c - np.array([0, 2.0, 0]), 26.0, 3.0, lati=12, asse=(0, 1, 0))
    porta.cilindro("Runa", np.array([0.0, py, z + 420.0]), 50.0, 3.0, lati=16, asse=(0, 1, 0))
    segni_dei_nani(porta, [0.0, py + 1.0, z + 700.0], (0, 1, 0), 9, 77)
    # i sarcofagi in giro, i mucchi d'ossa, il trono del capitano
    for k in range(8):
        a = 2 * math.pi * (k + 0.5) / 8
        sx, sy = cx + math.cos(a) * 760.0, cy + math.sin(a) * 760.0
        if abs(math.sin(a) - 1.0) < 0.2:
            continue   # lascia libera la scala
        m.scatola("Pietra", [sx, sy, z + 45], [math.cos(a) * 110, math.sin(a) * 110, 0], [-math.sin(a) * 45, math.cos(a) * 45, 0], [0, 0, 45])
        m.scatola("Pietra", [sx, sy, z + 98], [math.cos(a) * 118, math.sin(a) * 118, 0], [-math.sin(a) * 52, math.cos(a) * 52, 0], [0, 0, 8])
    for k in range(7):
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(150, 600)
        mucchio_ossa(m, cx + math.cos(a) * d, cy + math.sin(a) * d, z, rng, 12, 70.0)
    m.blocco("Pietra", -260.0, 260.0, cy - 760.0, cy - 520.0, z, z + 50.0)               # la pedana davanti alla porta
    m.blocco("Pietra", -70.0, 70.0, cy - 740.0, cy - 680.0, z + 50.0, z + 260.0)        # lo schienale del trono
    m.blocco("Pietra", -75.0, 75.0, cy - 700.0, cy - 610.0, z + 50.0, z + 110.0)        # il sedile
    g.metti("Sala delle ossa", m)
    g.metti("Porta dei dormienti", porta)
    g.pianta.append(("rotonda", [(cx, cy, r)]))
    g.aggiungi("ossario", "Il capitano delle ossa", 0.0, cy - 560.0, z + 60.0, scheletro="Capitano", quanti=1, raggio=40.0, minuti=60.0)
    g.aggiungi("ossario", "Scheletri con lo scudo", 0.0, cy - 150.0, z + 20.0, scheletro="ScheletroScudo", quanti=2, raggio=450.0, minuti=40.0)
    g.aggiungi("forziere", "Il forziere dei nani", 380.0, cy - 600.0, z + 50.0, 200.0, ricchezza=2)
    g.aggiungi("traccia", "La lanterna di Cecco", -230.0, cy + r - 160.0, z + 5.0, 70.0, oggetto="Lanterna_Cecco", asset="SM_GrottaNord_Lanterna")
    g.aggiungi("iscrizione", "La porta dei dormienti", 0.0, py + 150.0, z, 90.0, titolo="La porta dei dormienti",
               avvisi=["Una porta alta come tre case, di una pietra sola. Cinque cerchi in giro, e uno grande nel mezzo.",
                       "Non c'e' una maniglia, non c'e' una serratura. C'e' un conto, sopra: lo stesso delle colonne, finito.",
                       "Appoggi la mano. La pietra e' tiepida. Dall'altra parte, piano, qualcosa batte: un colpo, poi niente, poi un altro."])
    g.aggiungi("suono", "Martelli dietro la porta", 0.0, py + 200.0, z + 300.0, suono="S_Martelli", quando="Sempre", vicino=500.0, lontano=2600.0, volume=0.8)
    g.aggiungi("suono", "Gocce della grotta del nord", 0.0, -3800.0, -100.0, suono="S_Grotta", quando="Sempre", vicino=400.0, lontano=2200.0, volume=0.7)
    for k in range(6):
        a = 2 * math.pi * k / 6 + 0.3
        x, y = cx + math.cos(a) * 960.0, cy + math.sin(a) * 960.0
        g.aggiungi("torcia", "Torcia della sala delle ossa %d" % (k + 1), x, y, z + 240.0, math.degrees(a) + 180.0)
        g.luce("Luce della sala delle ossa %d" % (k + 1), x * 0.85, cy + (y - cy) * 0.85, z + 280.0, intensita=5.0, raggio=1200.0)
    g.luce("Luce delle rune della porta", 0.0, py + 120.0, z + 420.0, colore=(0.35, 0.7, 1.0), intensita=4.0, raggio=900.0)



# --------------------------------------------------------------------------------------------------- (11/11) l'ala segreta

def ala_segreta(g, rng):
    """Dietro la porta segreta a est della sala delle colonne: il corridoio delle trappole (le lance, la botola sul pozzo,
    i due pendoli) e la stanza del tesoro, dove il forziere chiama le ossa e la grata si chiude."""
    m = Mesh("SM_GrottaNord_AlaSegreta")
    z = -300.0
    y = -3850.0
    alto = 360.0
    pareti_dritte(m, (1060.0, y), (2250.0, y), z, z, 300.0, alto)
    pareti_dritte(m, (2250.0, y), (2650.0, y), z, z, 300.0, alto, passo=400.0, terra=False)   # sopra il pozzo
    pareti_dritte(m, (2650.0, y), (3460.0, y), z, z, 300.0, alto)
    # la piastra delle lance: appena piu' scura (chi guarda per terra la vede)
    m.quad("Roccia", [1600.0, -3990.0, z + 0.6], [1800.0, -3990.0, z + 0.6], [1800.0, -3710.0, z + 0.6], [1600.0, -3710.0, z + 0.6],
           normale=np.array([0.0, 0.0, 1.0]))
    # il pozzo sotto la botola: quindici metri, e il cornicione stretto lungo il muro a sud (38 cm) per chi sa
    fondo = -1800.0
    for (a, b, n) in (((2250.0, -4000.0), (2650.0, -4000.0), (0, 1, 0)), ((2650.0, -3700.0), (2250.0, -3700.0), (0, -1, 0)),
                      ((2250.0, -3700.0), (2250.0, -4000.0), (1, 0, 0)), ((2650.0, -4000.0), (2650.0, -3700.0), (-1, 0, 0))):
        m.quad("Roccia", [a[0], a[1], fondo], [b[0], b[1], fondo], [b[0], b[1], z], [a[0], a[1], z], normale=np.array(n, float))
    m.quad("Roccia", [2250.0, -4000.0, fondo], [2650.0, -4000.0, fondo], [2650.0, -3700.0, fondo], [2250.0, -3700.0, fondo],
           normale=np.array([0.0, 0.0, 1.0]))
    m.blocco("Pietra", 2250.0, 2650.0, -3738.0, -3700.0, z - 30.0, z)
    for k in range(6):
        _sasso(m, [rng.uniform(2280, 2620), rng.uniform(-3980, -3720), fondo + 15], rng.uniform(15, 30), rng, 1500 + k)
    # la stanza del tesoro
    x0, x1, y0, y1 = 3460.0, 4660.0, -4450.0, -3250.0
    sala(m, x0, x1, y0, y1, z, 520.0, buchi_o=[(450.0, 750.0, 0.0, 360.0)])
    segni = Mesh("SM_GrottaNord_Tesoro_Segni")
    segni_dei_nani(segni, [x1 - 2.0, -3500.0, z + 250.0], (-1, 0, 0), 10, 911)
    segni_dei_nani(segni, [x1 - 2.0, -4200.0, z + 250.0], (-1, 0, 0), 10, 912)
    for k in range(5):
        cassa(m, 4500.0 - k * 160.0, -4350.0, z, rng.uniform(-15, 15))
    for k in range(4):
        m.cilindro("Ferro", [rng.uniform(3700, 4500), rng.uniform(-3450, -3350), z], rng.uniform(25, 40), rng.uniform(8, 18), lati=10,
                   raggio_alto=8.0)   # mucchi di monete annerite
    m.blocco("Pietra", 4380.0, 4620.0, -3990.0, -3710.0, z, z + 35.0)              # la pedana del forziere
    g.metti("Ala segreta", m)
    g.metti("Segni della stanza del tesoro", segni, collisione=False)
    g.pianta.append(("galleria", [(1060, -4000), (3460, -4000), (3460, -3700), (1060, -3700)]))
    g.pianta.append(("sala", [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]))
    g.aggiungi("porta_segreta", "La porta segreta del conto", 1030.0, y, z, 180.0, testo="Completa il conto dei nani",
               iscrizione="I segni sulle colonne", minuti=10.0)
    g.aggiungi("trappola", "Le lance", 1700.0, y, z, 0.0, trappola="Lance", misura=[100.0, 140.0, 60.0], ritardo=0.45, ripristino=5.0)
    g.aggiungi("trappola", "La botola sul pozzo", 2450.0, -3869.0, z, 0.0, trappola="Botola", misura=[195.0, 128.0, 40.0], ritardo=0.7,
               ripristino=6.0)
    g.aggiungi("trappola", "Il fondo del pozzo", 2450.0, y, fondo, 0.0, trappola="Fondo", misura=[200.0, 150.0, 80.0])
    for k, x in enumerate((3000.0, 3250.0)):
        g.aggiungi("trappola", "Il pendolo %d" % (k + 1), x, y, z, 0.0, trappola="Pendolo", periodo=3.0, fase=0.5 * k, ampiezza=26.0,
                   braccio=300.0)
    g.aggiungi("forziere", "Il forziere del conto", 4500.0, y, z + 35.0, 180.0, ricchezza=3, allarme=True)
    g.aggiungi("allarme", "Le ossa del tesoro", 4060.0, y, z, 0.0, scheletro="Scheletro", quanti=6, raggio=420.0,
               grata=[-600.0, 0.0, 0.0], grata_gradi=0.0)
    g.aggiungi("iscrizione", "Il conto, finito", 4630.0, -3500.0, z, 180.0, titolo="Il conto, finito",
               avvisi=["Sulla parete in fondo, lo stesso conto delle colonne. Qui e' finito: l'ultimo gruppo ha tutti e cinque i segni.",
                       "Sotto il conto, un segno solo, inciso piu' a fondo: un forziere, e sopra il forziere un occhio aperto.",
                       "I nani lasciavano i loro tesori ai ladri. Ma non lasciavano mai che se ne andassero."])
    for k, x in enumerate((1400.0, 2100.0, 2850.0)):
        g.luce("Luce dell'ala segreta %d" % (k + 1), x, y + 120.0, z + 300.0, colore=(0.8, 0.55, 0.3), intensita=2.5, raggio=700.0)
    g.luce("Luce del tesoro 1", 3800.0, -3850.0, z + 420.0, colore=(1.0, 0.75, 0.35), intensita=4.0, raggio=1100.0)
    g.luce("Luce del tesoro 2", 4400.0, -3850.0, z + 420.0, colore=(1.0, 0.75, 0.35), intensita=4.0, raggio=1100.0)


# --------------------------------------------------------------------------------------------------- (11/11) le miniere allagate

def miniere_allagate(g, rng):
    """A ovest del campo della spedizione: la galleria scende sott'acqua (nove metri da fare in apnea) e risale nella cava
    d'argento dei nani, con il suo forziere."""
    m = Mesh("SM_GrottaNord_Miniere")
    y = -3850.0
    tratti = [((-3230.0, y), (-3700.0, y), -300.0, -300.0), ((-3700.0, y), (-4100.0, y), -300.0, -900.0),
              ((-4100.0, y), (-4700.0, y), -900.0, -900.0), ((-4700.0, y), (-5100.0, y), -900.0, -300.0),
              ((-5100.0, y), (-5130.0, y), -300.0, -300.0)]
    for a, b, za, zb in tratti:
        pareti_dritte(m, a, b, za, zb, 300.0, 350.0, mat_muro="Roccia", mat_terra="Roccia", passo=100.0)
    for k in range(10):
        x = rng.uniform(-5000, -3800)
        _sasso(m, [x, rng.uniform(-3980, -3720), -880.0 if -4700 < x < -4100 else -600.0], rng.uniform(15, 35), rng, 1600 + k)
    # la cava d'argento
    x0, x1, y0, y1 = -6330.0, -5130.0, -4450.0, -3250.0
    sala(m, x0, x1, y0, y1, -300.0, 450.0, mat_muro="Roccia", buchi_e=[(450.0, 750.0, 0.0, 350.0)])
    vene = Mesh("SM_GrottaNord_Vene")
    for k in range(14):
        lato = rng.integers(0, 3)
        a = rng.uniform(0.1, 0.9)
        zz = rng.uniform(-200, 80)
        if lato == 0:
            p0 = np.array([x0 + 2.0, y0 + (y1 - y0) * a, zz])
            d = np.array([0.0, rng.uniform(-1, 1), rng.uniform(-0.5, 0.5)])
            su = (1, 0, 0)
        elif lato == 1:
            p0 = np.array([x0 + (x1 - x0) * a, y0 + 2.0, zz])
            d = np.array([rng.uniform(-1, 1), 0.0, rng.uniform(-0.5, 0.5)])
            su = (0, 1, 0)
        else:
            p0 = np.array([x0 + (x1 - x0) * a, y1 - 2.0, zz])
            d = np.array([rng.uniform(-1, 1), 0.0, rng.uniform(-0.5, 0.5)])
            su = (0, -1, 0)
        prev = p0
        for t in range(5):
            q = prev + normalizza(d + rng.normal(0, 0.3, 3) * [abs(d[0]) > 0, abs(d[1]) > 0, 1]) * rng.uniform(40, 80)
            vene.trave("Intonaco", prev, q, 6.0, 2.0, su=su)
            prev = q
    acqua = Mesh("SM_GrottaNord_Acqua")
    acqua.quad("Acqua", [-5100.0, -4000.0, -320.0], [-3700.0, -4000.0, -320.0], [-3700.0, -3700.0, -320.0], [-5100.0, -3700.0, -320.0],
               normale=np.array([0.0, 0.0, 1.0]))
    g.metti("Miniere allagate", m)
    g.metti("Vene d'argento", vene, collisione=False)
    g.metti("L'acqua delle miniere", acqua, collisione=False)
    g.pianta.append(("galleria", [(-3230, -4000), (-5130, -4000), (-5130, -3700), (-3230, -3700)]))
    g.pianta.append(("sala", [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]))
    g.aggiungi("acqua", "L'acqua delle miniere", -4400.0, y, -320.0, 0.0,
               riva=[[-5150.0, -4010.0], [-3650.0, -4010.0], [-3650.0, -3690.0], [-5150.0, -3690.0]], pelo=-320.0)
    g.aggiungi("forziere", "Il forziere della cava", -6150.0, y, -300.0, 0.0, ricchezza=2)
    g.aggiungi("iscrizione", "La cava d'argento", -5600.0, -4420.0, -300.0, 90.0, titolo="La cava d'argento",
               avvisi=["Le pareti sono rigate di vene bianche che brillano alla luce. Argento, o qualcosa che gli somiglia.",
                       "I nani l'hanno scavata e poi l'hanno lasciata all'acqua, con gli attrezzi ancora appoggiati al muro.",
                       "Chi e' arrivato fin qui ha trattenuto il fiato a lungo. Respira, adesso."])
    g.luce("Luce della cava 1", -5500.0, -3600.0, 50.0, colore=(0.7, 0.8, 1.0), intensita=3.0, raggio=900.0)
    g.luce("Luce della cava 2", -6000.0, -4100.0, 50.0, colore=(0.7, 0.8, 1.0), intensita=3.0, raggio=900.0)
    g.luce("Luce sotto l'acqua", -4400.0, y, -700.0, colore=(0.25, 0.55, 0.6), intensita=1.5, raggio=900.0)


# --------------------------------------------------------------------------------------------------- (11/11) la forgia dei nani

def forgia(g, rng):
    """A ovest della sala delle ossa una galleria scende alla forgia dei nani: quattro forni spenti, le incudini, il canale
    delle rune, e al centro il guardiano di pietra (forza 80) che non teme il fuoco."""
    m = Mesh("SM_GrottaNord_Forgia")
    cy = -6900.0
    pareti_dritte(m, (-980.0, cy), (-1700.0, cy), -900.0, -900.0, 460.0, 450.0)
    pareti_dritte(m, (-1700.0, cy), (-2900.0, cy), -900.0, -1500.0, 460.0, 450.0, passo=150.0)
    z = -1500.0
    x0, x1, y0, y1 = -4900.0, -2900.0, -7900.0, -5900.0
    sala(m, x0, x1, y0, y1, z, 1000.0, buchi_e=[(770.0, 1230.0, 0.0, 450.0)], mat_volta="Roccia")
    fuochi = Mesh("SM_GrottaNord_Forgia_Brace")
    for k, fx in enumerate((-4500.0, -3900.0, -3300.0)):
        m.blocco("Pietra", fx - 160, fx + 160, y0 + 10, y0 + 210, z, z + 320)
        m.cilindro("Pietra", [fx, y0 + 110, z + 320], 70.0, 680.0, lati=10, raggio_alto=45.0)       # il camino
        m.blocco("Ferro", fx - 90, fx + 90, y0 + 205, y0 + 212, z + 60, z + 180)                     # la bocca, nera
        fuochi.blocco("Fiamma", fx - 70, fx + 70, y0 + 150, y0 + 204, z + 70, z + 90)                # la brace che resta
        # l'incudine davanti a ogni forno
        ax, ay = fx, y0 + 520
        m.blocco("Pietra", ax - 45, ax + 45, ay - 35, ay + 35, z, z + 60)
        m.blocco("Ferro", ax - 70, ax + 70, ay - 28, ay + 28, z + 60, z + 95)
        m.cilindro("Ferro", [ax + 70, ay, z + 78], 14.0, 30.0, lati=8, raggio_alto=2.0, asse=(1, 0, 0))
    # il canale delle rune: dai forni al centro
    for k in range(12):
        t0, t1 = k / 12, (k + 0.6) / 12
        a = np.array([-3900.0, y0 + 600 + (cy - (y0 + 600)) * t0])
        b = np.array([-3900.0, y0 + 600 + (cy - (y0 + 600)) * t1])
        fuochi.quad("Runa", [a[0] - 18, a[1], z + 1], [a[0] + 18, a[1], z + 1], [b[0] + 18, b[1], z + 1], [b[0] - 18, b[1], z + 1],
                    normale=np.array([0.0, 0.0, 1.0]))
    m.cilindro("Pietra", [-3900.0, cy, z], 260.0, 30.0, lati=20)                                   # la pedana del guardiano
    for k in range(10):
        _sasso(m, [rng.uniform(-4800, -3000), rng.uniform(-7000, -6000), z + 10], rng.uniform(20, 50), rng, 1700 + k)
    for k in range(6):
        cassa(m, -4800.0 + k * 140.0, -6020.0, z, rng.uniform(-10, 10), 110.0, 70.0, 70.0)
    g.metti("La forgia dei nani", m)
    g.metti("Le braci della forgia", fuochi, collisione=False)
    g.pianta.append(("galleria", [(-980, -7130), (-2900, -7130), (-2900, -6670), (-980, -6670)]))
    g.pianta.append(("sala", [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]))
    g.aggiungi("ossario", "Il guardiano della forgia", -3900.0, cy, z + 40.0, scheletro="Guardiano", quanti=1, raggio=10.0, minuti=120.0)
    g.aggiungi("forziere", "Il forziere della forgia", -4700.0, -6100.0, z, 0.0, ricchezza=3)
    g.aggiungi("iscrizione", "La forgia dei nani", -3050.0, -6450.0, z, 180.0, titolo="La forgia dei nani",
               avvisi=["Tre forni alti come torri, spenti. Nel fondo di ognuno, sotto la cenere, la brace e' ancora rossa.",
                       "Mille anni, e la brace non si e' spenta. O qualcuno, ogni tanto, la ravviva.",
                       "Sulla pedana in mezzo alla sala, qualcosa di pietra e' rimasto a guardia. Ha la forma di un nano, ma e' alto il doppio di un uomo."])
    for k, (lx, ly) in enumerate(((-4500.0, -7500.0), (-3900.0, -7500.0), (-3300.0, -7500.0))):
        g.luce("Luce dei forni %d" % (k + 1), lx, ly, z + 120.0, colore=(1.0, 0.35, 0.1), intensita=8.0, raggio=1500.0)
    g.luce("Luce delle rune della forgia", -3900.0, cy, z + 200.0, colore=(0.35, 0.7, 1.0), intensita=3.0, raggio=1200.0)
    for k, (tx, ty, gr) in enumerate(((-2920.0, -6200.0, 180.0), (-2920.0, -7600.0, 180.0), (-4880.0, -6400.0, 0.0), (-4880.0, -7400.0, 0.0))):
        g.aggiungi("torcia", "Torcia della forgia %d" % (k + 1), tx, ty, z + 250.0, gr)
    g.aggiungi("suono", "Il respiro della forgia", -3900.0, -7600.0, z + 200.0, suono="S_Grotta", quando="Sempre", vicino=600.0,
               lontano=2600.0, volume=0.6)


# --------------------------------------------------------------------------------------------------- (11/11) i pezzi delle trappole

def pezzi_trappole(g):
    lance = Mesh("SM_Trappola_Lance")
    for i in range(5):
        for j in range(7):
            x = -80.0 + 40.0 * i
            yy = -120.0 + 40.0 * j
            lance.cilindro("Ferro", [x, yy, 0.0], 4.0, 110.0, lati=6, raggio_alto=0.3)
    g.mesh(lance)
    botola = Mesh("SM_Trappola_Botola")
    botola.blocco("Pietra", -200.0, 200.0, -131.0, 131.0, -30.0, 0.0)
    g.mesh(botola)
    punte = Mesh("SM_Trappola_Punte")
    for i in range(8):
        for j in range(6):
            punte.cilindro("Ferro", [-175.0 + 50.0 * i, -125.0 + 50.0 * j, 0.0], 6.0, 70.0, lati=6, raggio_alto=0.4)
    g.mesh(punte)
    lama = Mesh("SM_Trappola_Lama")
    lama.trave("Ferro", [0.0, 0.0, 0.0], [0.0, 0.0, 300.0], 8.0, 8.0)                              # il braccio fino al perno
    prev = None
    for k in range(13):
        a = math.radians(-70.0 + 140.0 * k / 12)
        p = np.array([0.0, 95.0 * math.sin(a), -60.0 + 95.0 * (1 - math.cos(a)) * 0.35 + 20.0])
        if prev is not None:
            lama.trave("Ferro", prev, p, 26.0, 3.0, su=(1, 0, 0))
        prev = p
    g.mesh(lama)
    porta = Mesh("SM_GrottaNord_PortaSegreta")
    porta.blocco("Pietra", -30.0, 30.0, -150.0, 150.0, 0.0, 360.0)
    segni_dei_nani(porta, [31.0, 0.0, 40.0], (1, 0, 0), 3, 4242)            # la meta' del conto che manca, in basso
    g.mesh(porta)
    grata = Mesh("SM_GrottaNord_Grata")
    for k in range(11):
        yy = -150.0 + 30.0 * k
        grata.cilindro("Ferro", [0.0, yy, 0.0], 3.0, 360.0, lati=6)
    for zz in (60.0, 180.0, 300.0):
        grata.blocco("Ferro", -4.0, 4.0, -152.0, 152.0, zz, zz + 10.0)
    g.mesh(grata)


def guardiano(g):
    """I pezzi del guardiano della forgia: come lo scheletro (stessi perni), ma di pietra squadrata, con gli occhi di runa
    e un martello al posto della spada. Il gioco lo ingrandisce (1,9)."""
    p = "Pietra"
    t = Mesh("SM_Guardiano_Teschio")
    t.blocco(p, -9, 11, -10, 10, 0, 20)
    t.blocco(p, -7, 13, -11, 11, 18, 24)
    for s in (-1, 1):
        t.blocco("Runa", 11.0, 12.5, s * 4.5 - 2.0, s * 4.5 + 2.0, 10, 13)
    g.mesh(t)
    to = Mesh("SM_Guardiano_Torace")
    to.blocco(p, -12, 14, -22, 22, -16, 40)
    to.blocco("Runa", 14.0, 15.0, -4, 4, 12, 22)                                  # il segno sul petto
    g.mesh(to)
    b = Mesh("SM_Guardiano_Bacino")
    b.blocco(p, -10, 10, -17, 17, -10, 8)
    g.mesh(b)
    br = Mesh("SM_Guardiano_Braccio")
    br.blocco(p, -6, 6, -6, 6, -32, 2)
    br.blocco(p, -6.5, 6.5, -6.5, 6.5, -60, -32)
    br.blocco(p, -5, 9, -7, 7, -70, -60)
    g.mesh(br)
    ga = Mesh("SM_Guardiano_Gamba")
    ga.blocco(p, -7, 7, -7, 7, -46, 2)
    ga.blocco(p, -7, 7, -7, 7, -88, -46)
    ga.blocco(p, -8, 16, -8, 8, -92, -86)
    g.mesh(ga)
    ma = Mesh("SM_Guardiano_Spada")
    ma.cilindro("Trave", [-8, 0, 0], 2.5, 70.0, lati=6, asse=(1, 0, 0))
    ma.blocco("Ferro", 58, 78, -12, 12, -10, 10)
    g.mesh(ma)
    sc = Mesh("SM_Guardiano_Scudo")
    sc.blocco(p, 6, 10, -20, 20, -20, 20)
    g.mesh(sc)


# --------------------------------------------------------------------------------------------------- (11/11) le stanze segrete della valle

def stanza(nome, x0, x1, y0, y1, alto, mat="Pietra"):
    m = Mesh(nome)
    sala(m, x0, x1, y0, y1, 0.0, alto, mat_muro=mat, mat_terra=mat)
    return m


def segreti(g, rng):
    """Due stanze segrete nella valle, sotto terra come le gallerie (si entra con un passaggio segreto, da vicinissimo).
    Ogni stanza ha la sua 'ancora': l'attore del villaggio da cui si misura (il mulino, le rovine)."""
    fuori = []
    # 1. la cantina del mulino: una botola sotto la paglia
    m = stanza("SM_Segreto_Cantina", -300.0, 300.0, -250.0, 250.0, 260.0, mat="Assi")
    for k in range(9):
        x = rng.uniform(-260, -60)
        yy = -220.0 + 55.0 * k
        m.cilindro("Stoffa", [x, yy, 0.0], 28.0, 70.0, lati=10, raggio_alto=22.0)     # i sacchi di farina
    m.blocco("Assi", 150.0, 290.0, -240.0, 240.0, 120.0, 128.0)                       # lo scaffale
    m.blocco("Trave", 150.0, 160.0, -240.0, -230.0, 0.0, 128.0)
    m.blocco("Trave", 150.0, 160.0, 230.0, 240.0, 0.0, 128.0)
    m.trave("Assi", [0.0, 230.0, 0.0], [0.0, 160.0, 260.0], 70.0, 8.0)                # la scala a pioli verso la botola
    g.mesh(m)
    fuori.append({
        "nome": "La cantina del mulino", "ancora": "Mulino", "ingresso": [0.0, 0.0], "giu": 1800.0,
        "testo": "Una botola, sotto la paglia", "torna": "Risali dalla botola",
        "cose": [
            {"tipo": "mesh", "nome": "La cantina del mulino", "asset": "SM_Segreto_Cantina", "pos": [0, 0, 0], "gradi": 0, "collisione": True},
            {"tipo": "arrivo", "nome": "Arrivo nella cantina", "pos": [0.0, 150.0, 100.0], "gradi": -90.0},
            {"tipo": "uscita", "nome": "Risali dalla botola", "pos": [0.0, 210.0, 0.0], "gradi": 0.0},
            {"tipo": "iscrizione", "nome": "Il conto sul muro", "pos": [290.0, 0.0, 0.0], "gradi": 180.0, "titolo": "Il conto sul muro",
             "avvisi": ["Sacchi di farina, troppi per un mulino che dice di non averne mai abbastanza.",
                        "Sul muro, a carbone, una colonna di numeri e una parola sola, sottolineata due volte: inverno.",
                        "La grafia e' di una donna. Ordinata, paziente, e un po' spaventata."]},
            {"tipo": "forziere", "nome": "La cassetta della cantina", "pos": [200.0, -150.0, 0.0], "gradi": 180.0, "ricchezza": 1},
            {"tipo": "luce", "nome": "Luce della cantina", "pos": [0.0, 0.0, 220.0], "gradi": 0, "colore": [1.0, 0.7, 0.4], "intensita": 2.0,
             "raggio": 700.0},
        ]})
    # 2. la cantina murata delle rovine: una pietra della cinta che si muove
    m = stanza("SM_Segreto_Rovine", -350.0, 350.0, -300.0, 300.0, 320.0)
    rune = Mesh("SM_Segreto_Rovine_Rune")
    for k in range(5):
        a = 2 * math.pi * k / 5 - math.pi / 2
        c = np.array([348.0, math.cos(a) * 110.0, 170.0 + math.sin(a) * 110.0])
        rune.cilindro("Runa", c, 18.0, 2.0, lati=10, asse=(-1, 0, 0))
    m.blocco("Pietra", -60.0, 60.0, -60.0, 60.0, 0.0, 90.0)                           # un altare basso
    g.mesh(m)
    g.mesh(rune)
    fuori.append({
        "nome": "La cantina murata delle rovine", "ancora": "Rovine degli Antichi", "ingresso": [0.0, 1150.0], "giu": 2200.0,
        "testo": "Una pietra della cinta si muove", "torna": "Torna su, tra le rovine",
        "cose": [
            {"tipo": "mesh", "nome": "La cantina murata", "asset": "SM_Segreto_Rovine", "pos": [0, 0, 0], "gradi": 0, "collisione": True},
            {"tipo": "mesh", "nome": "Le rune della cantina", "asset": "SM_Segreto_Rovine_Rune", "pos": [0, 0, 0], "gradi": 0,
             "collisione": False, "rune": "sempre"},
            {"tipo": "arrivo", "nome": "Arrivo nella cantina murata", "pos": [-250.0, 0.0, 100.0], "gradi": 0.0},
            {"tipo": "uscita", "nome": "Torna su, tra le rovine", "pos": [-320.0, 0.0, 0.0], "gradi": 180.0},
            {"tipo": "trappola", "nome": "Le lance della cantina", "pos": [-100.0, 0.0, 0.0], "gradi": 0.0, "trappola": "Lance",
             "misura": [60.0, 140.0, 60.0], "ritardo": 0.5, "ripristino": 6.0},
            {"tipo": "iscrizione", "nome": "Il muro degli Antichi", "pos": [330.0, 0.0, 0.0], "gradi": 180.0, "titolo": "Il muro degli Antichi",
             "avvisi": ["Una stanza piccola, murata dagli uomini e non dagli Antichi: i mattoni dell'ingresso sono di un'altra pietra.",
                        "Sul muro in fondo, i cinque posti in cerchio, come sulla porta della grotta a est.",
                        "rune: Aeth Nir Dorn Sil Lume",
                        "Qualcuno, tanto tempo fa, ha voluto che questa stanza non la trovasse nessuno. Sull'altare, la polvere ha la forma di una mano."]},
            {"tipo": "forziere", "nome": "La cassa sotto l'altare", "pos": [0.0, -200.0, 0.0], "gradi": 90.0, "ricchezza": 1},
            {"tipo": "luce", "nome": "Luce delle rune della cantina", "pos": [250.0, 0.0, 170.0], "gradi": 0, "colore": [0.35, 0.7, 1.0],
             "intensita": 2.5, "raggio": 800.0},
        ]})
    return fuori


def suono_scatto(uscita):
    fr = 44100
    n = int(fr * 0.35)
    t = np.arange(n) / fr
    rng = np.random.default_rng(77)
    clic = np.zeros(n)
    for k, t0 in enumerate((0.0, 0.05, 0.12)):
        i = int(t0 * fr)
        l = int(0.015 * fr)
        tt = np.arange(l) / fr
        clic[i:i + l] += np.sin(2 * np.pi * (1800 - 500 * k) * tt) * np.exp(-tt / 0.004) * (1.0 - 0.3 * k)
    clic += rng.normal(0, 1, n) * np.exp(-t / 0.03) * 0.15
    scrivi_wav(os.path.join(uscita, "S_Trappola_Scatto.wav"), clic * 0.8)


# --------------------------------------------------------------------------------------------------- piccoli pezzi

def pezzi_piccoli(g, rng):
    sasso = Mesh("SM_GrottaNord_Sasso")
    _sasso(sasso, [0, 0, 0], 70.0, rng, 4242)
    g.mesh(sasso)
    f = Mesh("SM_GrottaNord_Forziere")
    f.blocco("Assi", -55, 55, -35, 35, 0, 55)
    f.blocco("Assi", -57, 57, -37, 37, 55, 70)
    for s in (-40, 0, 40):
        f.blocco("Ferro", s - 3, s + 3, -38, 38, -1, 72)
    f.blocco("Ferro", -8, 8, 36, 40, 40, 58)
    g.mesh(f)
    t = Mesh("SM_GrottaNord_Taccuino")
    t.blocco("Stoffa", -10, 10, -14, 14, 0, 4)
    t.blocco("Intonaco", -9, 9, -13, 13, 4, 5)
    g.mesh(t)
    lan = Mesh("SM_GrottaNord_Lanterna")
    lan.cilindro("Ferro", [0, 0, 0], 9.0, 4.0, lati=8)
    lan.cilindro("Vetro", [0, 0, 4], 8.0, 18.0, lati=8)
    lan.cilindro("Ferro", [0, 0, 22], 9.0, 5.0, lati=8, raggio_alto=4.0)
    lan.trave("Ferro", [-6, 0, 27], [6, 0, 27], 2.0, 2.0)
    g.mesh(lan)
    g.mesh(torcia())
    prova = Mesh("SM_GrottaNord_ProvaAssi")
    prova.blocco("Pietra", 0, 100, -5, 5, 0, 10)
    prova.blocco("Pietra", -5, 5, 0, 300, 0, 10)
    prova.blocco("Pietra", -5, 5, -5, 5, 0, 50)
    g.mesh(prova)


def scheletro(g):
    """I pezzi dello scheletro (ValdorsoScheletri.cpp): ogni pezzo con il perno sulla sua giuntura, verso il basso."""
    osso = "Ossa"
    teschio = Mesh("SM_Scheletro_Teschio")
    teschio.cilindro(osso, [0, 0, 0], 4.0, 6.0, lati=6)                              # il collo
    teschio.cilindro(osso, [2, 0, 4], 9.5, 12.0, lati=12, raggio_alto=10.5)
    teschio.cilindro(osso, [2, 0, 16], 10.5, 7.0, lati=12, raggio_alto=4.0)
    teschio.blocco(osso, 4, 13, -6, 6, 0, 5)                                           # la mascella
    for s in (-1, 1):
        teschio.cilindro("Ferro", [11.0, s * 4.0, 12.0], 2.6, 2.0, lati=8, asse=(1, 0, 0))  # le orbite
    g.mesh(teschio)
    torace = Mesh("SM_Scheletro_Torace")
    torace.cilindro(osso, [0, 0, -16], 3.0, 56.0, lati=6)                            # la colonna
    for k in range(5):
        zz = 4.0 + 6.5 * k
        largo = 15.0 - abs(k - 2) * 1.5
        for s in (-1, 1):
            prev = np.array([-3.0, 0.0, zz + 4])
            for t in np.linspace(0.2, 1.0, 5):
                a = t * math.pi * 0.95
                q = np.array([-3.0 + 14.0 * math.sin(a), s * largo * math.sin(a * 0.55 + 0.2), zz + 4 - 4 * t])
                torace.trave(osso, prev, q, 2.2, 2.2)
                prev = q
    torace.trave(osso, [0, -21, 36], [0, 21, 36], 3.0, 3.0)                           # le clavicole
    torace.trave(osso, [10, 0, 6], [10, 0, 32], 3.0, 2.0)                             # lo sterno
    g.mesh(torace)
    bacino = Mesh("SM_Scheletro_Bacino")
    for s in (-1, 1):
        bacino.trave(osso, [0, 0, 6], [2, s * 15, 2], 6.0, 4.0)
        bacino.trave(osso, [2, s * 15, 2], [6, s * 10, -8], 4.0, 4.0)
    bacino.cilindro(osso, [0, 0, 0], 4.0, 10.0, lati=6)
    g.mesh(bacino)
    braccio = Mesh("SM_Scheletro_Braccio")
    braccio.cilindro(osso, [0, 0, -30], 2.6, 30.0, lati=6)                            # l'omero
    braccio.cilindro(osso, [0, 0, -33], 3.8, 4.0, lati=8)                             # il gomito
    braccio.cilindro(osso, [0, 0, -58], 2.2, 26.0, lati=6)                            # l'avambraccio
    braccio.blocco(osso, -3, 6, -4, 4, -66, -58)                                       # la mano
    g.mesh(braccio)
    gamba = Mesh("SM_Scheletro_Gamba")
    gamba.cilindro(osso, [0, 0, -45], 3.2, 45.0, lati=6)                              # il femore
    gamba.cilindro(osso, [0, 0, -48], 4.5, 5.0, lati=8)                               # il ginocchio
    gamba.cilindro(osso, [0, 0, -86], 2.6, 39.0, lati=6)                              # la tibia
    gamba.blocco(osso, -4, 14, -4, 4, -90, -85)                                        # il piede
    g.mesh(gamba)
    spada = Mesh("SM_Scheletro_Spada")
    spada.cilindro("Trave", [-8, 0, 0], 2.2, 14.0, lati=6, asse=(1, 0, 0))            # l'impugnatura
    spada.blocco("Ferro", 5, 8, -11, 11, -2, 2)                                        # la guardia
    spada.blocco("Ferro", 8, 78, -2.5, 2.5, -0.8, 0.8)                                 # la lama (arrugginita)
    spada.blocco("Ferro", 78, 86, -1.2, 1.2, -0.6, 0.6)
    g.mesh(spada)
    scudo = Mesh("SM_Scheletro_Scudo")
    scudo.cilindro("Assi", [8, 0, 0], 30.0, 4.0, lati=16, asse=(1, 0, 0))
    scudo.cilindro("Ferro", [12, 0, 0], 8.0, 3.0, lati=10, asse=(1, 0, 0))
    g.mesh(scudo)


# --------------------------------------------------------------------------------------------------- i suoni

def scrivi_wav(percorso, segnale, frequenza=44100):
    s = np.clip(segnale, -1.0, 1.0)
    dati = (s * 32000).astype(np.int16)
    with wave.open(percorso, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(frequenza)
        w.writeframes(dati.tobytes())


def suoni(uscita):
    fr = 44100
    rng = np.random.default_rng(31)

    def rumore_basso(secondi, taglio):
        n = int(fr * secondi)
        x = rng.normal(0, 1, n)
        # un passa-basso semplice (media mobile ripetuta)
        k = max(1, int(fr / taglio))
        for _ in range(3):
            x = np.convolve(x, np.ones(k) / k, mode="same")
        return x / (np.abs(x).max() + 1e-9)

    # la frana: un boato che sale e poi rotola via, con i sassi
    n = int(fr * 4.5)
    t = np.arange(n) / fr
    boato = rumore_basso(4.5, 120) * np.minimum(1.0, t / 0.4) * np.exp(-np.maximum(0, t - 0.8) / 1.4)
    sassi = np.zeros(n)
    for _ in range(60):
        i = int(rng.uniform(0.2, 3.8) * fr)
        l = int(rng.uniform(0.02, 0.08) * fr)
        sassi[i:i + l] += rng.normal(0, 1, len(sassi[i:i + l])) * np.exp(-np.arange(len(sassi[i:i + l])) / (l * 0.3)) * rng.uniform(0.1, 0.5)
    scrivi_wav(os.path.join(uscita, "S_Frana.wav"), boato * 0.9 + sassi * 0.5)

    # lo scheletro che si sveglia: ossa che si toccano (clic secchi) e uno strofinio
    n = int(fr * 1.8)
    s = np.zeros(n)
    for k in range(18):
        i = int((0.05 + k * 0.09 + rng.uniform(-0.02, 0.02)) * fr)
        l = int(0.012 * fr)
        f0 = rng.uniform(900, 1800)
        tt = np.arange(l) / fr
        s[i:i + l] += np.sin(2 * np.pi * f0 * tt) * np.exp(-tt / 0.003) * rng.uniform(0.3, 0.8)
    s += rumore_basso(1.8, 2500) * 0.08 * np.sin(np.linspace(0, np.pi, n))
    scrivi_wav(os.path.join(uscita, "S_Scheletro_Sveglia.wav"), s)

    # le ossa che crollano: tanti clic che cadono e un tonfo
    n = int(fr * 1.4)
    s = np.zeros(n)
    for k in range(30):
        i = int(rng.uniform(0.0, 0.9) * fr)
        l = int(0.01 * fr)
        tt = np.arange(l) / fr
        s[i:i + l] += np.sin(2 * np.pi * rng.uniform(700, 1500) * tt) * np.exp(-tt / 0.004) * rng.uniform(0.2, 0.7)
    tonfo = rumore_basso(1.4, 200) * np.exp(-np.arange(n) / fr / 0.25)
    scrivi_wav(os.path.join(uscita, "S_Scheletro_Crollo.wav"), s + tonfo * 0.6)


# --------------------------------------------------------------------------------------------------- l'anteprima

def anteprima(g, percorso):
    W, H = 1400, 1100
    img = Image.new("RGB", (W, H), (24, 22, 20))
    d = ImageDraw.Draw(img)
    # da x -3400..1200, y 600..-8200 (il nord in alto)

    def q(x, y):
        return ((x + 6600.0) / 11500.0 * W, (y + 8200.0) / 9000.0 * H)

    for tipo, punti in g.pianta:
        if tipo == "rotonda":
            cx, cy, r = punti[0]
            a, b = q(cx - r, cy - r), q(cx + r, cy + r)
            d.ellipse([a[0], a[1], b[0], b[1]], outline=(200, 180, 140), width=3)
        elif tipo == "colonna":
            x, y = q(*punti[0])
            d.ellipse([x - 6, y - 6, x + 6, y + 6], fill=(160, 150, 130))
        else:
            d.polygon([q(*p) for p in punti], outline=(200, 180, 140))
    colori = {"ossario": (220, 60, 50), "forziere": (230, 200, 60), "traccia": (90, 200, 120), "iscrizione": (120, 160, 230),
              "luce": (250, 150, 60), "passaggio": (255, 255, 255), "arrivo": (255, 255, 255), "suono": (180, 120, 220),
              "trappola": (255, 0, 255), "porta_segreta": (0, 255, 255), "allarme": (255, 80, 0), "acqua": (60, 140, 255)}
    for c in g.cose:
        if c["tipo"] in colori:
            x, y = q(c["pos"][0], c["pos"][1])
            r = 9 if c["tipo"] in ("ossario", "arrivo", "passaggio") else 5
            d.ellipse([x - r, y - r, x + r, y + r], outline=colori[c["tipo"]], width=2)
            if c["tipo"] in ("ossario", "forziere", "traccia", "iscrizione", "arrivo", "trappola", "porta_segreta"):
                d.text((x + 10, y - 6), c["nome"][:34], fill=colori[c["tipo"]])
    d.text((10, 10), "Grotta del nord (nord in alto). Rosso: scheletri, giallo: forziere, verde: storia di Lucia, blu: iscrizioni",
           fill=(220, 210, 190))
    img.save(percorso)


def principale(uscita):
    os.makedirs(uscita, exist_ok=True)
    rng = np.random.default_rng(1312)
    g = Grotta(uscita)
    galleria_della_frana(g, rng)
    sala_delle_colonne(g, rng)
    campo_della_spedizione(g, rng)
    scala_dei_nani(g, rng)
    sala_delle_ossa(g, rng)
    # (11/11) la grotta grande: l'ala segreta con le trappole e il tesoro, le miniere allagate, la forgia con il guardiano
    ala_segreta(g, rng)
    miniere_allagate(g, rng)
    forgia(g, rng)
    pezzi_piccoli(g, rng)
    pezzi_trappole(g)
    scheletro(g)
    guardiano(g)
    stanze = segreti(g, rng)
    suoni(uscita)
    suono_scatto(uscita)
    # l'arrivo (AValdorsoFrana::Dentro) e l'uscita (AValdorsoPassaggio "Torna alla luce"), guardando verso nord
    g.aggiungi("arrivo", "Arrivo nelle gallerie", 0.0, 0.0, 110.0, -90.0)
    g.aggiungi("passaggio", "Torna alla luce", 0.0, 100.0, 18.0, 90.0, testo="Torna alla luce (risali dalla frana)")
    dati = {"versione": 2, "giu_cm": GIU, "mesh": sorted(g.scritte), "cose": g.cose, "segreti": stanze}
    with open(os.path.join(uscita, "GrottaNord.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)
    anteprima(g, os.path.join(uscita, "Anteprima_GrottaNord.png"))
    tri = 0
    print("grotta del nord: %d pezzi, %d cose (%s)" % (len(g.scritte), len(g.cose),
                                                      ", ".join("%s %d" % (t, sum(1 for c in g.cose if c["tipo"] == t))
                                                                for t in sorted(set(c["tipo"] for c in g.cose)))))
    return tri


if __name__ == "__main__":
    principale(sys.argv[1] if len(sys.argv) > 1 else "grotta_nord")
