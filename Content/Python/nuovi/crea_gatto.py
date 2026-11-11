# Valdorso - Il gatto della locanda (v0.1.6, La locanda viva). Scritto da Claude l'11/11/2026.
# Si lancia in L_Valle (menu Valdorso -> "Gatto della locanda", oppure Strumenti -> Esegui script Python).
# Si puo' rilanciare: toglie il gatto messo prima (etichetta "ValdorsoGatto") e lo rimette.
#
# Legge V:/Texture/valdorso/gatto (Tools/Generatori/gatto.py; se c'e' gatto.zip accanto, lo scompatta):
#   SM_Gatto_*.glb   -> /Game/Valle/Gatto          (corpo, testa, coda: li monta AValdorsoGatto)
#   S_Gatto_*.wav    -> /Game/Valle/Pericoli/Suoni (le fusa, il miagolio)
# e mette AValdorsoGatto nella Locanda di Bruno, due metri davanti a Bruno, sul pavimento.
# Resoconto in Saved/Claude/gatto.txt.

import os
import zipfile

import unreal

CARTELLA = "V:/Texture/valdorso/gatto"
DEST = "/Game/Valle/Gatto"
SUONI = "/Game/Valle/Pericoli/Suoni"
MATERIALI = "/Game/Valle/Villaggio/Materiali"
ETICHETTA = "ValdorsoGatto"

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Gatto: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Gatto: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "gatto.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe))


def compito(file, destinazione):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", file)
    c.set_editor_property("destination_path", destinazione)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    return c


def cerca_e_metti_a_posto(cartella, nome, classe):
    """L'importazione dei .glb puo' mettere il modello in <cartella>/<nome>/StaticMeshes: lo si sposta dove lo cerca il C++."""
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
        if vecchio != giusto and not libreria.does_asset_exist(giusto) and libreria.rename_asset(vecchio, giusto):
            scrivi("spostato %s -> %s" % (vecchio, giusto))
            x = unreal.load_asset("%s.%s" % (giusto, nome))
        return x
    return None


def istanza(nome, tinta, luce=0.0, ruvidezza=0.85):
    genitore = unreal.load_asset(MATERIALI + "/M_VillaggioColore")
    if genitore is None:
        avviso("manca M_VillaggioColore (prima il villaggio)")
        return None
    percorso = "%s/%s" % (MATERIALI, nome)
    mi = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if mi is None:
        mi = strumenti.create_asset(nome, MATERIALI, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, genitore)
    mel.set_material_instance_vector_parameter_value(mi, "Tinta", unreal.LinearColor(tinta[0], tinta[1], tinta[2], 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "Ruvidezza", ruvidezza)
    mel.set_material_instance_scalar_parameter_value(mi, "Luce", luce)
    mel.update_material_instance(mi)
    libreria.save_loaded_asset(mi)
    return mi


def veste(mesh, mat):
    for i, sm in enumerate(mesh.get_editor_property("static_materials")):
        nome = str(sm.get_editor_property("material_slot_name"))
        mi = sm.get_editor_property("material_interface")
        if (not nome or nome == "None") and mi is not None:
            nome = mi.get_name()
        chiave = nome.split("MV_", 1)[-1].split("_")[0] if "MV_" in nome else None
        if chiave in mat and mat[chiave] is not None:
            mesh.set_material(i, mat[chiave])
        else:
            avviso("%s: slot %s senza materiale" % (mesh.get_name(), nome))
    libreria.save_loaded_asset(mesh)


def principale():
    zf = CARTELLA + ".zip"
    segno = CARTELLA + "/SM_Gatto_Corpo.glb"
    if os.path.exists(zf) and (not os.path.exists(segno) or os.path.getmtime(zf) > os.path.getmtime(segno)):
        os.makedirs(CARTELLA, exist_ok=True)
        with zipfile.ZipFile(zf) as z:
            z.extractall(CARTELLA)
        os.utime(segno, None)
        scrivi("scompattato %s" % zf)
    if not os.path.exists(segno):
        avviso("manca %s (e %s)" % (segno, zf))
        return

    # 1. i pezzi, i materiali, i suoni
    nomi = ["SM_Gatto_Corpo", "SM_Gatto_Testa", "SM_Gatto_Coda"]
    strumenti.import_asset_tasks([compito("%s/%s.glb" % (CARTELLA, n), DEST) for n in nomi])
    mat = {"Pelo": istanza("MI_Gatto_Pelo", (1.0, 1.0, 1.0), 0.0, 0.9),
           "Occhi": istanza("MI_Gatto_Occhi", (0.55, 0.48, 0.08), 0.35, 0.2),
           "Stoffa": unreal.load_asset(MATERIALI + "/MI_Villaggio_Stoffa")}
    for n in nomi:
        m = cerca_e_metti_a_posto(DEST, n, unreal.StaticMesh)
        if m is None:
            avviso("pezzo non importato: %s" % n)
            continue
        veste(m, mat)
        try:
            ns = m.get_editor_property("nanite_settings")
            ns.set_editor_property("enabled", False)          # piccolo e si muove: senza Nanite
            m.set_editor_property("nanite_settings", ns)
        except Exception:  # noqa: BLE001
            pass
        libreria.save_loaded_asset(m)
    scrivi("pezzi pronti in %s" % DEST)
    suoni = [n for n in ("S_Gatto_Fusa", "S_Gatto_Miao") if os.path.exists("%s/%s.wav" % (CARTELLA, n))]
    strumenti.import_asset_tasks([compito("%s/%s.wav" % (CARTELLA, n), SUONI) for n in suoni])
    for n in suoni:
        s = cerca_e_metti_a_posto(SUONI, n, unreal.SoundWave)
        if s is None:
            avviso("suono non importato: %s" % n)
            continue
        s.set_editor_property("looping", False)
        libreria.save_loaded_asset(s)
    scrivi("suoni: %s" % ", ".join(suoni))

    # 2. il gatto nella locanda, davanti a Bruno
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
    classe = getattr(unreal, "ValdorsoGatto", None)
    classe_png = getattr(unreal, "ValdorsoPNG", None)
    if classe is None or classe_png is None:
        avviso("manca la classe ValdorsoGatto: serve compilare il C++ (v0.1.6)")
        return
    bruno = None
    for a in attori.get_all_level_actors():
        if isinstance(a, classe_png) and str(a.get_editor_property("dialogo")) == "Bruno":
            bruno = a
            break
    if bruno is None:
        avviso("Bruno non c'e' nel livello (prima metti_png.py): il gatto non ha una casa")
        return
    p = bruno.get_actor_location()
    avanti = bruno.get_actor_forward_vector()
    dove = unreal.Vector(p.x + avanti.x * 200.0, p.y + avanti.y * 200.0, p.z)
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    z = p.z - 90.0
    try:
        r = unreal.SystemLibrary.line_trace_single(mondo, unreal.Vector(dove.x, dove.y, p.z + 40.0), unreal.Vector(dove.x, dove.y, p.z - 300.0),
                                                   unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [bruno], unreal.DrawDebugTrace.NONE, True)
        hit = r[0] if isinstance(r, tuple) else r
        if hit is not None:
            t = hit.to_tuple()
            if t[0]:
                z = t[4].z
    except Exception as e:  # noqa: BLE001
        avviso("pavimento non trovato (%s): il gatto va 90 cm sotto Bruno" % e)
    g = attori.spawn_actor_from_class(classe, unreal.Vector(dove.x, dove.y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=bruno.get_actor_rotation().yaw))
    g.set_actor_label("Il gatto della locanda")
    g.set_folder_path("Valle/Villaggio/Vita")
    g.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    scrivi("il gatto e' nella locanda a (%.0f, %.0f, %.0f)" % (dove.x, dove.y, z))
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("salvato")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): salvare a mano con Ctrl+Maiusc+S" % e)
    scrivi("FATTO. In gioco: E vicino al gatto. Console: Valdorso.Gatto.Qui")


try:
    principale()
finally:
    salva_resoconto()
