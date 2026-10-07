# Valdorso - Le rocce vere della valle, prima parte: importarle (v0.1.3). Scritto da Claude il 07/10/2026.
# Rocce fotografate dal vero di Poly Haven (CC0: libere per qualsiasi uso, anche senza citarle), scaricate da Fra
# come zip FBX 2K. Si lancia nell'editor: Strumenti -> Esegui script Python -> questo file. Si può rilanciare.
#
# Cosa fa:
#   1. apre gli zip (dalla cartella Download di Fra) in V:/Texture/polyhaven/rocce, se non sono già aperti;
#   2. importa modelli e texture in /Game/Fab/PolyHaven/<roccia> (cartella fuori da GitHub, come gli asset pesanti);
#   3. crea un materiale solo, M_RocciaScansione (colore, normale, ruvidità, un poco più scuro e regolabile),
#      e un'istanza per ogni roccia con le sue texture;
#   4. accende Nanite sui modelli (milioni di triangoli senza pesare sulla scheda video);
#   5. scrive nel Registro output la misura di ogni modello: serve a Claude per lo script che le sparge nella gola.
# Non mette ancora niente nel livello.

import os
import zipfile

import unreal

DOWNLOAD = "C:/Users/Francesco/Downloads"
DESTINAZIONE_FILE = "V:/Texture/polyhaven/rocce"
DESTINAZIONE = "/Game/Fab/PolyHaven"
ROCCE = ["rock_face_01", "rock_face_02", "coastal_cliff_04", "boulder_01", "rock_moss_set_01", "rock_moss_set_02"]

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(testo):
    unreal.log("[Valdorso] Rocce: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] Rocce: " + testo)


def apri_zip(nome):
    cartella = os.path.join(DESTINAZIONE_FILE, nome + "_2k")
    fbx = os.path.join(cartella, nome + "_2k.fbx")
    if os.path.exists(fbx):
        return cartella
    zip_ = os.path.join(DOWNLOAD, nome + "_2k.fbx.zip")
    if not os.path.exists(zip_):
        avviso("manca " + zip_ + " (scaricalo da polyhaven.com: 2K, FBX)")
        return None
    os.makedirs(cartella, exist_ok=True)
    with zipfile.ZipFile(zip_) as z:
        z.extractall(cartella)
    log("aperto " + os.path.basename(zip_))
    return cartella


def importa(file, cartella_ue):
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", file.replace("\\", "/"))
    compito.set_editor_property("destination_path", cartella_ue)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])
    return [unreal.load_asset(p) for p in compito.get_editor_property("imported_object_paths")]


def texture(file, cartella_ue, tipo):
    importati = [a for a in importa(file, cartella_ue) if isinstance(a, unreal.Texture2D)]
    if not importati:
        nome = os.path.splitext(os.path.basename(file))[0]
        a = unreal.load_asset(cartella_ue + "/" + nome)
        importati = [a] if isinstance(a, unreal.Texture2D) else []
    if not importati:
        avviso("non riesco a importare " + file)
        return None
    t = importati[0]
    if tipo == "colore":
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        t.set_editor_property("srgb", True)
    elif tipo == "normale":
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        t.set_editor_property("srgb", False)
        t.set_editor_property("flip_green_channel", True)     # Poly Haven "nor_gl" = OpenGL; Unreal vuole DirectX
    else:
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        t.set_editor_property("srgb", False)
    t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    libreria.save_loaded_asset(t)
    return t


def materiale_base():
    percorso = DESTINAZIONE + "/M_RocciaScansione"
    if libreria.does_asset_exist(percorso):
        libreria.delete_asset(percorso)
    m = strumenti.create_asset("M_RocciaScansione", DESTINAZIONE, unreal.Material, unreal.MaterialFactoryNew())

    def parametro_texture(nome, tipo, x, y):
        p = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        p.set_editor_property("parameter_name", nome)
        p.set_editor_property("sampler_type", tipo)
        return p

    colore = parametro_texture("Colore", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -800, -200)
    normale = parametro_texture("Normale", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -800, 100)
    ruvido = parametro_texture("Ruvidita", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, -800, 400)
    tinta = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, -350)
    tinta.set_editor_property("parameter_name", "Luminosita")
    tinta.set_editor_property("default_value", 0.8)          # la nostra roccia è scura e grigia
    per = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, -250)
    mel.connect_material_expressions(colore, "RGB", per, "A")
    mel.connect_material_expressions(tinta, "", per, "B")
    mel.connect_material_property(per, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(normale, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(ruvido, "R", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.layout_material_expressions(m)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    return m


def istanza(base, nome, cartella_ue, colore, normale, ruvido):
    percorso = cartella_ue + "/MI_" + nome
    if libreria.does_asset_exist(percorso):
        libreria.delete_asset(percorso)
    mi = strumenti.create_asset("MI_" + nome, cartella_ue, unreal.MaterialInstanceConstant,
                                unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", base)
    for parametro, t in (("Colore", colore), ("Normale", normale), ("Ruvidita", ruvido)):
        if t is not None:
            mel.set_material_instance_texture_parameter_value(mi, parametro, t)
    libreria.save_loaded_asset(mi)
    return mi


def esegui():
    base = materiale_base()
    for nome in ROCCE:
        cartella = apri_zip(nome)
        if cartella is None:
            continue
        cartella_ue = DESTINAZIONE + "/" + nome
        cartella_tex = os.path.join(cartella, "textures")
        def trova(parte):
            for f in sorted(os.listdir(cartella_tex)):
                if parte in f:
                    return os.path.join(cartella_tex, f)
            return None
        colore = texture(trova("_diff_"), cartella_ue, "colore") if trova("_diff_") else None
        normale = texture(trova("_nor_gl_"), cartella_ue, "normale") if trova("_nor_gl_") else None
        ruvido = texture(trova("_rough_"), cartella_ue, "ruvido") if trova("_rough_") else None
        mi = istanza(base, nome, cartella_ue, colore, normale, ruvido)

        importa(os.path.join(cartella, nome + "_2k.fbx"), cartella_ue)
        mesh = [unreal.load_asset(p) for p in libreria.list_assets(cartella_ue, recursive=True)]
        mesh = [a for a in mesh if isinstance(a, unreal.StaticMesh)]
        if not mesh:
            avviso(nome + ": nessun modello importato")
            continue
        for sm in mesh:
            for i in range(len(sm.static_materials)):
                sm.set_material(i, mi)
            try:
                nanite = sm.get_editor_property("nanite_settings")
                nanite.set_editor_property("enabled", True)
                sm.set_editor_property("nanite_settings", nanite)
            except Exception as e:
                avviso("{}: Nanite non acceso ({})".format(sm.get_name(), e))
            libreria.save_loaded_asset(sm)
            s = sm.get_bounding_box()
            d = s.max - s.min
            log("{} | {} | misura X {:.0f} Y {:.0f} Z {:.0f} cm | da X {:.0f} Y {:.0f} Z {:.0f} | triangoli {}".format(
                nome, sm.get_name(), d.x, d.y, d.z, s.min.x, s.min.y, s.min.z, sm.get_num_triangles(0)))
    log("fatto. Le rocce sono in Content/Fab/PolyHaven (fuori da GitHub).")


esegui()
