# Valdorso - Porta in Valdorso le animazioni della panca del Game Animation Sample (sedersi, stare seduti, alzarsi).
# Scritto da Claude il 03/11/2026.
#
# VA LANCIATO NEL PROGETTO GameAnimationSample (V:\GameAnimationSample), NON in Valdorso:
#   Strumenti -> Esegui script Python -> scegli questo file (V:\Valdorso\Content\Python\nuovi\porta_panca_da_gasp.py).
#
# Cosa fa, come per le animazioni di Traversal:
#   1. prende le 14 animazioni di Characters/UEFN_Mannequin/Animations/Interactions/Bench (non i montaggi);
#   2. le adatta allo scheletro del nostro manichino (SKM_Manny_Simple di Characters/Mannequins) con il retargeter di
#      Epic RTG_UEFN_to_UE5_Mannequin;
#   3. le mette in /Game/Fab/GameAnimationSample/Panca (nomi senza M_interaction_bench_, per esempio Siede_F_Lfoot);
#   4. copia i file in V:\Valdorso\Content\Fab\GameAnimationSample\Panca, dove Valdorso le trova da solo.
# Si può rilanciare: rifà tutto da capo.

import os
import shutil
import unreal

ORIGINE = "/Game/Characters/UEFN_Mannequin/Animations/Interactions/Bench"
DESTINAZIONE = "/Game/Fab/GameAnimationSample/Panca"
MESH_ORIGINE = "/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin"
MESH_VALDORSO = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
RETARGETER = "/Game/Characters/UE5_Mannequins/Rigs/RTG_UEFN_to_UE5_Mannequin"
CARTELLA_VALDORSO = r"V:\Valdorso\Content\Fab\GameAnimationSample\Panca"
SUFFISSO = "_PerValdorso"

# Il nome nuovo di ogni animazione (più corto e in italiano).
NOMI = {
    "M_interaction_bench_into_F_Lfoot": "Siede_F_Lfoot",
    "M_interaction_bench_into_F_Rfoot": "Siede_F_Rfoot",
    "M_interaction_bench_into_L_090_Lfoot": "Siede_L90_Lfoot",
    "M_interaction_bench_into_L_090_Rfoot": "Siede_L90_Rfoot",
    "M_interaction_bench_into_R_090_Lfoot": "Siede_R90_Lfoot",
    "M_interaction_bench_into_R_090_Rfoot": "Siede_R90_Rfoot",
    "M_interaction_bench_into_BL_LFoot": "Siede_BL_Lfoot",
    "M_interaction_bench_into_BL_RFoot": "Siede_BL_Rfoot",
    "M_interaction_bench_into_BR_LFoot": "Siede_BR_Lfoot",
    "M_interaction_bench_into_BR_RFoot": "Siede_BR_Rfoot",
    "M_interaction_bench_idle_loop": "Seduto",
    "M_interaction_bench_out_to_stand": "Alza_InPiedi",
    "M_interaction_bench_out_to_walk": "Alza_Cammina",
    "M_interaction_bench_out_to_run": "Alza_Corre",
}


def scrivi(testo):
    unreal.log("[Valdorso] Panca: " + testo)


def principale():
    libreria = unreal.EditorAssetLibrary
    mesh_origine = libreria.load_asset(MESH_ORIGINE)
    mesh_valdorso = libreria.load_asset(MESH_VALDORSO)
    retargeter = libreria.load_asset(RETARGETER)
    if not (mesh_origine and mesh_valdorso and retargeter):
        unreal.log_error("[Valdorso] Panca: manca qualcosa (mesh UEFN, SKM_Manny_Simple o RTG_UEFN_to_UE5_Mannequin). Sei nel progetto GameAnimationSample?")
        return

    # Le animazioni da portare (solo le sequenze, non i montaggi).
    dati = []
    for percorso in libreria.list_assets(ORIGINE, recursive=False, include_folder=False):
        nome = percorso.split("/")[-1].split(".")[0]
        if nome in NOMI:
            dati.append(libreria.find_asset_data(percorso))
    scrivi("trovate %d animazioni su %d" % (len(dati), len(NOMI)))
    if not dati:
        return

    # Rifà da capo: via la cartella di prima.
    if libreria.does_directory_exist(DESTINAZIONE):
        libreria.delete_directory(DESTINAZIONE)

    # 1-2. Adatta allo scheletro di Valdorso (le copie nascono accanto alle originali, con il suffisso).
    # (03/11) In UE 5.8 la funzione vuole anche la cartella di arrivo (target_path): le copie nascono già lì.
    # Si provano le forme possibili, dalla più semplice; se nessuna va, si scrive come la vuole il motore.
    funzione = unreal.IKRetargetBatchOperation.duplicate_and_retarget
    tentativi = [
        (dati, mesh_origine, mesh_valdorso, retargeter, "", "", "", SUFFISSO, DESTINAZIONE),
        (dati, mesh_origine, mesh_valdorso, retargeter, "", "", "", SUFFISSO, DESTINAZIONE, False),
        (dati, mesh_origine, mesh_valdorso, retargeter, "", "", "", SUFFISSO, DESTINAZIONE, False, True),
    ]
    nuove = None
    ultimo = None
    for argomenti in tentativi:
        try:
            nuove = funzione(*argomenti)
            break
        except Exception as problema:
            ultimo = problema
    if nuove is None:
        unreal.log_error("[Valdorso] Panca: il retarget automatico non parte (%s)." % ultimo)
        unreal.log_error("[Valdorso] Panca: come la vuole il motore: %s" % (funzione.__doc__ or "").replace("\n", " | ")[:900])
        unreal.log_error("[Valdorso] Panca: manda queste righe rosse a Claude.")
        return
    scrivi("adattate %d animazioni" % len(nuove))

    # 3. Nella cartella di Valdorso, con i nomi nuovi.
    spostate = 0
    for dato in nuove:
        vecchio = str(dato.package_name)
        base = vecchio.split("/")[-1]
        originale = base[:-len(SUFFISSO)] if base.endswith(SUFFISSO) else base
        nuovo_nome = NOMI.get(originale)
        if not nuovo_nome:
            continue
        if libreria.rename_asset(vecchio, "%s/%s" % (DESTINAZIONE, nuovo_nome)):
            spostate += 1
    libreria.save_directory(DESTINAZIONE, only_if_is_dirty=False, recursive=True)
    scrivi("spostate e salvate %d animazioni in %s" % (spostate, DESTINAZIONE))

    # 4. Copia dei file in Valdorso.
    sorgente = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()),
                            "Fab", "GameAnimationSample", "Panca")
    os.makedirs(CARTELLA_VALDORSO, exist_ok=True)
    copiati = 0
    for nome in os.listdir(sorgente):
        if nome.endswith(".uasset"):
            shutil.copy2(os.path.join(sorgente, nome), os.path.join(CARTELLA_VALDORSO, nome))
            copiati += 1
    scrivi("FINITO: copiati %d file in %s. Ora chiudi questo progetto e apri Valdorso." % (copiati, CARTELLA_VALDORSO))


principale()
