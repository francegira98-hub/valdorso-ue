# Valdorso - Le Rovine degli Antichi nel bosco grande (v0.1.3). Scritto da Claude il 06/11/2026.
# Dal Design: "le rovine degli Antichi con le rune", "una cripta degli Antichi sotto le rovine: ingresso tra le rovine
# sigillato dalle rune che di notte si accendono, da aprire più avanti", "iscrizioni sulle rovine e la prima parte
# della leggenda dell'Orso". Nella lore sono la città bassa di Calendra (la collina dell'Orso).
#
# Cosa c'è (metri intorno al luogo "Rovine degli Antichi", x est, y sud):
#   - la cinta spezzata di un recinto di pietra (36 x 26 m), con muri da mezzo metro a tre metri e mezzo e varchi,
#     e blocchi caduti intorno;
#   - una colonnata di otto colonne (alcune intere con il capitello, altre spezzate) e due colonne cadute a terra;
#   - il cerchio delle cinque pietre ritte (gli elementi), con una fascia di rune su ognuna e il pentagono di rune
#     intarsiato nella pietra piatta al centro: di notte si accendono e pulsano con il battito (AValdorsoRuneNotte);
#   - il tumulo con l'ingresso della cripta: due stipiti, l'architrave con le rune e la lastra che sigilla la scala
#     (non si apre: la cripta è di una versione successiva);
#   - due iscrizioni da leggere con E (AValdorsoIscrizione): la leggenda dell'Orso, come la sa la gente, e le rune
#     della lastra, che nessuno sa più leggere del tutto.
# Le parti in materiale "Runa" (MI_Villaggio_Runa) le accende di notte il C++; le cose con "rune": true nel JSON
# crea_villaggio.py le etichetta "ValdorsoRune".

import math

import numpy as np

import alfabeto_rune as ar
from villaggio_mesh import Mesh

S = "Rovine"

# (07/11 sera) Le rune vere (alfabeto_rune.py, dalle Cronache): ogni pietra del cerchio porta il suo elemento e una
# parola che protegge, letta dall'alto in basso; sulla pietra piatta, accanto a ogni punta della stella, l'elemento
# della pietra che le sta davanti; sull'architrave della cripta Ur, da sola; sulla lastra tre parole che fermano.
PAROLE_PIETRE = ["Aeth-Ka-Vel", "Nir-Ren-Ka", "Dorn-Ka-Ren", "Sil-Ka-Ren", "Lume-Ren-Ka"]
PAROLE_LASTRA = ["Lume-Tor", "Dorn-Tor", "Sil-Tor"]  # dall'alto in basso


def rovine(pr, rng):
    t = pr.t
    cx, cy, _ = t.luogo("Rovine degli Antichi")
    q0 = t.quota(cx, cy)

    def R(x, y):
        return cx + x * 100.0, cy + y * 100.0

    def z(x, y):
        """La quota del terreno in quel punto, rispetto al centro (cm)."""
        return t.quota(*R(x, y)) - q0

    m = Mesh("SM_Rovine_Antichi")
    rune = Mesh("SM_Rovine_Rune")

    # ---- la cinta spezzata: blocchi da 1,5 m lungo i quattro lati, alti a onde, con i varchi
    W, D = 36.0, 26.0
    lati = [((-W / 2, -D / 2), (W / 2, -D / 2)), ((W / 2, -D / 2), (W / 2, D / 2)),
            ((W / 2, D / 2), (-W / 2, D / 2)), ((-W / 2, D / 2), (-W / 2, -D / 2))]
    caduti = []
    for k, (a, b) in enumerate(lati):
        a, b = np.array(a), np.array(b)
        L = float(np.linalg.norm(b - a))
        u = np.append((b - a) / L, 0.0)
        n = int(L / 1.5)
        fase = rng.uniform(0, 6.28)
        for i in range(n):
            s = (i + 0.5) / n
            p = a + (b - a) * s
            # l'entrata a sud (lato 2, in mezzo) resta aperta; altri varchi a caso
            if (k == 2 and abs(p[0]) < 3.0) or rng.random() < 0.18:
                if rng.random() < 0.6:
                    caduti.append(p + np.array([-u[1], u[0]]) * rng.uniform(1.0, 3.0) * rng.choice([-1, 1]))
                continue
            alto = 60 + 150 * (0.5 + 0.5 * math.sin(fase + s * 7.0)) + rng.uniform(-30, 120)
            alto = max(50.0, min(360.0, alto))
            base = z(*p) - 40
            m.scatola("Pietra", [p[0] * 100, p[1] * 100, base + alto / 2], list(u * 76), [-u[1] * 50, u[0] * 50, 0], [0, 0, alto / 2])
            # la cima rotta: un blocco più piccolo storto
            if alto > 120 and rng.random() < 0.5:
                g = rng.uniform(-0.3, 0.3)
                ux = np.array([math.cos(g) * u[0] - math.sin(g) * u[1], math.sin(g) * u[0] + math.cos(g) * u[1], 0.0])
                m.scatola("Pietra", [p[0] * 100 + rng.uniform(-20, 20), p[1] * 100, base + alto + 20], list(ux * 40),
                          [-ux[1] * 35, ux[0] * 35, 0], [0, 0, 20])
    for p in caduti:
        g = rng.uniform(0, math.pi)
        u = np.array([math.cos(g), math.sin(g), 0.0])
        zz = z(*p)
        m.scatola("Pietra", [p[0] * 100, p[1] * 100, zz + 15], list(u * rng.uniform(40, 70)), [-u[1] * 35, u[0] * 35, 0], [0, 0, 30])
    pr.disegni.append((S, "linea", [R(*a) for a, _ in lati] + [R(*lati[0][0])], "Cinta delle rovine"))

    # ---- la colonnata: otto colonne a nord del cerchio, due cadute
    for i in range(8):
        x = -10.5 + i * 3.0
        y = -7.5
        zz = z(x, y)
        intera = i in (1, 2, 5)
        alto = 520.0 if intera else rng.uniform(80, 330)
        m.blocco("Pietra", x * 100 - 55, x * 100 + 55, y * 100 - 55, y * 100 + 55, zz - 40, zz + 25)          # base
        m.cilindro("Pietra", [x * 100, y * 100, zz + 25], 34, alto, lati=12, raggio_alto=30)
        if intera:
            m.blocco("Pietra", x * 100 - 50, x * 100 + 50, y * 100 - 50, y * 100 + 50, zz + 25 + alto, zz + 25 + alto + 30)
    for (x, y, g) in ((-6.0, -4.5, 0.4), (8.0, -5.0, -0.2)):
        zz = z(x, y)
        d = (math.cos(g), math.sin(g), 0.0)
        m.cilindro("Pietra", [x * 100 - d[0] * 220, y * 100 - d[1] * 220, zz + 30], 33, 440, lati=12, asse=d)
    # un architrave caduto tra le colonne intere 1 e 2
    m.scatola("Pietra", [-6.0 * 100, -7.5 * 100 + 140, z(-6.0, -6.1) + 25], [160, 0, 0], [0, 40, 0], [0, 0, 25])

    # ---- il cerchio delle cinque pietre (gli elementi) e la pietra piatta al centro con il pentagono
    cc = np.array([0.0, 4.5])
    raggio = 6.0
    for k in range(5):
        a = -math.pi / 2 + 2 * math.pi * k / 5
        p = cc + np.array([math.cos(a), math.sin(a)]) * raggio
        zz = z(*p)
        verso = -np.array([math.cos(a), math.sin(a), 0.0])        # guarda il centro
        lato = np.array([-verso[1], verso[0], 0.0])
        alto = rng.uniform(240, 300)
        storta = rng.uniform(-0.06, 0.06)
        su = np.array([storta * lato[0], storta * lato[1], 1.0])
        su = su / np.linalg.norm(su)
        centro = np.array([p[0] * 100, p[1] * 100, zz - 30 + alto / 2])
        m.scatola("Pietra", list(centro), list(lato * 45), list(verso * 28), list(su * alto / 2))
        # la fascia di rune sulla faccia verso il centro: la parola della pietra, in colonna (l'elemento in cima)
        c = centro + su * (alto * 0.68 - alto / 2) + verso * 29.5
        _incidi(rune, c, verso, su, 11.0, PAROLE_PIETRE[k], verticale=True)
    # la pietra piatta al centro (altare basso) e il pentagono di rune
    zz = z(*cc)
    m.cilindro("Pietra", [cc[0] * 100, cc[1] * 100, zz - 30], 170, 65, lati=20)
    vertici = []
    for k in range(5):
        a = -math.pi / 2 + 2 * math.pi * k / 5
        vertici.append(np.array([cc[0] * 100 + math.cos(a) * 130, cc[1] * 100 + math.sin(a) * 130, zz + 35.5]))
    for k in range(5):
        _linea_runa(rune, vertici[k], vertici[(k + 2) % 5], 5.0)        # la stella
        _linea_runa(rune, vertici[k], vertici[(k + 1) % 5], 4.0)        # il giro
    for k in range(5):
        # l'elemento della pietra davanti alla punta, sulla pietra piatta, letto da chi sta al centro guardando fuori
        fuori_ = (vertici[k] - np.array([cc[0] * 100, cc[1] * 100, zz + 35.5]))
        fuori_ = fuori_ / np.linalg.norm(fuori_)
        _incidi(rune, vertici[k] + fuori_ * 22.0, np.array([0.0, 0.0, 1.0]), fuori_, 8.5, ar.ELEMENTI[k])
    pr.disegni.append((S, "cerchio", [R(*cc)], raggio * 100 + 100))

    # ---- il tumulo con l'ingresso della cripta, contro il muro di nord-est
    tc = np.array([11.0, -2.0])
    zz = z(*tc)
    _tumulo(m, tc[0] * 100, tc[1] * 100, zz, 520.0, 300.0, rng)
    # l'ingresso guarda a sud-ovest, verso il cerchio
    g = math.atan2(cc[1] - tc[1], cc[0] - tc[0])
    av = np.array([math.cos(g), math.sin(g), 0.0])
    la = np.array([-av[1], av[0], 0.0])
    fronte = np.array([tc[0] * 100, tc[1] * 100, zz]) + av * 430
    for s in (-1, 1):
        m.scatola("Pietra", list(fronte + la * s * 110 + [0, 0, 120]), list(av * 45), list(la * 30), [0, 0, 160])
    m.scatola("Pietra", list(fronte + [0, 0, 300]), list(av * 55), list(la * 150), [0, 0, 30])        # architrave
    m.scatola("Pietra", list(fronte - av * 25 + [0, 0, 110]), list(av * 12), list(la * 80), [0, 0, 120])  # la lastra
    # Ur sull'architrave, da sola (la runa del custode); sulla lastra le tre parole che fermano
    _incidi(rune, fronte + av * 56 + [0, 0, 300], av, np.array([0, 0, 1.0]), 19.0, "Ur")
    for j, parola in enumerate(PAROLE_LASTRA):
        _incidi(rune, fronte - av * 12.5 + [0, 0, 170 - j * 55], av, np.array([0, 0, 1.0]), 11.0, parola)
    # tre gradini davanti
    for k in range(3):
        m.scatola("Pietra", list(fronte + av * (80 + k * 45) + [0, 0, -10 - k * 10]), list(av * 22), list(la * (110 - k * 5)), [0, 0, 18])
    pr.disegni.append((S, "edificio", pr.rettangolo(*R(*tc), 1100, 1100, math.degrees(g)), "Tumulo della cripta"))

    pr.al_suolo(S, "Rovine degli Antichi", m, cx, cy, q0, collisione=True)
    pr.al_suolo(S, "Rune delle rovine", rune, cx, cy, q0, collisione=False)
    pr.cose[-1]["rune"] = True

    # ---- le luci delle rune (di notte: le accende il C++, di giorno sono spente)
    for k, (x, y, h) in enumerate(((cc[0], cc[1], 150.0), (*(fronte[:2] / 100 + av[:2] * 1.5), 220.0))):
        X, Y = R(x, y)
        d = pr.aggiungi(S, "Luce delle rune %d" % k, "luce", X, Y, t.quota(X, Y) + h, intensita=6.0, raggio=1400.0,
                        colore=[0.35, 0.7, 1.0])
        d["rune"] = True

    # ---- le iscrizioni (si leggono con E)
    # (l'iscrizione ha una tavola invisibile che si tocca: la si mette sulla pietra piatta, al centro, letta da nord)
    X, Y = R(*cc)
    pr.aggiungi(S, "Iscrizione del cerchio", "iscrizione", X, Y, t.quota(X, Y), -90.0,
                titolo="Le rune del cerchio",
                avvisi=["Sulla pietra piatta, intorno alla stella, corrono segni che nessuno sa più leggere del tutto.",
                        "rune: Aeth Nir Dorn Sil Lume",
                        "Qualcuno, tanto tempo fa, ha inciso sotto le rune la parte che si capiva:",
                        "\"Qui stava l'Orso, il più grande dei Vecchi Dei, custode dei boschi e dei cicli.\"",
                        "\"Davanti agli eserciti del Senza Nome tenne la valle per sette giorni e sette notti,\"",
                        "\"finché gli Antichi compirono il loro rito. Sulla collina più alta cadde.\"",
                        "\"E la valle prese il suo nome.\"",
                        "Il resto è consumato dalla pioggia. I vecchi dicono che nelle notti di nebbia, tra gli alberi, si vede ancora un'ombra enorme."])
    # nel vano della porta, davanti alla lastra, letta da chi sta davanti al tumulo
    lx, ly = (fronte[:2] + av[:2] * 5) / 100
    X, Y = R(lx, ly)
    pr.aggiungi(S, "Iscrizione della lastra", "iscrizione", X, Y, t.quota(X, Y), math.degrees(g),
                titolo="La lastra della cripta",
                avvisi=["Una lastra di pietra chiude la scala che scende sotto le rovine. Non c'è maniglia, né serratura.",
                        "Sull'architrave, un segno solo:",
                        "rune: Ur",
                        "Sulla lastra, tre righe di due segni. Il secondo è sempre lo stesso: i vecchi lo chiamano Tor, la parola del muro.",
                        "rune: Lume-Tor Dorn-Tor Sil-Tor",
                        "Le rune incise sulla lastra e sull'architrave si accendono di notte, a tempo con un battito lento: due colpi e una pausa.",
                        "Spingere non serve. Qualunque cosa ci sia sotto, per ora resta chiusa."])
    # una zona senza erba vera intorno al cerchio e davanti al tumulo (il resto delle rovine resta nel bosco)
    pr.zone_senza_erba.append((*R(*cc), raggio * 100 + 250, raggio * 100 + 250))
    print("rovine: cinta %d x %d m, 8 colonne, 5 pietre, tumulo della cripta" % (W, D))


def _runa(m, centro, lato, su, misura, seme):
    """Un segno di rune su una faccia: tre o quattro tratti sottili (materiale Runa), un poco staccati dalla pietra."""
    r = np.random.default_rng(1000 + seme)
    c = np.asarray(centro, np.float64)
    lato = np.asarray(lato, np.float64)
    su = np.asarray(su, np.float64)
    normale = np.cross(lato, su)
    normale = normale / (np.linalg.norm(normale) + 1e-9)
    # l'asta verticale sempre, poi due o tre rami
    tratti = [((0.0, -1.0), (0.0, 1.0))]
    for _ in range(r.integers(2, 4)):
        y0 = r.uniform(-0.8, 0.8)
        y1 = y0 + r.choice([-0.6, 0.6])
        tratti.append(((0.0, y0), (r.choice([-0.7, 0.7]), y1)))
    for (a, b) in tratti:
        A = c + lato * a[0] * misura + su * a[1] * misura
        B = c + lato * b[0] * misura + su * b[1] * misura
        _tratto(m, A, B, normale, 2.2)


def _incidi(m, centro, normale, su, misura, parola, verticale=False, largo=None):
    """Incide una parola di rune vere (alfabeto_rune.py) su una faccia di pietra, nel materiale Runa."""
    n = np.asarray(normale, np.float64)
    spessore = largo if largo is not None else max(1.6, misura * 0.2)
    ar.incidi(lambda a, b: _tratto(m, a, b, n, spessore), centro, n, su, misura, parola, verticale)


def _linea_runa(m, a, b, largo):
    a, b = np.asarray(a, np.float64), np.asarray(b, np.float64)
    _tratto(m, a, b, np.array([0.0, 0.0, 1.0]), largo)


def _tratto(m, a, b, normale, largo):
    d = b - a
    L = np.linalg.norm(d)
    if L < 1e-6:
        return
    u = d / L
    w = np.cross(normale, u)
    w = w / (np.linalg.norm(w) + 1e-9) * largo / 2
    off = normale * 0.6
    m.quad("Runa", a - w + off, b - w + off, b + w + off, a + w + off, normale=normale)


def _tumulo(m, x, y, z, raggio, alto, rng, lati=18, anelli=6):
    """Una collinetta di terra e pietre (mezza goccia schiacciata): la cripta sta sotto."""
    P, N, UV, T = [], [], [], []
    for i in range(anelli + 1):
        f = i / anelli
        r = raggio * math.cos(f * math.pi / 2)
        h = alto * math.sin(f * math.pi / 2)
        for k in range(lati + 1):
            a = 2 * math.pi * k / lati
            sporgi = 1.0 + (0.06 * math.sin(a * 3 + i) if 0 < i < anelli else 0.0)
            P.append([x + math.cos(a) * r * sporgi, y + math.sin(a) * r * sporgi, z - 60 + h])
            n = np.array([math.cos(a) * math.cos(f * math.pi / 2), math.sin(a) * math.cos(f * math.pi / 2), math.sin(f * math.pi / 2)])
            N.append(list(n))
            UV.append([k / lati * 6, f * 3])
    for i in range(anelli):
        for k in range(lati):
            a = i * (lati + 1) + k
            b = a + lati + 1
            T += [[a, a + 1, b], [a + 1, b + 1, b]]
    m.triangoli("Pietra", P, N, UV, T)
