# Valdorso - Importa la pergamena e la calligrafia del Registro (passo 4.2b, seconda parte). Scritto da Claude il 05/10/2026.
# Si lancia nell'editor: Strumenti -> Esegui script Python -> questo file. Si può rilanciare: rimpiazza quello che c'è.
#
# Cosa fa:
#   - importa V:/Texture/valdorso/T_Pergamena.png e T_Cuoio.png (05/10) in Content/UI/Registro, con le impostazioni da interfaccia
#     (gruppo UI, compressione per interfacce, niente mipmap, colore sRGB);
#   - importa Tangerine (Regular e Bold, licenza OFL) da V:/Font/Tangerine in Content/UI/Font, come Font Face;
#   - (05/10) importa i suoni da V:/Texture/valdorso/suoni in Content/Audio/Registro (fuoco, battito e pennino
#     si ripetono da soli: "looping");
#   - salva tutto e scrive nel Registro output "[Valdorso] Registro: importato" e cosa ha trovato.
# Dopo: chiudere l'editor prima di compilare, come sempre.

import unreal

PERGAMENA = "V:/Texture/valdorso/T_Pergamena.png"
CUOIO = "V:/Texture/valdorso/T_Cuoio.png"       # (05/10) i pulsanti di cuoio
CARTELLA_REGISTRO = "/Game/UI/Registro"
FONT = ["V:/Font/Tangerine/Tangerine-Regular.ttf", "V:/Font/Tangerine/Tangerine-Bold.ttf"]
CARTELLA_FONT = "/Game/UI/Font"
SUONI = "V:/Texture/valdorso/suoni"
CARTELLA_SUONI = "/Game/Audio/Registro"
SUONI_RIPETUTI = ["S_Battito", "S_Fuoco", "S_Pennino"]
NOMI_SUONI = SUONI_RIPETUTI + ["S_Campana", "S_Ceralacca", "S_Fede_Solara", "S_Fede_Ignar", "S_Fede_Nereia",
                               "S_Fede_Torvald", "S_Fede_Zefira", "S_Fede_VecchiDei", "S_Fede_Nessuna"]

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
    tex = None
    for file in [PERGAMENA, CUOIO]:
        una = importa(file, CARTELLA_REGISTRO)
        if una is not None:
            una.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            una.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            una.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            una.set_editor_property("srgb", True)
            libreria.save_loaded_asset(una)
            if file == PERGAMENA:
                tex = una

    trovati = 0
    for file in FONT:
        faccia = importa(file, CARTELLA_FONT)
        if faccia is not None:
            trovati += 1
            libreria.save_loaded_asset(faccia)

    suoni = 0
    for nome in NOMI_SUONI:
        onda = importa(SUONI + "/" + nome + ".wav", CARTELLA_SUONI)
        if onda is not None:
            suoni += 1
            onda.set_editor_property("looping", nome in SUONI_RIPETUTI)
            libreria.save_loaded_asset(onda)
    unreal.log("[Valdorso] Registro: suoni importati " + str(suoni) + " su " + str(len(NOMI_SUONI)))

    esito = "importato" if tex is not None and trovati == len(FONT) and suoni == len(NOMI_SUONI) else "importato in parte (vedi gli avvisi sopra)"
    unreal.log("[Valdorso] Registro: " + esito)


esegui()
