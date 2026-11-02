# Valdorso - Crea (o aggiorna) la tabella dei cursori del volto DT_VoltoValdorso in /Game/Volto
# leggendo Plugins/VoltoValdorso/Dati/VoltoValdorso.csv. Scritto da Claude il 17/10/2026.
# Si lancia da Strumenti -> Esegui script Python. Si può rilanciare ogni volta che il CSV cambia.

import os
import unreal

CARTELLA = "/Game/Volto"
NOME = "DT_VoltoValdorso"
CSV = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_plugins_dir()),
                   "VoltoValdorso", "Dati", "VoltoValdorso.csv")


def scrivi(testo):
    unreal.log("[Valdorso] Volto: " + testo)


def principale():
    if not os.path.isfile(CSV):
        unreal.log_error("[Valdorso] Volto: non trovo " + CSV)
        return

    riga = unreal.load_object(None, "/Script/VoltoValdorso.VoltoValdorsoRiga")
    if riga is None:
        unreal.log_error("[Valdorso] Volto: il plugin VoltoValdorso non è caricato (manca la struttura VoltoValdorsoRiga).")
        return

    percorso = CARTELLA + "/" + NOME
    tabella = unreal.EditorAssetLibrary.load_asset(percorso) if unreal.EditorAssetLibrary.does_asset_exist(percorso) else None
    if tabella is None:
        fabbrica = unreal.DataTableFactory()
        fabbrica.set_editor_property("struct", riga)
        tabella = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NOME, CARTELLA, unreal.DataTable, fabbrica)
        if tabella is None:
            unreal.log_error("[Valdorso] Volto: non riesco a creare " + percorso)
            return
        scrivi("creata " + percorso)
    elif tabella.get_editor_property("row_struct") != riga:
        unreal.log_error("[Valdorso] Volto: " + percorso + " esiste ma non è di tipo VoltoValdorsoRiga: cancellala e rilancia lo script.")
        return

    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(tabella, CSV):
        unreal.log_error("[Valdorso] Volto: errore leggendo il CSV (guarda le righe sopra nel Registro output).")
        return

    unreal.EditorAssetLibrary.save_loaded_asset(tabella)
    righe = unreal.DataTableFunctionLibrary.get_data_table_row_names(tabella)
    scrivi("tabella pronta: %d righe da %s" % (len(righe), CSV))


principale()
