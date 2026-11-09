# Valdorso - I pericoli fatti da zero, provvisori (v0.1.4, 09/11/2026, chiesto da Fra: "fai tu, dopo si cambiano").
# Scritto da Claude. Tre cose, finche' non arrivano quelle di Fab (poi si cambia solo il modello, il resto resta):
#   il lupo a pezzi: SM_Lupo_Corpo, SM_Lupo_Testa, SM_Lupo_Zampa (la stessa per le quattro), SM_Lupo_Coda. Ogni pezzo ha
#     l'origine dove si piega (la spalla, l'anca, il collo, l'attacco della coda): il C++ (ValdorsoLupo.h) li muove
#     mentre il lupo cammina, corre, morde e cade. Il lupo guarda +X. Il pelo e' nel colore dei vertici (grigio, il
#     dorso piu' scuro, la pancia e il muso piu' chiari, il naso nero), materiale Canna (bianco x colore dei vertici);
#   le armi: SM_Arma_<Id> per gli oggetti dello zaino che si impugnano (coltello, pugnale, falcetto, martello, bastone
#     da pastore, spada corta, scudo di legno). L'impugnatura sta nell'origine, la lama verso +X; lo scudo ha la
#     maniglia nell'origine e la faccia verso -Y. Legno (Trave/Assi), ferro (Canna grigia: niente metallo vero per ora);
#   i suoni dei pericoli (WAV, fatti con i numeri): i colpi (S_Colpo_Pugno, _Lama, _Botta, _Morso, _Scudo), il lupo
#     (S_Lupo_Ringhio, S_Lupo_Ululato, S_Lupo_Guaito), l'acqua (S_Bolle, S_Respiro).
# Uso: python pericoli.py <uscita>   (poi pericoli.zip in V:/Texture/valdorso, lo legge crea_pericoli.py)

import math
import os
import sys
import wave

import numpy as np

from villaggio_mesh import Mesh, normalizza

RNG_SEME = 909

GRIGIO = np.array([0.42, 0.40, 0.37])
DORSO = np.array([0.24, 0.22, 0.20])
PANCIA = np.array([0.72, 0.68, 0.60])
NERO = np.array([0.05, 0.05, 0.05])


# ------------------------------------------------------------------------------------------------ forme

def tubo(m, mat, centri, raggi_y, raggi_z, colore, lati=12, chiudi=(True, True)):
    """Un tubo lungo una linea (lungo X di solito): sezioni ellittiche, colore per vertice (funzione di punto e normale)."""
    P, N, UV, C, T = [], [], [], [], []
    for i, c in enumerate(centri):
        c = np.asarray(c, np.float64)
        d = np.asarray(centri[min(i + 1, len(centri) - 1)], np.float64) - np.asarray(centri[max(i - 1, 0)], np.float64)
        a = normalizza(d)
        e1 = normalizza(np.cross(a, [0, 0, 1]) if abs(a[2]) < 0.95 else np.cross(a, [1, 0, 0]))
        e2 = np.cross(e1, a)
        for k in range(lati + 1):
            ang = 2 * math.pi * k / lati
            n = e1 * math.cos(ang) * raggi_z[i] + e2 * math.sin(ang) * raggi_y[i]
            p = c + e1 * math.cos(ang) * raggi_y[i] + e2 * math.sin(ang) * raggi_z[i]
            nn = normalizza(e1 * math.cos(ang) / max(raggi_y[i], 0.1) + e2 * math.sin(ang) / max(raggi_z[i], 0.1))
            P.append(p)
            N.append(nn)
            UV.append([k / lati, i / max(1, len(centri) - 1)])
            C.append(list(colore(p, nn)) + [1.0])
    for i in range(len(centri) - 1):
        for k in range(lati):
            a0 = i * (lati + 1) + k
            b0 = a0 + lati + 1
            T += [[a0, b0, a0 + 1], [a0 + 1, b0, b0 + 1]]
    m.triangoli(mat, P, N, UV, T, C)
    for fine, idx in ((0, 0), (1, len(centri) - 1)):
        if not chiudi[fine]:
            continue
        c = np.asarray(centri[idx], np.float64)
        anello = P[idx * (lati + 1): idx * (lati + 1) + lati]
        d = normalizza(np.asarray(centri[-1], np.float64) - np.asarray(centri[0], np.float64)) * (1 if fine else -1)
        col = list(colore(c, d)) + [1.0]
        Tc = [[0, k + 1, (k + 1) % lati + 1] for k in range(lati)]
        m.triangoli(mat, [c] + anello, [d] * (lati + 1), [[0.5, 0.5]] * (lati + 1), Tc, [col] * (lati + 1))


def colore_pelo(dorso_z=0.0, scala=1.0):
    def f(p, n):
        # il dorso (normale in su) piu' scuro, la pancia (normale in giu') chiara, con un po' di variazione
        su = n[2]
        c = GRIGIO.copy()
        if su > 0.2:
            c = c * (1 - (su - 0.2)) + DORSO * (su - 0.2)
        elif su < -0.3:
            c = c * (1 + (su + 0.3)) + PANCIA * (-(su + 0.3))
        rumore = 0.92 + 0.16 * (math.sin(p[0] * 0.37 + p[1] * 0.71) * 0.5 + 0.5)
        return np.clip(c * rumore, 0, 1)
    return f


def lupo():
    pezzi = {}
    # il corpo: da -55 (groppa) a +48 (petto), piu' alto alle spalle
    m = Mesh("SM_Lupo_Corpo")
    xs = np.linspace(-55, 48, 12)
    centri, ry, rz = [], [], []
    for x in xs:
        t = (x + 55) / 103.0
        alto = 14 + 6 * math.sin(math.pi * min(1.0, t * 1.1))
        centri.append([x, 0, 1.5 * math.sin(math.pi * t) + (2.0 if t > 0.7 else 0.0)])
        ry.append(9 + 6 * math.sin(math.pi * t) ** 0.6)
        rz.append(alto * (0.75 if t < 0.15 or t > 0.95 else 1.0))
    tubo(m, "Canna", centri, ry, rz, colore_pelo(), lati=14)
    # la gorgiera chiara sul petto
    pezzi["SM_Lupo_Corpo"] = m

    # la testa: dal collo (origine) in avanti e in su, il muso, le orecchie, il naso
    m = Mesh("SM_Lupo_Testa")
    centri = [[0, 0, 0], [10, 0, 6], [20, 0, 11], [28, 0, 13], [36, 0, 12], [44, 0, 9], [52, 0, 7], [58, 0, 6]]
    ry = [10, 9.5, 8.5, 8.5, 7.5, 5, 3.8, 2.8]
    rz = [11, 10, 9, 9, 7.5, 5, 4, 3]

    def col_testa(p, n):
        c = colore_pelo()(p, n)
        if p[0] > 40:
            c = c * 0.6 + PANCIA * 0.4   # il muso piu' chiaro
        if p[0] > 56:
            c = NERO                     # il naso
        return c
    tubo(m, "Canna", centri, ry, rz, col_testa, lati=12)
    for lato in (-1, 1):
        base = np.array([24, lato * 6.0, 19.0])
        cima = base + np.array([-3, lato * 2.5, 11])
        a = base + np.array([3, 0, 0])
        b = base + np.array([-4, lato * 3.0, -1])
        c2 = base + np.array([-4, -lato * 1.0, 0])
        col = list(DORSO * 0.9) + [1.0]
        for tri in ((a, b, cima), (b, c2, cima), (c2, a, cima)):
            nrm = normalizza(np.cross(tri[1] - tri[0], tri[2] - tri[0]))
            m.triangoli("Canna", list(tri), [nrm] * 3, [[0, 0], [1, 0], [0.5, 1]], [[0, 1, 2]], [col] * 3)
    # gli occhi: due puntini scuri
    for lato in (-1, 1):
        m.cilindro("Canna", [38, lato * 6.2, 15], 1.2, 0.5, lati=6, asse=(0, lato, 0))
    pezzi["SM_Lupo_Testa"] = m

    # la zampa: dall'origine (la spalla o l'anca) giu' fino a terra (-56), con il gomito un po' indietro e la zampa
    m = Mesh("SM_Lupo_Zampa")
    centri = [[0, 0, 0], [1, 0, -12], [-2, 0, -26], [0, 0, -40], [1.5, 0, -51], [4, 0, -55]]
    ry = [6.5, 5.5, 4, 3.2, 3.0, 4.2]
    rz = [6.5, 5.5, 4, 3.2, 3.0, 2.0]
    tubo(m, "Canna", centri, ry, rz, colore_pelo(), lati=10)
    pezzi["SM_Lupo_Zampa"] = m

    # la coda: folta, all'indietro e in giu'
    m = Mesh("SM_Lupo_Coda")
    centri = [[0, 0, 0], [-8, 0, -4], [-18, 0, -12], [-28, 0, -20], [-38, 0, -26], [-46, 0, -29]]
    r = [3.5, 5, 7, 7.5, 6, 2]

    def col_coda(p, n):
        c = colore_pelo()(p, n)
        return DORSO * 0.8 if p[0] < -40 else c
    tubo(m, "Canna", centri, r, r, col_coda, lati=10)
    pezzi["SM_Lupo_Coda"] = m
    return pezzi


# ------------------------------------------------------------------------------------------------ armi

LEGNO = (0.42, 0.29, 0.17)
CUOIO = (0.30, 0.18, 0.10)
FERRO = (0.52, 0.53, 0.55)
CORNO = (0.80, 0.74, 0.62)


def _colorato(m, colore, disegna):
    tmp = Mesh("tmp")
    disegna(tmp)
    for mat, parte in tmp.parti.items():
        inizio = 0
        for P, N, UV, C, T in zip(*[parte[k] for k in range(5)]):
            m.triangoli(mat, P, N, UV, np.asarray(T) - inizio, [list(colore) + [1.0]] * len(P))
            inizio += len(P)


def lama(m, da, a, largo, spesso, punta=0.25, colore=FERRO):
    """Una lama piatta lungo X (da 'da' ad 'a'), larga in Z, sottile in Y, che finisce a punta."""
    def d(t):
        L = a - da
        x0 = da + L * 0.0
        xp = a - L * punta
        P = []
        # contorno: sopra, punta, sotto
        sopra = [np.array([x0, 0, largo / 2]), np.array([xp, 0, largo / 2]), np.array([a, 0, largo * 0.05])]
        sotto = [np.array([x0, 0, -largo / 2]), np.array([xp, 0, -largo / 2 * 0.7]), np.array([a, 0, -largo * 0.05])]
        for s in (-1, 1):
            n = np.array([0, s, 0.0])
            off = np.array([0, s * spesso / 2, 0])
            for i in range(2):
                quad = [sotto[i] + off, sotto[i + 1] + off, sopra[i + 1] + off, sopra[i] + off]
                t.quad("Canna", *quad, normale=n)
        # il filo di sopra e di sotto (lo spessore)
        for lato in (sopra, sotto):
            for i in range(2):
                a1, b1 = lato[i], lato[i + 1]
                t.quad("Canna", a1 + [0, -spesso / 2, 0], b1 + [0, -spesso / 2, 0], b1 + [0, spesso / 2, 0], a1 + [0, spesso / 2, 0],
                       normale=normalizza(np.array([0, 0, 1.0 if lato is sopra else -1.0])))
    _colorato(m, colore, d)


def manico(m, da, a, r, colore=LEGNO, mat="Canna"):
    _colorato(m, colore, lambda t: t.cilindro(mat, [da, 0, 0], r, a - da, lati=8, asse=(1, 0, 0)))


def arma(nome):
    m = Mesh("SM_Arma_" + nome)
    if nome == "Coltello":
        manico(m, -10, 1, 1.4, CORNO)
        lama(m, 1, 14, 2.4, 0.4)
    elif nome == "Pugnale":
        manico(m, -10, 1, 1.5, CUOIO)
        _colorato(m, FERRO, lambda t: t.blocco("Canna", 1, 2.2, -1.2, 1.2, -5, 5))
        lama(m, 2.2, 26, 3.4, 0.5, punta=0.35)
    elif nome == "Falcetto":
        manico(m, -12, 1, 1.6, LEGNO)
        # la lama curva: tanti pezzetti lungo un arco
        pts = [np.array([1 + 12 * math.sin(a), 0, 12 - 12 * math.cos(a) * 0.9]) for a in np.linspace(0, 2.4, 9)]
        for i in range(len(pts) - 1):
            _colorato(m, FERRO, lambda t, i=i: t.trave("Canna", pts[i], pts[i + 1], 2.4, 0.4, su=(0, 1, 0)))
    elif nome == "Martello":
        manico(m, -14, 26, 1.6, LEGNO)
        _colorato(m, FERRO, lambda t: t.blocco("Canna", 21, 29, -2.6, 2.6, -3, 9))
    elif nome == "Bastone_Pastore":
        manico(m, -60, 100, 1.8, LEGNO)
        # il manico ricurvo in cima
        pts = [np.array([100 + 8 * math.sin(a), 0, 8 - 8 * math.cos(a)]) for a in np.linspace(0, math.pi, 8)]
        for i in range(len(pts) - 1):
            _colorato(m, LEGNO, lambda t, i=i: t.trave("Canna", pts[i], pts[i + 1], 3.4, 3.4, su=(0, 1, 0)))
    elif nome == "Spada_Corta":
        manico(m, -12, 0, 1.6, CUOIO)
        _colorato(m, FERRO, lambda t: t.cilindro("Canna", [-13.5, 0, 0], 2.4, 2, lati=8, asse=(1, 0, 0)))   # il pomo
        _colorato(m, FERRO, lambda t: t.blocco("Canna", 0, 2, -1.5, 1.5, -9, 9))                              # la guardia
        lama(m, 2, 62, 4.4, 0.6, punta=0.15)
    elif nome == "Scudo_Legno":
        # tondo, 60 cm, di assi con il bordo e l'umbone di ferro; la maniglia nell'origine, la faccia verso -Y
        def faccia(t):
            lati = 20
            P = [np.array([0, -6, 0])] + [np.array([30 * math.cos(2 * math.pi * k / lati), -6, 30 * math.sin(2 * math.pi * k / lati)]) for k in range(lati)]
            UV = [[0.5, 0.5]] + [[0.5 + 0.5 * math.cos(2 * math.pi * k / lati), 0.5 + 0.5 * math.sin(2 * math.pi * k / lati)] for k in range(lati)]
            t.triangoli("Assi", P, [np.array([0, -1.0, 0])] * len(P), UV, [[0, k + 1, (k + 1) % lati + 1] for k in range(lati)])
            P2 = [p + np.array([0, 2.0, 0]) for p in P]
            t.triangoli("Assi", P2, [np.array([0, 1.0, 0])] * len(P2), UV, [[0, (k + 1) % lati + 1, k + 1] for k in range(lati)])
        faccia(m)
        _colorato(m, FERRO, lambda t: t.cilindro("Canna", [0, -4, 0], 30.5, 2.2, lati=20, asse=(0, -1, 0), tappo=False))
        _colorato(m, FERRO, lambda t: t.cilindro("Canna", [0, -6, 0], 7, 3.5, lati=12, asse=(0, -1, 0), raggio_alto=4))
        manico(m, -6, 6, 1.3, CUOIO)
    return m


ARMI = ["Coltello", "Pugnale", "Falcetto", "Martello", "Bastone_Pastore", "Spada_Corta", "Scudo_Legno"]


# ------------------------------------------------------------------------------------------------ suoni

FREQ = 44100


def _scrivi(cartella, nome, x, picco=0.85):
    x = np.asarray(x, np.float64)
    x = x / (np.max(np.abs(x)) + 1e-9) * picco
    with wave.open(os.path.join(cartella, nome + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(FREQ)
        w.writeframes((x * 32767).astype(np.int16).tobytes())


def _env(n, attacco, caduta):
    t = np.arange(n) / FREQ
    return np.minimum(1.0, t / max(attacco, 1e-4)) * np.exp(-t / max(caduta, 1e-4))


def _filtra(x, basso, alto):
    f = np.fft.rfftfreq(len(x), 1 / FREQ)
    X = np.fft.rfft(x)
    X[(f < basso) | (f > alto)] = 0
    return np.fft.irfft(X, len(x))


def suoni(cartella, rng):
    n = lambda s: int(FREQ * s)
    # pugno: un tonfo sordo
    x = _filtra(rng.standard_normal(n(0.25)), 40, 900) * _env(n(0.25), 0.002, 0.04)
    x += np.sin(2 * np.pi * 85 * np.arange(n(0.25)) / FREQ) * _env(n(0.25), 0.001, 0.05) * 0.8
    _scrivi(cartella, "S_Colpo_Pugno", x)
    # lama: un fendente (fruscio che sale) e il taglio
    t = np.arange(n(0.35)) / FREQ
    fr = _filtra(rng.standard_normal(n(0.35)), 1500, 7000) * np.exp(-((t - 0.08) / 0.05) ** 2)
    taglio = _filtra(rng.standard_normal(n(0.35)), 300, 3000) * _env(n(0.35), 0.001, 0.05) * np.roll(np.ones(n(0.35)), 0) * (t > 0.1)
    _scrivi(cartella, "S_Colpo_Lama", fr * 0.6 + taglio)
    # botta: legno pieno
    x = _filtra(rng.standard_normal(n(0.3)), 60, 1500) * _env(n(0.3), 0.001, 0.06)
    x += np.sin(2 * np.pi * 140 * np.arange(n(0.3)) / FREQ) * _env(n(0.3), 0.001, 0.08)
    _scrivi(cartella, "S_Colpo_Botta", x)
    # morso: denti che schioccano e un ringhio corto
    t = np.arange(n(0.4)) / FREQ
    schiocco = _filtra(rng.standard_normal(n(0.4)), 800, 6000) * _env(n(0.4), 0.0005, 0.015)
    ringhio = np.sin(2 * np.pi * (95 + 20 * np.sin(2 * np.pi * 31 * t)) * t) * (0.5 + 0.5 * _filtra(rng.standard_normal(n(0.4)), 20, 80)) * _env(n(0.4), 0.02, 0.15)
    _scrivi(cartella, "S_Colpo_Morso", schiocco + ringhio * 0.5)
    # scudo: legno cavo e ferro che vibra
    t = np.arange(n(0.6)) / FREQ
    x = _filtra(rng.standard_normal(n(0.6)), 80, 1200) * _env(n(0.6), 0.001, 0.05)
    x += (np.sin(2 * np.pi * 620 * t) + 0.6 * np.sin(2 * np.pi * 1340 * t)) * _env(n(0.6), 0.001, 0.25) * 0.4
    _scrivi(cartella, "S_Colpo_Scudo", x)
    # il lupo che ringhia (2 s, in giro)
    t = np.arange(n(2.0)) / FREQ
    mod = 0.6 + 0.4 * np.sin(2 * np.pi * 0.7 * t)
    x = np.sin(2 * np.pi * (80 + 15 * np.sin(2 * np.pi * 27 * t)) * t) * _filtra(rng.standard_normal(n(2.0)), 15, 60) * 4
    x = x * mod + _filtra(rng.standard_normal(n(2.0)), 100, 600) * 0.2 * mod
    _scrivi(cartella, "S_Lupo_Ringhio", x, 0.7)
    # l'ululato: una nota che sale, tiene e scende, con un po' di vibrato e il fiato
    durata = 4.5
    t = np.arange(n(durata)) / FREQ
    f = 380 + 260 * np.clip(t / 0.8, 0, 1) - 180 * np.clip((t - 3.0) / 1.5, 0, 1) + 6 * np.sin(2 * np.pi * 5.5 * t)
    fase = 2 * np.pi * np.cumsum(f) / FREQ
    voce = np.sin(fase) + 0.35 * np.sin(2 * fase) + 0.12 * np.sin(3 * fase)
    inv = np.clip(t / 0.4, 0, 1) * np.clip((durata - t) / 0.9, 0, 1)
    fiato = _filtra(rng.standard_normal(len(t)), 500, 3000) * 0.08
    x = (voce * 0.7 + fiato) * inv
    # l'eco della valle
    eco = np.zeros_like(x)
    ritardo = int(0.45 * FREQ)
    eco[ritardo:] = x[:-ritardo] * 0.3
    _scrivi(cartella, "S_Lupo_Ululato", x + eco, 0.8)
    # il guaito di chi scappa
    t = np.arange(n(0.6)) / FREQ
    f = 900 - 500 * t / 0.6
    fase = 2 * np.pi * np.cumsum(f) / FREQ
    _scrivi(cartella, "S_Lupo_Guaito", (np.sin(fase) + 0.3 * np.sin(2 * fase)) * _env(n(0.6), 0.01, 0.25))
    # le bolle sott'acqua (in giro)
    x = np.zeros(n(3.0))
    for _ in range(40):
        k = int(rng.uniform(0, n(2.9)))
        L = int(rng.uniform(0.02, 0.06) * FREQ)
        ff = rng.uniform(300, 900)
        tt = np.arange(L) / FREQ
        bolla = np.sin(2 * np.pi * ff * (1 + 3 * tt) * tt) * np.exp(-tt / 0.015)
        x[k:k + L] += bolla[: max(0, min(L, len(x) - k))] * rng.uniform(0.3, 1.0)
    x += _filtra(rng.standard_normal(len(x)), 50, 300) * 0.15
    _scrivi(cartella, "S_Bolle", x, 0.5)
    # il respiro a pieni polmoni quando si riemerge
    t = np.arange(n(0.9)) / FREQ
    x = _filtra(rng.standard_normal(len(t)), 300, 2500) * np.clip(t / 0.15, 0, 1) * np.exp(-np.clip(t - 0.3, 0, None) / 0.2)
    _scrivi(cartella, "S_Respiro", x, 0.6)


def main(uscita):
    os.makedirs(uscita, exist_ok=True)
    rng = np.random.default_rng(RNG_SEME)
    for nome, m in lupo().items():
        m.scrivi(os.path.join(uscita, nome + ".glb"))
        print(nome, m.conta(), "triangoli")
    for nome in ARMI:
        m = arma(nome)
        m.scrivi(os.path.join(uscita, m.nome + ".glb"))
        lo, hi = m.scatola_limiti()
        print(m.nome, m.conta(), "triangoli", lo.round(), hi.round())
    suoni(uscita, rng)
    print("suoni fatti")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "uscita_pericoli")
