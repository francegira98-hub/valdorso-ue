# Valdorso - I PNG che si siedono, dormono e aprono le porte (14/11/2026, scritto da Claude mentre Fra era fuori,
# approvato da Fra: "Si', tutto").
#
# Il C++ (ValdorsoPNG.h, FValdorsoTappa) sa fare tre cose nuove in una tappa della giornata:
#   - movenza "Panca": si siede sul posto della panca piu' vicina a Dove, con lo stesso seduto dei coloni;
#   - movenza "Letto": dorme nel letto piu' vicino a Dove, con lo stesso sonno dei coloni (prima la notte spariva);
#   - Porta (e Apri porta): arrivando alla tappa apre (o chiude) la porta che sta li';
#   - (14/11, "PNG che camminano") Strade: la strada a piedi per arrivarci, una per ogni fase del villaggio (vedi sotto,
#     "cammini").
# Questo script le mette nelle giornate dei PNG che ci sono gia' nel livello (quelli di metti_png.py e di crea_volti.py),
# senza toccare il resto. Si puo' rilanciare: rifa' gli stessi posti. Lo lanciano da soli anche crea_volti.py e
# metti_png.py alla fine (perche' loro rifanno i PNG da capo). Menu: Valdorso -> Villaggio -> "PNG: panche, letti e porte".
#
# Cosa cambia (le ore sono quelle della valle):
#   Nonna Edda   alle 19 sulla sua panca accanto al fuoco; alle 22 a letto nella capanna; alle 6 di nuovo al fuoco.
#   Bruno        alle 6:30 apre la locanda e va al bancone; alle 23:30 la chiude e dorme in un letto di sopra.
#   Fulvio       (guardia del passo) dall'1 alle 6 dorme sulla branda della Guardia; Tebaldo veglia.
#   Marta        la sera alla locanda seduta (non piu' in piedi accanto al tavolo).
#   Teodoro      alle 6 apre il mulino; la sera seduto nel suo angolo della locanda.
#   Agnese       a meta' mattina seduta sulla panca del pozzo (le voci); alle 21 chiude il mulino.
#   Elric        la sera seduto nel tempio; di notte dorme alla locanda (e' un ospite della valle).
#   Sibilla      (i giorni della carovana) la sera seduta alla locanda, di notte in un letto della locanda.
# Se una panca, un letto o una porta non si trovano, quella parte si salta e lo scrive.
# Il resoconto in Saved/Claude/giornate_vive.txt.

import math
import os

import unreal

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MEZZA_ALTEZZA = 92.0
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Giornate vive: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Giornate vive: " + testo)


def al_mondo(x, y, gradi, lx, ly):
    a = math.radians(gradi)
    return x + lx * math.cos(a) - ly * math.sin(a), y + lx * math.sin(a) + ly * math.cos(a)


def vettore_zero(v):
    return abs(v.x) + abs(v.y) + abs(v.z) < 1.0


class Valle:
    def __init__(self):
        self.tutti = attori.get_all_level_actors()
        self.per_nome = {}
        for a in self.tutti:
            self.per_nome.setdefault(a.get_actor_label(), a)
        c_panca = getattr(unreal, "ValdorsoPanca", None)
        c_letto = getattr(unreal, "ValdorsoLetto", None)
        c_png = getattr(unreal, "ValdorsoPNG", None)
        self.panche = [a for a in self.tutti if c_panca and isinstance(a, c_panca)]
        self.letti = [a for a in self.tutti if c_letto and isinstance(a, c_letto)]
        self.png = {}
        for a in self.tutti:
            if c_png and isinstance(a, c_png):
                self.png.setdefault(str(a.get_editor_property("dialogo")), a)
        self.usati = set()

    def _sedili(self, mobile, panca):
        q = mobile.get_actor_location()
        g = mobile.get_actor_rotation().yaw
        posti = list(mobile.get_editor_property("posti")) or [0.0]
        avanti = mobile.get_editor_property("avanti_posto") if panca else 0.0
        fuori = []
        for i, lungo in enumerate(posti):
            # la panca: i posti lungo la Y, un poco avanti in X; il letto: i posti di traverso, lungo la X
            x, y = al_mondo(q.x, q.y, g, avanti, lungo) if panca else al_mondo(q.x, q.y, g, lungo, 0.0)
            fuori.append((i, x, y, q.z, g))
        return fuori

    def posto(self, vicino, panca, raggio, chi):
        """Il posto libero (di una panca o di un letto) piu' vicino a vicino (x, y, z): (x, y, z_piedi, gradi) o None."""
        if vicino is None:
            return None
        x0, y0, z0 = vicino
        migliore = None
        for m in (self.panche if panca else self.letti):
            for i, x, y, z, g in self._sedili(m, panca):
                chiave = (m.get_path_name(), i)
                if chiave in self.usati or abs(z - z0) > 400.0:
                    continue
                d = math.hypot(x - x0, y - y0)
                if d <= raggio and (migliore is None or d < migliore[0]):
                    migliore = (d, x, y, z, g, chiave, m.get_actor_label())
        if migliore is None:
            avviso("%s: nessun%s entro %.0f m" % (chi, "a panca" if panca else " letto", raggio / 100.0))
            return None
        self.usati.add(migliore[5])
        scrivi("%s: %s \"%s\" a %.1f m" % (chi, "panca" if panca else "letto", migliore[6], migliore[0] / 100.0))
        return migliore[1:5]

    def porta(self, nome, chi):
        a = self.per_nome.get(nome)
        if a is None:
            avviso("%s: non trovo la porta \"%s\"" % (chi, nome))
            return None
        return a.get_actor_location()


def leggi(t):
    d = {k: t.get_editor_property(k) for k in ("ora", "dove", "gradi", "movenza", "assente", "porta", "apri_porta")}
    try:
        d["strade"] = list(t.get_editor_property("strade"))
    except Exception:  # noqa: BLE001
        pass
    return d


def fai(d):
    t = unreal.ValdorsoTappa()
    for k, v in d.items():
        t.set_editor_property(k, v)
    return t


def tappa(ora, posto=None, movenza="", assente=False, porta=None, apri=True, base=None):
    """Una tappa (come dizionario): posto (x, y, z_piedi, gradi) o base (una tappa letta) per il posto."""
    d = {"ora": float(ora), "dove": unreal.Vector(0, 0, 0), "gradi": 0.0, "movenza": unreal.Name(movenza),
         "assente": bool(assente), "porta": unreal.Vector(0, 0, 0), "apri_porta": bool(apri)}
    if base is not None:
        d["dove"], d["gradi"] = base["dove"], base["gradi"]
    if posto is not None:
        x, y, z, g = posto
        d["dove"] = unreal.Vector(x, y, z + MEZZA_ALTEZZA)
        d["gradi"] = float(g)
    if porta is not None:
        d["porta"] = porta
    return d


def centro(d):
    v = d["dove"]
    return None if vettore_zero(v) else (v.x, v.y, v.z - MEZZA_ALTEZZA)


def giornata(a):
    return [leggi(t) for t in a.get_editor_property("giornata")]


def salva(a, nome, giorno):
    giorno = sorted(giorno, key=lambda d: d["ora"])
    a.set_editor_property("giornata", [fai(d) for d in giorno])
    def cosa(d):
        if d["assente"]:
            return "a casa"
        m = str(d["movenza"])
        return "in piedi" if m in ("", "None") else m
    scrivi("%s: %s" % (nome, ", ".join("%g %s%s" % (d["ora"], cosa(d), "" if vettore_zero(d["porta"]) else
                                                   (" (apre la porta)" if d["apri_porta"] else " (chiude la porta)"))
                                       for d in giorno)))


def base_fissa(a, giorno, ora):
    """La tappa di giorno di un PNG messo da metti_png.py (senza giornata: dove l'hanno messo)."""
    for d in giorno:
        if not d["assente"] and str(d["movenza"]) in ("", "None") and not vettore_zero(d["dove"]):
            d = dict(d)
            d["ora"] = float(ora)
            return d
    p = a.get_actor_location()
    return {"ora": float(ora), "dove": p, "gradi": float(a.get_actor_rotation().yaw), "movenza": unreal.Name(""),
            "assente": False, "porta": unreal.Vector(0, 0, 0), "apri_porta": True}


def cambia(giorno, ora, nuova):
    """Mette nuova al posto della tappa di quell'ora (o la aggiunge)."""
    fuori = [d for d in giorno if abs(d["ora"] - ora) > 0.01]
    if nuova is not None:
        fuori.append(nuova)
    return fuori


def alla(giorno, ora):
    for d in giorno:
        if abs(d["ora"] - ora) <= 0.01:
            return d
    return None


def seduto(V, giorno, ora, chi, raggio=600.0):
    """La tappa di quell'ora diventa "seduto sulla panca piu' vicina" (se ce n'e' una)."""
    d = alla(giorno, ora)
    if d is None or d["assente"]:
        avviso("%s: manca la tappa delle %g" % (chi, ora))
        return giorno
    p = V.posto(centro(d), True, raggio, chi)
    if p is None:
        return giorno
    nuova = tappa(ora, p, "Panca", porta=d["porta"] if not vettore_zero(d["porta"]) else None, apri=d["apri_porta"])
    return cambia(giorno, ora, nuova)


# =====================================================================================================================
# (14/11, "PNG che camminano") Le strade dei PNG da una tappa all'altra, calcolate qui sulla pianta del villaggio
# (V:/Texture/valdorso/villaggio/Pianta_3000.json: case, muri, palizzate, strade, ponti) e scritte nella tappa
# (Percorso, PorteSulPercorso). Il gioco poi le percorre a piedi. Come:
#   - la pianta diventa una griglia a quadretti di 1 m: le case (un poco piu' grandi, per le spalle), i muri, le
#     palizzate, i recinti e le cose del villaggio (botti, carri, panche) non si attraversano; sulle strade e sui ponti
#     si cammina volentieri (costano meno), sull'erba un po' meno volentieri;
#   - chi sta dentro una casa esce dalla sua porta (dentro, sulla soglia: li' la apre, fuori) e chi arriva in una casa
#     entra dalla porta; "a casa" vuol dire dentro la porta della casa piu' vicina al suo lavoro (Tobia: la canonica);
#   - la strada piu' corta (A*), poi raddrizzata dove si puo' senza lasciare la strada;
#   - (14/11) una strada per ogni fase del villaggio (1 oggi, 2 l'anello di case e la palizzata larga, 3 il muro di
#     pietra): per ogni fase contano solo le case, le palizzate, i muri e le porte che ci sono in quella fase; "a casa"
#     e' una casa che c'e' in tutte le fasi;
#   - non si cammina: tra posti lontani (piu' di 300 m: le Rovine), su e giu' dai piani (i letti di sopra della
#     locanda: li' entra e poi compare di sopra), dentro la stessa casa, e alle porte alte (la capanna di Nonna Edda);
#     allora cambia posto come prima, quando nessuno lo vede.

import heapq
import json

PIANTA = "V:/Texture/valdorso/villaggio/Pianta_3000.json"
VILLAGGIO = "V:/Texture/valdorso/villaggio/Villaggio_3000.json"
QUADRETTO = 100.0           # cm
SPALLE = 45.0               # quanto si allargano le case e i muri (il PNG e' largo 72 cm)
MARGINE = 2500.0            # quanto oltre i due posti si guarda la griglia
LONTANO = 30000.0           # oltre, niente cammino
COSTO_STRADA = 1.0
COSTO_ERBA = 2.2
MAX_NODI = 400000
DENTRO = 90.0               # cm dentro la porta
FUORI = 120.0               # cm fuori dalla porta
# (15/11) i mugnai no nel mulino: la porta del mulino e' alta (con i gradini), li' non si cammina; vanno nella casa piu' vicina
CASA_DI = {"Tobia": "Canonica", "Marta": "Fucina_di_Marta"}
# (15/11) le case senza porta (il tempio e' aperto): dove sta l'ingresso, in cm nel sistema dell'edificio
INGRESSI_APERTI = {"Tempio_del_Cuore": (0.0, -1100.0)}
FASI = (1, 2, 3)


def c_e_nella_fase(fase, dati):
    """Come ValdorsoFasi::CeNellaFase: "fase": [da, fino] (fino 0 = per sempre); senza = sempre."""
    f = dati.get("fase")
    if not isinstance(f, list) or len(f) < 2:
        return True
    da, fino = int(f[0]), int(f[1])
    return fase >= da and (fino == 0 or fase <= fino)


def sempre(dati):
    return all(c_e_nella_fase(f, dati) for f in FASI)
PORTE_DI = {"Capanna di Edda": "Capanna di Nonna Edda"}


def dentro_poligono(x, y, punti):
    dentro = False
    j = len(punti) - 1
    for i in range(len(punti)):
        xi, yi = punti[i]
        xj, yj = punti[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / ((yj - yi) or 1e-9) + xi:
            dentro = not dentro
        j = i
    return dentro


def distanza_segmento(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    l2 = dx * dx + dy * dy
    t = 0.0 if l2 < 1e-6 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / l2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


class Pianta:
    def __init__(self, angolo, fase=1):
        self.ok = False
        self.fase = fase
        self.ax, self.ay = angolo.x, angolo.y
        try:
            with open(PIANTA, "r", encoding="utf-8") as f:
                pianta = json.load(f)
            with open(VILLAGGIO, "r", encoding="utf-8") as f:
                cose = json.load(f).get("cose", [])
        except Exception as e:  # noqa: BLE001
            avviso("cammini: non leggo la pianta (%s): i PNG cambiano posto come prima" % e)
            return
        # tutto in coordinate del mondo (x, y): la pianta e' in cm dall'angolo del paesaggio
        m = lambda p: (p[0] + self.ax, p[1] + self.ay)  # noqa: E731
        pianta = [c for c in pianta if c_e_nella_fase(fase, c)]
        cose = [c for c in cose if c_e_nella_fase(fase, c)]
        self.case = [(c["nome"], [m(p) for p in c["punti"]]) for c in pianta if c.get("tipo") == "edificio"]
        self.case_sempre = {c["nome"] for c in pianta if c.get("tipo") == "edificio" and sempre(c)}
        self.linee = []
        self.strade = []
        for c in pianta:
            t = c.get("tipo")
            punti = [m(p) for p in c.get("punti", [])]
            nome = c.get("nome", "")
            if t == "strada" or (t == "linea" and ("Ponte" in nome or "Passerella" in nome)):
                self.strade.append((punti, max(250.0, float(c.get("larga", 0) or 300)) / 2.0))
            elif t in ("palizzata", "muro") or (t == "linea" and nome in ("Cortile", "Cimitero", "Orto", "Recinto", "Lotto")):
                self.linee.append(punti)
        self.cose = []
        for c in cose:
            if c.get("tipo") in ("fab", "panca", "bacheca", "letto") and c.get("pos"):
                x, y = m(c["pos"])
                self.cose.append((x, y, 70.0 if c.get("tipo") != "panca" else 110.0))
        # (15/11) gli alberi in citta' (crea_alberi_citta.py): il tronco e un po' di spazio intorno
        try:
            for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
                if a and a.actor_has_tag("ValdorsoAlberoCitta"):
                    q = a.get_actor_location()
                    self.cose.append((q.x, q.y, 90.0 * max(1.0, a.get_actor_scale3d().x)))
        except Exception:  # noqa: BLE001
            pass
        self.porte = {}
        for c in cose:
            if c.get("tipo") == "porta" and c.get("pos"):
                x, y = m(c["pos"])
                alta = float(c["pos"][2]) - float(c.get("terreno", c["pos"][2])) > 60.0
                self.porte[c["nome"]] = (x, y, float(c.get("gradi", 0.0)), alta)
        self.ok = True
        scrivi("cammini: pianta della fase %d (%d case, %d strade, %d muri e recinti, %d cose, %d porte)"
               % (fase, len(self.case), len(self.strade), len(self.linee), len(self.cose), len(self.porte)))

    def casa_di(self, x, y):
        for nome, punti in self.case:
            if dentro_poligono(x, y, punti):
                return nome, punti
        return None, None

    def porta_di(self, casa, punti, V):
        """La porta di fuori di una casa: (dentro, soglia, fuori) come (x, y, z) del mondo, o None."""
        nome = PORTE_DI.get(casa, casa) + " porta"
        dati = self.porte.get(nome)
        a = V.per_nome.get(nome)
        if (dati is None or a is None) and casa in INGRESSI_APERTI:
            # un ingresso senza porta: si passa senza fermarsi (niente da aprire)
            e = V.per_nome.get(casa)
            if e is None:
                return None
            q = e.get_actor_location()
            g = e.get_actor_rotation().yaw
            lx, ly = INGRESSI_APERTI[casa]
            x, y = al_mondo(q.x, q.y, g, lx, ly)
            n = math.hypot(x - q.x, y - q.y) or 1.0
            nx, ny = (x - q.x) / n, (y - q.y) / n
            z = q.z
            return ((x - nx * DENTRO, y - ny * DENTRO, z), None, (x + nx * FUORI, y + ny * FUORI, z))
        if dati is None or a is None:
            return None
        if dati[3]:
            return None                                # una porta alta (con la scaletta): niente cammino
        q = a.get_actor_location()
        cx = sum(p[0] for p in punti) / len(punti)
        cy = sum(p[1] for p in punti) / len(punti)
        g = math.radians(a.get_actor_rotation().yaw)
        nx, ny = math.cos(g), math.sin(g)
        if (q.x - cx) * nx + (q.y - cy) * ny < 0:     # la normale verso fuori dalla casa
            nx, ny = -nx, -ny
        z = q.z
        return ((q.x - nx * DENTRO, q.y - ny * DENTRO, z), (q.x, q.y, z), (q.x + nx * FUORI, q.y + ny * FUORI, z))

    # --- la griglia e la strada piu' corta
    def strada(self, da, a):
        x0, x1 = min(da[0], a[0]) - MARGINE, max(da[0], a[0]) + MARGINE
        y0, y1 = min(da[1], a[1]) - MARGINE, max(da[1], a[1]) + MARGINE
        w = int((x1 - x0) / QUADRETTO) + 1
        h = int((y1 - y0) / QUADRETTO) + 1
        chiuso = bytearray(w * h)
        costo = [COSTO_ERBA] * (w * h)

        def cella(x, y):
            return int((x - x0) / QUADRETTO), int((y - y0) / QUADRETTO)

        def centro(i, j):
            return x0 + (i + 0.5) * QUADRETTO, y0 + (j + 0.5) * QUADRETTO

        def segna_vicino_a_segmenti(punti, raggio, fai_cella):
            for k in range(len(punti) - 1):
                (ax_, ay_), (bx_, by_) = punti[k], punti[k + 1]
                ia, ja = cella(min(ax_, bx_) - raggio, min(ay_, by_) - raggio)
                ib, jb = cella(max(ax_, bx_) + raggio, max(ay_, by_) + raggio)
                for i in range(max(0, ia), min(w, ib + 1)):
                    for j in range(max(0, ja), min(h, jb + 1)):
                        cx_, cy_ = centro(i, j)
                        if distanza_segmento(cx_, cy_, ax_, ay_, bx_, by_) <= raggio:
                            fai_cella(i, j)

        def strada_qui(i, j):
            costo[j * w + i] = COSTO_STRADA

        def muro_qui(i, j):
            chiuso[j * w + i] = 1

        for punti, mezza in self.strade:
            segna_vicino_a_segmenti(punti, mezza, strada_qui)
        for nome, punti in self.case:
            xs = [p[0] for p in punti]
            ys = [p[1] for p in punti]
            if max(xs) < x0 or min(xs) > x1 or max(ys) < y0 or min(ys) > y1:
                continue
            ia, ja = cella(min(xs) - SPALLE, min(ys) - SPALLE)
            ib, jb = cella(max(xs) + SPALLE, max(ys) + SPALLE)
            chiusa = punti + [punti[0]]
            for i in range(max(0, ia), min(w, ib + 1)):
                for j in range(max(0, ja), min(h, jb + 1)):
                    cx_, cy_ = centro(i, j)
                    if dentro_poligono(cx_, cy_, punti) or any(
                            distanza_segmento(cx_, cy_, chiusa[k][0], chiusa[k][1], chiusa[k + 1][0], chiusa[k + 1][1]) < SPALLE
                            for k in range(len(punti))):
                        chiuso[j * w + i] = 1
        for punti in self.linee:
            segna_vicino_a_segmenti(punti, SPALLE, muro_qui)
        for x, y, r in self.cose:
            if x0 <= x <= x1 and y0 <= y <= y1:
                segna_vicino_a_segmenti([(x, y), (x, y)], r, muro_qui)

        def libera_vicina(x, y):
            i, j = cella(x, y)
            for r in range(0, 4):
                for di in range(-r, r + 1):
                    for dj in range(-r, r + 1):
                        ii, jj = i + di, j + dj
                        if 0 <= ii < w and 0 <= jj < h and not chiuso[jj * w + ii]:
                            return ii, jj
            return None

        s = libera_vicina(da[0], da[1])
        g = libera_vicina(a[0], a[1])
        if s is None or g is None:
            return None
        gi, gj = g
        aperti = [(0.0, 0.0, s)]
        venuto = {s: None}
        speso = {s: 0.0}
        nodi = 0
        while aperti:
            _, c, nodo = heapq.heappop(aperti)
            if nodo == g:
                break
            if c > speso.get(nodo, 1e18):
                continue
            nodi += 1
            if nodi > MAX_NODI:
                return None
            i, j = nodo
            for di, dj, passo in ((1, 0, 1.0), (-1, 0, 1.0), (0, 1, 1.0), (0, -1, 1.0),
                                  (1, 1, 1.414), (1, -1, 1.414), (-1, 1, 1.414), (-1, -1, 1.414)):
                ii, jj = i + di, j + dj
                if not (0 <= ii < w and 0 <= jj < h) or chiuso[jj * w + ii]:
                    continue
                if di and dj and (chiuso[j * w + ii] or chiuso[jj * w + i]):
                    continue                            # non taglia gli spigoli delle case
                nc = c + passo * costo[jj * w + ii]
                if nc < speso.get((ii, jj), 1e18):
                    speso[(ii, jj)] = nc
                    venuto[(ii, jj)] = nodo
                    heapq.heappush(aperti, (nc + math.hypot(ii - gi, jj - gj) * COSTO_STRADA, nc, (ii, jj)))
        if g not in venuto:
            return None
        celle = []
        n = g
        while n is not None:
            celle.append(n)
            n = venuto[n]
        celle.reverse()

        # raddrizzata: da ogni punto, il piu' lontano che si vede senza muri e senza costare di piu'
        def costo_linea(p, q):
            (pi_, pj_), (qi_, qj_) = p, q
            passi = max(abs(qi_ - pi_), abs(qj_ - pj_)) * 2 + 1
            tot = 0.0
            for k in range(passi + 1):
                t = k / passi
                i = int(round(pi_ + (qi_ - pi_) * t))
                j = int(round(pj_ + (qj_ - pj_) * t))
                if chiuso[j * w + i]:
                    return None
                tot += costo[j * w + i]
            return tot / (passi + 1) * math.hypot(qi_ - pi_, qj_ - pj_)

        dritti = [celle[0]]
        k = 0
        while k < len(celle) - 1:
            migliore = k + 1
            for m_ in range(len(celle) - 1, k + 1, -1):
                cl = costo_linea(celle[k], celle[m_])
                if cl is not None and cl <= (speso[celle[m_]] - speso[celle[k]]) * 1.03 + 0.5:
                    migliore = m_
                    break
            dritti.append(celle[migliore])
            k = migliore
        punti = [centro(i, j) for i, j in dritti]
        punti[0] = (da[0], da[1])
        punti[-1] = (a[0], a[1])
        return punti


def quota(x, y, z_vicino):
    """La quota dei piedi in (x, y), vicino a z_vicino (il paesaggio o un pavimento)."""
    try:
        mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        r = unreal.SystemLibrary.line_trace_multi_for_objects(
            mondo, unreal.Vector(x, y, z_vicino + 200.0), unreal.Vector(x, y, z_vicino - 600.0),
            [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], True, [], unreal.DrawDebugTrace.NONE, True)
        colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
        zs = []
        for hit in colpi or []:
            t = hit.to_tuple()
            zs.append(t[5].z if t[5] is not None else t[4].z)
        if zs:
            return max(zs)
    except Exception:  # noqa: BLE001
        pass
    return z_vicino


def luogo_della_tappa(d, chi, P, V, casa):
    """Dove sta in quella tappa: (x, y, z_piedi) del punto dove arriva a piedi, e la casa in cui e' (o None)."""
    if d["assente"]:
        return casa
    if vettore_zero(d["dove"]):
        return None
    v = d["dove"]
    x, y, z = v.x, v.y, v.z - MEZZA_ALTEZZA
    g = math.radians(d["gradi"])
    m = str(d["movenza"])
    if m == "Panca":                                   # davanti alla panca (poi si siede)
        x, y = x + math.cos(g) * 70.0, y + math.sin(g) * 70.0
    elif m == "Letto":                                 # accanto al letto (poi si corica)
        x, y = x + math.cos(g) * 110.0, y + math.sin(g) * 110.0
    nome, punti = P.casa_di(x, y)
    return {"p": (x, y, z), "casa": nome, "punti": punti, "a_casa": False}


def casa_del_png(chi, giorno, P, V):
    """Dove va "a casa": dentro la porta della casa piu' vicina al suo primo posto del giorno (Tobia: la canonica)."""
    voluta = CASA_DI.get(chi)
    primo = None
    for d in sorted(giorno, key=lambda d: d["ora"]):
        if not d["assente"] and not vettore_zero(d["dove"]):
            primo = (d["dove"].x, d["dove"].y)
            break
    migliore = None
    for nome, punti in P.case:
        if voluta and nome != voluta:
            continue
        if not voluta and (not nome.startswith("Casa_") or nome not in P.case_sempre):
            continue
        porta = P.porta_di(nome, punti, V)
        if porta is None:
            continue
        d_ = 0.0 if primo is None else math.hypot(porta[2][0] - primo[0], porta[2][1] - primo[1])
        if migliore is None or d_ < migliore[0]:
            migliore = (d_, nome, punti, porta)
    if migliore is None:
        return None
    return {"p": migliore[3][0], "casa": migliore[1], "punti": migliore[2], "a_casa": True}


def cammino(A, B, P, V, chi):
    """La strada da A a B: (punti come unreal.Vector, indici delle soglie) o None (allora niente cammino)."""
    if A is None or B is None:
        return None
    if math.hypot(A["p"][0] - B["p"][0], A["p"][1] - B["p"][1]) > LONTANO:
        return None
    if A["casa"] and A["casa"] == B["casa"]:
        return None                                    # dentro la stessa casa (su e giu' dai piani): come prima
    testa, soglie, coda = [], [], []
    da = A["p"]
    if A["casa"]:
        porta = P.porta_di(A["casa"], A["punti"], V)
        if porta is None:
            return None
        if abs(A["p"][2] - porta[0][2]) <= 150.0:
            testa.append(A["p"])
        testa.append(porta[0])
        if porta[1] is not None:
            soglie.append(len(testa))
            testa.append(porta[1])
        testa.append(porta[2])
        da = porta[2]
    else:
        testa.append(A["p"])
    a = B["p"]
    if B["casa"]:
        porta = P.porta_di(B["casa"], B["punti"], V)
        if porta is None:
            return None
        coda = [porta[2]] + ([porta[1]] if porta[1] is not None else []) + [porta[0]]
        if not B["a_casa"] and abs(B["p"][2] - porta[0][2]) <= 150.0:
            coda.append(B["p"])
        soglia_in_coda = porta[1] is not None
        a = porta[2]
    mezzo = P.strada(da, a)
    if mezzo is None:
        avviso("%s: non trovo la strada da (%.0f, %.0f) a (%.0f, %.0f)" % (chi, da[0], da[1], a[0], a[1]))
        return None
    punti = list(testa)
    z = testa[-1][2]
    for x, y in mezzo[1:-1]:
        z = quota(x, y, z)
        punti.append((x, y, z))
    if coda:
        punti.append(coda[0])
        if soglia_in_coda:
            soglie.append(len(punti))
        punti += coda[1:]
    else:
        punti.append(a)
    # toglie i punti doppi (a meno di 10 cm), tenendo giuste le soglie
    puliti, nuove = [], []
    for k, p in enumerate(punti):
        if puliti and math.hypot(p[0] - puliti[-1][0], p[1] - puliti[-1][1]) < 10.0 and k not in soglie:
            continue
        if k in soglie:
            nuove.append(len(puliti))
        puliti.append(p)
    lungo = sum(math.hypot(puliti[k][0] - puliti[k - 1][0], puliti[k][1] - puliti[k - 1][1]) for k in range(1, len(puliti)))
    vettori = [unreal.Vector(p[0], p[1], p[2] + MEZZA_ALTEZZA) for p in puliti]
    return vettori, nuove, lungo


def metti_cammini(V):
    paesaggi = [a for a in V.tutti if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        avviso("cammini: nel livello non c'e' il paesaggio (apri L_Valle)")
        return
    angolo = paesaggi[0].get_actor_location()
    piante = [Pianta(angolo, f) for f in FASI]
    if not all(P.ok for P in piante):
        return
    for dialogo, a in sorted(V.png.items()):
        giorno = sorted(giornata(a), key=lambda d: d["ora"])
        if len(giorno) < 2:
            continue
        chi = a.get_actor_label().split(",")[0]
        assente = any(d["assente"] for d in giorno)
        # la casa: la stessa in tutte le fasi (scelta sulla pianta della fase 1, tra le case che ci sono sempre)
        casa = casa_del_png(dialogo, giorno, piante[0], V) if assente else None
        for d in giorno:
            d["strade"] = []
        righe_png = []
        for fase, P in zip(FASI, piante):
            luoghi = [luogo_della_tappa(d, chi, P, V, casa) for d in giorno]
            fatti = []
            for k, d in enumerate(giorno):
                strada = unreal.ValdorsoStrada()
                r = cammino(luoghi[k - 1], luoghi[k], P, V, chi)
                if r is None:
                    fatti.append("%g no" % d["ora"])
                else:
                    strada.set_editor_property("punti", r[0])
                    strada.set_editor_property("porte", r[1])
                    fatti.append("%g %.0f m%s" % (d["ora"], r[2] / 100.0, (" (%d porte)" % len(r[1])) if r[1] else ""))
                d["strade"].append(strada)
            righe_png.append("fase %d: %s" % (fase, ", ".join(fatti)))
        a.set_editor_property("giornata", [fai(d) for d in giorno])
        scrivi("cammini di %s%s: %s" % (chi, (" (casa: %s)" % casa["casa"]) if casa else "", " | ".join(righe_png)))


def principale():
    try:
        unreal.ValdorsoTappa().get_editor_property("apri_porta")
    except Exception:  # noqa: BLE001
        unreal.log_error("[Valdorso] Giornate vive: il C++ e' vecchio (manca Porta nella tappa). Compila Valdorso e riapri l'editor.")
        return
    V = Valle()
    scrivi("%d PNG, %d panche, %d letti nel livello" % (len(V.png), len(V.panche), len(V.letti)))
    locanda = V.per_nome.get("Locanda_di_Bruno")
    p_locanda = None
    if locanda is not None:
        q = locanda.get_actor_location()
        p_locanda = (q.x, q.y, q.z)
    porta_locanda = V.porta("Locanda_di_Bruno porta", "Bruno")
    porta_mulino = V.porta("Mulino porta", "Teodoro")

    # --- i PNG di metti_png.py (senza giornata: la si fa qui)
    a = V.png.get("NonnaEdda")
    if a:
        g = giornata(a)
        b = base_fissa(a, g, 6.0)
        giorno = [b]
        p = V.posto(centro(b), True, 800.0, "Nonna Edda")
        if p:
            giorno.append(tappa(19.0, p, "Panca"))
        p = V.posto(centro(b), False, 1500.0, "Nonna Edda")
        if p:
            giorno.append(tappa(22.0, p, "Letto"))
        salva(a, "Nonna Edda", giorno)

    a = V.png.get("Bruno")
    if a:
        g = giornata(a)
        b = base_fissa(a, g, 6.5)
        if porta_locanda is not None:
            b["porta"], b["apri_porta"] = porta_locanda, True
        giorno = [b]
        p = V.posto(p_locanda or centro(b), False, 1500.0, "Bruno")
        if p:
            giorno.append(tappa(23.5, p, "Letto", porta=porta_locanda, apri=False))
        elif porta_locanda is not None:
            giorno.append(tappa(23.5, None, assente=True, porta=porta_locanda, apri=False))
        salva(a, "Bruno", giorno)

    a = V.png.get("GuardiaOttone")
    if a:
        g = giornata(a)
        b = base_fissa(a, g, 6.0)
        giorno = [b]
        p = V.posto(centro(b), False, 3000.0, "Fulvio")
        if p:
            giorno.append(tappa(1.0, p, "Letto"))
        salva(a, "Fulvio", giorno)

    # --- i PNG di crea_volti.py (hanno gia' la giornata: si cambiano solo alcune tappe)
    a = V.png.get("Marta")
    if a:
        salva(a, "Marta", seduto(V, giornata(a), 19.0, "Marta"))

    a = V.png.get("Teodoro")
    if a:
        giorno = seduto(V, giornata(a), 18.0, "Teodoro")
        d = alla(giorno, 6.0)
        if d is not None and porta_mulino is not None:
            d["porta"], d["apri_porta"] = porta_mulino, True
        salva(a, "Teodoro", giorno)

    a = V.png.get("Agnese")
    if a:
        giorno = seduto(V, giornata(a), 10.0, "Agnese", 800.0)
        d = alla(giorno, 21.0)
        if d is not None and porta_mulino is not None:
            d["porta"], d["apri_porta"] = porta_mulino, False
        salva(a, "Agnese", giorno)

    a = V.png.get("Elric")
    if a:
        giorno = seduto(V, giornata(a), 18.0, "Elric")
        p = V.posto(p_locanda, False, 1500.0, "Elric")
        if p:
            giorno = cambia(giorno, 21.5, tappa(21.5, p, "Letto"))
        salva(a, "Elric", giorno)

    a = V.png.get("Sibilla")
    if a:
        giorno = giornata(a)
        if p_locanda:
            p = V.posto(p_locanda, True, 1000.0, "Sibilla")
            if p:
                giorno = cambia(giorno, 20.0, tappa(20.0, p, "Panca"))
            p = V.posto(p_locanda, False, 1500.0, "Sibilla")
            if p:
                giorno = cambia(giorno, 22.5, tappa(22.5, p, "Letto"))
        salva(a, "Sibilla", giorno)

    # (14/11) le strade da una tappa all'altra
    try:
        metti_cammini(Valle())
    except Exception as e:  # noqa: BLE001
        avviso("cammini non fatti (%s): i PNG cambiano posto come prima" % e)
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): Ctrl+Maiusc+S" % e)
    scrivi("FATTO. In Play, per vederli subito (cambiano posto quando nessun colono e' vicino, o dopo 10 minuti): "
           "Valdorso.Ora 20 (la sera: seduti alla locanda e al fuoco di Nonna Edda), Valdorso.Ora 2 (tutti a letto).")


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "giornate_vive.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
