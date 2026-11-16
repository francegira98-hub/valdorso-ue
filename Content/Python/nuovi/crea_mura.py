# Valdorso - Le mura da castello e il tempio che cresce (16/11/2026, scritto da Claude; v0.2.6 "Le mura" e v0.2.7
# "Il tempio", approvate da Fra il 15/11). Mette nel livello i pezzi fatti dalla chat 3D (V:/Texture/valdorso/mura:
# 36 .glb, Valdorso_mura_tempio.json con le schede, posizioni_mura_valdorso.json con le 197 posizioni delle mura).
#
# Voci nel menu Valdorso -> Villaggio:
#   "Mura: 1. prova degli assi"           importa solo SM_Prova_Assi e lo mette davanti alla telecamera: la freccia deve
#                                          guardare +Y, il cubetto di ferro +X. Lo script lo controlla da solo e lo scrive.
#   "Mura: 2. costruisci le mura"          importa tutti i pezzi, spegne le mura vecchie della fase 3 (il muro di cinta,
#                                          le due torri di ogni porta, gli archi, le torri d'angolo 8 e 9: non si
#                                          cancellano, si spengono) e mette le mura nuove: tratti, snodi, 17 torri, 4 torri
#                                          della porta con le ante (AValdorsoPorta), la grata (AValdorsoGrata), la leva con
#                                          il lucchetto (AValdorsoLeva, Chiave_Porta) e l'argano; in ogni torre le
#                                          porticine (quella a terra chiusa con la Chiave_Torri, quelle sul cammino aperte).
#                                          E la stalla del mulo al posto di guardia del passo.
#   "Mura: 3. il tempio che cresce"        il Tempio del Cuore: fase 1 la cappella di oggi, fase 2 la chiesa con il
#                                          campanile, fase 3 la pieve (nello stesso punto, la porta verso la piazza).
#   "Mura: torna come prima"               toglie tutto il nuovo e riaccende il vecchio (mura, torri, cappella).
# Tutti i pezzi nuovi delle mura sono della fase 3 (etichetta "ValdorsoFase 3-0"); il tempio ha le sue fasi.
# Se Unreal specchia la Y (come con la grotta del nord), lo si vede dalla prova degli assi e si corregge da solo
# (scala Y = -1 sui modelli). Le Z: dal terreno, con il pezzo affondato di 40 cm (la scarpa copre i salti).
# Resoconto in Saved/Claude/mura_rapporto_<voce>.txt (assi, mura, tempio, indietro). Dopo: "PNG: panche, letti e porte" (i PNG imparano le porte nuove).

import json
import math
import os

import unreal

MODO = str(globals().get("MURA_MODO", "assi"))

CARTELLA = "V:/Texture/valdorso/mura"
PEZZI = "/Game/Valle/Mura/Pezzi"
MATERIALI = "/Game/Valle/Villaggio/Materiali"
ETICHETTA = "ValdorsoMuraNuove"
SPENTA = "ValdorsoMuraVecchiaSpenta"
TEMPIO_CAMBIATO = "ValdorsoTempioCambiato"
CONTORNO = "Valle/Mura"
AFFONDA = 40.0
FASE_MURA = "ValdorsoFase 3-0"
# le cose vecchie della fase 3 che le mura nuove sostituiscono (per nome nel livello)
VECCHIE = ("Muro di cinta", "Torre_della_porta_", "Porta di pietra", "Torre_d_angolo_8", "Torre_d_angolo_9")
# i materiali del villaggio per gli slot dei pezzi
SLOT = {"pietra": "Pietra", "legno": "Trave", "ferro": "Ferro", "coppi": "Coppi"}

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(t):
    righe.append(t)
    unreal.log("[Valdorso] Mura: " + t)


def avviso(t):
    righe.append("ATTENZIONE: " + t)
    unreal.log_warning("[Valdorso] Mura: " + t)


# ------------------------------------------------------------------------------------------------ importare

def compito(f, dest):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", f)
    c.set_editor_property("destination_path", dest)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    return c


def cerca_e_metti_a_posto(nome):
    """L'importazione dei .glb (5.8) mette il modello in <cartella>/<nome>/StaticMeshes/<nome>: lo si sposta in
    <cartella>/<nome> (come per la grotta del nord)."""
    giusto = "%s/%s" % (PEZZI, nome)
    if libreria.does_asset_exist(giusto):
        a = unreal.load_asset("%s.%s" % (giusto, nome))
        if isinstance(a, unreal.StaticMesh):
            return a
    for percorso in libreria.list_assets(PEZZI, recursive=True, include_folder=False):
        if percorso.rsplit("/", 1)[-1].split(".")[0] != nome:
            continue
        x = unreal.load_asset(percorso)
        if not isinstance(x, unreal.StaticMesh):
            continue
        vecchio = percorso.split(".")[0]
        if vecchio != giusto and not libreria.does_asset_exist(giusto) and libreria.rename_asset(vecchio, giusto):
            x = unreal.load_asset("%s.%s" % (giusto, nome))
        return x
    return None


def veste(mesh, mat):
    """I materiali del villaggio negli slot (pietra, legno, ferro, coppi), la collisione vera (le scale a chiocciola, i
    vani delle porte e il cammino di ronda devono restare percorribili), Nanite acceso."""
    for i, sm in enumerate(mesh.get_editor_property("static_materials")):
        nome = str(sm.get_editor_property("material_slot_name")).lower()
        mi = sm.get_editor_property("material_interface")
        if mi is not None:
            nome += " " + mi.get_name().lower()
        chiave = next((v for k, v in SLOT.items() if k in nome), None)
        if chiave and chiave in mat:
            mesh.set_material(i, mat[chiave])
        else:
            avviso("%s: lo slot %s resta con il suo colore" % (mesh.get_name(), nome))
    try:
        bs = mesh.get_editor_property("body_setup")
        if bs is not None:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            mesh.set_editor_property("body_setup", bs)
    except Exception as e:  # noqa: BLE001
        avviso("%s: collisione non impostata (%s)" % (mesh.get_name(), e))
    try:
        ns = mesh.get_editor_property("nanite_settings")
        if not ns.get_editor_property("enabled"):
            ns.set_editor_property("enabled", True)
            mesh.set_editor_property("nanite_settings", ns)
    except Exception as e:  # noqa: BLE001
        avviso("%s: Nanite non controllato (%s)" % (mesh.get_name(), e))
    libreria.save_loaded_asset(mesh)


def importa(nomi, mat):
    compiti = []
    for n in nomi:
        f = "%s/glb/%s.glb" % (CARTELLA, n)
        if os.path.exists(f):
            compiti.append(compito(f, PEZZI))
        else:
            avviso("manca il file %s" % f)
    scrivi("importo %d pezzi..." % len(compiti))
    strumenti.import_asset_tasks(compiti)
    pezzi = {}
    for n in nomi:
        a = cerca_e_metti_a_posto(n)
        if a is None:
            avviso("pezzo non importato: %s" % n)
            continue
        if n != "SM_Prova_Assi" and mat:
            veste(a, mat)
        pezzi[n] = a
    scrivi("pezzi pronti: %d su %d" % (len(pezzi), len(nomi)))
    return pezzi


def materiali():
    mat = {}
    for n in set(SLOT.values()):
        a = unreal.load_asset("%s/MI_Villaggio_%s" % (MATERIALI, n))
        if a is None:
            avviso("manca MI_Villaggio_%s (prima il villaggio)" % n)
        else:
            mat[n] = a
    return mat


def specchio_y(pezzi):
    """SM_Prova_Assi: x da -30 a 100, y da -10 a 120, z da 0 a 100. Se la y arriva al contrario, Unreal ha specchiato:
    -1 (si rimette con la scala Y = -1 sui modelli)."""
    m = pezzi.get("SM_Prova_Assi")
    if m is None:
        avviso("manca SM_Prova_Assi: non so se Unreal ha specchiato la Y (penso di no)")
        return 1.0
    b = m.get_bounding_box()
    scrivi("prova assi: x da %.0f a %.0f, y da %.0f a %.0f, z da %.0f a %.0f"
           % (b.min.x, b.max.x, b.min.y, b.max.y, b.min.z, b.max.z))
    if b.max.y > 80 and b.max.x > 80 and b.max.z > 80:
        scrivi("prova assi: GIUSTA (freccia verso +Y, ferro verso +X, paletto in alto)")
        return 1.0
    if b.min.y < -80 and b.max.x > 80 and b.max.z > 80:
        scrivi("prova assi: Unreal ha SPECCHIATO la Y (come con la grotta): la correggo con la scala Y = -1")
        return -1.0
    avviso("prova assi: orientamento strano (girato?): guardare a occhio e dirlo alla chat 3D")
    return 1.0


# ------------------------------------------------------------------------------------------------ nel livello

def angolo_paesaggio():
    for a in attori.get_all_level_actors():
        if isinstance(a, unreal.Landscape):
            return a.get_actor_location()
    return None


# (16/11 sera, secondo giro) chi copriva il terreno nei punti dove non lo trovavo: nome -> volte
COPERTO = {}


def terreno(mondo, x, y, vicino):
    """La quota del paesaggio in (x, y), da +2 km a -2 km di quota; se il raggio tocca il paesaggio piu' volte
    (montagne, grotte) prende il punto piu' vicino alla quota 'vicino'.
    (16/11 sera, secondo giro) Il raggio si ferma alla prima cosa "piena" che tocca (un pezzo di mura appena messo,
    l'acqua del fiume, un albero, una roccia...): allora si rifa' il raggio ignorando quella cosa, fino a 12 volte,
    e si segna chi copriva (COPERTO) per il resoconto."""
    ignora = []
    for _ in range(12):
        trovate = []
        fermato = None
        try:
            r = unreal.SystemLibrary.line_trace_multi(mondo, unreal.Vector(x, y, 200000.0), unreal.Vector(x, y, -200000.0),
                                                      unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora,
                                                      unreal.DrawDebugTrace.NONE, True)
            colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
        except Exception:  # noqa: BLE001
            colpi = []
        for hit in colpi or []:
            t = hit.to_tuple()
            if t[9] is None:
                continue
            if "Landscape" in t[9].get_class().get_name():
                trovate.append(t[4].z)
            elif t[0]:
                fermato = t[9]
        if trovate:
            return min(trovate, key=lambda z: abs(z - vicino))
        if fermato is None or fermato in ignora:
            return None
        ignora.append(fermato)
        if not fermato.actor_has_tag(ETICHETTA):
            n = fermato.get_actor_label()
            COPERTO[n] = COPERTO.get(n, 0) + 1
    return None


def camera_editor():
    """(16/11 sera) In Unreal 5.8 get_level_viewport_camera_info da' due valori (posizione, rotazione), non tre."""
    r = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_level_viewport_camera_info()
    if isinstance(r, tuple) and len(r) == 3:
        return r
    if isinstance(r, tuple) and len(r) == 2:
        return (r[0] is not None, r[0], r[1])
    return (False, None, None)


def al_mondo(o, yaw, locale):
    """Un punto nel sistema di un pezzo (x, y, z) nel mondo (o = origine del pezzo, yaw in gradi)."""
    a = math.radians(yaw)
    x, y, z = locale
    return unreal.Vector(o.x + x * math.cos(a) - y * math.sin(a), o.y + x * math.sin(a) + y * math.cos(a), o.z + z)


def finisci(a, nome, fase=FASE_MURA, cartella=CONTORNO):
    a.set_actor_label(nome)
    a.set_folder_path(cartella)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)] + ([unreal.Name(fase)] if fase else []))
    return a


def metti_mesh(mesh, dove, yaw, nome, sy, fase=FASE_MURA, cartella=CONTORNO):
    a = attori.spawn_actor_from_object(mesh, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    if a is None:
        avviso("non riesco a mettere %s" % nome)
        return None
    if sy < 0:
        a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
    return finisci(a, nome, fase, cartella)


def mesh_su(componente, mesh, sy, yaw_relativo=0.0):
    """Mette un modello vero su un componente di un attore C++ (l'anta, la grata, l'asta della leva)."""
    componente.set_editor_property("static_mesh", mesh)
    componente.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
    componente.set_relative_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw_relativo), False, False)
    componente.set_relative_scale3d(unreal.Vector(1.0, sy, 1.0))
    # (16/11 sera) il C++ da' all'anta, all'asta e alla grata un materiale suo (bianco): si rimettono quelli del modello
    # (legno, ferro), slot per slot, altrimenti porte e leva restano bianche
    try:
        for i in range(componente.get_num_materials()):
            componente.set_material(i, mesh.get_material(i))
    except Exception as e:  # noqa: BLE001
        avviso("%s: materiali del modello non rimessi (%s)" % (mesh.get_name(), e))


def porta_vera(classe, mesh, dove, yaw, verso, chiave, nome, sy):
    """Un'anta o una porticina: AValdorsoPorta con il modello vero sulla cerniera (l'origine del modello e' il cardine)."""
    p = attori.spawn_actor_from_class(classe, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    if p is None:
        return None
    cerniera = p.get_editor_property("cerniera")
    anta = p.get_editor_property("anta")
    cerniera.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
    mesh_su(anta, mesh, sy)
    p.set_editor_property("angolo_apertura", 90.0)
    p.set_editor_property("verso_fisso", int(verso))
    if chiave:
        p.set_editor_property("chiusa_a_chiave", True)
        p.set_editor_property("chiave", unreal.Name(chiave))
    return finisci(p, nome)


def tutti_per_nome():
    d = {}
    for a in attori.get_all_level_actors():
        if a:
            d.setdefault(a.get_actor_label(), a)
    return d


MAI = "ValdorsoFase 99-99"            # (16/11 sera) una fase che non arriva mai: il C++ la nasconde e le toglie la collisione
PRIMA = "ValdorsoFasePrima "           # la fase che aveva prima di essere spenta (per "torna come prima")


def spegni(a, si):
    """Spegne (o riaccende) una cosa vecchia senza cancellarla.
    (16/11 sera) Prima si spegnevano solo i suoi pezzi (invisibili, senza collisione), ma in Play tornavano: le torri
    vecchie delle porte si vedevano e i loro muri invisibili chiudevano le porticine. Ora le si da' anche la fase 99,
    che non arriva mai: cosi' la nasconde il sistema delle fasi, nel gioco e in Play, con la collisione spenta."""
    try:
        a.modify()
    except Exception:  # noqa: BLE001
        pass
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        try:
            c.modify()
        except Exception:  # noqa: BLE001
            pass
        c.set_visibility(not si, True)
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION if si else unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    vecchie = [str(t) for t in a.get_editor_property("tags")]
    tags = [t for t in vecchie if t != SPENTA and t != MAI]
    if si:
        if not any(t.startswith(PRIMA) for t in tags):
            fase = [t for t in tags if t.startswith("ValdorsoFase ")]
            tags.append(PRIMA + (fase[0][len("ValdorsoFase "):] if fase else "nessuna"))
        tags = [t for t in tags if not t.startswith("ValdorsoFase ")] + [SPENTA, MAI]
    else:
        prima = [t for t in tags if t.startswith(PRIMA)]
        tags = [t for t in tags if not t.startswith(PRIMA)]
        if prima and prima[0][len(PRIMA):] != "nessuna":
            tags.append("ValdorsoFase " + prima[0][len(PRIMA):])
    a.set_editor_property("tags", [unreal.Name(t) for t in tags])


def togli_nuovi(solo=None):
    via = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(ETICHETTA) and (solo is None or solo(a)):
            attori.destroy_actor(a)
            via += 1
    return via


def apri_dati():
    with open(CARTELLA + "/Valdorso_mura_tempio.json", "r", encoding="utf-8") as f:
        schede = json.load(f)
    with open(CARTELLA + "/posizioni_mura_valdorso.json", "r", encoding="utf-8") as f:
        posizioni = json.load(f)
    return schede, posizioni


def numero_specchio():
    """Il risultato della prova degli assi, scritto da "Mura: 1." (se non c'e', la si rifa' qui)."""
    f = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude", "mura_assi.txt")
    if os.path.exists(f):
        with open(f, "r", encoding="utf-8") as g:
            return -1.0 if g.read().strip() == "-1" else 1.0
    return None


def salva_specchio(sy):
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "mura_assi.txt"), "w", encoding="utf-8") as g:
        g.write("-1" if sy < 0 else "1")


# ------------------------------------------------------------------------------------------------ i modi

def fai_assi():
    pezzi = importa(["SM_Prova_Assi"], None)
    sy = specchio_y(pezzi)
    salva_specchio(sy)
    m = pezzi.get("SM_Prova_Assi")
    if m is None:
        return
    togli_nuovi(lambda a: a.get_actor_label() == "Prova degli assi (mura)")
    ok, dove, gira = camera_editor()
    if ok:
        yaw = gira.yaw
        p = unreal.Vector(dove.x + math.cos(math.radians(yaw)) * 400.0, dove.y + math.sin(math.radians(yaw)) * 400.0,
                          dove.z - 150.0)
        a = metti_mesh(m, p, 0.0, "Prova degli assi (mura)", 1.0, fase=None)
        if a:
            scrivi("messa davanti alla telecamera con yaw 0 e SENZA correzione: guardala con la freccia degli assi in "
                   "basso a sinistra (la freccia del pezzo deve andare come la Y verde, il cubetto come la X rossa)")
    scrivi("FATTO: ora 'Mura: 2. costruisci le mura'")


def fai_mura():
    schede, posizioni = apri_dati()
    angolo = angolo_paesaggio()
    if angolo is None:
        avviso("nel livello non c'e' il paesaggio (apri L_Valle)")
        return
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    mat = materiali()
    nomi = sorted(set([p["pezzo"] for p in posizioni["pezzi"]] + ["SM_Porta_Anta_Sinistra", "SM_Porta_Anta_Destra",
                                                                  "SM_Grata", "SM_Leva", "SM_Argano",
                                                                  "SM_Torre_Porticina", "SM_Stalla_Mulo",
                                                                  "SM_Prova_Assi"]))
    pezzi = importa(nomi, mat)
    sy = numero_specchio()
    if sy is None:
        sy = specchio_y(pezzi)
        salva_specchio(sy)
    scrivi("correzione della Y: %s" % ("scala -1 (Unreal ha specchiato)" if sy < 0 else "nessuna"))
    via = togli_nuovi(lambda a: not a.actor_has_tag(TEMPIO_CAMBIATO) and "Tempio" not in a.get_actor_label())
    if via:
        scrivi("tolti %d pezzi delle mura di prima" % via)

    # le mura vecchie della fase 3 si spengono
    spente = 0
    for a in attori.get_all_level_actors():
        if a and a.get_actor_label().startswith(VECCHIE) and not a.actor_has_tag(ETICHETTA):
            spegni(a, True)
            spente += 1
    scrivi("spente %d cose vecchie (muro di cinta, torri delle porte, archi, torri d'angolo 8 e 9)" % spente)

    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    classe_grata = getattr(unreal, "ValdorsoGrata", None)
    classe_leva = getattr(unreal, "ValdorsoLeva", None)
    if not (classe_porta and classe_grata and classe_leva):
        avviso("mancano le classi del C++ della v0.2.6 (ValdorsoGrata, ValdorsoLeva): compila; metto solo i muri")
    sch = schede["pezzi"]
    conti = {}
    zeta = None
    for k, p in enumerate(posizioni["pezzi"]):
        nome = p["pezzo"]
        m = pezzi.get(nome)
        if m is None:
            continue
        x, y = p["posizione_cm"][0] + angolo.x, p["posizione_cm"][1] + angolo.y
        yaw = float(p["yaw"])
        # la quota: il punto piu' basso del terreno sotto il pezzo (all'inizio, al centro, alla fine), meno 40 cm
        ing = sch.get(nome, {}).get("ingombro_cm", {"x": [0, 0]})
        quote = []
        vicino = zeta if zeta is not None else angolo.z + 5000.0
        for lx in (ing["x"][0], (ing["x"][0] + ing["x"][1]) / 2.0, ing["x"][1]):
            w = al_mondo(unreal.Vector(x, y, 0.0), yaw, (lx, 0.0, 0.0))
            q = terreno(mondo, w.x, w.y, vicino)
            if q is not None:
                quote.append(q)
        if not quote:
            avviso("%s %d: non trovo il terreno, lo salto" % (nome, k))
            continue
        # (le torri meno: le porte a terra e il passaggio della porta devono restare alti abbastanza)
        z = min(quote) - (10.0 if nome in ("SM_Torre", "SM_Torre_Porta") else AFFONDA)
        zeta = z
        o = unreal.Vector(x, y, z)
        etichetta = "Mura ~ %03d %s" % (k + 1, nome.replace("SM_", ""))
        a = metti_mesh(m, o, yaw, etichetta, sy)
        if a is None:
            continue
        conti[nome] = conti.get(nome, 0) + 1
        if nome not in ("SM_Torre", "SM_Torre_Porta") or not classe_porta:
            continue
        s = sch[nome]
        # le porticine: quella a terra chiusa con la chiave delle torri, quelle sul cammino aperte
        porticina = pezzi.get("SM_Torre_Porticina")
        for j, pc in enumerate(s.get("porticine", [])):
            if porticina is None:
                break
            apre = pc.get("apre", "")
            verso = 1 if "+90" in apre else -1
            a_terra = pc["cardine"][2] < 100
            porta_vera(classe_porta, porticina, al_mondo(o, yaw, pc["cardine"]), yaw + float(pc.get("yaw_chiusa", 0)),
                       verso, "Chiave_Torri" if a_terra else None, "%s porticina %d" % (etichetta, j + 1), sy)
            conti["porticine"] = conti.get("porticine", 0) + 1
        if nome != "SM_Torre_Porta":
            continue
        # la porta della citta': le due ante, la grata, la leva con il lucchetto, l'argano
        for lato, verso in (("SM_Porta_Anta_Sinistra", -1), ("SM_Porta_Anta_Destra", 1)):
            b = s["battenti"][lato]
            if pezzi.get(lato):
                porta_vera(classe_porta, pezzi[lato], al_mondo(o, yaw, b["cardine"]), yaw + float(b.get("yaw_chiusa", 0)),
                           verso, None, "%s %s" % (etichetta, "anta sinistra" if verso < 0 else "anta destra"), sy)
        grata = None
        if pezzi.get("SM_Grata"):
            g = s["grata"]
            # la grata del C++ sta nel piano Y-Z (si passa lungo la sua X): girata di 90 gradi rispetto alla torre
            grata = attori.spawn_actor_from_class(classe_grata, al_mondo(o, yaw, g["posizione_chiusa"]),
                                                  unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 90.0))
            if grata:
                grata.set_editor_property("modello", pezzi["SM_Grata"])
                grata.set_editor_property("larghezza", 440.0)
                grata.set_editor_property("altezza", 540.0)
                grata.set_editor_property("corsa", float(g.get("corsa_cm", 540)))
                mesh_su(grata.get_editor_property("ferro"), pezzi["SM_Grata"], sy, -90.0)
                finisci(grata, "%s grata" % etichetta)
        if pezzi.get("SM_Leva") and grata:
            lv = s["leva"]
            # la leva del C++ gira intorno alla sua Y: girata di 90 gradi, gira intorno alla X della torre
            leva = attori.spawn_actor_from_class(classe_leva, al_mondo(o, yaw, lv["perno"]),
                                                 unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 90.0))
            if leva:
                leva.set_editor_property("grata", grata)
                perno = leva.get_editor_property("perno")
                perno.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
                mesh_su(leva.get_editor_property("asta"), pezzi["SM_Leva"], sy, -90.0)
                base = leva.get_editor_property("base")
                base.set_relative_location(unreal.Vector(0.0, 0.0, -25.0), False, False)
                base.set_relative_scale3d(unreal.Vector(0.4, 0.3, 0.5))
                base.set_visibility(False, True)
                finisci(leva, "%s leva della grata" % etichetta)
        if pezzi.get("SM_Argano"):
            metti_mesh(pezzi["SM_Argano"], al_mondo(o, yaw, s["argano"]["asse"]), yaw, "%s argano" % etichetta, sy)
    scrivi("messi: " + ", ".join("%s %d" % (n.replace("SM_", ""), c) for n, c in sorted(conti.items())))
    if COPERTO:
        scrivi("sopra il terreno c'era (ignorato per trovare la quota): " +
               ", ".join("%s %d" % (n, c) for n, c in sorted(COPERTO.items(), key=lambda v: -v[1])[:15]))

    # la stalla del mulo al posto di guardia del passo (la proposta della chat 3D)
    st = schede.get("posto_di_guardia", {}).get("SM_Stalla_Mulo", {}).get("proposta_nel_mondo")
    if st and pezzi.get("SM_Stalla_Mulo"):
        x, y = st["posizione"][0] + angolo.x, st["posizione"][1] + angolo.y
        q = terreno(mondo, x, y, angolo.z + 10000.0)
        if q is not None:
            metti_mesh(pezzi["SM_Stalla_Mulo"], unreal.Vector(x, y, q - 10.0), float(st.get("yaw", 0)),
                       "Stalla del mulo", sy, fase=None, cartella="Valle/Posto di guardia")
            scrivi("la stalla del mulo accanto alla casermetta del passo")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("FATTO e salvato. Ora: 'PNG: panche, letti e porte'; per guardarle: 'Villaggio: mostra la fase 3'")


def fai_tempio():
    schede, _ = apri_dati()
    mat = materiali()
    pezzi = importa(["SM_Tempio_Fase2", "SM_Tempio_Fase3", "SM_Prova_Assi"], mat)
    sy = numero_specchio()
    if sy is None:
        sy = specchio_y(pezzi)
        salva_specchio(sy)
    per_nome = tutti_per_nome()
    cappella = per_nome.get("Tempio_del_Cuore")
    if cappella is None:
        avviso("nel livello non c'e' 'Tempio_del_Cuore'")
        return
    togli_nuovi(lambda a: a.get_actor_label().startswith("Tempio del Cuore, fase"))
    o = cappella.get_actor_location()
    # la cappella ha yaw 180 e la porta a (0, -1100) suoi; i pezzi nuovi hanno la porta a (0, +1100): yaw 0 la mette
    # nello stesso punto del mondo, verso la piazza (lo controllo con il vettore porta)
    yaw_cappella = cappella.get_actor_rotation().yaw
    yaw = yaw_cappella + 180.0
    porta_vecchia = al_mondo(o, yaw_cappella, (0.0, -1100.0, 0.0))
    porta_nuova = al_mondo(o, yaw, (0.0, 1100.0, 0.0))
    scrivi("porta della cappella (%.0f, %.0f), porta delle fasi nuove (%.0f, %.0f)"
           % (porta_vecchia.x, porta_vecchia.y, porta_nuova.x, porta_nuova.y))
    for fase, nome in ((2, "SM_Tempio_Fase2"), (3, "SM_Tempio_Fase3")):
        m = pezzi.get(nome)
        if m is None:
            continue
        metti_mesh(m, unreal.Vector(o.x, o.y, o.z), yaw, "Tempio del Cuore, fase %d" % fase, sy,
                   fase="ValdorsoFase %d-%d" % (fase, 0 if fase == 3 else 2), cartella="Valle/Tempio")
    # la cappella resta solo nella fase 1
    tags = [t for t in cappella.get_editor_property("tags") if not str(t).startswith("ValdorsoFase")
            and str(t) != TEMPIO_CAMBIATO]
    cappella.set_editor_property("tags", tags + [unreal.Name("ValdorsoFase 1-1"), unreal.Name(TEMPIO_CAMBIATO)])
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("FATTO: fase 1 la cappella, fase 2 la chiesa, fase 3 la pieve (Villaggio: mostra la fase 2 / 3 per guardarle)")


def fai_indietro():
    via = togli_nuovi()
    accese = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(SPENTA):
            spegni(a, False)
            accese += 1
        if a and a.actor_has_tag(TEMPIO_CAMBIATO):
            tags = [t for t in a.get_editor_property("tags") if not str(t).startswith("ValdorsoFase")
                    and str(t) != TEMPIO_CAMBIATO]
            a.set_editor_property("tags", tags + [unreal.Name("ValdorsoFase 1-0")])
            scrivi("la cappella torna in tutte le fasi")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("FATTO: tolti %d pezzi nuovi, riaccese %d cose vecchie" % (via, accese))


try:
    if MODO == "assi":
        fai_assi()
    elif MODO == "mura":
        fai_mura()
    elif MODO == "tempio":
        fai_tempio()
    elif MODO == "indietro":
        fai_indietro()
    else:
        avviso("modo sconosciuto: %s" % MODO)
except Exception as e:  # noqa: BLE001
    avviso("errore: %s" % e)
    raise
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    # (16/11 sera) un resoconto per ogni voce: prima il tempio copriva quello delle mura
    with open(os.path.join(cartella, "mura_rapporto_%s.txt" % MODO), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
