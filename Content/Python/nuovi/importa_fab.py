# Valdorso - Importa i pacchetti di Fab scaricati (zip o file sciolti nella cartella Download) in Content/Fab.
# Scritto da Claude il 03/11/2026 (seconda versione la notte del 03/11: case con le texture a parte).
# Si può rilanciare: salta i pacchetti che sono già nel progetto.
#
# Per ogni pacchetto: estrae dallo zip i modelli e le texture in Saved/Fab/<Nome> (fuori da GitHub) e li importa in
# /Game/Fab/<Nome>. Per i pacchetti con le texture a parte (le case) importa le texture in /Game/Fab/<Nome>/Texture e
# costruisce da solo un materiale per ogni pezzo, cercando le texture con lo stesso nome del materiale del modello.
# Scrive in Saved/Claude/importa_fab.txt cosa è stato creato e quanto è grande ogni modello (così Claude controlla
# la scala). Content/Fab è fuori da GitHub (licenze): niente di tutto questo va nel commit.

import os
import zipfile
import unreal

DOWNLOAD = os.path.join(os.path.expanduser("~"), "Downloads")

# (cartella in Content/Fab, file nella cartella Download, crediti, altro)
# altro: "texture": (zip delle texture, pezzo del percorso da prendere) e "materiali": True = costruire i materiali.
PACCHETTI = [
    ("Taverna", "fbx_medieval_tavern_kit_v10.zip", "Modular Medieval Tavern v.1.0 - Danilo Waselciac (Fab, Standard License)", {}),
    ("Mercato_Tenda1", "medieval-market-tent-prop-01.zip", "Medieval Market / Tent Prop 01 (Fab) - autore e licenza da segnare", {}),
    ("Mercato_Tenda2", "medieval-market-tent-prop-02.zip", "Medieval Market / Tent Prop 02 (Fab) - autore e licenza da segnare", {}),
    ("Letto", "bed.glb", "Medieval Bed - Grim Edge Studio (Fab, Standard License)", {}),
    ("Letto2", "medieval_bed.zip", "Medieval Bed - Jordan F (Sketchfab, CC BY 4.0: nei CREDITI)", {}),
    ("Casa_Camino", "fbx_unique_village_house_with_stone_fireplace.zip",
     "Unique Village House with Stone Fireplace (Fab) - autore e licenza da segnare", {"materiali": True}),
    ("Case_Medievali", "medieval_houses_10.fbx", "Medieval Houses (Fab) - autore e licenza da segnare",
     {"texture": ("medieval_houses_textures.zip", "Unreal_textures/"), "materiali": True}),
]

MODELLI = (".fbx", ".glb", ".gltf")
IMMAGINI = (".jpg", ".jpeg", ".png", ".tga")
righe = []


def scrivi(testo=""):
    righe.append(testo)
    unreal.log("[Valdorso] Fab: " + testo)


def cartella_saved(*pezzi):
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), *pezzi)
    os.makedirs(cartella, exist_ok=True)
    return cartella


def estrai(voce, archivio, arrivo):
    if os.path.exists(arrivo) and os.path.getsize(arrivo) == voce.file_size:
        return
    with archivio.open(voce) as dentro, open(arrivo, "wb") as fuori:
        while True:
            pezzo = dentro.read(1 << 20)
            if not pezzo:
                break
            fuori.write(pezzo)


def prepara(nome, file):
    """Ritorna (modelli da importare, immagini estratte)."""
    sorgente = os.path.join(DOWNLOAD, file)
    if not os.path.exists(sorgente):
        scrivi("%s: manca %s nella cartella Download (saltato)" % (nome, file))
        return [], []
    if not file.lower().endswith(".zip"):
        return [sorgente], []
    cartella = cartella_saved("Fab", nome)
    modelli, immagini = [], []
    with zipfile.ZipFile(sorgente) as archivio:
        # I .gltf puntano a scene.bin e a textures/... con il loro percorso: lo zip si estrae com'è.
        gltf = [v.filename for v in archivio.infolist() if v.filename.lower().endswith(".gltf")]
        if gltf:
            archivio.extractall(cartella)
            return [os.path.join(cartella, *g.split("/")) for g in gltf], []
        for voce in archivio.infolist():
            basso = voce.filename.lower()
            if voce.is_dir() or not basso.endswith(MODELLI + IMMAGINI):
                continue
            # Le texture vanno accanto al modello, con il nome che il modello si aspetta (.jpg, non .jpeg).
            nome_file = os.path.basename(voce.filename)
            if basso.endswith(".jpeg"):
                nome_file = nome_file[:-5] + ".jpg"
            arrivo = os.path.join(cartella, nome_file)
            estrai(voce, archivio, arrivo)
            (modelli if basso.endswith(MODELLI) else immagini).append(arrivo)
    return modelli, immagini


def texture_a_parte(nome, file, pezzo):
    """Estrae dallo zip delle texture solo quelle del percorso indicato (es. le texture per Unreal)."""
    sorgente = os.path.join(DOWNLOAD, file)
    if not os.path.exists(sorgente):
        scrivi("%s: manca %s nella cartella Download (texture saltate)" % (nome, file))
        return []
    cartella = cartella_saved("Fab", nome, "Texture")
    immagini = []
    with zipfile.ZipFile(sorgente) as archivio:
        for voce in archivio.infolist():
            if voce.is_dir() or pezzo not in voce.filename or not voce.filename.lower().endswith(IMMAGINI):
                continue
            arrivo = os.path.join(cartella, os.path.basename(voce.filename))
            estrai(voce, archivio, arrivo)
            immagini.append(arrivo)
    return immagini


def importa(file_list, destinazione):
    compiti = []
    for file in file_list:
        compito = unreal.AssetImportTask()
        compito.filename = file
        compito.destination_path = destinazione
        compito.automated = True          # niente finestre di domande
        compito.replace_existing = True
        compito.save = True
        compiti.append(compito)
    if compiti:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(compiti)
    fatti = 0
    for compito in compiti:
        fatti += len(compito.get_editor_property("imported_object_paths") or [])
    return fatti


# --- Materiali costruiti dalle texture (per le case) ---------------------------------------------------------------

TIPI = {
    "basecolor": "colore", "basemap": "colore", "albedo": "colore",
    "normal": "normale",
    "occlusionroughnessmetallic": "orm",       # R occlusione, G ruvidità, B metallo
    "metallicroughness": "mr",                  # come il glTF: G ruvidità, B metallo
}


def tipo_di(nome_texture):
    basso = nome_texture.lower()
    for fine, tipo in TIPI.items():
        if basso.endswith(fine):
            return tipo
    return None


def sistema_texture(texture, tipo):
    if tipo == "normale":
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("srgb", False)
    elif tipo in ("orm", "mr"):
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        texture.set_editor_property("srgb", False)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)


def costruisci_materiale(cartella, nome_slot, scelte):
    lib = unreal.MaterialEditingLibrary
    strumenti = unreal.AssetToolsHelpers.get_asset_tools()
    nome = "M_" + nome_slot
    percorso = cartella + "/" + nome
    if unreal.EditorAssetLibrary.does_asset_exist(percorso):
        return unreal.load_asset(percorso)
    materiale = strumenti.create_asset(nome, cartella, unreal.Material, unreal.MaterialFactoryNew())

    def campione(texture, y, tipo_campione):
        nodo = lib.create_material_expression(materiale, unreal.MaterialExpressionTextureSample, -450, y)
        nodo.set_editor_property("texture", texture)
        nodo.set_editor_property("sampler_type", tipo_campione)
        return nodo

    if "colore" in scelte:
        nodo = campione(scelte["colore"], -300, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        lib.connect_material_property(nodo, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    if "normale" in scelte:
        nodo = campione(scelte["normale"], 0, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        lib.connect_material_property(nodo, "RGB", unreal.MaterialProperty.MP_NORMAL)
    maschera = scelte.get("orm") or scelte.get("mr")
    if maschera:
        nodo = campione(maschera, 300, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        lib.connect_material_property(nodo, "G", unreal.MaterialProperty.MP_ROUGHNESS)
        lib.connect_material_property(nodo, "B", unreal.MaterialProperty.MP_METALLIC)
        if "orm" in scelte:
            lib.connect_material_property(nodo, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    lib.recompile_material(materiale)
    unreal.EditorAssetLibrary.save_loaded_asset(materiale)
    return materiale


def materiali_dalle_texture(nome):
    """Per ogni pezzo di ogni modello: cerca le texture che cominciano con il nome del suo materiale e ne fa uno."""
    cartella = "/Game/Fab/" + nome
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    texture = []
    for dato in registro.get_assets_by_path(cartella, recursive=True):
        if str(dato.asset_class_path.asset_name) == "Texture2D":
            oggetto = unreal.load_asset(str(dato.package_name))
            tipo = tipo_di(str(dato.asset_name))
            if tipo:
                sistema_texture(oggetto, tipo)
                texture.append((str(dato.asset_name).lower(), tipo, oggetto))
    fatti, senza = 0, []
    for dato in registro.get_assets_by_path(cartella, recursive=False):
        if str(dato.asset_class_path.asset_name) != "StaticMesh":
            continue
        mesh = unreal.load_asset(str(dato.package_name))
        pezzi = mesh.get_editor_property("static_materials")
        for indice, pezzo in enumerate(pezzi):
            nome_slot = str(pezzo.get_editor_property("material_slot_name"))
            chiave = nome_slot.lower()
            scelte = {}
            for nome_tex, tipo, oggetto in texture:
                if nome_tex.startswith(chiave) and tipo not in scelte:
                    scelte[tipo] = oggetto
            if "colore" not in scelte:
                senza.append(nome_slot)
                continue
            materiale = costruisci_materiale(cartella + "/Materiali", nome_slot, scelte)
            mesh.set_material(indice, materiale)
            fatti += 1
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    scrivi("   materiali costruiti dalle texture: %d" % fatti)
    if senza:
        scrivi("   pezzi senza texture trovate (restano col materiale importato): %s" % ", ".join(sorted(set(senza))))


def descrivi(nome):
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    risorse = registro.get_assets_by_path("/Game/Fab/" + nome, recursive=True)
    conta = {}
    for dato in risorse:
        classe = str(dato.asset_class_path.asset_name)
        conta[classe] = conta.get(classe, 0) + 1
        if classe == "StaticMesh":
            mesh = unreal.load_asset(str(dato.package_name))
            scatola = mesh.get_bounding_box()
            misura = scatola.max - scatola.min
            pezzi = mesh.get_editor_property("static_materials")
            scrivi("   %s: %.0f x %.0f x %.0f cm, %d materiali (%s)" % (
                dato.asset_name, misura.x, misura.y, misura.z, len(pezzi),
                ", ".join(str(p.get_editor_property("material_slot_name")) for p in pezzi[:8])))
    scrivi("   in tutto: " + ", ".join("%d %s" % (n, c) for c, n in sorted(conta.items())))


def principale():
    scrivi("Import da %s" % DOWNLOAD)
    for nome, file, crediti, altro in PACCHETTI:
        destinazione = "/Game/Fab/" + nome
        if unreal.EditorAssetLibrary.does_directory_exist(destinazione) and \
                unreal.EditorAssetLibrary.list_assets(destinazione):
            scrivi("%s: c'è già in Content/Fab (per rifarlo cancella la cartella)" % nome)
            descrivi(nome)
            continue
        modelli, immagini = prepara(nome, file)
        if not modelli:
            continue
        scrivi("%s: importo %d modelli (%s)" % (nome, len(modelli), crediti))
        fatti = importa(modelli, destinazione)
        scrivi("%s: creati %d oggetti" % (nome, fatti))
        if altro.get("materiali"):
            if "texture" in altro:
                immagini = immagini + texture_a_parte(nome, *altro["texture"])
            scrivi("%s: importo %d texture" % (nome, len(immagini)))
            importa(immagini, destinazione + "/Texture")
            materiali_dalle_texture(nome)
        descrivi(nome)
    with open(os.path.join(cartella_saved("Claude"), "importa_fab.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))
    scrivi("Fatto: resoconto in Saved/Claude/importa_fab.txt")


principale()
