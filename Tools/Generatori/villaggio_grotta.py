# Valdorso - La grotta est e la porta degli Antichi (v0.1.3). Scritto da Claude il 06/11/2026.
# Dal piano (v0.1.3): "due grotte, la est esplorabile con la porta degli Antichi". Dalla lore: le gallerie sotto la
# collina scavate dai nani portano verso est fino alla sala rotonda dell'Archivio degli Antichi; la porta l'ha chiusa
# con Ur il protagonista del Capitolo I ("la porta che nessuno ha mai aperto"): nessuna parola sola la riapre,
# servono cinque voci insieme (fiamma, onda, monte, piuma, sole) e una mano al centro che tiene il tempo.
#
# Com'è fatta (metri dal luogo "Grotta est", x est, y sud): una rupe di roccia ai piedi dei monti di est, lunga una
# cinquantina di metri lungo il pendio. Dentro:
#   - la bocca a sud-ovest, vicino a dove arriva la strada, con gli scalini di roccia;
#   - la grotta naturale (4-6 m di larghezza), che gira verso nord seguendo il pendio, con il muschio che fa luce;
#   - l'arco squadrato dei nani e il cunicolo scalpellato (3 m), con i loro segni sull'arco;
#   - l'anticamera rotonda (10 m) con il pavimento di lastre e in fondo la porta degli Antichi: il cerchio dei cinque
#     posti e la mano aperta incisi sulla porta, Ur da solo sull'architrave. Le rune della porta battono sempre (due
#     colpi e una pausa), giorno e notte: le accende AValdorsoRuneNotte con l'etichetta "ValdorsoRuneSempre".
#   - due iscrizioni da leggere con E (l'arco dei nani, la porta).
# Perché una rupe e non un buco nella montagna: il paesaggio di Unreal non si buca senza dipingere la visibilità a
# mano; così la grotta sta tutta sopra il terreno (il pavimento è sempre almeno 35 cm sopra il paesaggio) e la rupe la
# copre. La parte della rupe sotto il pavimento scende un metro sotto il terreno.

import math

import numpy as np

import alfabeto_rune as ar
import villaggio_rovine as rv
from villaggio_mesh import Mesh, normalizza

S = "Grotta"
TILE_ROCCIA = 400.0


# --------------------------------------------------------------------------------------------------- il rumore

def _hash(i, j, k, seme):
    v = np.sin(i * 127.1 + j * 311.7 + k * 74.7 + seme * 19.19) * 43758.5453
    return v - np.floor(v)


def rumore3(P, scala, seme=0):
    """Rumore 'a valori' liscio in 3D (-1..1), P in cm (array N x 3), scala in cm."""
    Q = np.asarray(P, np.float64) / scala
    i0 = np.floor(Q)
    f = Q - i0
    f = f * f * (3 - 2 * f)
    tot = 0.0
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                w = ((f[:, 0] if dx else 1 - f[:, 0]) * (f[:, 1] if dy else 1 - f[:, 1]) * (f[:, 2] if dz else 1 - f[:, 2]))
                tot = tot + w * _hash(i0[:, 0] + dx, i0[:, 1] + dy, i0[:, 2] + dz, seme)
    return tot * 2 - 1


def frastaglio(P, scala, seme):
    return rumore3(P, scala, seme) * 0.65 + rumore3(P, scala * 0.4, seme + 7) * 0.25 + rumore3(P, scala * 0.15, seme + 13) * 0.10


def smussa(x, a, b):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


# --------------------------------------------------------------------------------------------------- il percorso

def _catmull(C, passo):
    """Punti lungo una Catmull-Rom per i punti C (m), uno ogni 'passo' m circa (ricampionati a passo costante)."""
    C = [np.asarray(c, np.float64) for c in C]
    C = [C[0] * 2 - C[1]] + C + [C[-1] * 2 - C[-2]]
    fitti = []
    for i in range(1, len(C) - 2):
        p0, p1, p2, p3 = C[i - 1], C[i], C[i + 1], C[i + 2]
        for t in np.linspace(0, 1, 40, endpoint=False):
            fitti.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t
                                + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
    fitti.append(C[-2])
    fitti = np.array(fitti)
    d = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(fitti, axis=0), axis=1))])
    L = d[-1]
    s = np.linspace(0, L, int(L / passo) + 1)
    return np.column_stack([np.interp(s, d, fitti[:, 0]), np.interp(s, d, fitti[:, 1])]), s


def _normali_griglia(G, verso_fuori):
    """Normali di una griglia (stazioni x anello x 3); verso_fuori(i, j) dà la direzione di riferimento."""
    du = np.gradient(G, axis=1)
    ds = np.gradient(G, axis=0)
    N = np.cross(ds, du)
    N /= (np.linalg.norm(N, axis=2, keepdims=True) + 1e-9)
    for i in range(G.shape[0]):
        for j in range(G.shape[1]):
            if np.dot(N[i, j], verso_fuori(i, j)) < 0:
                N[i, j] = -N[i, j]
    return N


def _griglia(m, mat, G, N, UV, chiudi=False):
    ns, nr = G.shape[:2]
    P = G.reshape(-1, 3)
    T = []
    for i in range(ns - 1):
        for j in range(nr - 1):
            a = i * nr + j
            T += [[a, a + 1, a + nr], [a + 1, a + nr + 1, a + nr]]
    m.triangoli(mat, P, N.reshape(-1, 3), UV.reshape(-1, 2), T)


def _griglia_tri(m, mat, G, N, tile):
    """(06/11, sera) Come _griglia, ma con le UV "a tre piani": ogni triangolo prende la texture dal piano più vicino
    al suo (dall'alto, da est o da sud), così la roccia non si stira mai (prima, vicino alla bocca, faceva strisce).
    Le normali restano quelle lisce della griglia; i vertici non si condividono (le cuciture la roccia le nasconde)."""
    ns, nr = G.shape[:2]
    P = G.reshape(-1, 3)
    Nn = N.reshape(-1, 3)
    T = []
    for i in range(ns - 1):
        for j in range(nr - 1):
            a = i * nr + j
            T += [[a, a + 1, a + nr], [a + 1, a + nr + 1, a + nr]]
    T = np.array(T)
    A, B, C = P[T[:, 0]], P[T[:, 1]], P[T[:, 2]]
    fn = np.abs(np.cross(B - A, C - A))
    asse = np.argmax(fn, axis=1)
    PP = P[T].reshape(-1, 3)
    NN = Nn[T].reshape(-1, 3)
    asse3 = np.repeat(asse, 3)
    U = np.where(asse3 == 0, PP[:, 1], PP[:, 0])
    V = np.where(asse3 == 2, PP[:, 1], -PP[:, 2])          # sulle pareti la v scende con la quota: strati orizzontali
    UV = np.stack([U / tile, V / tile], -1)
    m.triangoli(mat, PP, NN, UV, np.arange(len(PP)).reshape(-1, 3))


def _uv(G, tile):
    """UV: u lungo l'anello (lunghezza vera), v lungo il percorso."""
    ns, nr = G.shape[:2]
    u = np.zeros((ns, nr))
    u[:, 1:] = np.cumsum(np.linalg.norm(np.diff(G, axis=1), axis=2), axis=1)
    v = np.zeros((ns, nr))
    v[1:, :] = np.cumsum(np.linalg.norm(np.diff(G, axis=0), axis=2), axis=0)
    return np.stack([u / tile, v / tile], -1)


def _ventaglio(m, mat, anello, centro, normale, tile):
    """Chiude un anello con un ventaglio di triangoli verso il centro."""
    P = [centro] + list(anello) + [anello[0]]
    asse_u = normalizza(np.cross(normale, [0, 0, 1.0]))
    if np.linalg.norm(asse_u) < 1e-6:
        asse_u = np.array([1.0, 0, 0])
    asse_v = np.cross(normale, asse_u)
    UV = [[np.dot(p - centro, asse_u) / tile, np.dot(p - centro, asse_v) / tile] for p in P]
    T = [[0, k + 1, k + 2] for k in range(len(anello))]
    m.triangoli(mat, P, [normale] * len(P), UV, T)


# --------------------------------------------------------------------------------------------------- la grotta

def grotta(pr, rng):
    t = pr.t
    gx, gy, _ = t.luogo("Grotta est")
    q0 = t.quota(gx, gy)

    def zt(x, y):
        """Quota del terreno (cm, rispetto al luogo) nel punto locale (cm)."""
        return t.quota(gx + x, gy + y) - q0

    # ---- il percorso (m): la bocca a sud-ovest (lungo il pendio: così gli scalini arrivano presto a terra), poi a nord
    controllo = [(2.4, 8.5), (3.6, 3.0), (5.2, -4.8), (5.0, -11.5), (5.7, -18.5), (5.2, -25.5), (5.6, -31.5),
                 (6.0, -37.5), (6.3, -43.5)]
    xy, s = _catmull(controllo, 0.5)
    xy *= 100.0
    s *= 100.0
    L = s[-1]
    ns = len(s)
    T = np.zeros((ns, 3))
    T[:, :2] = np.gradient(xy, axis=0)
    T /= np.linalg.norm(T, axis=1, keepdims=True)
    Lat = np.column_stack([-T[:, 1], T[:, 0], np.zeros(ns)])      # a destra di chi entra

    # ---- le misure lungo il percorso (cm): grotta naturale, cunicolo dei nani, anticamera rotonda
    sm = s / 100.0
    Lm = L / 100.0
    nani = smussa(sm, Lm - 24.0, Lm - 21.5)                       # 0 = roccia viva, 1 = scalpellato
    c_sala, r_sala = Lm - 3.6, 5.2
    sala = np.sqrt(np.clip(r_sala ** 2 - (sm - c_sala) ** 2, 0, None)) * 2       # larghezza del tondo
    w_nat = 4.7 + 0.7 * np.sin(sm * 0.45) + 1.6 * (1 - smussa(sm, 0, 4.5))
    h_nat = 3.9 + 0.5 * np.sin(sm * 0.33 + 1.0) + 0.9 * (1 - smussa(sm, 0, 4.5))
    w = (w_nat * (1 - nani) + 3.1 * nani)
    w = np.maximum(w, sala)
    insala = smussa(sm, c_sala - r_sala, c_sala - r_sala + 3.0)
    h = (h_nat * (1 - nani) + 3.45 * nani)
    h = np.maximum(h, 3.3 + 3.0 * insala)
    esp = 2.2 * (1 - nani) + 7.0 * nani * (1 - insala) + 2.6 * insala         # forma: 2 = volta tonda, 7 = squadrata
    amp = 50.0 * (1 - nani) + 5.0 * nani * (1 - insala) + 12.0 * insala
    w *= 100.0
    h *= 100.0

    # ---- il pavimento: sempre almeno 35 cm sopra il terreno sotto tutta la larghezza, pendenza al massimo 16%
    richiesto = np.zeros(ns)
    for i in range(ns):
        q = [zt(*(xy[i] + Lat[i, :2] * u)) for u in np.linspace(-w[i] / 2 - 60, w[i] / 2 + 60, 9)]
        richiesto[i] = max(q) + 35.0
    f = richiesto.copy()
    ds = 50.0
    for _ in range(4):
        for i in range(1, ns):
            f[i] = max(f[i], f[i - 1] - 0.16 * ds)
        for i in range(ns - 2, -1, -1):
            f[i] = max(f[i], f[i + 1] - 0.16 * ds)
        k = np.exp(-np.linspace(-2, 2, 9) ** 2)
        k /= k.sum()
        f = np.maximum(np.convolve(np.pad(f, 4, mode="edge"), k, mode="valid"), richiesto)
    # nella sala il pavimento è piano (lastre)
    piano = f[sm >= c_sala - r_sala].max() if np.any(sm >= c_sala - r_sala) else f[-1]
    f = np.where(sm >= c_sala - r_sala, piano, f)
    for i in range(ns - 2, -1, -1):
        if sm[i] < c_sala - r_sala:
            f[i] = max(f[i], f[i + 1] - 0.16 * ds)

    # ---- la parete di dentro: anello di K+1 punti (dal piede a destra, su per la volta, al piede a sinistra)
    K = 28
    phi = np.linspace(0, math.pi, K + 1)
    dentro = np.zeros((ns, K + 1, 3))
    centri = np.zeros((ns, K + 1, 3))
    for i in range(ns):
        e = esp[i]
        cu = -np.sign(np.cos(phi)) * np.abs(np.cos(phi)) ** (2 / e) * w[i] / 2
        cv = np.abs(np.sin(phi)) ** (2 / e) * h[i]
        base = np.array([xy[i, 0], xy[i, 1], f[i]])
        dentro[i] = base + Lat[i] * cu[:, None] + np.array([0, 0, 1.0]) * cv[:, None]
        centri[i] = base + np.array([0, 0, h[i] * 0.35])
    # il frastaglio: verso l'esterno (dal centro dell'anello), nullo ai piedi (si attacca al pavimento)
    piedi = np.abs(np.sin(phi)) ** 0.7
    rad = dentro - centri
    rad /= (np.linalg.norm(rad, axis=2, keepdims=True) + 1e-9)
    nz = frastaglio(dentro.reshape(-1, 3), 160.0, 3).reshape(ns, K + 1)
    dentro += rad * (nz * amp[:, None] * piedi[None, :])[..., None]
    Nd = _normali_griglia(dentro, lambda i, j: centri[i, j] - dentro[i, j])
    m = Mesh("SM_Grotta_Est")
    _griglia_tri(m, "Roccia", dentro, Nd, TILE_ROCCIA)

    # ---- il pavimento: terra nella grotta, lastre (Pietra) nel cunicolo e nella sala
    M = 6
    pav = np.zeros((ns, M + 1, 3))
    for i in range(ns):
        for k in range(M + 1):
            pav[i, k] = dentro[i, 0] + (dentro[i, K] - dentro[i, 0]) * k / M
    pz = frastaglio(pav.reshape(-1, 3), 120.0, 11).reshape(ns, M + 1) * (6.0 * (1 - nani))[:, None]
    bordo = np.zeros(M + 1)
    bordo[1:-1] = 1.0
    pav[..., 2] += pz * bordo[None, :]
    Np = np.zeros_like(pav)
    Np[..., 2] = 1.0
    taglio = int(np.searchsorted(sm, Lm - 23.0))
    _griglia(m, "Terra", pav[:taglio + 1], Np[:taglio + 1], _uv(pav[:taglio + 1], 400.0))
    _griglia(m, "Pietra", pav[taglio:], Np[taglio:], _uv(pav[taglio:], 200.0))

    # ---- la rupe di fuori: più larga e più alta, i piedi un metro sotto il terreno; a valle una parete quasi dritta,
    # a monte un fianco lungo che scende piano nel pendio. Le misure cambiano lungo la rupe (rumore lento), e sopra
    # c'è un frastaglio grosso (2 m), medio e fine: deve sembrare una rupe, non un tubo.
    lento = [frastaglio(np.array([[sm[i] * 100.0, 0.0, 77.0 * k]]), 900.0, 40 + k)[0] for i in range(ns) for k in range(3)]
    lento = np.array(lento).reshape(ns, 3)
    fuori = np.zeros((ns, K + 1, 3))
    for i in range(ns):
        a_sx = zt(*(xy[i] + Lat[i, :2] * 400))
        a_dx = zt(*(xy[i] - Lat[i, :2] * 400))
        monte = 1.0 if a_sx > a_dx else -1.0                       # +1: il monte è a sinistra
        giu = f[i] - min(a_sx, a_dx) + 100.0
        spess_alto = 260.0 + 200.0 * lento[i, 0]
        valle = 230.0 + 120.0 * lento[i, 1]
        monte_largo = 750.0 + 300.0 * lento[i, 2]
        for j, ph in enumerate(phi):
            lato = -math.cos(ph)                                   # -1 destra, +1 sinistra
            a_monte = lato * monte > 0
            largo = w[i] / 2 + (monte_largo if a_monte else valle)
            u = lato * largo
            # a valle la parete sale dritta (seno alla 0.6), a monte il fianco è più basso e lungo
            alto = h[i] + spess_alto + giu
            v = (abs(math.sin(ph)) ** (1.6 if a_monte else 0.6)) * alto - giu
            p = np.array([xy[i, 0], xy[i, 1], f[i]]) + Lat[i] * u + np.array([0, 0, v])
            fuori[i, j] = p
    asse = np.array([[xy[i, 0], xy[i, 1], f[i] + h[i] * 0.4] for i in range(ns)])
    radf = fuori - asse[:, None, :]
    radf /= (np.linalg.norm(radf, axis=2, keepdims=True) + 1e-9)
    nz1 = frastaglio(fuori.reshape(-1, 3), 650.0, 5).reshape(ns, K + 1)
    nz2 = frastaglio(fuori.reshape(-1, 3), 230.0, 9).reshape(ns, K + 1)
    nz3 = frastaglio(fuori.reshape(-1, 3), 85.0, 15).reshape(ns, K + 1)
    fuori += radf * (nz1 * 210.0 + nz2 * 70.0 + nz3 * 18.0)[..., None]
    # mai più vicina di 110 cm alla parete di dentro, misurando dal centro dell'anello (in 3D resta sopra i 40 cm)
    for i in range(ns):
        for j in range(K + 1):
            r_in = np.linalg.norm(dentro[i, j] - asse[i])
            d = fuori[i, j] - asse[i]
            r = np.linalg.norm(d)
            if r < r_in + 110.0:
                fuori[i, j] = asse[i] + d / (r + 1e-9) * (r_in + 110.0)
    # i piedi della rupe: sempre almeno 80 cm sotto il terreno dove stanno
    for i in range(ns):
        for j in (0, 1, K - 1, K):
            fuori[i, j, 2] = min(fuori[i, j, 2], zt(fuori[i, j, 0], fuori[i, j, 1]) - 80.0)
    Nf = _normali_griglia(fuori, lambda i, j: fuori[i, j] - asse[i])
    _griglia_tri(m, "Roccia", fuori, Nf, TILE_ROCCIA)

    # ---- la bocca: il labbro tra la parete di dentro e la rupe (tre anelli in mezzo, frastagliati verso fuori)
    anelli = [dentro[0]]
    for k, tt in enumerate((0.3, 0.6, 0.85)):
        a = dentro[0] + (fuori[0] - dentro[0]) * tt
        sp = frastaglio(a, 140.0, 21 + k)
        a = a - T[0] * (40.0 + 90.0 * math.sin(tt * math.pi) + 40.0 * sp)[:, None]
        anelli.append(a)
    anelli.append(fuori[0])
    labbro = np.array(anelli)
    Nl = np.tile(-T[0], (labbro.shape[0], labbro.shape[1], 1))
    _griglia_tri(m, "Roccia", labbro, Nl, TILE_ROCCIA)
    # sotto il pavimento della bocca: la faccia giù fino al terreno, e i due triangoli ai lati
    giu0 = min(zt(*dentro[0, 0, :2]), zt(*dentro[0, K, :2])) - 80.0
    a0, aK = dentro[0, 0], dentro[0, K]
    m.quad("Roccia", aK, a0, np.array([a0[0], a0[1], giu0]), np.array([aK[0], aK[1], giu0]), normale=-T[0])
    for piede, est in ((a0, fuori[0, 0]), (aK, fuori[0, K])):
        m.triangoli("Roccia", [piede, np.array([piede[0], piede[1], giu0]), est], [-T[0]] * 3,
                    [[0, 0], [0, 1], [1, 1]], [[0, 1, 2]])

    # ---- la fine: la parete della porta (dentro) e la cupola della rupe (fuori)
    fine_c = dentro[-1].mean(0)
    fine_c[2] = f[-1] + h[-1] * 0.45
    _ventaglio(m, "Roccia", dentro[-1], fine_c - T[-1] * 10.0, -T[-1], TILE_ROCCIA)
    cupola = fuori[-1].mean(0) + T[-1] * 350.0
    _ventaglio(m, "Roccia", fuori[-1], cupola, T[-1], TILE_ROCCIA)

    # ---- gli scalini di roccia davanti alla bocca, giù fino al terreno
    scalini = 0
    z = f[0]
    passo = 0
    while True:
        passo += 1
        centro = xy[0] - T[0, :2] * (passo * 55.0 - 10.0)
        terra = zt(*centro)
        z -= 24.0
        if z < terra + 12.0 or passo > 14:
            break
        gira = math.radians(rng.uniform(-7, 7))
        av0 = np.array([T[0, 0] * math.cos(gira) - T[0, 1] * math.sin(gira), T[0, 0] * math.sin(gira) + T[0, 1] * math.cos(gira), 0])
        la0 = np.array([-av0[1], av0[0], 0])
        c2 = centro + la0[:2] * rng.uniform(-25, 25)
        m.scatola("Roccia", [c2[0], c2[1], (z + terra - 60.0) / 2], list(av0 * rng.uniform(28, 36)),
                  list(la0 * rng.uniform(130.0, 175.0)), [0, 0, (z - terra + 60.0) / 2 + rng.uniform(0, 4)])
        scalini += 1

    # ---- l'arco dei nani all'inizio del cunicolo: stipiti e architrave squadrati (Pietra)
    ia = int(np.searchsorted(sm, Lm - 21.5))
    pa = np.array([xy[ia, 0], xy[ia, 1], f[ia]])
    for lato in (-1, 1):
        p = pa + Lat[ia] * lato * (w[ia] / 2 - 25.0)
        m.scatola("Pietra", list(p + np.array([0, 0, 135.0])), list(T[ia] * 45.0), list(Lat[ia] * 35.0), [0, 0, 135.0])
    m.scatola("Pietra", list(pa + np.array([0, 0, 300.0])), list(T[ia] * 50.0), list(Lat[ia] * (w[ia] / 2 + 10.0)),
              [0, 0, 30.0])
    rune = Mesh("SM_Grotta_Rune")
    segni_nani = Mesh("SM_Grotta_SegniNani")
    # i segni dei nani: tratti diritti e angoli (non rune degli Antichi), incisi in Ferro scuro sull'architrave
    for k in range(7):
        c = pa + np.array([0, 0, 300.0]) + Lat[ia] * (k - 3) * 38.0 - T[ia] * 51.0
        r = np.random.default_rng(700 + k)
        for _ in range(3):
            a = c + Lat[ia] * r.uniform(-12, 12) + np.array([0, 0, r.uniform(-10, 4)])
            b = a + (Lat[ia] if r.random() < 0.5 else np.array([0, 0, 1.0])) * r.choice([-14.0, 14.0])
            _tratto(segni_nani, "Ferro", a, b, -T[ia], 3.0)

    # ---- la porta degli Antichi, in fondo alla sala
    fondo = np.array([xy[-1, 0], xy[-1, 1], f[-1]]) - T[-1] * 30.0
    av = -T[-1]                     # verso chi guarda la porta
    la = Lat[-1]
    porta = Mesh("SM_Grotta_Porta")
    # soglia, stipiti, architrave, battente
    porta.scatola("Pietra", list(fondo + av * 40 + np.array([0, 0, 8.0])), list(av * 70.0), list(la * 200.0), [0, 0, 8.0])
    for lato in (-1, 1):
        porta.scatola("Pietra", list(fondo + la * lato * 175.0 + np.array([0, 0, 230.0])), list(av * 45.0), list(la * 32.0),
                      [0, 0, 230.0])
    porta.scatola("Pietra", list(fondo + np.array([0, 0, 495.0])), list(av * 50.0), list(la * 215.0), [0, 0, 40.0])
    porta.scatola("Pietra", list(fondo - av * 5 + np.array([0, 0, 228.0])), list(av * 22.0), list(la * 143.0), [0, 0, 212.0])
    # le rune della porta (battono sempre): il cerchio dei cinque posti, la mano, Ur sull'architrave, colonne di rune
    viso = fondo + av * 17.5
    c0 = viso + np.array([0, 0, 235.0])
    giro = 24
    for k in range(giro):
        a1, a2 = 2 * math.pi * k / giro, 2 * math.pi * (k + 1) / giro
        _tratto(rune, "Runa", c0 + la * math.cos(a1) * 92 + np.array([0, 0, math.sin(a1) * 92]),
                c0 + la * math.cos(a2) * 92 + np.array([0, 0, math.sin(a2) * 92]), av, 3.0)
    # (07/11 sera) i cinque posti portano le rune vere degli elementi (alfabeto_rune.py): Aeth la fiamma, Nir l'onda,
    # Dorn il monte, Sil la piuma (il vento), Lume il sole; Ur sull'architrave; nelle colonne ai lati gli elementi a
    # sinistra e i verbi a destra, come un abbecedario inciso per chi viene dopo
    su_ = np.array([0, 0, 1.0])
    for k, segno in enumerate(ar.ELEMENTI):
        a = math.pi / 2 + 2 * math.pi * k / 5
        p = c0 + la * math.cos(a) * 92 + np.array([0, 0, math.sin(a) * 92])
        for q in range(10):          # il posto: un anellino
            b1, b2 = 2 * math.pi * q / 10, 2 * math.pi * (q + 1) / 10
            _tratto(rune, "Runa", p + la * math.cos(b1) * 19 + np.array([0, 0, math.sin(b1) * 19]),
                    p + la * math.cos(b2) * 19 + np.array([0, 0, math.sin(b2) * 19]), av, 2.2)
        ar.incidi(lambda a, b: _tratto(rune, "Runa", a, b, av, 1.8), p, av, su_, 11.0, segno)
    for (x1, y1), (x2, y2) in _segno("mano"):
        _tratto(rune, "Runa", c0 + la * x1 * 30 + np.array([0, 0, y1 * 30]), c0 + la * x2 * 30 + np.array([0, 0, y2 * 30]), av, 2.6)
    ur = fondo + av * 50.6 + np.array([0, 0, 495.0])
    ar.incidi(lambda a, b: _tratto(rune, "Runa", a, b, av, 4.0), ur, av, su_, 24.0, "Ur")
    destra = np.cross(av, su_)          # la destra di chi guarda la porta (vedi ar.incidi)
    for lato, colonna in ((-1, ar.ELEMENTI), (1, ar.VERBI)):
        for k, nome in enumerate(colonna):
            # dall'alto in basso: la prima runa della colonna sta in cima
            centro = fondo + av * 45.6 + destra * lato * 175.0 + np.array([0, 0, 370.0 - k * 75.0])
            ar.incidi(lambda a, b: _tratto(rune, "Runa", a, b, av, 2.2), centro, av, su_, 15.0, nome)
    # ---- il muschio che fa luce (chiazze sulle pareti della grotta naturale) e le sue luci
    muschio = Mesh("SM_Grotta_Muschio")
    luci_muschio = []
    for k in range(26):
        i = int(rng.integers(6, max(7, taglio - 2)))
        j = int(rng.integers(3, K - 2))
        p, n = dentro[i, j], Nd[i, j]
        a1 = normalizza(np.cross(n, [0, 0, 1.0]))
        a2 = np.cross(n, a1)
        for _ in range(int(rng.integers(5, 10))):
            q = p + a1 * rng.normal(0, 35) + a2 * rng.normal(0, 25)
            _chiazza(muschio, q + n * 2.0, n, rng.uniform(6, 22), rng)
        if k % 9 == 0:
            luci_muschio.append(p + n * 60.0)

    # ---- nel progetto
    pr.al_suolo(S, "Grotta est", m, gx, gy, q0, collisione=True)
    pr.al_suolo(S, "Porta degli Antichi", porta, gx, gy, q0, collisione=True)
    pr.al_suolo(S, "Segni dei nani", segni_nani, gx, gy, q0, collisione=False)
    pr.al_suolo(S, "Muschio lucente", muschio, gx, gy, q0, collisione=False)
    pr.al_suolo(S, "Rune della porta", rune, gx, gy, q0, collisione=False)
    pr.cose[-1]["rune"] = "sempre"
    for k, p in enumerate(luci_muschio):
        pr.aggiungi(S, "Luce del muschio %d" % k, "luce", gx + p[0], gy + p[1], q0 + p[2], suolo=True, intensita=2.5, raggio=800.0,
                    colore=[0.45, 1.0, 0.72], ombre=True)
    lp = fondo + av * 160.0 + np.array([0, 0, 260.0])
    d = pr.aggiungi(S, "Luce della porta", "luce", gx + lp[0], gy + lp[1], q0 + lp[2], suolo=True, intensita=1.8, raggio=900.0,
                    colore=[0.35, 0.7, 1.0], ombre=True)
    d["rune"] = "sempre"
    # le iscrizioni: la tavola invisibile (si tocca con E) contro la parete, girata verso chi legge (+X = davanti)
    pi = pa[:2] + Lat[ia, :2] * (w[ia] / 2 - 15.0) - T[ia, :2] * 90.0
    gradi_arco = math.degrees(math.atan2(-Lat[ia, 1], -Lat[ia, 0]))
    pr.aggiungi(S, "Iscrizione dell'arco", "iscrizione", gx + pi[0], gy + pi[1], q0 + f[ia], gradi_arco, suolo=True,
                titolo="L'arco squadrato",
                avvisi=["Qui la grotta smette di essere grotta. Le pareti diventano diritte, la volta piatta, e sulla pietra",
                        "si vedono ancora i colpi dello scalpello, fitti e regolari, come se chi scavava non avesse mai fretta.",
                        "Sull'architrave, segni squadrati: non sono le rune delle rovine. Sono più semplici, più pesanti.",
                        "Chi li ha incisi aveva mani più larghe delle nostre.",
                        "Il cunicolo va avanti verso nord, nel buio."])
    pv = fondo + av * 25.0
    gradi_porta = math.degrees(math.atan2(av[1], av[0]))
    pr.aggiungi(S, "Iscrizione della porta", "iscrizione", gx + pv[0], gy + pv[1], q0 + f[-1], gradi_porta, suolo=True,
                titolo="La porta in fondo alla grotta",
                avvisi=["Una porta di pietra, liscia, senza maniglia e senza serratura. Non ha mai avuto una chiave.",
                        "Nel mezzo è inciso un cerchio con cinque posti: una fiamma, un'onda, un monte, una piuma, un sole.",
                        "rune: Aeth Nir Dorn Sil Lume",
                        "Al centro del cerchio, una mano aperta.",
                        "Sull'architrave c'è un segno solo, inciso più a fondo di tutti gli altri.",
                        "rune: Ur",
                        "Sugli stipiti, due colonne: a sinistra gli stessi cinque segni del cerchio, a destra cinque segni che sembrano gesti.",
                        "rune: Var Ka Ren Vel Tor",
                        "Le rune battono piano, giorno e notte: due colpi e una pausa. Come un cuore.",
                        "Al villaggio la chiamano \"la porta che nessuno ha mai aperto\". Spingere non serve."])
    # massi di Poly Haven ai piedi della rupe e davanti alla bocca
    for k in range(14):
        i = int(rng.integers(0, ns))
        lato = rng.choice([-1.0, 1.0]) if i > 3 else -1.0
        p = xy[i] + Lat[i, :2] * lato * (w[i] / 2 + rng.uniform(320, 520))
        if i <= 3:
            p = xy[0] - T[0, :2] * rng.uniform(250, 600) + Lat[0, :2] * rng.choice([-1, 1]) * rng.uniform(220, 450)
        pr.oggetto(S, "Masso della grotta %d" % k, "/Game/Fab/PolyHaven/" + ("boulder_01" if k % 3 == 0 else
                   "rock_moss_set_0%d" % (1 + k % 2)), gx + p[0], gy + p[1], rng.uniform(0, 360),
                   scala=rng.uniform(1.2, 2.4), dz=-30)
    # niente erba vera dentro e sotto la rupe; i disegni per la pianta e la mappa
    for i in range(0, ns, 8):
        pr.zone_senza_erba.append((gx + xy[i, 0], gy + xy[i, 1], w[i] / 2 + 450, w[i] / 2 + 450))
    contorno = [(gx + xy[i, 0] + Lat[i, 0] * (w[i] / 2 + 300), gy + xy[i, 1] + Lat[i, 1] * (w[i] / 2 + 300)) for i in range(ns)]
    contorno += [(gx + xy[i, 0] - Lat[i, 0] * (w[i] / 2 + 300), gy + xy[i, 1] - Lat[i, 1] * (w[i] / 2 + 300))
                 for i in range(ns - 1, -1, -1)]
    # (come linee: sulla mappa di carta la grotta c'è già, scritta da valle.py; qui solo per l'anteprima)
    pr.disegni.append((S, "linea", contorno + [contorno[0]], "Rupe"))
    pr.disegni.append((S, "linea", [(gx + p[0], gy + p[1]) for p in xy[::4]], "Grotta"))
    sopra = float(np.max(fuori[..., 2]) - np.min([zt(*p[:2]) for p in xy]))
    print("grotta est: %.0f m di grotta, pavimento da %.1f a %.1f m, rupe alta fino a %.1f m, %d scalini" %
          (Lm, f[0] / 100, f[-1] / 100, sopra / 100, scalini))
    return {"xy": xy, "f": f, "w": w, "h": h, "T": T, "Lat": Lat, "dentro": dentro, "fuori": fuori}


# --------------------------------------------------------------------------------------------------- disegni

def _tratto(m, mat, a, b, normale, largo):
    a, b = np.asarray(a, np.float64), np.asarray(b, np.float64)
    normale = normalizza(np.asarray(normale, np.float64))
    d = b - a
    if np.linalg.norm(d) < 1e-6:
        return
    u = d / np.linalg.norm(d)
    q = np.cross(normale, u)
    q = q / (np.linalg.norm(q) + 1e-9) * largo / 2
    off = normale * 0.8
    m.quad(mat, a - q + off, b - q + off, b + q + off, a + q + off, normale=normale)


def _segno(nome):
    """I tratti (coppie di punti, unità -1..1, x a destra, y in su) dei segni della porta."""
    if nome == "fiamma":
        return [((-0.5, -0.7), (0.0, 0.8)), ((0.0, 0.8), (0.5, -0.7)), ((0.5, -0.7), (-0.5, -0.7)), ((0.0, -0.7), (0.0, 0.1))]
    if nome == "onda":
        p = [(-0.8, 0.0), (-0.4, 0.4), (0.0, 0.0), (0.4, -0.4), (0.8, 0.0)]
        q = [(x, y - 0.5) for x, y in p]
        return list(zip(p[:-1], p[1:])) + list(zip(q[:-1], q[1:]))
    if nome == "monte":
        return [((-0.8, -0.6), (-0.2, 0.6)), ((-0.2, 0.6), (0.2, 0.0)), ((0.2, 0.0), (0.45, 0.3)), ((0.45, 0.3), (0.8, -0.6))]
    if nome == "piuma":
        t = [((0.0, -0.8), (0.0, 0.8))]
        for y in (-0.3, 0.05, 0.4):
            t += [((0.0, y), (-0.45, y + 0.3)), ((0.0, y), (0.45, y + 0.3))]
        return t
    if nome == "sole":
        t = []
        for k in range(8):
            a1, a2 = 2 * math.pi * k / 8, 2 * math.pi * (k + 1) / 8
            t.append(((0.4 * math.cos(a1), 0.4 * math.sin(a1)), (0.4 * math.cos(a2), 0.4 * math.sin(a2))))
            t.append(((0.55 * math.cos(a1), 0.55 * math.sin(a1)), (0.85 * math.cos(a1), 0.85 * math.sin(a1))))
        return t
    if nome == "mano":
        t = []
        for k in range(10):          # il palmo
            a1, a2 = 2 * math.pi * k / 10, 2 * math.pi * (k + 1) / 10
            t.append(((0.38 * math.cos(a1), -0.25 + 0.42 * math.sin(a1)), (0.38 * math.cos(a2), -0.25 + 0.42 * math.sin(a2))))
        for x, y, lx, ly in ((-0.30, 0.05, -0.12, 0.55), (-0.12, 0.15, -0.04, 0.75), (0.06, 0.16, 0.05, 0.80),
                             (0.24, 0.10, 0.12, 0.62), (0.36, -0.35, 0.45, 0.25)):
            t.append(((x, y), (x + lx, y + ly)))
        return t
    if nome == "ur":
        # Ur: un arco (una U rovesciata) con la gamba destra più corta, come l'orso che si alza
        return [((-0.6, -1.0), (-0.6, 0.6)), ((-0.6, 0.6), (-0.2, 1.0)), ((-0.2, 1.0), (0.5, 0.8)), ((0.5, 0.8), (0.6, -0.3))]
    return []


def _chiazza(m, centro, normale, raggio, rng):
    """Una chiazza di muschio: un poligono irregolare sul piano della parete."""
    n = normalizza(np.asarray(normale, np.float64))
    a = normalizza(np.cross(n, [0, 0, 1.0]))
    if np.linalg.norm(a) < 1e-6:
        a = np.array([1.0, 0, 0])
    b = np.cross(n, a)
    lati = 9
    P = [centro]
    for k in range(lati + 1):
        ang = 2 * math.pi * (k % lati) / lati
        r = raggio * rng.uniform(0.55, 1.0) if k < lati else None
        if k == lati:
            P.append(P[1])
            break
        P.append(centro + a * math.cos(ang) * r + b * math.sin(ang) * r * 0.7)
    UV = [[0.5, 0.5]] + [[0.5 + 0.5 * math.cos(2 * math.pi * k / lati), 0.5 + 0.5 * math.sin(2 * math.pi * k / lati)]
                         for k in range(lati + 1)]
    T = [[0, k + 1, k + 2] for k in range(lati)]
    m.triangoli("Muschio", P, [n] * len(P), UV, T)


# --------------------------------------------------------------------------------------------------- la grotta nord

def frana(pr, rng):
    """(06/11, sera) La grotta nord, chiusa dalla frana. Dalla lore: le gallerie dei nani dove passava la strada di
    Orvanna, fatte crollare dopo i sette giorni (-688); di là c'è la sala dei dormienti (la razza segreta Nano, v0.6).
    Lucia della Gilda ne ha la mappa; dal 309, di notte, si sentono i martelli (S_Martelli, crea_suoni_valle.py).
    Cosa c'è (metri dal luogo "Grotta nord", y a sud): una rupe di roccia contro il pendio ripido dei monti del nord e,
    davanti, il portale squadrato dei nani (zoccolo, due pilastri, l'architrave spezzato in due, la cornice rotta, i
    loro segni), chiuso da una frana di sassi che esce dal vano e scende sul pendio; in alto resta una fessura buia.
    Un'iscrizione da leggere con E."""
    t = pr.t
    gx, gy, _ = t.luogo("Grotta nord")
    q0 = t.quota(gx, gy)

    def zt(x, y):
        return t.quota(gx + x, gy + y) - q0

    yf = -50.0                                   # la faccia del portale (cm), guarda a sud (+y)
    sotto = [zt(x, y) for x in np.linspace(-500, 500, 11) for y in (yf, yf - 150.0)]
    b0 = min(sotto) - 50.0                       # il piede dello zoccolo
    m = Mesh("SM_Grotta_Nord")
    segni = Mesh("SM_Grotta_Nord_Segni")
    av = np.array([0.0, 1.0, 0.0])

    # ---- la rupe: un ellissoide frastagliato, appoggiato al pendio dietro il portale (la parte sotto il terreno non si vede)
    cr = np.array([0.0, yf - 550.0, b0 + 300.0])
    rx, ry, rz = 850.0, 600.0, 900.0
    nlat, nlon = 18, 32
    G = np.zeros((nlat + 1, nlon + 1, 3))
    for i in range(nlat + 1):
        th = -math.pi / 2 + math.pi * i / nlat
        for j in range(nlon + 1):
            ph = 2 * math.pi * j / nlon
            G[i, j] = cr + np.array([math.cos(th) * math.cos(ph) * rx, math.cos(th) * math.sin(ph) * ry, math.sin(th) * rz])
    rad = G - cr
    rad /= (np.linalg.norm(rad, axis=2, keepdims=True) + 1e-9)
    nz = (frastaglio(G.reshape(-1, 3), 500.0, 31) * 170.0 + frastaglio(G.reshape(-1, 3), 160.0, 37) * 45.0).reshape(G.shape[:2])
    G += rad * nz[..., None]
    G[..., 1] = np.minimum(G[..., 1], yf - 100.0)           # davanti, dove sta il portale, la rupe è tagliata
    Ng = _normali_griglia(G, lambda i, j: G[i, j] - cr)
    Ng[0], Ng[-1] = (0.0, 0.0, -1.0), (0.0, 0.0, 1.0)          # i poli (lì la griglia non dà una normale)
    _griglia_tri(m, "Roccia", G, Ng, TILE_ROCCIA)

    # ---- il portale dei nani (Pietra): zoccolo, pilastri, architrave spezzato, cornice rotta
    def blocco(cx, cy, cz, hx, hy, hz, mat="Pietra", mesh=m):
        mesh.scatola(mat, [cx, cy, cz], [hx, 0, 0], [0, hy, 0], [0, 0, hz])

    top0 = b0 + 80.0
    blocco(0, yf - 75.0, b0 + 40.0, 520.0, 110.0, 40.0)                   # zoccolo
    for lato in (-1, 1):
        blocco(lato * 290.0, yf - 80.0, top0 + 325.0, 70.0, 80.0, 325.0)  # pilastri (vano largo 4,4 m, alto 6,5)
    hl = top0 + 650.0
    blocco(-200.0, yf - 75.0, hl + 70.0, 230.0, 75.0, 70.0)               # architrave, metà sinistra
    pezzo = Mesh("tmp")
    pezzo.scatola("Pietra", [0, 0, 0], [195.0, 0, 0], [0, 75.0, 0], [0, 0, 70.0])
    m.unisci(pezzo, (240.0, yf - 75.0, hl + 55.0), 0.0)                   # metà destra, scesa di 15 cm
    blocco(-210.0, yf - 70.0, hl + 165.0, 240.0, 85.0, 25.0)              # cornice (resta solo sopra la metà sinistra)
    giu_pezzo = min(zt(560.0 + dx, yf + 260.0 + dy) for dx in (-120.0, 120.0) for dy in (-60.0, 60.0))
    blocco(560.0, yf + 260.0, giu_pezzo + 20.0, 120.0, 60.0, 30.0)       # il pezzo di cornice caduto, a lato della frana
    # i segni dei nani sull'architrave
    for k, xk in enumerate((-380.0, -300.0, -220.0, -140.0, -60.0, 120.0, 220.0, 320.0)):
        c = np.array([xk, yf + 1.0, hl + 70.0 - (15.0 if xk > 40.0 else 0.0)])
        r = np.random.default_rng(900 + k)
        for _ in range(3):
            a = c + np.array([r.uniform(-14, 14), 0, r.uniform(-22, 8)])
            b = a + (np.array([1.0, 0, 0]) if r.random() < 0.5 else np.array([0, 0, 1.0])) * r.choice([-16.0, 16.0])
            _tratto(segni, "Ferro", a, b, av, 3.0)
    # la fessura buia in alto, tra la frana e l'architrave; (11/11) il buio va giu' fino allo zoccolo: quando la frana si
    # apre (AValdorsoFrana nasconde SM_Grotta_Nord_Frana) il vano resta nero, la galleria che entra nella montagna
    m.quad("Ferro", [-220.0, yf - 95.0, top0], [220.0, yf - 95.0, top0], [220.0, yf - 95.0, hl],
           [-220.0, yf - 95.0, hl], normale=av)

    # ---- la frana: un cumulo che riempie il vano fino a 70 cm dall'architrave ed esce sul pendio per 4-5 m.
    # (11/11, v0.1.7) in un pezzo suo (SM_Grotta_Nord_Frana, etichetta "ValdorsoFrana"): quando i coloni la liberano,
    # AValdorsoFrana (C++) la nasconde e il portale si apre
    fr = Mesh("SM_Grotta_Nord_Frana")
    xs = np.arange(-650.0, 651.0, 25.0)
    ys = np.arange(yf - 140.0, yf + 640.0, 25.0)
    F = np.zeros((len(ys), len(xs), 3))
    pieno = np.zeros((len(ys), len(xs)))
    cima = hl - 70.0
    for i, y in enumerate(ys):
        for j, x in enumerate(xs):
            ter = zt(x, y)
            if y <= yf:
                # dentro il vano: fino alla cima (i lati li coprono i pilastri)
                alto = cima - ter if abs(x) < 300.0 else 0.0
            else:
                d = (y - yf) / 100.0
                largo = 260.0 + d * 70.0
                lat = max(0.0, 1.0 - (x / largo) ** 2) ** 0.6
                alto = max(0.0, (cima - top0) * max(0.0, 1.0 - d / 4.2) ** 1.5 + 25.0 * max(0.0, 1.0 - d / 5.0)) * lat
            pieno[i, j] = alto
            # al bordo il cumulo entra nel terreno (fino a 25 cm sotto): le due superfici si incrociano, non si sovrappongono
            F[i, j] = [x, y, ter + alto - 25.0 * (1.0 - float(smussa(alto, 0.0, 45.0)))]
    nzr = frastaglio(F.reshape(-1, 3), 140.0, 51).reshape(pieno.shape) * np.clip(pieno / 100.0, 0, 1) * 45.0
    F[..., 2] += nzr
    Nr = np.zeros_like(F)
    dzx = np.gradient(F[..., 2], axis=1) / 25.0
    dzy = np.gradient(F[..., 2], axis=0) / 25.0
    Nr[..., 0], Nr[..., 1], Nr[..., 2] = -dzx, -dzy, 1.0
    Nr /= np.linalg.norm(Nr, axis=2, keepdims=True)
    _griglia_tri(fr, "Roccia", F, Nr, TILE_ROCCIA)
    # i massi sopra la frana
    sassi = 0
    for k in range(60):
        i, j = int(rng.integers(0, len(ys))), int(rng.integers(0, len(xs)))
        if pieno[i, j] < 60.0 or (ys[i] <= yf and abs(xs[j]) > 230.0):
            continue
        r = rng.uniform(35.0, 120.0) * (0.6 if ys[i] <= yf else 1.0)
        _sasso(fr, F[i, j] + np.array([0, 0, -0.25 * r]), r, rng, 200 + k)
        sassi += 1

    pr.al_suolo("Grotta nord", "Grotta nord", m, gx, gy, q0, collisione=True)
    pr.al_suolo("Grotta nord", "Segni dei nani della grotta nord", segni, gx, gy, q0, collisione=False)
    pr.al_suolo("Grotta nord", "La frana del nord", fr, gx, gy, q0, collisione=True)
    pr.cose[-1]["etichetta"] = "ValdorsoFrana"
    # l'iscrizione: davanti al pilastro di destra (lì c'è lo zoccolo e non la frana), verso sud, mai sotto il terreno
    zi = max(top0, zt(290.0, yf + 20.0))
    pr.aggiungi("Grotta nord", "Iscrizione della frana", "iscrizione", gx + 290.0, gy + yf + 20.0, q0 + zi, 90.0, suolo=True,
                titolo="La frana",
                avvisi=["Un portale di pietra squadrata, largo e basso, piantato nella montagna. L'architrave è spezzato in due.",
                        "Il vano è pieno di sassi fino in cima: qualcuno, tanto tempo fa, ha fatto crollare la galleria.",
                        "In alto resta una fessura nera. Ci passa solo l'aria, ed è fredda.",
                        "Sull'architrave, gli stessi segni squadrati dell'arco nella grotta di est.",
                        "Al villaggio dicono che Lucia, alla Gilda, abbia una vecchia mappa di queste gallerie.",
                        "E qualcuno giura che di notte, da qui sotto, si sente battere: un colpo, poi niente, poi un altro."])
    # massi di Poly Haven ai lati
    for k in range(8):
        lato = -1.0 if k % 2 == 0 else 1.0
        x = lato * rng.uniform(560.0, 900.0)
        y = yf + rng.uniform(-100.0, 450.0)
        pr.oggetto("Grotta nord", "Masso della frana %d" % k, "/Game/Fab/PolyHaven/" + ("boulder_01" if k % 3 == 0 else
                   "rock_moss_set_0%d" % (1 + k % 2)), gx + x, gy + y, rng.uniform(0, 360), scala=rng.uniform(1.2, 2.2), dz=-30)
    pr.zone_senza_erba.append((gx, gy + yf + 250.0, 700.0, 450.0))
    pr.disegni.append(("Grotta nord", "linea", [(gx - 520.0, gy + yf), (gx + 520.0, gy + yf)], "Portale"))
    pr.disegni.append(("Grotta nord", "linea", [(gx + x, gy + y) for x, y in ((-300, yf), (-450, yf + 500), (450, yf + 500), (300, yf))],
                       "Frana"))
    print("grotta nord: portale dei nani e frana (%d sassi), piede a %.1f m" % (sassi, b0 / 100.0))
    return {"yf": yf, "b0": b0, "hl": hl}


def _sasso(m, centro, raggio, rng, seme):
    """Un sasso: un icosaedro diviso una volta, schiacciato e frastagliato (Roccia)."""
    p = (1 + 5 ** 0.5) / 2
    V = [(-1, p, 0), (1, p, 0), (-1, -p, 0), (1, -p, 0), (0, -1, p), (0, 1, p), (0, -1, -p), (0, 1, -p),
         (p, 0, -1), (p, 0, 1), (-p, 0, -1), (-p, 0, 1)]
    V = [np.array(v, float) / np.linalg.norm(v) for v in V]
    F = [(0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11), (1, 5, 9), (5, 11, 4), (11, 10, 2), (10, 7, 6),
         (7, 1, 8), (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9), (4, 9, 5), (2, 4, 11), (6, 2, 10), (8, 6, 7), (9, 8, 1)]
    medi = {}

    def medio(a, b):
        k = (min(a, b), max(a, b))
        if k not in medi:
            v = V[a] + V[b]
            V.append(v / np.linalg.norm(v))
            medi[k] = len(V) - 1
        return medi[k]

    F2 = []
    for a, b, c in F:
        ab, bc, ca = medio(a, b), medio(b, c), medio(c, a)
        F2 += [(a, ab, ca), (b, bc, ab), (c, ca, bc), (ab, bc, ca)]
    r = np.random.default_rng(seme)
    scala = np.array([r.uniform(0.8, 1.2), r.uniform(0.8, 1.2), r.uniform(0.5, 0.8)]) * raggio
    P = np.array(V) * scala
    P *= (1 + 0.22 * rumore3(P * 3.0 + seme * 100.0, raggio * 1.2, seme))[:, None]
    gira = r.uniform(0, 2 * math.pi)
    c, s = math.cos(gira), math.sin(gira)
    P = P @ np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]]).T + np.asarray(centro)
    # facce piatte (sasso spaccato): ogni triangolo con la sua normale
    PP, NN, UU, TT = [], [], [], []
    for a, b, cc in F2:
        A, B, C = P[a], P[b], P[cc]
        n = normalizza(np.cross(B - A, C - A))
        if np.dot(n, A - np.asarray(centro)) < 0:
            n = -n
        k = len(PP)
        PP += [A, B, C]
        NN += [n, n, n]
        UU += [[A[0] / TILE_ROCCIA, A[2] / TILE_ROCCIA], [B[0] / TILE_ROCCIA, B[2] / TILE_ROCCIA], [C[0] / TILE_ROCCIA, C[2] / TILE_ROCCIA]]
        TT.append([k, k + 1, k + 2])
    m.triangoli("Roccia", PP, NN, UU, TT)
