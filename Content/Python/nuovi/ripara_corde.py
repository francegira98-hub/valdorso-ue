# Valdorso - Le corde rotte della tenda grande del mercato (08/11/2026, scritto da Claude).
# Il pacchetto del gioco si fermava ("Cook failed"): rope_001 e rope_002 di Fab (Mercato_Tenda1) hanno la geometria
# vuota ("Bad MeshDescription") e Unreal, cuocendo, lo conta come errore. Questo script toglie quelle due mesh dai pezzi
# di BP_Mercato_Tenda1 (le altre corde della tenda restano) e salva il Blueprint. Si può rilanciare.
# Si lancia da solo con "Tools\Ripara e pacchettizza Valdorso.cmd", oppure nell'editor: Strumenti -> Esegui script Python.
# Resoconto in Saved/Claude/ripara_corde.txt.

import os

import unreal

BLUEPRINT = "/Game/Fab/Mercato_Tenda1/BP_Mercato_Tenda1"
ROTTE = ("rope_001", "rope_002")
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Corde: " + testo)


def esegui():
    bp = unreal.load_asset(BLUEPRINT)
    if bp is None:
        scrivi("non trovo %s: niente da fare" % BLUEPRINT)
        return
    oggetti_sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    dati = unreal.SubobjectDataBlueprintFunctionLibrary
    tolte = 0
    for maniglia in oggetti_sub.k2_gather_subobject_data_for_blueprint(bp):
        oggetto = dati.get_object(dati.get_data(maniglia))
        if not isinstance(oggetto, unreal.StaticMeshComponent):
            continue
        mesh = oggetto.get_editor_property("static_mesh")
        if mesh is not None and mesh.get_name() in ROTTE:
            oggetto.set_editor_property("static_mesh", None)
            tolte += 1
            scrivi("tolta la mesh %s dal pezzo %s" % (mesh.get_name(), oggetto.get_name()))
    if tolte:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
        scrivi("FATTO: %d pezzi senza le corde rotte, Blueprint salvato" % tolte)
    else:
        scrivi("FATTO: nessuna corda rotta nei pezzi (forse già sistemato)")


try:
    esegui()
except Exception as e:  # noqa: BLE001
    scrivi("ERRORE: %s" % e)
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "ripara_corde.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe))
