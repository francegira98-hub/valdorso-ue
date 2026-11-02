# Valdorso - Leggi i punti "attach" dal Game Animation Sample (versione 3)
# Percorso: V:\Valdorso\Content\Python\leggi_attach_gasp_v3.py
#
# Si lancia NEL PROGETTO GameAnimationSample, non in Valdorso.
# Per ogni rettangolo MotionWarping dei 6 montaggi originali di Epic legge
# dove sta l'osso "attach" alla fine del rettangolo, e scrive i numeri in un
# file per Valdorso (V:\Valdorso\Saved\Claude\punti_attach.json, fuori da GitHub).
# Nel Registro output scrive solo un riassunto: i numeri li usera' il secondo
# strumento dentro Valdorso.
#
# Versione 3: la sequenza dentro il montaggio si trova seguendo i collegamenti
# del montaggio (le sue "dipendenze"), non per nome.
# Versione 2: in Unreal 5.8 la funzione del Motion Warping vuole un AnimInstance,
# che in uno script dell'editor non c'e'. Allora si legge l'animazione di base
# (la sequenza dentro il montaggio) osso per osso, dalla radice fino ad "attach",
# e si sommano i pezzi: e' lo stesso conto che fa Unreal.

import json
import os
import unreal

MONTAGGI = [
    "AM_M_Neutral_Traversal_Vault_1_0_stand_F_Lfoot",
    "AM_M_Neutral_Traversal_Vault_1_0_walk_F_Rfoot",
    "AM_M_Neutral_Traversal_Vault_1_0_run_F_Lfoot",
    "AM_M_Neutral_Traversal_Mantle_1_0_stand_F_Lfoot",
    "AM_M_Neutral_Traversal_Mantle_1_0_walk_F_Lfoot",
    "AM_M_Neutral_Traversal_Mantle_1_0_run_F_Lfoot",
]
FILE_USCITA = r"V:\Valdorso\Saved\Claude\punti_attach.json"
LIB = unreal.AnimationLibrary


def scrivi(testo):
    unreal.log("[Valdorso] " + testo)


def cerca_asset(classe, nomi):
    """Cerca per nome gli asset di una classe, saltando le copie in /Game/Fab."""
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    filtro = unreal.ARFilter(
        class_paths=[unreal.TopLevelAssetPath("/Script/Engine", classe)],
        package_paths=["/Game"],
        recursive_paths=True,
    )
    trovati = {}
    for dati in registro.get_assets(filtro):
        nome = str(dati.asset_name)
        percorso = str(dati.package_name)
        if nome in nomi and not percorso.startswith("/Game/Fab/"):
            trovati[nome] = percorso
    return trovati


def sequenza_del_montaggio(percorso_montaggio):
    """Trova la sequenza usata dal montaggio seguendo le sue dipendenze."""
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    opzioni = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=False,
        include_hard_package_references=True,
        include_searchable_names=False,
        include_soft_management_references=False,
        include_hard_management_references=False,
    )
    for pacchetto in registro.get_dependencies(percorso_montaggio, opzioni) or []:
        pacchetto = str(pacchetto)
        if not pacchetto.startswith("/Game/"):
            continue
        asset = unreal.EditorAssetLibrary.load_asset(pacchetto)
        if isinstance(asset, unreal.AnimSequence):
            return asset, pacchetto
    return None, None


def trasformazione_in_dati(t):
    loc = t.translation
    q = t.rotation
    return {"posizione": [loc.x, loc.y, loc.z], "quaternione": [q.x, q.y, q.z, q.w]}


def osso_nello_spazio_del_personaggio(sequenza, nome_osso, tempo, con_root_motion):
    """Somma le posizioni locali dalla radice fino all'osso."""
    percorso = [str(n) for n in LIB.find_bone_path_to_root(sequenza, nome_osso)]
    # find_bone_path_to_root va dall'osso alla radice: lo giriamo
    percorso.reverse()
    totale = None
    for osso in percorso:
        locale = LIB.get_bone_pose_for_time(sequenza, osso, tempo, con_root_motion)
        totale = locale if totale is None else locale.multiply(totale)
    return totale, percorso


def leggi():
    montaggi = cerca_asset("AnimMontage", MONTAGGI)
    scrivi("Montaggi trovati: %d su 6 (versione 3)" % len(montaggi))

    risultato = {}
    for nome in MONTAGGI:
        if nome not in montaggi:
            scrivi("  MANCA il montaggio: " + nome)
            continue
        montaggio = unreal.EditorAssetLibrary.load_asset(montaggi[nome])
        sequenza, percorso_seq = sequenza_del_montaggio(montaggi[nome])
        if sequenza is None:
            scrivi("  MANCA la sequenza del montaggio: " + nome)
            continue
        scrivi("  sequenza: " + percorso_seq)
        try:
            scrivi("%s: durata montaggio %.2f s, sequenza %.2f s"
                   % (nome, montaggio.get_play_length(), sequenza.get_play_length()))
        except Exception:
            scrivi("%s: (durate non lette)" % nome)
        rettangoli = []
        for evento in LIB.get_animation_notify_events(montaggio):
            stato = evento.get_editor_property("notify_state_class")
            if stato is None or not isinstance(stato, unreal.AnimNotifyState_MotionWarping):
                continue
            mod = stato.get_editor_property("root_motion_modifier")
            bersaglio = str(mod.get_editor_property("warp_target_name"))
            fornitore = mod.get_editor_property("warp_point_anim_provider")
            osso = str(mod.get_editor_property("warp_point_anim_bone_name"))
            inizio = LIB.get_anim_notify_event_trigger_time(evento)
            fine = inizio + LIB.get_anim_notify_event_duration(evento)
            voce = {"bersaglio": bersaglio, "fornitore": str(fornitore), "osso": osso,
                    "inizio": inizio, "fine": fine}
            if fornitore != unreal.WarpPointAnimProvider.BONE:
                esito = "non usa un osso: resta com'e'"
            else:
                try:
                    senza, catena = osso_nello_spazio_del_personaggio(sequenza, osso, fine, False)
                    con, _ = osso_nello_spazio_del_personaggio(sequenza, osso, fine, True)
                    voce["senza_root_motion"] = trasformazione_in_dati(senza)
                    voce["con_root_motion"] = trasformazione_in_dati(con)
                    esito = "letto (catena: %s)" % " > ".join(catena)
                except Exception as errore:
                    esito = "ERRORE: %s" % errore
            rettangoli.append(voce)
            scrivi("   %-10s %.2f-%.2f s  %s" % (bersaglio, inizio, fine, esito))
        risultato[nome] = rettangoli

    os.makedirs(os.path.dirname(FILE_USCITA), exist_ok=True)
    with open(FILE_USCITA, "w", encoding="utf-8") as f:
        json.dump(risultato, f, indent=2)
    scrivi("Rettangoli letti: %d. File scritto: %s"
           % (sum(len(r) for r in risultato.values()), FILE_USCITA))


leggi()
