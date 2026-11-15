# Valdorso - Il costruttore di case (15/11/2026, scritto da Claude: passo 2 della v0.2.5 "La valle piu' bella",
# approvata da Fra: "stessa forma, pelle nuova").
#
# Rifa' le case del villaggio con il kit del Medieval Village di Epic (portato con il Migrate in /Game/Megascans e
# /Game/Meshes), sulla stessa pianta: ogni casa nuova sta esattamente nel contorno di quella vecchia
# (V:/Texture/valdorso/villaggio/Pianta_3000.json) e la porta resta dov'e' (l'attore "<casa> porta"). Cosi' le strade dei
# PNG, le porte, le fasi e il Battito non cambiano.
#   - i muri: moduli da 3, 2, 1,5 e 1 m (alti 2 m), due piani, scelti per riempire ogni lato (stirati di pochi cm per
#     chiudere giusto); al piano terra, sul lato della porta, il modulo con la porta proprio dove sta la porta vera;
#     al piano di sopra muri di tipo diverso (varieta'), negli angoli i pilastri d'angolo;
#   - sui lati corti i timpani; sopra, uno dei tetti interi delle case di Epic, scelto per forma e adattato alla misura;
#   - la casa vecchia non si cancella: si spegne (invisibile e senza collisione), cosi' si torna indietro quando si vuole;
#   - i pezzi nuovi prendono le etichette delle fasi della casa vecchia (ValdorsoFase da-fino): compaiono e spariscono
#     con lei.
# Tre voci nel menu Valdorso -> Villaggio:
#   "Case vere: casa di prova (Casa_Sud_1)"   -> solo quella, da guardare insieme;
#   "Case vere: tutte le case"                -> tutte le case della pianta che hanno una casa vecchia nel livello;
#   "Case vere: torna come prima"             -> toglie i pezzi nuovi e riaccende le case vecchie.
# Il resoconto in Saved/Claude/case_vere.txt. Le cose da ritoccare a occhio sono qui sotto (MISURE).

import json
import math
import os

import unreal

MODO = globals().get("CASE_MODO", "prova")         # "prova", "tutte", "indietro"
PROVA = ["Casa_Sud_1"]
PIANTA = "V:/Texture/valdorso/villaggio/Pianta_3000.json"
ETICHETTA = "ValdorsoCasaVera"
SPENTA = "ValdorsoCasaVecchiaSpenta"
CARTELLA = "Valle/CaseVere"
# le case da non rifare mai (hanno dentro le cose dei PNG, o sono speciali): si fanno a mano piu' avanti
MAI = {"Locanda_di_Bruno", "Fucina_di_Marta", "Mulino", "Tempio_del_Cuore", "Canonica", "Capanna di Edda", "Gilda",
       "Casermetta_della_Guardia"}

K = "/Game/Megascans/3D_Assets/"
# (nome del modulo, larghezza in cm): i muri si scelgono tra questi
MURI_TERRA = [("MedievalModularWall3x2M/SM_MedievalModularWall3x2M", 301),
              ("MedievalModularWall2x2MD/SM_MedievalModularWall2x2MD", 202),
              ("MedievalModularWall15x2M/SM_MedievalModularWall15x2M", 156),
              ("MedievalModularWall1x2MA/SM_MedievalModularWall1x2MA", 102)]
MURI_SOPRA = [("MedievalModularWall3x2M/SM_MedievalModularWall3x2M", 301),
              ("MedievalModularWall2x2MG/SM_MedievalModularWall2x2MG", 200),
              ("MedievalModularWall2x2MH/SM_MedievalModularWall2x2MH", 204),
              ("MedievalModularWall15x2M/SM_MedievalModularWall15x2M", 156),
              ("MedievalModularWall1x2MA/SM_MedievalModularWall1x2MA", 102)]
PORTA = ("MedievalModularDoor15x2M/SM_MedievalModularDoor15x2M", 153)
ANGOLO = "MedievalModularCornerWall1x2M/SM_MedievalModularCornerWall1x2M"
TIMPANO = ("MedievalModularGable4M/SM_MedievalModularGable4M", 436)
TETTI = ["/Game/Meshes/Houses/Roofs/House02_Roof/SM_House02_Roof", "SM_Roof06", "SM_House07_Roof01", "SM_House08_Roof",
         "SM_House03_RoofC"]

# MISURE da ritoccare a occhio dopo la prova
MISURE = {
    "piani": 2,                 # quanti giri di muri da 2 m
    "altezza_modulo": 201.0,
    "fuori_e_piu_y": True,      # la faccia bella dei moduli guarda verso +Y del modulo (False: verso -Y)
    "sporgenza_tetto": 70.0,    # quanto il tetto sporge oltre i muri (cm, per lato)
    "pendenza_tetto": 0.55,     # altezza del tetto / larghezza della casa
    "timpani": True,
    "angoli": True,
}

libreria = unreal.EditorAssetLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Case vere: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Case vere: " + testo)


_cache = {}


def modello(nome):
    """Un modello del kit: un percorso intero, o "Cartella/SM_Nome" sotto Megascans, o solo il nome (lo cerca)."""
    if nome in _cache:
        return _cache[nome]
    m = None
    for percorso in ([nome] if nome.startswith("/Game/") else [K + nome]):
        if libreria.does_asset_exist(percorso):
            m = libreria.load_asset(percorso)
    if m is None:
        corto = nome.split("/")[-1]
        registro = unreal.AssetRegistryHelpers.get_asset_registry()
        for cartella in ("/Game/Meshes", "/Game/Megascans"):
            for d in registro.get_assets_by_path(cartella, recursive=True):
                if str(d.asset_name) == corto and str(d.asset_class_path.asset_name) == "StaticMesh":
                    m = libreria.load_asset(str(d.package_name))
                    break
            if m:
                break
    if m is None:
        avviso("manca il modello %s (il Migrate del Medieval Village e' fatto?)" % nome)
    _cache[nome] = m
    return m


def scatola(m):
    b = m.get_bounding_box()
    return b.min, b.max


def riempi(lungo, moduli):
    """Quali moduli (larghezze) mettere in fila per fare 'lungo' cm: il meno storto possibile, pochi pezzi."""
    if lungo < 60.0:
        return []
    migliore = None
    larghe = [w for _, w in moduli]
    # fino a 12 pezzi: si provano le combinazioni in ordine (dalla piu' larga)
    def cerca(resto, scelti, da):
        nonlocal migliore
        somma = sum(scelti)
        if scelti:
            errore = abs(lungo - somma) / lungo
            punti = errore * 10.0 + len(scelti) * 0.02
            if migliore is None or punti < migliore[0]:
                migliore = (punti, list(scelti))
        if len(scelti) >= 12 or somma > lungo * 1.08:
            return
        for i in range(da, len(larghe)):
            scelti.append(larghe[i])
            cerca(resto, scelti, i)
            scelti.pop()
    cerca(lungo, [], 0)
    if migliore is None:
        return []
    fuori = []
    for w in migliore[1]:
        fuori.append(next(m for m in moduli if m[1] == w))
    return fuori


def metti(m, x, y, z, gradi, sx=1.0, sy=1.0, sz=1.0, casa="", tag_fase=None, nome=""):
    a = attori.spawn_actor_from_object(m, unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=gradi))
    if a is None:
        avviso("non riesco a mettere %s" % nome)
        return None
    a.set_actor_scale3d(unreal.Vector(sx, sy, sz))
    a.set_actor_label("%s ~ %s" % (casa, nome))
    a.set_folder_path("%s/%s" % (CARTELLA, casa))
    a.set_editor_property("tags", [unreal.Name(ETICHETTA), unreal.Name("ValdorsoCasaVera " + casa)] + (tag_fase or []))
    return a


def posa_in_fila(m, larghezza_vera, a_x, a_y, b_x, b_y, z, fuori_x, fuori_y, stira, casa, tag_fase, nome):
    """Un modulo lungo il tratto A->B (da sinistra), alla quota z, con la faccia bella verso fuori."""
    mn, mx = scatola(m)
    dx, dy = b_x - a_x, b_y - a_y
    gradi = math.degrees(math.atan2(dy, dx))
    # la faccia bella guarda +Y (o -Y) del modulo: se guarda dentro, si gira di mezzo giro
    ny_x, ny_y = -math.sin(math.radians(gradi)), math.cos(math.radians(gradi))
    verso = 1.0 if MISURE["fuori_e_piu_y"] else -1.0
    if (ny_x * fuori_x + ny_y * fuori_y) * verso < 0:
        gradi += 180.0
        a_x, a_y, b_x, b_y = b_x, b_y, a_x, a_y
    # il centro del tratto, con il centro della scatola del modello sopra (e i piedi a z)
    cx, cy = (a_x + b_x) / 2.0, (a_y + b_y) / 2.0
    ox = (mn.x + mx.x) / 2.0 * stira
    oy = (mn.y + mx.y) / 2.0
    g = math.radians(gradi)
    px = cx - (ox * math.cos(g) - oy * math.sin(g))
    py = cy - (ox * math.sin(g) + oy * math.cos(g))
    return metti(m, px, py, z - mn.z, gradi, sx=stira, casa=casa, tag_fase=tag_fase, nome=nome)


def lato(a, b, porta, z0, casa, tag_fase, fuori):
    """Un lato della casa (da a a b), tutti i piani; porta = distanza dal punto a del centro della porta (o None)."""
    ax, ay = a
    bx, by = b
    lungo = math.hypot(bx - ax, by - ay)
    ux, uy = (bx - ax) / lungo, (by - ay) / lungo
    messi = 0
    for piano in range(MISURE["piani"]):
        z = z0 + piano * MISURE["altezza_modulo"]
        tratti = []
        if piano == 0 and porta is not None:
            mezza = PORTA[1] / 2.0
            inizio = max(0.0, min(lungo - PORTA[1], porta - mezza))
            tratti.append((0.0, inizio, None))
            tratti.append((inizio, inizio + PORTA[1], "porta"))
            tratti.append((inizio + PORTA[1], lungo, None))
        else:
            tratti.append((0.0, lungo, None))
        moduli = MURI_TERRA if piano == 0 else MURI_SOPRA
        for t0, t1, cosa in tratti:
            if t1 - t0 < 30.0:
                continue
            if cosa == "porta":
                fila = [PORTA]
            else:
                fila = riempi(t1 - t0, moduli)
            somma = sum(w for _, w in fila) or 1.0
            stira = (t1 - t0) / somma
            pos = t0
            for nome_m, w in fila:
                m = modello(nome_m)
                if m is None:
                    continue
                w_vero = w * stira
                p0 = (ax + ux * pos, ay + uy * pos)
                p1 = (ax + ux * (pos + w_vero), ay + uy * (pos + w_vero))
                if posa_in_fila(m, w, p0[0], p0[1], p1[0], p1[1], z, fuori[0], fuori[1], stira, casa, tag_fase,
                                "%s p%d" % (nome_m.split("/")[-1].replace("SM_MedievalModular", ""), piano)):
                    messi += 1
                pos += w_vero
    return messi


def costruisci(nome, punti, V):
    vecchia = V["per_nome"].get(nome)
    if vecchia is None:
        avviso("%s: nel livello non c'e' la casa vecchia, la salto" % nome)
        return 0
    if len(punti) != 4:
        avviso("%s: la pianta ha %d angoli (solo le case a 4 angoli, per ora)" % (nome, len(punti)))
        return 0
    z0 = vecchia.get_actor_location().z
    tag_fase = [t for t in vecchia.get_editor_property("tags") if str(t).startswith("ValdorsoFase")]
    cx = sum(p[0] for p in punti) / 4.0
    cy = sum(p[1] for p in punti) / 4.0
    porta_a = V["per_nome"].get(nome + " porta")
    porta = None if porta_a is None else (porta_a.get_actor_location().x, porta_a.get_actor_location().y)
    messi = 0
    lati = []
    for k in range(4):
        a, b = punti[k], punti[(k + 1) % 4]
        mx_, my_ = (a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0
        fuori = (mx_ - cx, my_ - cy)
        n = math.hypot(*fuori) or 1.0
        fuori = (fuori[0] / n, fuori[1] / n)
        lungo = math.hypot(b[0] - a[0], b[1] - a[1])
        dist_porta = None
        if porta is not None:
            ux, uy = (b[0] - a[0]) / lungo, (b[1] - a[1]) / lungo
            t = (porta[0] - a[0]) * ux + (porta[1] - a[1]) * uy
            d = abs((porta[0] - a[0]) * -uy + (porta[1] - a[1]) * ux)
            if 0.0 <= t <= lungo and d < 120.0:
                dist_porta = t
        lati.append((a, b, lungo, fuori, dist_porta))
    if porta is not None and not any(l[4] is not None for l in lati):
        avviso("%s: la porta non sta su nessun lato (la metto sul lato piu' vicino)" % nome)
        k = min(range(4), key=lambda i: abs((porta[0] - lati[i][0][0]) * -((lati[i][1][1] - lati[i][0][1]) / lati[i][2])
                                            + (porta[1] - lati[i][0][1]) * ((lati[i][1][0] - lati[i][0][0]) / lati[i][2])))
        a, b, lungo, fuori, _ = lati[k]
        ux, uy = (b[0] - a[0]) / lungo, (b[1] - a[1]) / lungo
        lati[k] = (a, b, lungo, fuori, (porta[0] - a[0]) * ux + (porta[1] - a[1]) * uy)
    for a, b, lungo, fuori, dist_porta in lati:
        messi += lato(a, b, dist_porta, z0, nome, tag_fase, fuori)
    altezza = MISURE["piani"] * MISURE["altezza_modulo"]
    # gli angoli
    if MISURE["angoli"]:
        m = modello(ANGOLO)
        if m:
            mn, mx = scatola(m)
            for piano in range(MISURE["piani"]):
                for (px, py) in punti:
                    if metti(m, px - (mn.x + mx.x) / 2.0, py - (mn.y + mx.y) / 2.0,
                             z0 + piano * MISURE["altezza_modulo"] - mn.z, 0.0, casa=nome, tag_fase=tag_fase,
                             nome="angolo p%d" % piano):
                        messi += 1
    # il lato lungo e quello corto
    lunghi = sorted(lati, key=lambda l: -l[2])
    lungo_casa, corto_casa = lunghi[0][2], lunghi[2][2]
    asse = math.degrees(math.atan2(lunghi[0][1][1] - lunghi[0][0][1], lunghi[0][1][0] - lunghi[0][0][0]))
    # i timpani sui due lati corti
    if MISURE["timpani"]:
        m = modello(TIMPANO[0])
        if m:
            s = corto_casa / TIMPANO[1]
            altezza_tetto = MISURE["pendenza_tetto"] * corto_casa
            mn, mx = scatola(m)
            sz = altezza_tetto / max(1.0, mx.z - mn.z)
            for a, b, lungo, fuori, _ in lunghi[2:]:
                g = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))
                cxl, cyl = (a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0
                gr = math.radians(g)
                ox, oy = (mn.x + mx.x) / 2.0 * s, (mn.y + mx.y) / 2.0
                if metti(m, cxl - (ox * math.cos(gr) - oy * math.sin(gr)), cyl - (ox * math.sin(gr) + oy * math.cos(gr)),
                         z0 + altezza - mn.z * sz, g, sx=s, sz=sz, casa=nome, tag_fase=tag_fase, nome="timpano"):
                    messi += 1
    # il tetto: quello di Epic con la forma piu' simile, girato con il colmo lungo il lato lungo
    # (15/11 sera, dopo la prova approvata da Fra) non sempre lo stesso: tra i tetti con una forma abbastanza simile
    # (al massimo 0,25 piu' lontana del migliore) se ne sceglie uno fisso per ogni casa (dal suo nome), cosi' il
    # villaggio ha tetti di colori diversi (coppi rossi, legno, ardesia) e rilanciando ogni casa resta uguale
    candidati = []
    for t in TETTI:
        m = modello(t)
        if m is None:
            continue
        mn, mx = scatola(m)
        tx, ty = mx.x - mn.x, mx.y - mn.y
        lungo_t, corto_t = max(tx, ty), min(tx, ty)
        forma = abs(math.log((lungo_t / corto_t) / (lungo_casa / corto_casa)))
        candidati.append((forma, m, t, tx >= ty))
    migliore = None
    if candidati:
        meglio = min(c[0] for c in candidati)
        buoni = sorted([c for c in candidati if c[0] <= meglio + 0.25], key=lambda c: c[2])
        migliore = buoni[sum(ord(ch) for ch in nome) % len(buoni)]
    if migliore:
        _, m, t, colmo_su_x = migliore
        mn, mx = scatola(m)
        sporge = 2.0 * MISURE["sporgenza_tetto"]
        tx, ty, tz = mx.x - mn.x, mx.y - mn.y, mx.z - mn.z
        lungo_t, corto_t = (tx, ty) if colmo_su_x else (ty, tx)
        s_lungo = (lungo_casa + sporge) / lungo_t
        s_corto = (corto_casa + sporge) / corto_t
        sz = MISURE["pendenza_tetto"] * corto_casa / max(1.0, tz)
        gradi = asse if colmo_su_x else asse - 90.0
        sx, sy = (s_lungo, s_corto) if colmo_su_x else (s_corto, s_lungo)
        g = math.radians(gradi)
        ox, oy = (mn.x + mx.x) / 2.0 * sx, (mn.y + mx.y) / 2.0 * sy
        if metti(m, cx - (ox * math.cos(g) - oy * math.sin(g)), cy - (ox * math.sin(g) + oy * math.cos(g)),
                 z0 + altezza - mn.z * sz, gradi, sx=sx, sy=sy, sz=sz, casa=nome, tag_fase=tag_fase,
                 nome="tetto " + t.split("/")[-1]):
            messi += 1
        scrivi("%s: tetto %s (scala %.2f x %.2f x %.2f)" % (nome, t.split("/")[-1], sx, sy, sz))
    # la casa vecchia si spegne (non si cancella)
    spegni(vecchia, True)
    scrivi("%s: %d pezzi, %.0f x %.0f cm, %d piani, porta %s" % (nome, messi, lungo_casa, corto_casa, MISURE["piani"],
                                                               "si'" if porta else "no"))
    return messi


def spegni(a, spenta):
    tags = [t for t in a.get_editor_property("tags") if str(t) != SPENTA]
    if spenta:
        tags.append(unreal.Name(SPENTA))
    a.set_editor_property("tags", tags)
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        c.set_visibility(not spenta, True)
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION if spenta else unreal.CollisionEnabled.QUERY_AND_PHYSICS)


def togli(solo=None):
    tolti = 0
    for a in attori.get_all_level_actors():
        tags = [str(t) for t in a.get_editor_property("tags")]
        if ETICHETTA in tags and (solo is None or ("ValdorsoCasaVera " + solo) in tags):
            attori.destroy_actor(a)
            tolti += 1
    for a in attori.get_all_level_actors():
        if SPENTA in [str(t) for t in a.get_editor_property("tags")] and (solo is None or a.get_actor_label() == solo):
            spegni(a, False)
    return tolti


def principale():
    tutti = attori.get_all_level_actors()
    paesaggi = [a for a in tutti if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Case vere: apri L_Valle")
        return
    if MODO == "indietro":
        scrivi("tolti %d pezzi, case vecchie riaccese" % togli())
        return
    angolo = paesaggi[0].get_actor_location()
    with open(PIANTA, "r", encoding="utf-8") as f:
        pianta = json.load(f)
    per_nome = {}
    for a in tutti:
        per_nome.setdefault(a.get_actor_label(), a)
    V = {"per_nome": per_nome}
    case = [(c["nome"], [(p[0] + angolo.x, p[1] + angolo.y) for p in c["punti"]]) for c in pianta
            if c.get("tipo") == "edificio" and c.get("sezione") == "Villaggio"]
    scelte = [c for c in case if (c[0] in PROVA if MODO == "prova" else c[0] not in MAI)]
    scrivi("%s: %d case da fare" % (MODO, len(scelte)))
    totale = 0
    for nome, punti in scelte:
        togli(nome)
        try:
            totale += costruisci(nome, punti, V)
        except Exception as e:  # noqa: BLE001
            avviso("%s: si e' fermato (%s)" % (nome, e))
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): Ctrl+Maiusc+S" % e)
    scrivi("FATTO: %d pezzi. Guarda la casa e manda a Claude uno screenshot da fuori (davanti e di lato)." % totale)


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "case_vere.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
