# Valdorso - Importa la pergamena e la calligrafia del Registro (passo 4.2b, seconda parte). Scritto da Claude il 05/10/2026.
# Si lancia nell'editor: Strumenti -> Esegui script Python -> questo file. Si può rilanciare: rimpiazza quello che c'è.
#
# Cosa fa:
#   - importa V:/Texture/valdorso/T_Pergamena.png in Content/UI/Registro, con le impostazioni da interfaccia
#     (gruppo UI, compressione per interfacce, niente mipmap, colore sRGB);
#   - importa Tangerine (Regular e Bold, licenza OFL) da V:/Font/Tangerine in Content/UI/Font, come Font Face;
#   - salva tutto e scrive nel Registro output "[Valdorso] Registro: importato" e cosa ha trovato.
# Dopo: chiudere l'editor prima di compilare, come sempre.

import unreal

PERGAMENA = "V:/Texture/valdorso/T_Pergamena.png"
CARTELLA_REGISTRO = "/Game/UI/Registro"
FONT = ["V:/Font/Tangerine/Tangerine-Regular.ttf", "V:/Font/Tangerine/Tangerine-Bold.ttf"]
CARTELLA_FONT = "/Game/UI/Font"

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary


def importa(file, destinazione):
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", file)
    compito.set_editor_property("destination_path", destinazione)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])
    nome = file.replace("\\", "/").split("/")[-1].rsplit(".", 1)[0]
    percorso = destinazione + "/" + nome + "." + nome
    asset = unreal.load_asset(percorso)
    if asset is None:
        unreal.log_warning("[Valdorso] Registro: non riesco a importare " + file)
    else:
        unreal.log("[Valdorso] Registro: " + percorso + " (" + asset.get_class().get_name() + ")")
    return asset


def esegui():
    tex = importa(PERGAMENA, CARTELLA_REGISTRO)
    if tex is not None:
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("srgb", True)
        libreria.save_loaded_asset(tex)

    trovati = 0
    for file in FONT:
        faccia = importa(file, CARTELLA_FONT)
        if faccia is not None:
            trovati += 1
            libreria.save_loaded_asset(faccia)

    esito = "importato" if tex is not None and trovati == len(FONT) else "importato in parte (vedi gli avvisi sopra)"
    unreal.log("[Valdorso] Registro: " + esito)


esegui()
