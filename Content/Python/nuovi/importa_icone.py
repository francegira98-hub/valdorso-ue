# Valdorso - Importa le icone ad acquerello degli oggetti (13/11/2026, scritto da Claude).
#
# Prende da V:/Texture/valdorso/icone i T_Icona_<Id>.png fatti da Tools/Generatori/icone/icone.py (uno per oggetto, più
# T_Icona_Moneta per gli Aureus), e la carta e la ceralacca della tessera (tessera.py), e li mette in /Game/UI/Icone con le impostazioni delle interfacce (gruppo UI,
# compressione per interfacce, niente mipmap, sRGB), come le texture del Registro. Si può rilanciare: le rimpiazza.
# Il gioco le trova da solo (SValdorsoZaino.cpp): zaino, barra delle azioni, baule e botteghe. /Game/UI va già sempre
# nel pacchetto (DirectoriesToAlwaysCook) e nella scatola 4.
# Scrive cosa ha fatto in Saved/Claude/importa_icone.txt.
# Si lancia dal menu Valdorso -> Strumenti -> "Icone: importa le icone ad acquerello".

import glob
import os
import unreal

CARTELLA = "V:/Texture/valdorso/icone"
DEST = "/Game/UI/Icone"

RIGHE = []


def scrivi(testo):
    RIGHE.append(testo)
    unreal.log("[Valdorso] icone: " + testo)


def principale():
    strumenti = unreal.AssetToolsHelpers.get_asset_tools()
    libreria = unreal.EditorAssetLibrary
    file = sorted(glob.glob(os.path.join(CARTELLA, "T_Icona_*.png")))
    # (13/11) anche la carta e la coccarda di ceralacca della tessera del colono (Tools/Generatori/icone/tessera.py)
    file += [f for f in (os.path.join(CARTELLA, n) for n in ("T_Tessera_Carta.png", "T_Ceralacca.png")) if os.path.exists(f)]
    if not file:
        scrivi("ATTENZIONE: nessuna icona in %s" % CARTELLA)
        return
    compiti = []
    for f in file:
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", DEST)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", False)
        compiti.append(c)
    strumenti.import_asset_tasks(compiti)
    fatte, mancate = 0, []
    for f in file:
        nome = os.path.splitext(os.path.basename(f))[0]
        t = unreal.load_asset("%s/%s" % (DEST, nome))
        if t is None:
            mancate.append(nome)
            continue
        t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        t.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        t.set_editor_property("srgb", True)
        libreria.save_loaded_asset(t, only_if_is_dirty=False)
        fatte += 1
    scrivi("importate: %d su %d (in %s)" % (fatte, len(file), DEST))
    for m in mancate:
        scrivi("ATTENZIONE: non importata %s" % m)
    scrivi("FATTO. Nel gioco: zaino (I), barra delle azioni, baule, botteghe. Valdorso.Icone.Acquerello 0 = le linee di prima.")


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "importa_icone.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(RIGHE) + "\n")


try:
    principale()
finally:
    salva_resoconto()
