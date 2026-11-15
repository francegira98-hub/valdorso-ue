# Valdorso - Il sentiero dei nani (15/11/2026, scritto da Claude; idea approvata da Fra: "la grotta a nord e' troppo
# ripida per salirci"). Una mulattiera di pietra a tornanti che sale dal pendio alla frana davanti alla grotta del nord,
# come le vecchie strade di montagna: lastre di pietra in piano, e dal lato a valle un muretto a secco che la regge.
# Non tocca il paesaggio (il Landscape): ci appoggia sopra i pezzi.
#
# Come fa:
#   1. legge l'altezza del terreno su una griglia di 2 m a sud del portale (il portale guarda a sud, verso la valle);
#   2. trova dove si cammina gia' bene (pendenza fino a ~35 gradi) collegato al bordo a valle della griglia: li' finisce
#      il sentiero;
#   3. cerca la strada piu' corta dalla frana fin li' senza mai salire piu' del 25% (14 gradi): sul ripido viene da
#      sola a tornanti; poi la raddrizza dove si puo';
#   4. mette i pezzi: una rampa invisibile che regge i piedi (cosi' non si scivola), le lastre di pietra sopra
#      (SM_JapaneseShrineStoneFloorC_00 del kit Medieval Village) e il muretto (SM_MossyStoneWallB) dove il terreno
#      scende sotto il sentiero. Se i pezzi del kit mancano, mette lo stesso la rampa e lo scrive nel resoconto.
# Le voci nel menu Valdorso -> Pericoli:
#   "Sentiero dei nani: costruisci (grotta del nord)"  -> SENTIERO_MODO = "fai"   (rifa' da capo, toglie il vecchio)
#   "Sentiero dei nani: togli"                         -> SENTIERO_MODO = "togli"
# Il resoconto in Saved/Claude/sentiero_nani.txt (lunghezza, tornanti, pendenza piu' forte, pezzi messi).

import heapq
import math
import os

import unreal

MODO = str(globals().get("SENTIERO_MODO", "fai"))

ETICHETTA = "ValdorsoSentieroNani"
CONTORNO = "Valle/Sentiero dei nani"
PASSO = 200.0               # griglia del terreno (cm)
LARGO_X = 8000.0            # quanto cercare a est e a ovest del portale (cm)
LUNGO_Y = 20000.0           # quanto cercare verso sud, a valle (cm)
PARTENZA_Y = 650.0          # il sentiero comincia ai piedi della frana (che e' a 470 cm dal portale, e sporge 4-5 m)
SALITA_MAX = 0.25           # pendenza massima del sentiero (25% = 14 gradi)
SALITA_RADDRIZZA = 0.30     # raddrizzando si accetta un poco di piu'
CAMMINABILE = 0.70          # sul terreno vero si cammina fino a 0,70 (35 gradi; Unreal regge fino a 44)
LARGHEZZA = 180.0           # il sentiero e' largo 1,8 m
SCARTO_MAX = 90.0           # raddrizzando: quanto il terreno puo' stare sopra o sotto la linea del sentiero (cm)
MURO_DA = 25.0              # sotto questo salto a valle non serve il muretto (cm)
CURVA = 2500.0              # quanto "costa" un tornante (in cm di strada): piu' alto = meno tornanti, piu' lunghi

LASTRA = "SM_JapaneseShrineStoneFloorC_00"
MURETTO = "SM_MossyStoneWallB"
SASSI = ("SM_MossyRocksB", "SM_MossyRock")

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Sentiero dei nani: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Sentiero dei nani: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "sentiero_nani.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")


def togli_vecchi():
    via = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            via += 1
    return via


def cerca_grotta():
    for a in attori.get_all_level_actors():
        if a and a.get_actor_label() == "Grotta nord":
            return a
    return None


def cerca_mesh(nome):
    """Il primo StaticMesh con questo nome in /Game (i pacchetti migrati possono stare in cartelle diverse)."""
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    filtro = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")],
                             package_paths=["/Game"], recursive_paths=True)
    for d in registro.get_assets(filtro):
        if str(d.asset_name) == nome:
            return unreal.EditorAssetLibrary.load_asset(str(d.package_name))
    return None


def terreno(mondo, x, y, z_alto, z_basso):
    """La quota del paesaggio (solo il Landscape) in (x, y), o None."""
    try:
        r = unreal.SystemLibrary.line_trace_multi(mondo, unreal.Vector(x, y, z_alto), unreal.Vector(x, y, z_basso),
                                                  unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                  unreal.DrawDebugTrace.NONE, True)
        colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
        for hit in colpi or []:
            t = hit.to_tuple()
            if t[9] is not None and "Landscape" in t[9].get_class().get_name():
                return t[4].z
    except Exception as e:  # noqa: BLE001
        avviso("traccia del terreno non riuscita (%s)" % e)
    return None


# ---------------------------------------------------------------- la griglia del terreno

class Griglia:
    def __init__(self, mondo, cx, y0, z_alto):
        self.nx = int(2 * LARGO_X / PASSO) + 1
        self.ny = int(LUNGO_Y / PASSO) + 1
        self.x0 = cx - LARGO_X
        self.y0 = y0
        self.z = [[None] * self.ny for _ in range(self.nx)]
        mancano = 0
        for i in range(self.nx):
            for j in range(self.ny):
                q = terreno(mondo, self.x0 + i * PASSO, self.y0 + j * PASSO, z_alto, -50000.0)
                self.z[i][j] = q
                if q is None:
                    mancano += 1
        scrivi("griglia del terreno: %d x %d punti ogni %.0f m (%d senza terreno)"
               % (self.nx, self.ny, PASSO / 100.0, mancano))

    def dentro(self, i, j):
        return 0 <= i < self.nx and 0 <= j < self.ny

    def xy(self, i, j):
        return self.x0 + i * PASSO, self.y0 + j * PASSO

    def quota(self, x, y):
        """Quota interpolata (bilineare) in un punto qualsiasi della griglia, o None."""
        fi = (x - self.x0) / PASSO
        fj = (y - self.y0) / PASSO
        i = int(math.floor(fi))
        j = int(math.floor(fj))
        if not (0 <= i < self.nx - 1 and 0 <= j < self.ny - 1):
            if self.dentro(int(round(fi)), int(round(fj))):
                return self.z[int(round(fi))][int(round(fj))]
            return None
        a, b = self.z[i][j], self.z[i + 1][j]
        c, d = self.z[i][j + 1], self.z[i + 1][j + 1]
        if None in (a, b, c, d):
            return None
        u, v = fi - i, fj - j
        return (a * (1 - u) + b * u) * (1 - v) + (c * (1 - u) + d * u) * v

    def pendenza(self, i, j):
        """La pendenza del terreno nella cella (0 = piano, 1 = 45 gradi)."""
        z = self.z[i][j]
        if z is None:
            return 99.0
        gx = gy = 0.0
        for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ii, jj = i + di, j + dj
            if self.dentro(ii, jj) and self.z[ii][jj] is not None:
                g = abs(self.z[ii][jj] - z) / PASSO
                if di:
                    gx = max(gx, g)
                else:
                    gy = max(gy, g)
        return math.hypot(gx, gy)


def mosse():
    """Le direzioni possibili tra le celle: fino a 3 celle, cosi' il sentiero puo' tagliare il pendio di sbieco."""
    m = []
    for di in range(-3, 4):
        for dj in range(-3, 4):
            if (di or dj) and math.gcd(abs(di), abs(dj)) == 1:
                m.append((di, dj, math.hypot(di, dj) * PASSO))
    return m


def arrivi_buoni(g, pend):
    """Le celle dove si cammina gia' bene, collegate al bordo a valle (sud) della griglia."""
    buone = set()
    coda = []
    for i in range(g.nx):
        j = g.ny - 1
        if pend[i][j] <= CAMMINABILE:
            buone.add((i, j))
            coda.append((i, j))
    while coda:
        i, j = coda.pop()
        for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)):
            ii, jj = i + di, j + dj
            if g.dentro(ii, jj) and (ii, jj) not in buone and pend[ii][jj] <= CAMMINABILE:
                buone.add((ii, jj))
                coda.append((ii, jj))
    return buone


def cerca_strada(g, pend, partenza, arrivi):
    """Dijkstra dalla frana alla cella buona piu' vicina, senza mai superare SALITA_MAX. Si ricorda anche la direzione
    in cui si arriva in ogni cella: ogni curva costa (un tornante intero costa come CURVA cm di strada), cosi' il
    sentiero fa pochi tornanti lunghi, ben staccati l'uno dall'altro, invece di cento zig-zag da 2 metri."""
    passi = mosse()
    versi = [(di / math.hypot(di, dj), dj / math.hypot(di, dj)) for di, dj, _ in passi]
    curva = [[CURVA * (1.0 - (versi[a][0] * versi[b][0] + versi[a][1] * versi[b][1])) / 2.0 for b in range(len(passi))]
             for a in range(len(passi))]
    inizio = (partenza[0], partenza[1], -1)
    dist = {inizio: 0.0}
    da = {}
    coda = [(0.0, inizio)]
    while coda:
        d, stato = heapq.heappop(coda)
        if d > dist.get(stato, 1e18):
            continue
        i, j, verso = stato
        if (i, j) in arrivi:
            strada = [stato]
            while strada[-1] in da:
                strada.append(da[strada[-1]])
            return [(s[0], s[1]) for s in reversed(strada)]
        z = g.z[i][j]
        for k, (di, dj, lungo) in enumerate(passi):
            ii, jj = i + di, j + dj
            if not g.dentro(ii, jj) or g.z[ii][jj] is None:
                continue
            if abs(g.z[ii][jj] - z) / lungo > SALITA_MAX:
                continue
            # (preferisce il terreno meno ripido di fianco: meno muretto da fare)
            costo = lungo * (1.0 + 0.6 * min(pend[ii][jj], 2.0))
            if verso >= 0:
                costo += curva[verso][k]
            nd = d + costo
            nuovo = (ii, jj, k)
            if nd < dist.get(nuovo, 1e18):
                dist[nuovo] = nd
                da[nuovo] = stato
                heapq.heappush(coda, (nd, nuovo))
    return None


def quota_sentiero(g, x, y, dx, dy):
    """A che altezza deve stare il piano del sentiero in (x, y): il punto piu' alto del terreno sulla sua larghezza
    (cosi' le lastre non finiscono sotto terra dal lato a monte)."""
    nx, ny = -dy, dx
    alto = None
    for f in (-0.5, -0.25, 0.0, 0.25, 0.5):
        q = g.quota(x + nx * LARGHEZZA * f, y + ny * LARGHEZZA * f)
        if q is not None:
            alto = q if alto is None else max(alto, q)
    return alto


def raddrizza(g, punti):
    """Toglie i gomiti inutili: da ogni punto va il piu' lontano possibile in linea dritta, se la salita resta
    sotto SALITA_RADDRIZZA e il terreno non si stacca troppo dalla linea."""
    def va_bene(a, b):
        ax, ay, az = a
        bx, by, bz = b
        lungo = math.hypot(bx - ax, by - ay)
        if lungo < 1.0:
            return True
        if abs(bz - az) / lungo > SALITA_RADDRIZZA:
            return False
        n = max(2, int(lungo / 100.0))
        for k in range(1, n):
            t = k / float(n)
            q = g.quota(ax + (bx - ax) * t, ay + (by - ay) * t)
            if q is None or abs(q - (az + (bz - az) * t)) > SCARTO_MAX:
                return False
        return True

    fuori = [punti[0]]
    i = 0
    while i < len(punti) - 1:
        j = len(punti) - 1
        while j > i + 1 and not va_bene(punti[i], punti[j]):
            j -= 1
        fuori.append(punti[j])
        i = j
    return fuori


def alza_sopra_terreno(g, punti):
    """Ogni punto alla quota del piano del sentiero; poi, dove in mezzo a un tratto il terreno sporge sopra la linea,
    si aggiunge un punto li'."""
    def direzione(k):
        a = punti[max(0, k - 1)]
        b = punti[min(len(punti) - 1, k + 1)]
        dx, dy = b[0] - a[0], b[1] - a[1]
        lu = math.hypot(dx, dy) or 1.0
        return dx / lu, dy / lu

    nuovi = []
    for k, (x, y, z) in enumerate(punti):
        dx, dy = direzione(k)
        q = quota_sentiero(g, x, y, dx, dy)
        nuovi.append((x, y, q if q is not None else z))
    for _giro in range(3):
        aggiunti = False
        fuori = [nuovi[0]]
        for a, b in zip(nuovi, nuovi[1:]):
            lungo = math.hypot(b[0] - a[0], b[1] - a[1])
            dx, dy = (b[0] - a[0]) / (lungo or 1.0), (b[1] - a[1]) / (lungo or 1.0)
            peggio, dove = 15.0, None
            n = max(2, int(lungo / 100.0))
            for k in range(1, n):
                t = k / float(n)
                x, y = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
                q = quota_sentiero(g, x, y, dx, dy)
                if q is not None and q - (a[2] + (b[2] - a[2]) * t) > peggio:
                    peggio, dove = q - (a[2] + (b[2] - a[2]) * t), (x, y, q)
            if dove:
                fuori.append(dove)
                aggiunti = True
            fuori.append(b)
        nuovi = fuori
        if not aggiunti:
            break
    # i punti troppo vicini (meno di 1,2 m) diventano uno solo: ai tornanti farebbero gradini
    stretti = [nuovi[0]]
    for p in nuovi[1:-1]:
        if math.hypot(p[0] - stretti[-1][0], p[1] - stretti[-1][1]) >= 120.0:
            stretti.append(p)
    if math.hypot(nuovi[-1][0] - stretti[-1][0], nuovi[-1][1] - stretti[-1][1]) < 120.0 and len(stretti) > 1:
        stretti.pop()
    stretti.append(nuovi[-1])
    # ai tornanti il terreno a monte puo' tirare su un punto troppo: lo si abbassa finche' la salita verso i vicini
    # torna sotto SALITA_RADDRIZZA (li' il sentiero entra un poco nel pendio, come uno scavo)
    zz = [p[2] for p in stretti]
    for _giro in range(60):
        cambiato = False
        for k in range(len(stretti)):
            for n in (k - 1, k + 1):
                if 0 <= n < len(stretti):
                    lungo = math.hypot(stretti[k][0] - stretti[n][0], stretti[k][1] - stretti[n][1])
                    limite = zz[n] + SALITA_RADDRIZZA * lungo
                    if zz[k] > limite + 0.5:
                        zz[k] = limite
                        cambiato = True
        if not cambiato:
            break
    return [(p[0], p[1], z) for p, z in zip(stretti, zz)]


# ---------------------------------------------------------------- i pezzi

def metti(mesh, posto, rot, scala, nome, collisione=True, visibile=True):
    a = attori.spawn_actor_from_object(mesh, posto, rot)
    if not a:
        return None
    a.set_actor_scale3d(scala)
    a.set_actor_label(nome)
    a.set_folder_path(CONTORNO)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    comp = a.get_component_by_class(unreal.StaticMeshComponent)
    if comp:
        if not collisione:
            comp.set_collision_profile_name("NoCollision")
        if not visibile:
            # (15/11 sera) invisibile anche nell'editor: i blocchi bianchi si mescolavano con le lastre. Un pezzo
            # invisibile regge lo stesso i piedi (la collisione non dipende da quello che si vede)
            a.set_actor_hidden_in_game(True)
            comp.set_editor_property("cast_shadow", False)
            comp.set_visibility(False, True)
    return a


def misure(mesh):
    """(centro, mezze misure) del pezzo, nel suo spazio, per metterlo dove serve qualunque sia il suo punto d'origine."""
    try:
        b = mesh.get_bounding_box()
        c = unreal.Vector((b.min.x + b.max.x) / 2, (b.min.y + b.max.y) / 2, (b.min.z + b.max.z) / 2)
        e = unreal.Vector((b.max.x - b.min.x) / 2, (b.max.y - b.min.y) / 2, (b.max.z - b.min.z) / 2)
        return c, e
    except Exception:  # noqa: BLE001
        o = mesh.get_bounds()
        return o.origin, o.box_extent


def ruota(v, yaw, pitch):
    """Gira un vettore locale (x avanti, y destra, z su) come fa Unreal con un Rotator (pitch poi yaw)."""
    cp, sp = math.cos(math.radians(pitch)), math.sin(math.radians(pitch))
    cy, sy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    x, y, z = v
    x, z = x * cp - z * sp, x * sp + z * cp
    return x * cy - y * sy, x * sy + y * cy, z


def costruisci(g, punti, mondo):
    cubo = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    lastra = cerca_mesh(LASTRA)
    muretto = cerca_mesh(MURETTO)
    sasso = None
    for n in SASSI:
        sasso = cerca_mesh(n)
        if sasso:
            break
    if not lastra:
        avviso("non trovo %s (kit Medieval Village): metto solo la rampa invisibile, il sentiero si cammina ma non si vede"
               % LASTRA)
    if not muretto:
        avviso("non trovo %s: niente muretto a valle" % MURETTO)
    c_lastra, e_lastra = misure(lastra) if lastra else (None, None)
    c_muro, e_muro = misure(muretto) if muretto else (None, None)
    conti = {"rampa": 0, "lastre": 0, "muretti": 0, "sassi": 0}
    seme = [11]

    def caso():
        seme[0] = (seme[0] * 9301 + 49297) % 233280
        return seme[0] / 233280.0

    for k, (a, b) in enumerate(zip(punti, punti[1:])):
        dx, dy, dz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
        piano = math.hypot(dx, dy)
        if piano < 20.0:
            continue
        lungo = math.hypot(piano, dz)
        yaw = math.degrees(math.atan2(dy, dx))
        pitch = math.degrees(math.atan2(dz, piano))
        rot = unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw)
        mx, my, mz = (a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2
        # 1. la rampa invisibile (il cubo dell'Engine e' 100 cm, centrato): spessa 60 cm, il piano di sopra sulla linea;
        #    un po' piu' lunga del tratto, cosi' i tratti si sovrappongono ai gomiti
        # (15/11 sera) il piano della rampa 3 cm sotto la linea: le lastre (2 cm sopra) si vedono pulite e i piedi
        # le toccano appena
        spesso = 60.0
        gx, gy, gz = ruota((0.0, 0.0, -spesso / 2 - 3.0), yaw, pitch)
        if metti(cubo, unreal.Vector(mx + gx, my + gy, mz + gz), rot,
                 unreal.Vector((lungo + LARGHEZZA * 0.6) / 100.0, (LARGHEZZA + 20.0) / 100.0, spesso / 100.0),
                 "Sentiero dei nani - rampa %d" % (k + 1), collisione=True, visibile=False):
            conti["rampa"] += 1
        # 2. le lastre, una ogni ~1,8 m, girate a caso di 180 gradi perche' non si veda la ripetizione
        ux, uy = dx / piano, dy / piano
        if lastra:
            n = max(1, int(round(lungo / 180.0)))
            passo = lungo / n
            sx = (passo + 6.0) / (2 * e_lastra.x)
            sy = LARGHEZZA / (2 * e_lastra.y)
            sz = 1.0
            for q in range(n):
                t = (q + 0.5) / n
                px, py, pz = a[0] + dx * t, a[1] + dy * t, a[2] + dz * t
                giro = 180.0 if caso() < 0.5 else 0.0
                r = unreal.Rotator(roll=0.0, pitch=pitch if not giro else -pitch, yaw=yaw + giro)
                # il centro del pezzo va sul sentiero, la faccia di sopra 2 cm sopra la linea
                ox, oy, oz = ruota((c_lastra.x * sx, c_lastra.y * sy, (c_lastra.z + e_lastra.z) * sz - 2.0),
                                   r.yaw, r.pitch)
                if metti(lastra, unreal.Vector(px - ox, py - oy, pz - oz), r, unreal.Vector(sx, sy, sz),
                         "Sentiero dei nani - lastra %d.%d" % (k + 1, q + 1), collisione=False):
                    conti["lastre"] += 1
        # 3. il muretto dal lato a valle, un pezzo ogni ~4 m, solo dove il terreno scende sotto il sentiero
        if muretto:
            n = max(1, int(round(piano / 380.0)))
            for q in range(n):
                t = (q + 0.5) / n
                px, py, pz = a[0] + dx * t, a[1] + dy * t, a[2] + dz * t
                # quale lato e' a valle?
                lati = []
                for s in (1.0, -1.0):
                    ex, ey = px + (-uy) * s * (LARGHEZZA / 2 + 40.0), py + ux * s * (LARGHEZZA / 2 + 40.0)
                    qz = g.quota(ex, ey)
                    if qz is not None:
                        lati.append((qz, s, ex, ey))
                if not lati:
                    continue
                qz, s, ex, ey = min(lati)
                salto = pz - qz
                if salto < MURO_DA:
                    continue
                alto = salto + 40.0 + 15.0          # dalla terra (sotto di 40) a 15 cm sopra il sentiero
                sz = max(0.6, min(3.5, alto / (2 * e_muro.z)))
                sx = (lungo / n + 30.0) / (2 * e_muro.x)
                sy = 1.0
                # (inclinato come il sentiero, cosi' la cima lo segue per tutto il pezzo)
                r = unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw)
                cx = px + (-uy) * s * (LARGHEZZA / 2 + e_muro.y * 0.6)
                cy = py + ux * s * (LARGHEZZA / 2 + e_muro.y * 0.6)
                cima = pz + 15.0
                ox, oy, oz = ruota((c_muro.x * sx, c_muro.y * sy, (c_muro.z + e_muro.z) * sz), yaw, pitch)
                if metti(muretto, unreal.Vector(cx - ox, cy - oy, cima - oz), r, unreal.Vector(sx, sy, sz),
                         "Sentiero dei nani - muretto %d.%d" % (k + 1, q + 1)):
                    conti["muretti"] += 1
        # 4. qualche sasso ai gomiti, dal lato a monte, perche' sembri un sentiero vecchio
        if sasso and k > 0 and caso() < 0.6:
            sx_, sy_ = -uy, ux
            lati = []
            for s in (1.0, -1.0):
                qz = g.quota(a[0] + sx_ * s * (LARGHEZZA / 2 + 120.0), a[1] + sy_ * s * (LARGHEZZA / 2 + 120.0))
                if qz is not None:
                    lati.append((qz, s))
            if lati:
                qz, s = max(lati)
                px, py = a[0] + sx_ * s * (LARGHEZZA / 2 + 120.0), a[1] + sy_ * s * (LARGHEZZA / 2 + 120.0)
                sc = 0.5 + caso() * 0.4
                if metti(sasso, unreal.Vector(px, py, qz - 15.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=caso() * 360.0),
                         unreal.Vector(sc, sc, sc), "Sentiero dei nani - sasso %d" % (k + 1)):
                    conti["sassi"] += 1
    return conti


# ---------------------------------------------------------------- principale

def principale():
    via = togli_vecchi()
    if via:
        scrivi("tolti %d pezzi del sentiero di prima" % via)
    if MODO == "togli":
        scrivi("FATTO: sentiero tolto")
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
        return
    grotta = cerca_grotta()
    if grotta is None:
        avviso("nel livello non c'e' l'attore 'Grotta nord': prima il villaggio (sezione Grotta nord)")
        return
    gp = grotta.get_actor_location()
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    scrivi("la grotta del nord e' a (%.0f, %.0f, %.0f); il portale guarda a sud" % (gp.x, gp.y, gp.z))

    g = Griglia(mondo, gp.x, gp.y + PARTENZA_Y, gp.z + 5000.0)
    pend = [[g.pendenza(i, j) for j in range(g.ny)] for i in range(g.nx)]
    partenza = (int(round(LARGO_X / PASSO)), 0)
    if g.z[partenza[0]][0] is None:
        avviso("non trovo il terreno ai piedi della frana: niente sentiero")
        return
    scrivi("partenza ai piedi della frana: quota %.0f, pendenza del terreno li' %.0f%%"
           % (g.z[partenza[0]][0], pend[partenza[0]][0] * 100))

    arrivi = arrivi_buoni(g, pend)
    scrivi("terreno dove si cammina gia' bene, collegato alla valle: %d celle su %d" % (len(arrivi), g.nx * g.ny))
    if not arrivi:
        avviso("nella griglia (%.0f x %.0f m a sud del portale) non c'e' terreno camminabile collegato alla valle: "
               "allargare LARGO_X o LUNGO_Y" % (2 * LARGO_X / 100, LUNGO_Y / 100))
        return
    if partenza in arrivi:
        scrivi("NOTA: ai piedi della frana si cammina gia' fino alla valle; il sentiero si fa lo stesso, piu' comodo")
        # allora l'arrivo e' dove il pendio smette di essere ripido (sotto il 15%) e piu' in basso di 5 m
        z0 = g.z[partenza[0]][0]
        arrivi = set(c for c in arrivi if pend[c[0]][c[1]] <= 0.15 and g.z[c[0]][c[1]] < z0 - 500.0) or arrivi

    celle = cerca_strada(g, pend, partenza, arrivi)
    if not celle:
        avviso("non trovo una strada sotto il %d%% di salita: la montagna qui e' una parete. Allargare la griglia "
               "o alzare SALITA_MAX" % int(SALITA_MAX * 100))
        return
    punti = [(g.xy(i, j)[0], g.xy(i, j)[1], g.z[i][j]) for i, j in celle]
    # un primo pezzo dritto dal portale ai piedi della frana (la frana stessa resta com'e')
    scrivi("strada sulla griglia: %d celle" % len(punti))
    punti = raddrizza(g, punti)
    punti = alza_sopra_terreno(g, punti)

    lunghezza = 0.0
    salita_max = 0.0
    tornanti = 0
    for k, (a, b) in enumerate(zip(punti, punti[1:])):
        piano = math.hypot(b[0] - a[0], b[1] - a[1])
        lunghezza += piano
        if piano > 1.0:
            salita_max = max(salita_max, abs(b[2] - a[2]) / piano)
        if 0 < k < len(punti) - 1:
            c = punti[k + 1] if k + 1 < len(punti) else None
            p = punti[k - 1]
            v1 = (a[0] - p[0], a[1] - p[1])
            v2 = (b[0] - a[0], b[1] - a[1])
            n1, n2 = math.hypot(*v1), math.hypot(*v2)
            if n1 > 1 and n2 > 1 and (v1[0] * v2[0] + v1[1] * v2[1]) / (n1 * n2) < -0.3 and c is not None:
                tornanti += 1
    arrivo = punti[-1]
    scrivi("sentiero: %d tratti, %.0f m, dislivello %.0f m, %d tornanti, salita piu' forte %.0f%%"
           % (len(punti) - 1, lunghezza / 100.0, (punti[0][2] - arrivo[2]) / 100.0, tornanti, salita_max * 100))
    scrivi("arriva a (%.0f, %.0f, %.0f): da li' si cammina gia' fino alla valle" % arrivo)
    for k, p in enumerate(punti):
        scrivi("  punto %2d: (%.0f, %.0f, %.0f)" % (k + 1, p[0], p[1], p[2]))

    conti = costruisci(g, punti, mondo)
    scrivi("messo: %d tratti di rampa (invisibili, reggono i piedi), %d lastre, %d pezzi di muretto, %d sassi"
           % (conti["rampa"], conti["lastre"], conti["muretti"], conti["sassi"]))
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("salvato il livello")
    scrivi("FATTO. Per provare: Play, vai alla grotta del nord (F9 -> Mondo, o Valdorso.Grotta.Dentro e poi esci) e scendi "
           "a piedi; o parti dalla valle e cerca le lastre.")


try:
    with unreal.ScopedEditorTransaction("Valdorso: sentiero dei nani"):
        principale()
except Exception as e:  # noqa: BLE001
    avviso("errore: %s" % e)
    raise
finally:
    salva_resoconto()
