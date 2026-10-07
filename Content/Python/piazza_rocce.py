# Valdorso - Le rocce vere della valle, seconda parte: metterle nella gola della cascata (v0.1.3). Scritto da Claude il 07/10/2026.
# Si lancia in L_Valle, con tutta la valle caricata, DOPO importa_rocce.py: Strumenti -> Esegui script Python.
# Si può rilanciare: toglie le rocce messe prima (etichetta "ValdorsoRocce") e le rimette.
#
# Legge V:/Texture/valdorso/valle/Rocce_3000.json (fatto da Tools/Generatori/rocce.py dalle altezze della valle) e mette
# ogni roccia al suo posto: la misura è quella scritta nel file (la scala si calcola dalla grandezza vera del modello),
# e la roccia viene affondata nel terreno quanto dice il file, così non sembra appoggiata.
# Rocce usate (tutte fuori da GitHub, in Content/Fab):
#   Megascans (licenza completa di Fra): Huge Nordic Coastal Cliff, Massive Nordic Coastal Cliff, Nordic Forest Cliff Large;
#   Poly Haven (CC0): rock_face_01/02, coastal_cliff_04, boulder_01, rock_moss_set_01/02.
# Gli attori finiscono nella cartella "Valle/Rocce" del Contorno.

import json

import unreal

FILE_ROCCE = "V:/Texture/valdorso/valle/Rocce_3000.json"
ETICHETTA = "ValdorsoRocce"
QUOTA_MAX_CM = 80000.0

MODELLI = {
    "mega_huge": "/Game/Fab/Megascans/3D/Huge_Nordic_Coastal_Cliff_venrdcgga",
    "mega_massive": "/Game/Fab/Megascans/3D/Massive_Nordic_Coastal_Cliff_vdssailfa",
    "mega_foresta": "/Game/Fab/Megascans/3D/Nordic_Forest_Cliff_Large_xibsff1",
    "rock_face_01": "/Game/Fab/PolyHaven/rock_face_01",
    "rock_face_02": "/Game/Fab/PolyHaven/rock_face_02",
    "coastal_cliff_04": "/Game/Fab/PolyHaven/coastal_cliff_04",
    "boulder_01": "/Game/Fab/PolyHaven/boulder_01",
    "rock_moss_set_01": "/Game/Fab/PolyHaven/rock_moss_set_01",
    "rock_moss_set_02": "/Game/Fab/PolyHaven/rock_moss_set_02",
}
# se un modello manca, si usa un altro dello stesso tipo
RISERVE = {"mega_huge": "coastal_cliff_04", "mega_massive": "rock_face_01", "mega_foresta": "rock_face_02",
           "coastal_cliff_04": "rock_face_01", "rock_face_01": "rock_face_02", "rock_face_02": "rock_face_01",
           "boulder_01": "rock_moss_set_01", "rock_moss_set_01": "boulder_01", "rock_moss_set_02": "rock_moss_set_01"}

libreria = unreal.EditorAssetLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def log(testo):
    unreal.log("[Valdorso] Rocce nella gola: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] Rocce nella gola: " + testo)


def modelli_di(cartella):
    """I modelli (StaticMesh) di una cartella; dei LOD separati di Poly Haven tiene solo il LOD0."""
    trovati = []
    if not libreria.does_directory_exist(cartella):
        return trovati
    for percorso in libreria.list_assets(cartella, recursive=True, include_folder=False):
        if libreria.find_asset_data(percorso).asset_class_path.asset_name != "StaticMesh":
            continue
        nome = percorso.rsplit(".", 1)[-1]
        if nome.endswith(("_LOD1", "_LOD2", "_LOD3")):
            continue
        sm = unreal.load_asset(percorso)
        if sm is not None:
            trovati.append(sm)
    return trovati


# (07/10) Le scogliere scansionate sono gusci: la roccia c'è solo davanti, dietro sono vuote. La "faccia" piena va
# rivolta verso valle e il dietro nel monte. Qui si può correggere a mano, in gradi, se una scogliera resta girata male
# (per esempio 180 per girarla del tutto); None = calcolata dallo script dalla forma del modello.
CORREZIONE_FACCIA = {"mega_huge": None, "mega_massive": None, "mega_foresta": None, "coastal_cliff_04": None}
# (valori a mano: una direzione (x, y, z) del modello, per esempio (0, 0, 1) se la faccia piena guarda in su)


def quat_tra(a, b):
    """Il quaternione (x, y, z, w) che gira il vettore a sul vettore b."""
    import math as m
    cx, cy, cz = a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]
    d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
    if d < -0.9999:                                   # opposti: mezzo giro intorno a un asse qualsiasi perpendicolare
        asse = (0.0, 0.0, 1.0) if abs(a[2]) < 0.9 else (1.0, 0.0, 0.0)
        cx, cy, cz = a[1] * asse[2] - a[2] * asse[1], a[2] * asse[0] - a[0] * asse[2], a[0] * asse[1] - a[1] * asse[0]
        l = m.sqrt(cx * cx + cy * cy + cz * cz)
        return (cx / l, cy / l, cz / l, 0.0)
    w = 1 + d
    l = m.sqrt(cx * cx + cy * cy + cz * cz + w * w)
    return (cx / l, cy / l, cz / l, w / l)


def quat_asse(asse, gradi):
    import math as m
    s = m.sin(m.radians(gradi) / 2)
    return (asse[0] * s, asse[1] * s, asse[2] * s, m.cos(m.radians(gradi) / 2))


def quat_per(q, r):
    x1, y1, z1, w1 = q
    x2, y2, z2, w2 = r
    return (w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2, w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
            w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2, w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2)


def faccia_di(sm):
    """La direzione (gradi, nel piano del modello) in cui guarda la faccia piena di un guscio: la somma delle normali
    dei triangoli (pesate per l'area). In una roccia chiusa si annulla; in un guscio aperto punta verso la faccia."""
    import math as m
    try:
        md = sm.get_static_mesh_description(0)
        n_tri = md.get_triangle_count()
        passo = max(1, n_tri // 20000)
        sx = sy = sz = 0.0
        for i in range(0, n_tri, passo):
            tid = unreal.TriangleID(i) if hasattr(unreal, "TriangleID") else i
            vs = md.get_triangle_vertices(tid)
            p = [md.get_vertex_position(v) for v in vs]
            ax, ay, az = p[1].x - p[0].x, p[1].y - p[0].y, p[1].z - p[0].z
            bx, by, bz = p[2].x - p[0].x, p[2].y - p[0].y, p[2].z - p[0].z
            sx += ay * bz - az * by
            sy += az * bx - ax * bz
            sz += ax * by - ay * bx
        l = m.sqrt(sx * sx + sy * sy + sz * sz)
        if l < 1e-6:
            return None
        # (07/10, v5) in Unreal i triangoli girano al contrario di come avevo contato: la somma esce dal lato vuoto.
        # Visto nel gioco da Fra: i gusci mostravano tutti il vuoto verso valle. Quindi si prende l'opposto.
        return (-sx / l, -sy / l, -sz / l)
    except Exception as e:
        avviso("{}: faccia non calcolata ({}), uso la parte più piatta della scatola".format(sm.get_name(), e))
        return None


def faccia_dalla_scatola(sm):
    """Di riserva: la faccia guarda lungo il lato più sottile della scatola (verso +)."""
    d = sm.get_bounding_box().max - sm.get_bounding_box().min
    lati = {(1.0, 0.0, 0.0): d.x, (0.0, 1.0, 0.0): d.y, (0.0, 0.0, 1.0): d.z}
    return min(lati, key=lati.get)


def esegui():
    with open(FILE_ROCCE, "r", encoding="utf-8") as f:
        rocce = json.load(f)["rocce"]
    paesaggi = [a for a in attori.get_all_level_actors() if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Rocce nella gola: nel livello aperto non c'è un Landscape. Apri L_Valle.")
        return
    angolo = paesaggi[0].get_actor_location()
    scala = paesaggi[0].get_actor_scale3d()

    def nel_mondo(p):
        valore = p[2] / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + p[0], angolo.y + p[1], z)

    modelli = {k: modelli_di(v) for k, v in MODELLI.items()}
    for k, lista in modelli.items():
        if lista:
            log("{}: {} modelli ({})".format(k, len(lista), ", ".join(m.get_name() for m in lista[:4])))
        else:
            avviso("{}: nessun modello in {}, uso la riserva".format(k, MODELLI[k]))

    # Nanite acceso anche sulle Megascans (di solito lo è già).
    for k in ("mega_huge", "mega_massive", "mega_foresta"):
        for sm in modelli[k]:
            try:
                ns = sm.get_editor_property("nanite_settings")
                if not ns.get_editor_property("enabled"):
                    ns.set_editor_property("enabled", True)
                    sm.set_editor_property("nanite_settings", ns)
                    libreria.save_loaded_asset(sm)
            except Exception as e:
                avviso("{}: Nanite non controllato ({})".format(sm.get_name(), e))

    # Via le rocce di un lancio precedente.
    tolte = 0
    for a in attori.get_all_level_actors():
        if ETICHETTA in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
            tolte += 1
    if tolte:
        log("tolte {} rocce del lancio precedente".format(tolte))

    contati = {}
    facce = {}
    with unreal.ScopedSlowTask(len(rocce), "Valdorso: metto le rocce nella gola") as compito:
        compito.make_dialog(True)
        for r in rocce:
            if compito.should_cancel():
                break
            compito.enter_progress_frame(1)
            chiave = r["chiave"]
            lista = modelli.get(chiave) or modelli.get(RISERVE.get(chiave, ""), [])
            if not lista:
                continue
            sm = lista[r["variante"] % len(lista)]
            rotazione = None
            if r.get("faccia_a_valle") and r.get("normale"):
                if sm.get_name() not in facce:
                    manuale = CORREZIONE_FACCIA.get(chiave)
                    f = faccia_di(sm) if manuale is None else None
                    facce[sm.get_name()] = manuale if manuale is not None else (f if f is not None else faccia_dalla_scatola(sm))
                    log("{}: faccia piena verso ({:.2f}, {:.2f}, {:.2f})".format(sm.get_name(), *facce[sm.get_name()]))
                # (07/10, v4) la faccia piena del guscio va sulla normale del pendio (fuori dal monte), il dietro nel monte;
                # poi un piccolo giro intorno alla normale, perché non siano tutte uguali.
                q = quat_tra(facce[sm.get_name()], r["normale"])
                q = quat_per(quat_asse(r["normale"], r.get("giro", 0.0)), q)
                rotazione = unreal.Quat(q[0], q[1], q[2], q[3]).rotator()
            scatola = sm.get_bounding_box()
            dim = scatola.max - scatola.min
            grande = max(dim.x, dim.y, dim.z, 1.0)
            s = r["misura_m"] * 100.0 / grande
            rollio, beccheggio, imbardata = r["rot"]
            posto = nel_mondo(r["pos_cm"])
            a = attori.spawn_actor_from_object(sm, posto, rotazione or unreal.Rotator(rollio, beccheggio, imbardata))
            if a is None:
                continue
            a.set_actor_scale3d(unreal.Vector(s, s, s))
            # Affondare: la base della roccia (dopo scala e rotazione) va sotto il terreno di 'affonda' x la sua altezza.
            origine, estensione = a.get_actor_bounds(False)
            fondo = origine.z - estensione.z
            alto = 2 * estensione.z
            # e il centro della roccia sul punto scritto nel file (il perno dei modelli spesso non è al centro)
            a.add_actor_world_offset(unreal.Vector(posto.x - origine.x, posto.y - origine.y,
                                                   posto.z - fondo - r["affonda"] * alto), False, False)
            a.tags = [unreal.Name(ETICHETTA), unreal.Name("ValdorsoRocce_" + r["gruppo"])]
            a.set_folder_path("Valle/Rocce/" + r["gruppo"])
            a.set_actor_label("Roccia_{}_{:03d}".format(r["gruppo"], contati.get(r["gruppo"], 0) + 1))
            comp = a.get_component_by_class(unreal.StaticMeshComponent)
            if r["gruppo"] == "sassi":
                comp.set_editor_property("cast_shadow", True)
                comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)   # nel torrente non devono bloccare
            contati[r["gruppo"]] = contati.get(r["gruppo"], 0) + 1
    log("messe: " + ", ".join("{} {}".format(v, k) for k, v in contati.items()))
    log("fatto. Nel Contorno sono in Valle/Rocce. Per rifarle diverse: cambia SEME in Tools/Generatori/rocce.py.")


esegui()
