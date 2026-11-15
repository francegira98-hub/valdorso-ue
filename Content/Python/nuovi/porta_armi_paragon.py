# Valdorso - Porta nel nostro colono i colpi degli eroi di Paragon (Grux per asce e mazze; Aurora per i pugnali, quando
# c'e') e i colpi di lancia della Free Animation Library (per le aste),
# e (14/11) l'animazione per aprire una porta, per i PNG che camminano (in /Game/Fab/Gesti). Scritto da Claude il 14/11/2026, per finire le "Armi vere" (v0.2.3): come porta_armi_greystone.py, ma il
# "traduttore" tra gli scheletri (IK Rig e Retargeter) prova a farlo da solo, senza il passo a mano.
#
# Si lancia dal menu Valdorso -> Strumenti -> "Armi: porta le animazioni di Grux e Aurora" (si puo' rilanciare: rifa'
# tutto da capo). Cosa fa, per ogni eroe che trova nel progetto:
#   1. il traduttore: in /Game/Fab/Armi/Riadatta_<Eroe> un IK Rig dell'eroe (fatto da solo dal motore) e un Retargeter
#      verso il nostro manichino (/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple). Per il manichino usa l'IK Rig
#      gia' esportato per Greystone, se c'e' (funziona gia'), altrimenti ne fa uno nuovo.
#   2. le animazioni scelte qui sotto, adattate al manichino, in /Game/Fab/Armi/<Eroe> con nomi italiani, ferme sul posto.
#   3. in Saved/Claude/armi_paragon.txt quanto dura ognuna (per il C++ dei colpi) e, se qualcosa non va, perche'.
# Il C++ (ValdorsoArmi.cpp) usa Grux per le asce e le mazze se le trova, altrimenti Greystone come prima.
# Se il traduttore automatico non riesce, lo script lo scrive: allora si fa il passo a mano come per Greystone
# (tasto destro su Idle -> Riadatta animazioni -> Esporta risorse di riadattamento in /Game/Fab/Armi/Riadatta_<Eroe>).

import os

import unreal

MANNY = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
RIADATTA_GREYSTONE = ["/Game/Fab/ParagonGreystone/Riadatta", "/Game/ParagonGreystone/Riadatta"]
SUFFISSO = "_PerValdorso"

EROI = {
    "Grux": {
        "radice": "/Game/ParagonGrux/Characters/Heroes/Grux",
        "mesh": "Meshes/Grux",
        "animazioni": "Animations",
        "nomi": {
            "Idle": "Grux_Guardia",
            "PrimaryAttack_RA": "Grux_Destro_A",
            "PrimaryAttack_RB": "Grux_Destro_B",
            "PrimaryAttack_RA_Fast": "Grux_Destro_Rapido",
            "PrimaryAttack_LA": "Grux_Sinistro_A",
            "PrimaryAttack_LB": "Grux_Sinistro_B",
            "PrimaryAttack_FourStrikes": "Grux_Quattro_Colpi",
            "Attack_Melee_Air": "Grux_Salto",
            "HitReact_Front": "Grux_Colpito",
            "Stun_Idle": "Grux_Stordito",
            "Emote_Master_BattleCry_T3": "Grux_Grido",
            "Ultimate_Roar": "Grux_Ruggito",
        },
    },
    # (14/11) le aste: i colpi di lancia della Free Animation Library (gia' nel progetto, fatti per un manichino uguale al
    # nostro ma con il suo scheletro: lo stesso traduttore automatico, ossa uguali, quindi va liscio)
    "Lancia": {
        "radice": "/Game/FreeAnimationLibrary",
        "mesh": "Demo/Characters/Mannequins/Meshes/SKM_Manny_Simple",
        "animazioni": "Animations/Update_2/Spear",
        "nomi": {
            "AS_Spear_Idle": "Lancia_Guardia",
            "AS_Spear_Attack_01": "Lancia_Colpo_1",
            "AS_Spear_Attack_02": "Lancia_Colpo_2",
            "AS_Spear_Attack_03": "Lancia_Colpo_3",
            "AS_Spear_Attack_04": "Lancia_Colpo_4",
            "AS_Spear_Attack_05": "Lancia_Colpo_5",
            "AS_Spear_Attack_06": "Lancia_Colpo_6",
        },
    },
    # (14/11, "PNG che camminano") aprire una porta, dalla stessa Free Animation Library: la usano i PNG sulla soglia
    "Gesti": {
        "radice": "/Game/FreeAnimationLibrary",
        "mesh": "Demo/Characters/Mannequins/Meshes/SKM_Manny_Simple",
        "animazioni": "Animations/Update_2/Door",
        "destinazione": "/Game/Fab/Gesti",
        "nomi": {
            "AS_OpenDoor": "Apre_Porta",
        },
    },
    # (15/11) Aurora: i nomi veri, letti dall'inventario del 15/11
    "Aurora": {
        "radice": "/Game/ParagonAurora/Characters/Heroes/Aurora",
        "mesh": "Meshes/Aurora",
        "animazioni": "Animations",
        "nomi": {
            "Idle": "Aurora_Guardia",
            "Primary_Attack_A": "Aurora_Rapido_A",
            "Primary_Attack_B": "Aurora_Rapido_B",
            "Primary_Attack_C": "Aurora_Rapido_C",
            "Primary_Attack_Fast_A": "Aurora_Lampo_A",
            "Primary_Attack_Fast_B": "Aurora_Lampo_B",
            "Primary_Attack_Fast_C": "Aurora_Lampo_C",
        },
    },
}

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
registro = unreal.AssetRegistryHelpers.get_asset_registry()
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Paragon: " + testo)


def errore(testo):
    righe.append("ERRORE: " + testo)
    unreal.log_error("[Valdorso] Paragon: " + testo)


def classe(dato):
    return str(dato.asset_class_path.asset_name)


def cerca(cartelle, nome_classe, contiene=""):
    for cartella in cartelle:
        if not libreria.does_directory_exist(cartella):
            continue
        for dato in registro.get_assets_by_path(cartella, recursive=True):
            if classe(dato) == nome_classe and contiene.lower() in str(dato.asset_name).lower():
                return libreria.load_asset(str(dato.package_name))
    return None


def prova(*modi):
    """Prova i modi in ordine (le funzioni cambiano nome tra le versioni di Unreal): il primo che va."""
    ultimo = None
    for modo in modi:
        try:
            return True, modo()
        except Exception as problema:  # noqa: BLE001
            ultimo = problema
    return False, ultimo


def ik_rig(nome, mesh, cartella):
    percorso = "%s/%s" % (cartella, nome)
    if libreria.does_asset_exist(percorso):
        libreria.delete_asset(percorso)
    rig = strumenti.create_asset(nome, cartella, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    if not rig:
        errore("non riesco a creare %s" % percorso)
        return None
    controllo = unreal.IKRigController.get_controller(rig)
    ok, _ = prova(lambda: controllo.set_skeletal_mesh(mesh))
    if not ok:
        errore("%s: non riesco a dargli il modello" % nome)
        return None
    ok, perche = prova(lambda: controllo.apply_auto_generated_retarget_definition())
    if not ok:
        errore("%s: le catene automatiche non partono (%s)" % (nome, perche))
        return None
    prova(lambda: controllo.apply_auto_fbik())
    libreria.save_loaded_asset(rig)
    scrivi("IK Rig %s fatto" % percorso)
    return rig


def retargeter(nome, sorgente, destinazione, mesh_sorgente, mesh_destinazione, cartella):
    percorso = "%s/%s" % (cartella, nome)
    if libreria.does_asset_exist(percorso):
        libreria.delete_asset(percorso)
    rtg = strumenti.create_asset(nome, cartella, unreal.IKRetargeter, unreal.IKRetargetFactory())
    if not rtg:
        errore("non riesco a creare %s" % percorso)
        return None
    c = unreal.IKRetargeterController.get_controller(rtg)
    S = getattr(unreal.RetargetSourceOrTarget, "SOURCE", None)
    T = getattr(unreal.RetargetSourceOrTarget, "TARGET", None)
    ok, perche = prova(lambda: (c.set_ik_rig(S, sorgente), c.set_ik_rig(T, destinazione)),
                       lambda: (c.set_source_ik_rig(sorgente), c.set_target_ik_rig(destinazione)))
    if not ok:
        errore("%s: non riesco a dargli i due IK Rig (%s)" % (nome, perche))
        return None
    prova(lambda: (c.set_preview_mesh(S, mesh_sorgente), c.set_preview_mesh(T, mesh_destinazione)))
    prova(lambda: c.add_default_ops())
    prova(lambda: c.auto_map_chains(unreal.AutoMapChainType.FUZZY, True), lambda: c.auto_map_chains(unreal.AutoMapChainType.FUZZY))
    prova(lambda: c.auto_align_all_bones(S), lambda: None)
    libreria.save_loaded_asset(rtg)
    scrivi("Retargeter %s fatto" % percorso)
    return rtg


def adatta(dati, mesh_origine, mesh_valdorso, rtg, destinazione):
    funzione = unreal.IKRetargetBatchOperation.duplicate_and_retarget
    ok, risultato = prova(
        lambda: funzione(dati, mesh_origine, mesh_valdorso, rtg, "", "", "", SUFFISSO, destinazione),
        lambda: funzione(dati, mesh_origine, mesh_valdorso, rtg, "", "", "", SUFFISSO, destinazione, False),
        lambda: funzione(dati, mesh_origine, mesh_valdorso, rtg, "", "", "", SUFFISSO, destinazione, False, True))
    if not ok:
        errore("il riadattamento non parte (%s)" % risultato)
        return None
    return risultato


def porta(eroe, dati_eroe, manny):
    radice = dati_eroe["radice"]
    mesh = libreria.load_asset("%s/%s" % (radice, dati_eroe["mesh"]))
    if not mesh:
        scrivi("%s: non e' nel progetto (manca %s/%s): lo salto" % (eroe, radice, dati_eroe["mesh"]))
        return
    cartella_rig = "/Game/Fab/Armi/Riadatta_%s" % eroe
    destinazione = dati_eroe.get("destinazione", "/Game/Fab/Armi/%s" % eroe)
    # il traduttore: prima uno gia' esportato a mano in Riadatta_<Eroe>, poi quello automatico
    rtg = cerca([cartella_rig + "/Amano"], "IKRetargeter")
    if rtg:
        scrivi("%s: uso il retargeter fatto a mano in %s/Amano" % (eroe, cartella_rig))
    else:
        # rilanciato: prima via il vecchio retargeter (usa gli IK Rig), poi si rifanno tutti e tre
        vecchio_rtg = "%s/RTG_%s_Manny" % (cartella_rig, eroe)
        if libreria.does_asset_exist(vecchio_rtg):
            libreria.delete_asset(vecchio_rtg)
        rig_manny = cerca(RIADATTA_GREYSTONE, "IKRigDefinition", "manny") or ik_rig("IK_Manny_Valdorso", manny, cartella_rig)
        rig_eroe = ik_rig("IK_%s_Valdorso" % eroe, mesh, cartella_rig)
        rtg = retargeter("RTG_%s_Manny" % eroe, rig_eroe, rig_manny, mesh, manny, cartella_rig) if (rig_eroe and rig_manny) else None
    if not rtg:
        errore("%s: niente traduttore. Passo a mano: tasto destro su un'animazione di %s -> Riadatta animazioni -> "
               "Sorgente %s, Destinazione SKM_Manny_Simple -> Esporta risorse di riadattamento in %s/Amano; poi rilancia."
               % (eroe, eroe, eroe, cartella_rig))
        return
    origine = "%s/%s" % (radice, dati_eroe["animazioni"])
    dati, per_nome = [], {}
    for sorgente, nome in dati_eroe["nomi"].items():
        percorso = "%s/%s" % (origine, sorgente)
        if libreria.does_asset_exist(percorso):
            dati.append(libreria.find_asset_data(percorso))
            per_nome[sorgente] = nome
        else:
            scrivi("%s: manca %s" % (eroe, sorgente))
    if not dati:
        presenti = [str(d.asset_name) for d in registro.get_assets_by_path(origine, recursive=False) if classe(d) == "AnimSequence"]
        errore("%s: nessuna delle animazioni scelte. Quelle che ci sono: %s" % (eroe, ", ".join(sorted(presenti))[:1500]))
        return
    if libreria.does_directory_exist(destinazione):
        libreria.delete_directory(destinazione)
    nuove = adatta(dati, mesh, manny, rtg, destinazione)
    if nuove is None:
        return
    fatte = 0
    for dato in nuove:
        vecchio = str(dato.package_name)
        base = vecchio.split("/")[-1]
        originale = base[:-len(SUFFISSO)] if base.endswith(SUFFISSO) else base
        nome = per_nome.get(originale)
        if not nome:
            continue
        nuovo = "%s/%s" % (destinazione, nome)
        if not libreria.rename_asset(vecchio, nuovo):
            errore("non riesco a rinominare %s in %s" % (vecchio, nuovo))
            continue
        animazione = libreria.load_asset(nuovo)
        if isinstance(animazione, unreal.AnimSequence):
            scrivi("%s: %.2f s" % (nome, animazione.get_play_length()))
            animazione.set_editor_property("enable_root_motion", False)
            animazione.set_editor_property("force_root_lock", True)
            libreria.save_loaded_asset(animazione)
        fatte += 1
    libreria.save_directory(destinazione, only_if_is_dirty=False, recursive=True)
    scrivi("%s FATTO: %d animazioni in %s." % (eroe, fatte, destinazione))


def principale():
    manny = libreria.load_asset(MANNY)
    if not manny:
        errore("manca il nostro manichino %s" % MANNY)
        return
    for eroe, dati_eroe in EROI.items():
        try:
            porta(eroe, dati_eroe, manny)
        except Exception as problema:  # noqa: BLE001
            errore("%s: si e' fermato (%s)" % (eroe, problema))
    scrivi("Ora in gioco: un'ascia o una mazza in mano e colpisci (o Valdorso.Fotografo Accetta Mazza, e Claude guarda le foto).")


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "armi_paragon.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
