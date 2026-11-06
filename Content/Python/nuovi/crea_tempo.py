# Valdorso - Il tempo della valle: la pioggia (v0.1.3). Scritto da Claude il 06/11/2026, mentre Fra dormiva.
# Si lancia una volta nell'editor (Strumenti -> Esegui script Python -> questo file), in qualsiasi livello.
#
# Importa da V:/Texture/valdorso/tempo (fatti da Tools/Generatori/tempo.py, da zero, senza licenze):
#   T_Pioggia.png -> /Game/Valle/Tempo/T_Pioggia   (le gocce)
#   S_Pioggia.wav -> /Game/Valle/Tempo/S_Pioggia   (il suono, che si ripete)
#   SM_TendaPioggia.glb -> /Game/Valle/Tempo/SM_TendaPioggia (la tenda delle gocce, un tubo senza tappi)
# e fa il materiale M_Pioggia: trasparente, illuminato dalla luce della scena (06/11: prima era senza luce, quasi
# invisibile di giorno e troppo chiaro di notte), a due facce; le gocce scendono (parametro Velocita:
# se in gioco salgono invece di scendere, mettere Velocita negativa nel materiale), Pioggia (0..1) e Scala li cambia
# AValdorsoTempo (C++), che mette sei tende di gocce intorno alla telecamera (da 1,5 m a 26 m).

import os

import unreal

CARTELLA = "V:/Texture/valdorso/tempo"
DEST = "/Game/Valle/Tempo"
libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def importa(file):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", file)
    c.set_editor_property("destination_path", DEST)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    strumenti.import_asset_tasks([c])


def nodo(m, classe, x, y):
    return mel.create_material_expression(m, classe, x, y)


def esegui():
    for n in ("T_Pioggia.png", "S_Pioggia.wav"):
        if not os.path.exists(os.path.join(CARTELLA, n)):
            unreal.log_error("[Valdorso] Tempo: manca %s/%s" % (CARTELLA, n))
            return
        importa(os.path.join(CARTELLA, n))
    if os.path.exists(os.path.join(CARTELLA, "SM_TendaPioggia.glb")):
        importa(os.path.join(CARTELLA, "SM_TendaPioggia.glb"))
    tex = unreal.load_asset(DEST + "/T_Pioggia")

    suono = unreal.load_asset(DEST + "/S_Pioggia")
    if suono is not None:
        suono.set_editor_property("looping", True)
        libreria.save_loaded_asset(suono)

    percorso = DEST + "/M_Pioggia"
    m = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if m is not None:
        mel.delete_all_material_expressions(m)
    else:
        m = strumenti.create_asset("M_Pioggia", DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("two_sided", True)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    # illuminate dal volume di luce delle cose trasparenti: costano poco e prendono sole, cielo e lanterne
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)

    # UV x Scala (la tenda grande ha le gocce più piccole) + il tempo che le fa scendere
    uv = nodo(m, unreal.MaterialExpressionTextureCoordinate, -1200, -100)
    uv.set_editor_property("u_tiling", 8.0)
    uv.set_editor_property("v_tiling", 2.0)
    scala = nodo(m, unreal.MaterialExpressionScalarParameter, -1200, 50)
    scala.set_editor_property("parameter_name", "Scala")
    scala.set_editor_property("default_value", 1.0)
    uvs = nodo(m, unreal.MaterialExpressionMultiply, -1000, -50)
    mel.connect_material_expressions(uv, "", uvs, "A")
    mel.connect_material_expressions(scala, "", uvs, "B")
    tempo = nodo(m, unreal.MaterialExpressionTime, -1200, 200)
    vel = nodo(m, unreal.MaterialExpressionScalarParameter, -1200, 300)
    vel.set_editor_property("parameter_name", "Velocita")
    vel.set_editor_property("default_value", -1.6)
    scorre = nodo(m, unreal.MaterialExpressionMultiply, -1000, 250)
    mel.connect_material_expressions(tempo, "", scorre, "A")
    mel.connect_material_expressions(vel, "", scorre, "B")
    zero = nodo(m, unreal.MaterialExpressionConstant, -1000, 150)
    zero.set_editor_property("r", 0.0)
    verso = nodo(m, unreal.MaterialExpressionAppendVector, -850, 200)
    mel.connect_material_expressions(zero, "", verso, "A")
    mel.connect_material_expressions(scorre, "", verso, "B")
    uvf = nodo(m, unreal.MaterialExpressionAdd, -700, 50)
    mel.connect_material_expressions(uvs, "", uvf, "A")
    mel.connect_material_expressions(verso, "", uvf, "B")
    tx = nodo(m, unreal.MaterialExpressionTextureSample, -500, 50)
    tx.set_editor_property("texture", tex)
    mel.connect_material_expressions(uvf, "", tx, "UVs")

    pioggia = nodo(m, unreal.MaterialExpressionScalarParameter, -500, 300)
    pioggia.set_editor_property("parameter_name", "Pioggia")
    pioggia.set_editor_property("default_value", 1.0)
    forza = nodo(m, unreal.MaterialExpressionConstant, -500, 420)
    forza.set_editor_property("r", 0.6)
    a1 = nodo(m, unreal.MaterialExpressionMultiply, -250, 150)
    mel.connect_material_expressions(tx, "R", a1, "A")
    mel.connect_material_expressions(pioggia, "", a1, "B")
    a2 = nodo(m, unreal.MaterialExpressionMultiply, -100, 250)
    mel.connect_material_expressions(a1, "", a2, "A")
    mel.connect_material_expressions(forza, "", a2, "B")
    mel.connect_material_property(a2, "", unreal.MaterialProperty.MP_OPACITY)

    colore = nodo(m, unreal.MaterialExpressionConstant3Vector, -250, -150)
    colore.set_editor_property("constant", unreal.LinearColor(0.75, 0.78, 0.82, 1.0))
    mel.connect_material_property(colore, "", unreal.MaterialProperty.MP_BASE_COLOR)
    ruvido = nodo(m, unreal.MaterialExpressionConstant, -250, -40)
    ruvido.set_editor_property("r", 0.3)
    mel.connect_material_property(ruvido, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    # la tenda: il materiale nel suo posto e niente collisione (la pioggia non si urta)
    tenda = None
    for p in libreria.list_assets(DEST, recursive=True, include_folder=False):
        if p.rsplit(".", 1)[-1] == "SM_TendaPioggia":
            tenda = unreal.load_asset(p)
    if isinstance(tenda, unreal.StaticMesh):
        tenda.set_material(0, m)
        libreria.save_loaded_asset(tenda)
    else:
        unreal.log_warning("[Valdorso] Tempo: SM_TendaPioggia non importata: si usa il cilindro di Unreal")
    unreal.log("[Valdorso] Tempo: FATTO, M_Pioggia, T_Pioggia e S_Pioggia in %s. In gioco: Valdorso.Tempo Pioggia" % DEST)


esegui()
