# Valdorso - Porta nel nostro colono le animazioni di spada di Paragon: Greystone (Fab, gratis per i progetti Unreal).
# Scritto da Claude il 12/11/2026, per la v0.2.2 "L'armaiolo" (oggi le armi usano ancora il gesto del pugno).
#
# Prima, una volta sola, a mano (2 minuti): il "traduttore" tra lo scheletro di Greystone e il nostro (il retargeter):
#   Cassetto dei contenuti -> ParagonGreystone/Characters/Heroes/Greystone/Animations -> tasto destro su "Idle" ->
#   Riadatta animazioni (Retarget Animations) -> Sorgente: Greystone, Destinazione: SKM_Manny_Simple, con il retargeter
#   generato da solo -> "Esporta risorse di riadattamento" (Export Retarget Assets) nella cartella
#   /Game/ParagonGreystone/Riadatta. Poi si chiude la finestra senza riadattare niente: lo fa questo script.
#
# Poi: Valdorso -> Strumenti -> Esegui script Python -> questo file (si può rilanciare: rifà tutto da capo).
# Cosa fa: prende le animazioni scelte qui sotto, le adatta al nostro manichino (/Game/Characters/Mannequins/Meshes/
# SKM_Manny_Simple), le mette in /Game/Fab/Armi/Spada con nomi italiani, ferme sul posto (il colono lo muove il codice:
# l'animazione è solo aspetto), e scrive in Saved/Claude/armi.txt quanto dura ognuna (per il codice dei colpi).
# Le usa il C++ della v0.2.2 (dopo la prova). Content/Fab e Content/ParagonGreystone non vanno su GitHub.

import os

import unreal

ORIGINE = "/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations"
MESH_ORIGINE = "/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone"
MESH_VALDORSO = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
# (12/11) Fra l'ha esportato in /Game/Fab/ParagonGreystone/Riadatta: va bene anche meglio (Fab non va su GitHub)
CARTELLE_RIADATTA = ["/Game/Fab/ParagonGreystone/Riadatta", "/Game/ParagonGreystone/Riadatta"]
DESTINAZIONE = "/Game/Fab/Armi/Spada"
SUFFISSO = "_PerValdorso"

# animazione di Greystone -> nome in Valdorso
NOMI = {
    "Idle": "Spada_Guardia",
    "Attack_PrimaryA": "Spada_Combo_1",
    "Attack_PrimaryB": "Spada_Combo_2",
    "Attack_PrimaryC": "Spada_Combo_3",
    "Attack_A_Fast": "Spada_Rapido",
    "Attack_B_Med": "Spada_Medio",
    "Attack_C_Slow": "Spada_Pesante",
    "Attack_C_Slow_Recovery": "Spada_Pesante_Ritorno",
    "Attack_D_Slow": "Spada_Dall_Alto",
    "Jump_Melee": "Spada_Salto",
    "Deflect_InPlace": "Spada_Parata",
    "HitReact_Front": "Colpito_Davanti",
    "HitReact_Back": "Colpito_Dietro",
    "HitReact_Left": "Colpito_Sinistra",
    "HitReact_Right": "Colpito_Destra",
    "Stun_Start": "Stordito_Inizio",
    "Stun_Idle": "Stordito",
    "Death": "Morte_Spada",
    "Emote_Taunt_SwordPoint_T1": "Gesto_Punta_Spada",
    "Emote_Master_SwordPlay_T3": "Gesto_Mulinello",
}

libreria = unreal.EditorAssetLibrary
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Armi: " + testo)


def errore(testo):
    righe.append("ERRORE: " + testo)
    unreal.log_error("[Valdorso] Armi: " + testo)


def trova_retargeter():
    """Il retargeter esportato a mano nella cartella Riadatta (il nome lo sceglie Unreal)."""
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    for cartella in CARTELLE_RIADATTA:
        for dato in registro.get_assets_by_path(cartella, recursive=True):
            if str(dato.asset_class_path.asset_name) == "IKRetargeter":
                return libreria.load_asset(str(dato.package_name)), str(dato.package_name)
    return None, ""


def adatta(dati, mesh_origine, mesh_valdorso, retargeter):
    funzione = unreal.IKRetargetBatchOperation.duplicate_and_retarget
    tentativi = [
        (dati, mesh_origine, mesh_valdorso, retargeter, "", "", "", SUFFISSO, DESTINAZIONE),
        (dati, mesh_origine, mesh_valdorso, retargeter, "", "", "", SUFFISSO, DESTINAZIONE, False),
        (dati, mesh_origine, mesh_valdorso, retargeter, "", "", "", SUFFISSO, DESTINAZIONE, False, True),
    ]
    ultimo = None
    for argomenti in tentativi:
        try:
            return funzione(*argomenti)
        except Exception as problema:  # noqa: BLE001
            ultimo = problema
    errore("il retarget non parte (%s). Come lo vuole il motore: %s" % (
        ultimo, (funzione.__doc__ or "").replace("\n", " | ")[:900]))
    return None


def principale():
    mesh_origine = libreria.load_asset(MESH_ORIGINE)
    mesh_valdorso = libreria.load_asset(MESH_VALDORSO)
    retargeter, dove = trova_retargeter()
    if not (mesh_origine and mesh_valdorso):
        errore("manca un manichino: Greystone %s, il nostro %s. Paragon: Greystone è nel progetto?" % (
            bool(mesh_origine), bool(mesh_valdorso)))
        return
    if not retargeter:
        errore("non trovo il retargeter in %s: prima il passo a mano (vedi in cima a questo file)" % " o ".join(CARTELLE_RIADATTA))
        return
    scrivi("retargeter: %s" % dove)

    dati, per_nome = [], {}
    for sorgente, nome in NOMI.items():
        percorso = "%s/%s" % (ORIGINE, sorgente)
        if not libreria.does_asset_exist(percorso):
            errore("manca %s" % percorso)
            continue
        dati.append(libreria.find_asset_data(percorso))
        per_nome[sorgente] = nome
    scrivi("trovate %d animazioni su %d" % (len(dati), len(NOMI)))
    if not dati:
        return

    if libreria.does_directory_exist(DESTINAZIONE):
        libreria.delete_directory(DESTINAZIONE)
    nuove = adatta(dati, mesh_origine, mesh_valdorso, retargeter)
    if nuove is None:
        return
    scrivi("adattate %d animazioni" % len(nuove))

    fatte = 0
    for dato in nuove:
        vecchio = str(dato.package_name)
        base = vecchio.split("/")[-1]
        originale = base[:-len(SUFFISSO)] if base.endswith(SUFFISSO) else base
        nome = per_nome.get(originale)
        if not nome:
            continue
        nuovo = "%s/%s" % (DESTINAZIONE, nome)
        if not libreria.rename_asset(vecchio, nuovo):
            errore("non riesco a rinominare %s in %s" % (vecchio, nuovo))
            continue
        animazione = libreria.load_asset(nuovo)
        if isinstance(animazione, unreal.AnimSequence):
            durata = animazione.get_play_length()
            scrivi("%s: %.2f s (root motion %s)" % (nome, durata, animazione.get_editor_property("enable_root_motion")))
            animazione.set_editor_property("enable_root_motion", False)
            animazione.set_editor_property("force_root_lock", True)
            libreria.save_loaded_asset(animazione)
        fatte += 1
    libreria.save_directory(DESTINAZIONE, only_if_is_dirty=False, recursive=True)
    scrivi("FATTO: %d animazioni in %s. Aprine una (doppio clic) e guarda il colono con la spada." % (fatte, DESTINAZIONE))


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "armi.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
