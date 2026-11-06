# Valdorso - La mappa di carta nel gioco (v0.1.3, tasto M). Scritto da Claude il 06/11/2026, mentre Fra dormiva.
# Si lancia una volta nell'editor (Strumenti -> Esegui script Python -> questo file), in qualsiasi livello.
#
# Importa V:/Texture/valdorso/villaggio/T_MappaValle.png (fatta da Tools/Generatori/mappa.py, che parte dalla mappa
# di valle.py e aggiunge la pianta del villaggio) in /Game/UI/Mappa/T_MappaValle, come immagine dell'interfaccia
# (niente mipmap, niente compressione che sporca le scritte). La mostra SValdorsoMappa (C++) con il tasto M.
# La texture è fatta da noi: può stare su GitHub (circa 4 MB).

import os

import unreal

FILE = "V:/Texture/valdorso/villaggio/T_MappaValle.png"
DEST = "/Game/UI/Mappa"


def esegui():
    if not os.path.exists(FILE):
        unreal.log_error("[Valdorso] Mappa: manca %s (scompattare villaggio.zip con crea_villaggio.py, o copiarla)" % FILE)
        return
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", FILE)
    c.set_editor_property("destination_path", DEST)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([c])
    t = unreal.load_asset(DEST + "/T_MappaValle")
    if t is None:
        unreal.log_error("[Valdorso] Mappa: importazione non riuscita")
        return
    t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    t.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    t.set_editor_property("never_stream", True)
    t.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(t)
    unreal.log("[Valdorso] Mappa: FATTO, /Game/UI/Mappa/T_MappaValle (%d x %d). In gioco: tasto M" % (t.blueprint_get_size_x(), t.blueprint_get_size_y()))


esegui()
