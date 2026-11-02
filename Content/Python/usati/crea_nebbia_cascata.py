# Valdorso - Il materiale degli spruzzi e della nebbia della cascata (v0.1.3). Scritto da Claude il 07/10/2026.
# Si lancia nell'editor: Strumenti -> Esegui script Python -> questo file. Si può rilanciare.
#
# Cosa fa:
#   1. importa V:/Texture/valdorso/valle/cascata/T_CascataNebbia.png (16 sbuffi di nebbia in 4 x 4, fatti da
#      Tools/Generatori/cascata.py) in Content/Valle/Cascata;
#   2. crea M_CascataNebbia, il materiale delle particelle di Niagara: trasparente, senza ombre, ogni particella
#      prende uno dei 16 sbuffi (Particle SubUV), con il colore e la trasparenza decisi da Niagara (Particle Color)
#      e che svanisce dove tocca la roccia o l'acqua (Depth Fade), così non si vedono tagli dritti.
# Il sistema Niagara (NS_CascataNebbia) lo crea Fra nell'editor, con i passi di Claude.

import unreal

FILE = "V:/Texture/valdorso/valle/cascata/T_CascataNebbia.png"
DESTINAZIONE = "/Game/Valle/Cascata"

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(testo):
    unreal.log("[Valdorso] Nebbia: " + testo)


def esegui():
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", FILE)
    compito.set_editor_property("destination_path", DESTINAZIONE)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])
    tex = unreal.load_asset(DESTINAZIONE + "/T_CascataNebbia")
    if tex is None:
        unreal.log_error("[Valdorso] Nebbia: non riesco a importare " + FILE)
        return
    tex.set_editor_property("srgb", True)
    libreria.save_loaded_asset(tex)

    percorso = DESTINAZIONE + "/M_CascataNebbia"
    if libreria.does_asset_exist(percorso):
        libreria.delete_asset(percorso)
    m = strumenti.create_asset("M_CascataNebbia", DESTINAZIONE, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    try:
        m.set_editor_property("used_with_niagara_sprites", True)
    except Exception:
        pass

    sub = mel.create_material_expression(m, unreal.MaterialExpressionParticleSubUV, -700, 0)
    sub.set_editor_property("texture", tex)
    sub.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    colore = mel.create_material_expression(m, unreal.MaterialExpressionParticleColor, -700, 300)

    per_rgb = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 0)
    mel.connect_material_expressions(sub, "RGB", per_rgb, "A")
    mel.connect_material_expressions(colore, "RGB", per_rgb, "B")
    mel.connect_material_property(per_rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    per_a = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 250)
    mel.connect_material_expressions(sub, "A", per_a, "A")
    mel.connect_material_expressions(colore, "A", per_a, "B")
    sfuma = mel.create_material_expression(m, unreal.MaterialExpressionDepthFade, -200, 250)
    sfuma.set_editor_property("fade_distance_default", 150.0)
    mel.connect_material_expressions(per_a, "", sfuma, "Opacity")
    mel.connect_material_property(sfuma, "", unreal.MaterialProperty.MP_OPACITY)

    mel.layout_material_expressions(m)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    log("creato M_CascataNebbia (16 sbuffi, colore e trasparenza da Niagara). Fatto.")


esegui()
