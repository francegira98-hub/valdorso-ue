# Valdorso - Mette nel livello la grotta del nord (v0.1.7): la frana che si apre, le gallerie dei nani sotto la montagna,
# gli scheletri, il forziere, le tracce della storia di Lucia. Scritto da Claude l'11/11/2026.
# Si lancia in L_Valle (dal menu Valdorso -> "Grotta del nord", oppure Strumenti -> Esegui script Python).
# Si puo' rilanciare: toglie tutto quello che ha messo prima (etichetta "ValdorsoGrottaNord") e lo rimette.
#
# Prima: il villaggio con la sezione "Grotta nord" (crea_villaggio.py: il portale, e dall'11/11 la frana in un pezzo suo con
# l'etichetta "ValdorsoFrana", che il C++ nasconde quando si apre). Serve l'attore "Grotta nord" nel livello.
#
# Legge V:/Texture/valdorso/grotta_nord (Tools/Generatori/grotta_nord.py; se c'e' grotta_nord.zip accanto, lo scompatta):
#   SM_GrottaNord_*.glb -> /Game/Valle/Grotta/Pezzi      (le gallerie, le sale, la torcia, il forziere, i sassi...)
#   SM_Scheletro_*.glb  -> /Game/Valle/Grotta/Scheletri  (i pezzi degli scheletri: li monta il C++)
#   S_*.wav             -> /Game/Valle/Pericoli/Suoni    (la frana, lo scheletro che si sveglia, le ossa che crollano)
#   GrottaNord.json     -> dove va ogni cosa, in cm dal punto d'arrivo (che sta 60 m sotto il luogo "Grotta nord")
# e mette: AValdorsoFrana davanti al portale, le gallerie (mesh), le torce con le luci, gli ossari (AValdorsoOssario),
# il forziere (AValdorsoForziere), le tracce (AValdorsoTraccia), le iscrizioni, i suoni, il passaggio per uscire.
# Cartella nel Contorno: Valle/Grotta nord. Resoconto in Saved/Claude/grotta_nord.txt. Alla fine salva.

import json
import os
import zipfile

import unreal

CARTELLA = "V:/Texture/valdorso/grotta_nord"
PEZZI = "/Game/Valle/Grotta/Pezzi"
SCHELETRI = "/Game/Valle/Grotta/Scheletri"
SUONI_NUOVI = "/Game/Valle/Pericoli/Suoni"
SUONI_VALLE = "/Game/Valle/Suoni"
MATERIALI = "/Game/Valle/Villaggio/Materiali"
ETICHETTA = "ValdorsoGrottaNord"
CONTORNO = "Valle/Grotta nord"

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Grotta nord: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Grotta nord: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "grotta_nord.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))


def compito(file, destinazione):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", file)
    c.set_editor_property("destination_path", destinazione)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    return c


def scompatta():
    zf = CARTELLA + ".zip"
    segno = CARTELLA + "/GrottaNord.json"
    if os.path.exists(zf) and (not os.path.exists(segno) or os.path.getmtime(zf) > os.path.getmtime(segno)):
        os.makedirs(CARTELLA, exist_ok=True)
        with zipfile.ZipFile(zf) as z:
            z.extractall(CARTELLA)
        os.utime(segno, None)
        scrivi("scompattato %s" % zf)


# ------------------------------------------------------------------------------------------------ materiali e pezzi

def materiale_ossa():
    """Le ossa: un colore (M_VillaggioColore, come il ferro e la stoffa), giallino, opaco."""
    percorso = MATERIALI + "/MI_Grotta_Ossa"
    genitore = unreal.load_asset(MATERIALI + "/M_VillaggioColore")
    if genitore is None:
        avviso("manca M_VillaggioColore (prima il villaggio): le ossa restano grigie")
        return None
    mi = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if mi is None:
        mi = strumenti.create_asset("MI_Grotta_Ossa", MATERIALI, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, genitore)
    mel.set_material_instance_vector_parameter_value(mi, "Tinta", unreal.LinearColor(0.62, 0.57, 0.45, 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "Ruvidezza", 0.85)
    mel.set_material_instance_scalar_parameter_value(mi, "Luce", 0.0)
    mel.update_material_instance(mi)
    libreria.save_loaded_asset(mi)
    return mi


def materiali():
    mat = {}
    for n in ("Pietra", "Intonaco", "Assi", "Trave", "Roccia", "Vetro", "Ferro", "Fiamma", "Stoffa", "Runa", "Muschio", "Acqua"):
        a = unreal.load_asset("%s/MI_Villaggio_%s" % (MATERIALI, n))
        if a is None:
            avviso("manca MI_Villaggio_%s (prima il villaggio)" % n)
        else:
            mat[n] = a
    ossa = materiale_ossa()
    if ossa is not None:
        mat["Ossa"] = ossa
    return mat


def veste(mesh, mat, collisione):
    """I materiali negli slot (MV_<Materiale>), la collisione (complessa = le pareti vere), Nanite acceso."""
    for i, sm in enumerate(mesh.get_editor_property("static_materials")):
        nome = str(sm.get_editor_property("material_slot_name"))
        mi = sm.get_editor_property("material_interface")
        if (not nome or nome == "None") and mi is not None:
            nome = mi.get_name()
        chiave = nome.split("MV_", 1)[-1].split("_")[0] if "MV_" in nome else None
        if chiave in mat:
            mesh.set_material(i, mat[chiave])
        else:
            avviso("%s: slot %s senza materiale" % (mesh.get_name(), nome))
    try:
        bs = mesh.get_editor_property("body_setup")
        if bs is not None:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
                                   if collisione else unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
            bs.set_editor_property("double_sided_geometry", bool(collisione))
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


def cerca_e_metti_a_posto(cartella, nome, classe):
    """L'importazione dei .glb (5.8) mette il modello in una sottocartella (<cartella>/<nome>/StaticMeshes/<nome>):
    lo si cerca e lo si sposta in <cartella>/<nome>, dove lo cerca il C++ (ValdorsoGrottaNord.cpp, ValdorsoScheletri.cpp)."""
    giusto = "%s/%s" % (cartella, nome)
    a = unreal.load_asset("%s.%s" % (giusto, nome)) if libreria.does_asset_exist(giusto) else None
    if isinstance(a, classe):
        return a
    for percorso in libreria.list_assets(cartella, recursive=True, include_folder=False):
        if percorso.rsplit("/", 1)[-1].split(".")[0] != nome:
            continue
        x = unreal.load_asset(percorso)
        if not isinstance(x, classe):
            continue
        vecchio = percorso.split(".")[0]
        if vecchio != giusto and not libreria.does_asset_exist(giusto):
            if libreria.rename_asset(vecchio, giusto):
                scrivi("spostato %s -> %s" % (vecchio, giusto))
                x = unreal.load_asset("%s.%s" % (giusto, nome))
        return x
    return None


def importa(nomi):
    compiti = []
    for n in nomi:
        f = "%s/%s.glb" % (CARTELLA, n)
        if os.path.exists(f):
            compiti.append(compito(f, SCHELETRI if n.startswith(("SM_Scheletro_", "SM_Guardiano_")) else PEZZI))
        else:
            avviso("manca il file %s" % f)
    scrivi("importo %d pezzi..." % len(compiti))
    strumenti.import_asset_tasks(compiti)
    pezzi = {}
    for n in nomi:
        cartella = SCHELETRI if n.startswith(("SM_Scheletro_", "SM_Guardiano_")) else PEZZI
        a = cerca_e_metti_a_posto(cartella, n, unreal.StaticMesh)
        if a is not None:
            pezzi[n] = a
        else:
            avviso("pezzo non importato: %s" % n)
    scrivi("pezzi pronti: %d su %d" % (len(pezzi), len(nomi)))
    return pezzi


def importa_suoni():
    compiti = []
    for n in ("S_Frana", "S_Scheletro_Sveglia", "S_Scheletro_Crollo", "S_Trappola_Scatto"):
        f = "%s/%s.wav" % (CARTELLA, n)
        if os.path.exists(f):
            compiti.append(compito(f, SUONI_NUOVI))
    strumenti.import_asset_tasks(compiti)
    fatti = 0
    for n in ("S_Frana", "S_Scheletro_Sveglia", "S_Scheletro_Crollo", "S_Trappola_Scatto"):
        s = cerca_e_metti_a_posto(SUONI_NUOVI, n, unreal.SoundWave)
        if s is None:
            avviso("suono non importato: %s" % n)
            continue
        s.set_editor_property("looping", False)
        libreria.save_loaded_asset(s)
        fatti += 1
    scrivi("suoni importati: %d" % fatti)


def controlla_assi(pezzi):
    """SM_GrottaNord_ProvaAssi: un braccio a +X (100), uno a +Y (300), uno a +Z (50). Unreal ha specchiato la Y?"""
    m = pezzi.get("SM_GrottaNord_ProvaAssi")
    if m is None:
        return 1.0
    s = m.get_bounding_box()
    if s.max.y > 250 and s.max.x > 80 and s.max.z > 40:
        return 1.0
    if s.min.y < -250 and s.max.x > 80 and s.max.z > 40:
        scrivi("prova assi: Unreal ha specchiato la Y, la rimetto con la scala Y = -1")
        return -1.0
    avviso("prova assi: orientamento strano, controllare a occhio")
    return 1.0


# ------------------------------------------------------------------------------------------------ nel livello

def togli_vecchi():
    via = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            via += 1
    if via:
        scrivi("tolti %d attori della volta prima" % via)


def aggiungi_acqua(c, qui):
    """(11/11) Uno specchio d'acqua in piu' nell'AValdorsoAcque del livello (sostituisce quello con lo stesso nome)."""
    classe = getattr(unreal, "ValdorsoAcque", None)
    acque = next((x for x in attori.get_all_level_actors() if classe and x and isinstance(x, classe)), None)
    if acque is None:
        avviso("%s: nel livello non c'e' ValdorsoAcque (crea_acque_nuoto.py): niente nuoto nelle miniere" % c["nome"])
        return False
    specchi = [x for x in acque.get_editor_property("specchi") if str(x.get_editor_property("nome")) != c["nome"]]
    nuovo = unreal.ValdorsoSpecchioAcqua()
    nuovo.set_editor_property("nome", c["nome"])
    riva = []
    for x, y in c["riva"]:
        w = qui([x, y, 0.0])
        riva.append(unreal.Vector2D(w.x, w.y))
    nuovo.set_editor_property("riva", riva)
    nuovo.set_editor_property("pelo", float(qui([0.0, 0.0, c["pelo"]]).z))
    specchi.append(nuovo)
    acque.set_editor_property("specchi", specchi)
    scrivi("acqua: '%s' aggiunta a ValdorsoAcque (pelo %.0f)" % (c["nome"], qui([0.0, 0.0, c["pelo"]]).z))
    return True


def cerca_grotta():
    for a in attori.get_all_level_actors():
        if a and a.get_actor_label() == "Grotta nord":
            return a
    return None


def terreno(mondo, x, y, z_alto, z_basso):
    """La quota del paesaggio (solo il Landscape) in (x, y), o None."""
    try:
        r = unreal.SystemLibrary.line_trace_multi(mondo, unreal.Vector(x, y, z_alto), unreal.Vector(x, y, z_basso),
                                                  unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
        colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
        for hit in colpi or []:
            t = hit.to_tuple()
            if t[9] is not None and "Landscape" in t[9].get_class().get_name():
                return t[4].z
    except Exception as e:  # noqa: BLE001
        avviso("traccia del terreno non riuscita (%s)" % e)
    return None


def finisci(a, nome, cartella=CONTORNO, sempre_caricato=False):
    a.set_actor_label(nome)
    a.set_folder_path(cartella)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    if sempre_caricato:
        try:
            a.set_editor_property("is_spatially_loaded", False)
        except Exception:  # noqa: BLE001
            pass
    return a


def principale():
    scompatta()
    segno = CARTELLA + "/GrottaNord.json"
    if not os.path.exists(segno):
        avviso("manca %s: prima Tools/Generatori/grotta_nord.py (o grotta_nord.zip in V:/Texture/valdorso)" % segno)
        return
    with open(segno, "r", encoding="utf-8") as f:
        dati = json.load(f)
    grotta = cerca_grotta()
    if grotta is None:
        avviso("nel livello non c'e' l'attore 'Grotta nord': prima il villaggio (sezione Grotta nord)")
        return
    g = grotta.get_actor_location()
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    scrivi("il luogo Grotta nord e' a (%.0f, %.0f, %.0f)" % (g.x, g.y, g.z))

    togli_vecchi()
    mat = materiali()
    importa_suoni()
    pezzi = importa(dati["mesh"])
    con_collisione = set(c["asset"] for c in dati["cose"] if c["tipo"] == "mesh" and c.get("collisione", True))
    # (11/11) le stanze segrete, e i pezzi che fanno da muro o da pavimento (la botola, la porta segreta, la grata)
    for st in dati.get("segreti", []):
        con_collisione |= set(c["asset"] for c in st["cose"] if c["tipo"] == "mesh" and c.get("collisione", True))
    con_collisione |= {"SM_Trappola_Botola", "SM_GrottaNord_PortaSegreta", "SM_GrottaNord_Grata"}
    for n, m in pezzi.items():
        veste(m, mat, n in con_collisione or n in ("SM_GrottaNord_Forziere", "SM_GrottaNord_Sasso"))
    specchio = controlla_assi(pezzi)

    giu = float(dati.get("giu_cm", 6000.0))
    origine = unreal.Vector(g.x, g.y, g.z - giu)

    def qui(p):
        return unreal.Vector(origine.x + p[0], origine.y + p[1], origine.z + p[2])

    # 1. sopra: la frana davanti al portale (il portale guarda a sud; la faccia sta 50 cm a nord del luogo)
    arrivo = next((c for c in dati["cose"] if c["tipo"] == "arrivo"), None)
    uscita = next((c for c in dati["cose"] if c["tipo"] == "passaggio"), None)
    classe_frana = getattr(unreal, "ValdorsoFrana", None)
    # (11/11) ai piedi della frana (il cumulo esce 4-5 m sul pendio): da li' la E la vede e la raggiunge
    davanti_y = g.y + 470.0
    z_davanti = terreno(mondo, g.x, davanti_y, g.z + 3000.0, g.z - 3000.0)
    if z_davanti is None:
        z_davanti = g.z
        avviso("non trovo il terreno davanti al portale: la frana va alla quota del luogo")
    if classe_frana is None:
        avviso("manca la classe ValdorsoFrana: serve compilare il C++ (v0.1.7)")
    else:
        fr = attori.spawn_actor_from_class(classe_frana, unreal.Vector(g.x, davanti_y, z_davanti), unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0))
        finisci(fr, "La frana (si apre)", sempre_caricato=True)
        if arrivo:
            fr.set_editor_property("dentro", qui(arrivo["pos"]))
            fr.set_editor_property("gradi_dentro", float(arrivo["gradi"]))
        scrivi("la frana e' davanti al portale, a %.0f cm" % z_davanti)
    frane = [a for a in attori.get_all_level_actors() if a and a.actor_has_tag("ValdorsoFrana")]
    if frane:
        scrivi("la frana vera (etichetta ValdorsoFrana): %d attori, spariscono quando si apre" % len(frane))
    else:
        avviso("nessun attore con l'etichetta ValdorsoFrana: rifare il villaggio, sezione 'Grotta nord' (dall'11/11 la "
               "frana e' un pezzo suo). Intanto la grotta funziona lo stesso: si entra dal portale anche con la frana in vista")

    # 2. sotto: le gallerie
    classi = {n: getattr(unreal, c, None) for n, c in (("ossario", "ValdorsoOssario"), ("forziere", "ValdorsoForziere"),
                                                       ("traccia", "ValdorsoTraccia"), ("passaggio", "ValdorsoPassaggio"),
                                                       ("iscrizione", "ValdorsoIscrizione"), ("suono", "ValdorsoSorgenteSuono"),
                                                       ("trappola", "ValdorsoTrappola"), ("porta_segreta", "ValdorsoPortaSegreta"),
                                                       ("allarme", "ValdorsoTesoroAllarme"))}
    conti = {}
    tipi_trappola = {"Lance": "LANCE", "Botola": "BOTOLA", "Pendolo": "PENDOLO", "Fondo": "FONDO"}

    def metti(c, p, rot):
        """Una cosa del JSON nel livello (le gallerie e le stanze segrete). Restituisce l'attore, o None."""
        tipo = c["tipo"]
        a = None
        if tipo == "mesh":
            m = pezzi.get(c["asset"])
            if m is None:
                return None
            a = attori.spawn_actor_from_object(m, p, rot)
            if a and specchio < 0:
                a.set_actor_scale3d(unreal.Vector(1, -1, 1))
            comp = a.get_component_by_class(unreal.StaticMeshComponent) if a else None
            if comp and not c.get("collisione", True):
                comp.set_collision_profile_name("NoCollision")
            if a and (c["asset"] == "SM_GrottaNord_PortaDormienti" or c.get("rune") == "sempre"):
                # le rune battono piano, giorno e notte (AValdorsoRuneNotte)
                a.set_editor_property("tags", [unreal.Name(ETICHETTA), unreal.Name("ValdorsoRuneSempre")])
        elif tipo == "torcia":
            m = pezzi.get("SM_GrottaNord_Torcia")
            if m is None:
                return None
            a = attori.spawn_actor_from_object(m, p, rot)
            comp = a.get_component_by_class(unreal.StaticMeshComponent) if a else None
            if comp:
                comp.set_collision_profile_name("NoCollision")
        elif tipo == "luce":
            a = attori.spawn_actor_from_class(unreal.PointLight, p, rot)
            lc = a.get_component_by_class(unreal.PointLightComponent)
            lc.set_mobility(unreal.ComponentMobility.MOVABLE)
            try:
                lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
            except Exception:  # noqa: BLE001
                pass
            lc.set_intensity(float(c.get("intensita", 5.0)))
            lc.set_attenuation_radius(float(c.get("raggio", 1000.0)))
            col = c.get("colore", [1.0, 0.55, 0.25])
            lc.set_light_color(unreal.LinearColor(col[0], col[1], col[2], 1.0))
            lc.set_cast_shadows(True)
        elif tipo == "ossario" and classi["ossario"]:
            a = attori.spawn_actor_from_class(classi["ossario"], p, rot)
            a.set_editor_property("tipo_scheletro", unreal.Name(c.get("scheletro", "Scheletro")))
            a.set_editor_property("quanti", int(c.get("quanti", 3)))
            a.set_editor_property("raggio", float(c.get("raggio", 500.0)))
            a.set_editor_property("minuti_ritorno", float(c.get("minuti", 30.0)))
        elif tipo == "forziere" and classi["forziere"]:
            a = attori.spawn_actor_from_class(classi["forziere"], p, rot)
            a.set_editor_property("ricchezza", int(c.get("ricchezza", 1)))
            a.set_editor_property("allarme", bool(c.get("allarme", False)))
            comp = a.get_component_by_class(unreal.StaticMeshComponent)
            if comp and pezzi.get("SM_GrottaNord_Forziere"):
                comp.set_static_mesh(pezzi["SM_GrottaNord_Forziere"])
        elif tipo == "traccia" and classi["traccia"]:
            a = attori.spawn_actor_from_class(classi["traccia"], p, rot)
            a.set_editor_property("oggetto", unreal.Name(c["oggetto"]))
            comp = a.get_component_by_class(unreal.StaticMeshComponent)
            if comp and pezzi.get(c.get("asset", "")):
                comp.set_static_mesh(pezzi[c["asset"]])
        elif tipo == "iscrizione" and classi["iscrizione"]:
            a = attori.spawn_actor_from_class(classi["iscrizione"], p, rot)
            a.set_editor_property("titolo", unreal.Text(c.get("titolo", c["nome"])))
            a.set_editor_property("avvisi", c.get("avvisi", []))
        elif tipo == "suono" and classi["suono"]:
            s = unreal.load_asset("%s/%s" % (SUONI_VALLE, c["suono"]))
            if s is None:
                avviso("%s: manca il suono %s (prima i suoni della valle)" % (c["nome"], c["suono"]))
                return None
            a = attori.spawn_actor_from_class(classi["suono"], p, rot)
            a.set_editor_property("suono", s)
            a.set_editor_property("quando", getattr(unreal.ValdorsoQuando, str(c.get("quando", "Sempre")).upper()))
            a.set_editor_property("vicino", float(c.get("vicino", 500.0)))
            a.set_editor_property("lontano", float(c.get("lontano", 2500.0)))
            a.set_editor_property("volume", float(c.get("volume", 1.0)))
        elif tipo == "trappola" and classi["trappola"]:
            a = attori.spawn_actor_from_class(classi["trappola"], p, rot)
            a.set_editor_property("tipo", getattr(unreal.ValdorsoTipoTrappola, tipi_trappola.get(c.get("trappola", "Lance"), "LANCE")))
            mi = c.get("misura", [100.0, 150.0, 60.0])
            a.set_editor_property("misura", unreal.Vector(mi[0], mi[1], mi[2]))
            for chiave, prop in (("ritardo", "ritardo"), ("ripristino", "ripristino"), ("periodo", "periodo"), ("fase", "fase"),
                                 ("ampiezza", "ampiezza"), ("braccio", "braccio")):
                if chiave in c:
                    a.set_editor_property(prop, float(c[chiave]))
            try:
                a.rerun_construction_scripts()
            except Exception:  # noqa: BLE001
                pass   # (in 5.8 per gli attori C++ non c'e': OnConstruction gira lo stesso al cambio delle proprieta')
        elif tipo == "porta_segreta" and classi["porta_segreta"]:
            a = attori.spawn_actor_from_class(classi["porta_segreta"], p, rot)
            a.set_editor_property("testo", c.get("testo", "Spingi la pietra"))
            a.set_editor_property("iscrizione_richiesta", c.get("iscrizione", ""))
            a.set_editor_property("minuti_aperta", float(c.get("minuti", 10.0)))
            comp = a.get_component_by_class(unreal.StaticMeshComponent)
            if comp and pezzi.get("SM_GrottaNord_PortaSegreta"):
                comp.set_static_mesh(pezzi["SM_GrottaNord_PortaSegreta"])
        elif tipo == "allarme" and classi["allarme"]:
            a = attori.spawn_actor_from_class(classi["allarme"], p, rot)
            a.set_editor_property("tipo_scheletro", unreal.Name(c.get("scheletro", "Scheletro")))
            a.set_editor_property("quanti", int(c.get("quanti", 6)))
            a.set_editor_property("raggio", float(c.get("raggio", 420.0)))
            gd = c.get("grata", [0.0, 0.0, 0.0])
            a.set_editor_property("grata_dove", unreal.Vector(gd[0], gd[1], gd[2]))
            a.set_editor_property("grata_gradi", float(c.get("grata_gradi", 0.0)))
            comp = a.get_component_by_class(unreal.StaticMeshComponent)
            if comp and pezzi.get("SM_GrottaNord_Grata"):
                comp.set_static_mesh(pezzi["SM_GrottaNord_Grata"])
        return a

    for c in dati["cose"]:
        tipo = c["tipo"]
        p = qui(c["pos"])
        rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(c.get("gradi", 0.0)))
        a = None
        try:
            if tipo == "passaggio":
                if classi["passaggio"] is None:
                    continue
                a = attori.spawn_actor_from_class(classi["passaggio"], p, rot)
                a.set_editor_property("testo", c.get("testo", "Torna alla luce"))
                # si esce davanti al portale, guardando a sud (verso la valle)
                fuori_y = g.y + 650.0
                z_fuori = terreno(mondo, g.x, fuori_y, g.z + 3000.0, g.z - 3000.0)
                a.set_editor_property("destinazione", unreal.Vector(g.x, fuori_y, (z_fuori if z_fuori is not None else g.z) + 110.0))
                a.set_editor_property("gradi_destinazione", 90.0)
            elif tipo == "arrivo":
                conti[tipo] = conti.get(tipo, 0) + 1
                continue
            elif tipo == "acqua":
                # (11/11) le miniere allagate: uno specchio d'acqua in piu' per AValdorsoAcque (cosi' ci si nuota)
                if aggiungi_acqua(c, qui):
                    conti[tipo] = conti.get(tipo, 0) + 1
                continue
            else:
                a = metti(c, p, rot)
                if a is None and tipo not in classi and tipo not in ("mesh", "torcia", "luce"):
                    avviso("%s: tipo sconosciuto %s" % (c["nome"], tipo))
                    continue
        except Exception as e:  # noqa: BLE001
            avviso("%s (%s): %s" % (c["nome"], tipo, e))
            continue
        if a is None:
            avviso("%s (%s): non messo" % (c["nome"], tipo))
            continue
        cartella = CONTORNO + ("/Gallerie" if tipo in ("mesh", "torcia", "luce") else "/Dentro")
        tags = [t for t in a.get_editor_property("tags")]
        finisci(a, c["nome"], cartella)
        if any(str(t) == "ValdorsoRuneSempre" for t in tags):
            a.set_editor_property("tags", [unreal.Name(ETICHETTA), unreal.Name("ValdorsoRuneSempre")])
        conti[tipo] = conti.get(tipo, 0) + 1

    # 2b. (11/11) le stanze segrete della valle: sotto il mulino e sotto le rovine (un passaggio segreto per entrare)
    for st in dati.get("segreti", []):
        ancora = next((x for x in attori.get_all_level_actors() if x and x.get_actor_label() == st["ancora"]), None)
        if ancora is None:
            avviso("%s: non trovo l'attore '%s' nel livello (prima il villaggio)" % (st["nome"], st["ancora"]))
            continue
        al = ancora.get_actor_location()
        ix, iy = al.x + st["ingresso"][0], al.y + st["ingresso"][1]
        zi = None
        try:
            r = unreal.SystemLibrary.line_trace_single(mondo, unreal.Vector(ix, iy, al.z + 250.0), unreal.Vector(ix, iy, al.z - 600.0),
                                                       unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
            hit = r[0] if isinstance(r, tuple) else r
            if hit is not None and hit.to_tuple()[0]:
                zi = hit.to_tuple()[4].z
        except Exception as e:  # noqa: BLE001
            avviso("%s: terreno dell'ingresso non trovato (%s)" % (st["nome"], e))
        if zi is None:
            zi = al.z
        base = unreal.Vector(ix, iy, zi - float(st["giu"]))

        def li(pp, base=base):
            return unreal.Vector(base.x + pp[0], base.y + pp[1], base.z + pp[2])

        arr = next((c for c in st["cose"] if c["tipo"] == "arrivo"), None)
        dentro = 0
        for c in st["cose"]:
            rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(c.get("gradi", 0.0)))
            try:
                if c["tipo"] == "arrivo":
                    continue
                if c["tipo"] == "uscita":
                    a = attori.spawn_actor_from_class(classi["passaggio"], li(c["pos"]), rot)
                    a.set_editor_property("testo", c["nome"])
                    a.set_editor_property("destinazione", unreal.Vector(ix, iy, zi + 110.0))
                    a.set_editor_property("gradi_destinazione", float(ancora.get_actor_rotation().yaw))
                else:
                    a = metti(c, li(c["pos"]), rot)
            except Exception as e:  # noqa: BLE001
                avviso("%s, %s: %s" % (st["nome"], c["nome"], e))
                continue
            if a is None:
                avviso("%s, %s: non messo" % (st["nome"], c["nome"]))
                continue
            tags = [str(t) for t in a.get_editor_property("tags")]
            finisci(a, c["nome"], "Valle/Segreti/" + st["nome"])
            if "ValdorsoRuneSempre" in tags:
                a.set_editor_property("tags", [unreal.Name(ETICHETTA), unreal.Name("ValdorsoRuneSempre")])
            dentro += 1
        # l'ingresso segreto, lassu'
        if classi["passaggio"] and arr:
            a = attori.spawn_actor_from_class(classi["passaggio"], unreal.Vector(ix, iy, zi), unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
            a.set_editor_property("testo", st["testo"])
            a.set_editor_property("segreto", True)
            a.set_editor_property("vicino", 130.0)
            a.set_editor_property("destinazione", li(arr["pos"]))
            a.set_editor_property("gradi_destinazione", float(arr["gradi"]))
            finisci(a, "Ingresso segreto: " + st["nome"], "Valle/Segreti/" + st["nome"])
        scrivi("stanza segreta '%s': %d cose, ingresso a (%.0f, %.0f, %.0f), stanza %.0f m sotto" % (
            st["nome"], dentro, ix, iy, zi, float(st["giu"]) / 100.0))
    scrivi("messo: " + ", ".join("%s %d" % (k, v) for k, v in sorted(conti.items())))
    for n, classe in classi.items():
        if classe is None:
            avviso("manca la classe C++ per '%s': serve compilare (v0.1.7)" % n)

    # 3. le gallerie non devono uscire dal paesaggio: quanta roccia c'e' sopra il soffitto, in qualche punto
    sopra = []
    for x, y, tetto in ((0, 0, 440), (0, -1500, 280), (0, -3800, 450), (-2700, -3850, 120), (0, -5300, -380),
                        (0, -6900, 280), (900, -6900, -100), (-900, -6900, -100),
                        # (11/11) l'ala segreta, il tesoro, le miniere, la cava, la forgia
                        (2400, -3850, 60), (4060, -3850, 220), (-4400, -3850, -550), (-5700, -3850, 150), (-3900, -6900, -500)):
        zt = terreno(mondo, origine.x + x, origine.y + y, origine.z + 20000.0, origine.z - 2000.0)
        if zt is not None:
            sopra.append((zt - (origine.z + tetto), x, y))
    if sopra:
        sopra.sort()
        scrivi("roccia sopra le gallerie: minimo %.0f m (a %.0f, %.0f dal punto d'arrivo), massimo %.0f m" % (
            sopra[0][0] / 100.0, sopra[0][1], sopra[0][2], sopra[-1][0] / 100.0))
        if sopra[0][0] < 300.0:
            avviso("le gallerie sono troppo vicine al paesaggio (o fuori): abbassare 'giu_cm' in grotta_nord.py e rifare")
    else:
        scrivi("roccia sopra le gallerie: non controllata (il paesaggio non e' caricato?)")

    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("salvato (livello e pezzi)")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): salvare a mano con Ctrl+Maiusc+S" % e)
    scrivi("FATTO. Per provare: Play, poi console Valdorso.Grotta.Apri e Valdorso.Grotta.Dentro (o F9 -> Mondo).")


try:
    principale()
finally:
    salva_resoconto()
