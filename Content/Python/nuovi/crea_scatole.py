# Valdorso - Il gioco in scatole: il .pak a pezzi (v0.2.1 "Dopo la prova"). Scritto da Claude il 12/11/2026.
# Deciso da Fra il 12/11: il gioco non è più un solo scatolone (cambiava un file e si riscaricavano 1,9 GB), ma sei
# scatole divise per quanto spesso cambiano. Il launcher 1.3 scarica solo i file cambiati (prepara_aggiornamento.py):
# con le scatole separate, una patch normale non deve più riscaricare i pacchetti di Fab o la musica.
#
#   scatola 0  la base: il motore, il codice, gli shader, tutto quello che non sta in un'altra scatola
#   scatola 1  i pacchetti comprati o scaricati (Fab, Kite Demo, animazioni, MetaHuman...): non cambiano quasi mai
#   scatola 2  la valle: la mappa L_Valle con il terreno e tutto quello che ci sta sopra
#   scatola 3  i suoni e la musica
#   scatola 4  le nostre cose: i pezzi del villaggio, l'interfaccia, i volti
#   scatola 5  (12/11, dopo il primo pacchetto) il menu: 300 MB di texture della scena dell'Orso, che non cambiano mai
#
# Come: in ogni cartella una "etichetta" (PrimaryAssetLabel) che dice "le cose di questa cartella vanno nella scatola N"
# (Etichetta assets nella mia cartella). L'etichetta non fa cucinare niente in più: finisce nel gioco solo quello che il
# gioco usa davvero (regola di cottura Unknown). Le scatole le fa il pacchetto perché Config/DefaultGame.ini ha
# bGenerateChunks=True.
#
# Si lancia dal menu Valdorso -> Pacchetto: prepara le scatole (o Strumenti -> Esegui script Python). Si può rilanciare:
# aggiorna le etichette che ci sono già. Va rilanciato su un PC nuovo o dopo aver importato un pacchetto nuovo in una
# cartella che non c'era (alcune cartelle, come Fab, non stanno su GitHub e le loro etichette nemmeno).
# Resoconto in Saved/Claude/scatole.txt.

import os

import unreal

# (cartella, scatola, priorità, che cos'è). Se una cosa sta in due cartelle con l'etichetta (Valle e Valle/Suoni),
# vince la priorità più alta.
SCATOLE = [
    ("/Game/Fab", 1, 10, "pacchetti"),
    ("/Game/KiteDemo", 1, 10, "pacchetti"),
    ("/Game/FreeAnimationLibrary", 1, 10, "pacchetti"),
    ("/Game/SampleAnimationPack", 1, 10, "pacchetti"),
    ("/Game/MetaHumans", 1, 10, "pacchetti"),
    ("/Game/Mixamo", 1, 10, "pacchetti"),
    ("/Game/LevelPrototyping", 1, 10, "pacchetti"),
    ("/Game/Characters", 1, 10, "pacchetti"),
    ("/Game/Prove", 2, 10, "valle"),
    ("/Game/Audio", 3, 20, "suoni"),
    ("/Game/Valle/Suoni", 3, 20, "suoni"),
    ("/Game/Valle", 4, 10, "nostre cose"),
    ("/Game/UI", 4, 10, "nostre cose"),
    ("/Game/Menu", 5, 10, "il menu"),
    ("/Game/Volto", 4, 10, "nostre cose"),
    ("/Game/Peli", 4, 10, "nostre cose"),
    ("/Game/Legami", 4, 10, "nostre cose"),
    ("/Game/Splash", 4, 10, "nostre cose"),
]
PREFISSO = "Scatola_"   # ogni etichetta ha un nome suo (Scatola_Fab, Scatola_Valle_Suoni): due uguali si darebbero fastidio

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Scatole: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Scatole: " + testo)


def nome_di(cartella):
    return PREFISSO + cartella[len("/Game/"):].replace("/", "_")


def etichetta(cartella):
    """L'etichetta della cartella: quella che c'è già, o una nuova."""
    nome = nome_di(cartella)
    percorso = "%s/%s" % (cartella, nome)
    if libreria.does_asset_exist(percorso):
        return libreria.load_asset(percorso)
    fabbrica = unreal.DataAssetFactory()
    fabbrica.set_editor_property("data_asset_class", unreal.PrimaryAssetLabel)
    return strumenti.create_asset(nome, cartella, unreal.PrimaryAssetLabel, fabbrica)


def principale():
    fatte = 0
    for cartella, scatola, priorita, cosa in SCATOLE:
        if not libreria.does_directory_exist(cartella):
            scrivi("%s: la cartella non c'è, salto" % cartella)
            continue
        e = etichetta(cartella)
        if e is None:
            avviso("%s: non riesco a creare l'etichetta" % cartella)
            continue
        regole = e.get_editor_property("rules")
        regole.set_editor_property("priority", priorita)
        regole.set_editor_property("chunk_id", scatola)
        regole.set_editor_property("apply_recursively", False)
        regole.set_editor_property("cook_rule", unreal.PrimaryAssetCookRule.UNKNOWN)
        e.set_editor_property("rules", regole)
        e.set_editor_property("label_assets_in_my_directory", True)
        libreria.save_loaded_asset(e, False)
        fatte += 1
        scrivi("%s -> scatola %d (%s), priorità %d" % (cartella, scatola, cosa, priorita))
    # le etichette le deve trovare l'Asset Manager (PrimaryAssetTypesToScan, di serie: PrimaryAssetLabel in /Game)
    try:
        trovate = unreal.SystemLibrary.get_primary_asset_id_list(unreal.PrimaryAssetType("PrimaryAssetLabel"))
        def testo(i):
            try:
                return unreal.SystemLibrary.conv_primary_asset_id_to_string(i)
            except Exception:  # noqa: BLE001
                return str(i)
        nostre = [n for n in (testo(x) for x in trovate) if PREFISSO in n]
        scrivi("l'Asset Manager vede %d etichette nostre su %d" % (len(nostre), fatte))
        if len(nostre) < fatte:
            avviso("l'Asset Manager non le vede tutte: potrebbe servire riaprire l'editor (o va controllato PrimaryAssetTypesToScan)")
    except Exception as ex:  # noqa: BLE001
        avviso("non riesco a chiedere all'Asset Manager: %s" % ex)
    scrivi("FATTO: %d etichette. Ora: chiudere Unreal e lanciare Tools\\Pacchettizza Valdorso.cmd" % fatte)
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "scatole.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe))


principale()
