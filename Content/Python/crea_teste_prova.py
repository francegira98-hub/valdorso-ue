# Valdorso - Crea e monta nuove teste MetaHuman partendo dai preset del MetaHuman Creator, per provare i cursori
# del volto su visi diversi. Scritto da Claude il 19/10/2026.
#
# Per ogni nome in TESTE: copia il preset in /Game/MH_<Nome> (come MH_Prova) e lo monta (pipeline Cinematic, come
# MH_Prova): il risultato va in /Game/MetaHumans/MH_<Nome>, con il suo Blueprint BP_MH_<Nome>.
# Prima fa una copia di sicurezza di ABP_Face_PostProcess (che ora contiene il nodo Volto Valdorso) e alla fine
# controlla che non sia stato sovrascritto. Ogni testa richiede qualche minuto: l'editor sembra fermo, è normale.

import os
import shutil
import unreal

TESTE = ["Jelena", "Walter"]   # preset disponibili: Ada, Aera, Aoi, Asha, Bo, Bruce, Cameron, Celeste, Dominic, Etta,
                               # Grace, Isaiah, Jelena, Jorge, Kelvin, Lani, Lorenzo, Mateo, Mikel, Omari, Orlando,
                               # Sook-ja, Sunita, Trey, Tuya, Victor, Vivian, Walter, Zuri

CONTENUTO = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
ABP = os.path.join(CONTENUTO, "MetaHumans", "Common", "Face", "ABP_Face_PostProcess.uasset")
COPIA = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude",
                     "copia_sicurezza", "ABP_Face_PostProcess.uasset")


def scrivi(testo):
    unreal.log("[Valdorso] Teste: " + testo)


def errore(testo):
    unreal.log_error("[Valdorso] Teste: " + testo)


def impronta(percorso):
    if not os.path.isfile(percorso):
        return None
    info = os.stat(percorso)
    return (info.st_size, info.st_mtime)


def crea_e_monta(sottosistema, nome):
    sorgente = "/MetaHumanCharacter/Optional/Presets/%s" % nome
    destinazione = "/Game/MH_%s" % nome.replace("-", "")
    if not unreal.EditorAssetLibrary.does_asset_exist(sorgente):
        errore("non trovo il preset " + sorgente)
        return False

    if unreal.EditorAssetLibrary.does_asset_exist(destinazione):
        scrivi(destinazione + " esiste già: lo rimonto")
        personaggio = unreal.EditorAssetLibrary.load_asset(destinazione)
    else:
        personaggio = unreal.EditorAssetLibrary.duplicate_asset(sorgente, destinazione)
        if personaggio is None:
            errore("non riesco a copiare " + sorgente)
            return False
        unreal.EditorAssetLibrary.save_loaded_asset(personaggio)
        scrivi("copiato " + sorgente + " in " + destinazione)

    if not sottosistema.try_add_object_to_edit(personaggio):
        errore("non riesco ad aprire " + destinazione + " per la modifica")
        return False
    try:
        if not sottosistema.can_build_meta_human(personaggio, True):
            errore(destinazione + " non si può montare (guarda il messaggio sopra)")
            return False
        parametri = unreal.MetaHumanCharacterEditorBuildParameters()
        parametri.set_editor_property("pipeline_type", unreal.MetaHumanDefaultPipelineType.CINEMATIC)
        scrivi("monto " + destinazione + " (qualche minuto)...")
        sottosistema.build_meta_human(personaggio, parametri)
        unreal.EditorAssetLibrary.save_loaded_asset(personaggio)
        scrivi("montato " + destinazione)
        return True
    finally:
        sottosistema.remove_object_to_edit(personaggio)


def principale():
    prima = impronta(ABP)
    if prima is None:
        errore("non trovo " + ABP)
        return
    os.makedirs(os.path.dirname(COPIA), exist_ok=True)
    shutil.copy2(ABP, COPIA)
    scrivi("copia di sicurezza di ABP_Face_PostProcess in " + COPIA)

    sottosistema = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    fatte = [nome for nome in TESTE if crea_e_monta(sottosistema, nome)]

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    if impronta(ABP) != prima:
        unreal.log_warning("[Valdorso] Teste: ATTENZIONE, ABP_Face_PostProcess è cambiato durante il montaggio: "
                           "controlla che il nodo Volto Valdorso ci sia ancora (la copia è in " + COPIA + ")")
    else:
        scrivi("ABP_Face_PostProcess intatto")
    scrivi("finito: %d teste montate su %d (%s)" % (len(fatte), len(TESTE), ", ".join(fatte)))


principale()
