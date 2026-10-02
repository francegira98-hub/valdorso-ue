# Valdorso - Scrivi i punti "attach" nei montaggi di scavalcamento (versione 2)
# Percorso: V:\Valdorso\Content\Python\scrivi_punti_attach_v2.py
#
# Versione 2: riscrive anche i rettangoli gia' a punto fisso (Static), cosi' si puo'
# provare l'altra misura cambiando solo la parola VARIANTE qui sotto.
#
# Si lancia IN VALDORSO, dopo leggi_attach_gasp_v3.py (lanciato nel GameAnimationSample).
# Legge V:\Valdorso\Saved\Claude\punti_attach.json e, nei 6 montaggi di
# /Game/Fab/GameAnimationSample/Traversal, cambia ogni rettangolo MotionWarping
# che cerca l'osso "attach" (che il nostro scheletro non ha): invece dell'osso
# usa un punto fisso (Static), cioe' la posizione che l'osso aveva in quel momento
# nell'animazione originale di Epic. Poi salva i montaggi.
# E' una riparazione da fare una volta sola: non sta nel menu Valdorso.

import json
import unreal

FILE_PUNTI = r"V:\Valdorso\Saved\Claude\punti_attach.json"
# Quale misura usare: "con_root_motion" (prima scelta) o "senza_root_motion".
# Se dopo la prova i segni FrontLedge stanno nel posto sbagliato, si cambia qui.
VARIANTE = "con_root_motion"


def scrivi(testo):
    unreal.log("[Valdorso] " + testo)


def montaggi_in_valdorso(nomi):
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    filtro = unreal.ARFilter(
        class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "AnimMontage")],
        package_paths=["/Game/Fab"],
        recursive_paths=True,
    )
    trovati = {}
    for dati in registro.get_assets(filtro):
        if str(dati.asset_name) in nomi:
            trovati[str(dati.asset_name)] = str(dati.package_name)
    return trovati


def dati_in_trasformazione(d):
    p = d["posizione"]
    q = d["quaternione"]
    rotazione = unreal.Quat(q[0], q[1], q[2], q[3]).rotator()
    return unreal.Transform(unreal.Vector(p[0], p[1], p[2]), rotazione, unreal.Vector(1.0, 1.0, 1.0))


def ripara():
    with open(FILE_PUNTI, encoding="utf-8") as f:
        punti = json.load(f)
    trovati = montaggi_in_valdorso(set(punti.keys()))
    scrivi("Montaggi trovati in Valdorso: %d su %d (variante %s)" % (len(trovati), len(punti), VARIANTE))

    cambiati_totale = 0
    for nome, voci in punti.items():
        if nome not in trovati:
            scrivi("  MANCA in Valdorso: " + nome)
            continue
        montaggio = unreal.EditorAssetLibrary.load_asset(trovati[nome])
        montaggio.modify()
        usate = set()
        cambiati = 0
        for evento in unreal.AnimationLibrary.get_animation_notify_events(montaggio):
            stato = evento.get_editor_property("notify_state_class")
            if stato is None or not isinstance(stato, unreal.AnimNotifyState_MotionWarping):
                continue
            mod = stato.get_editor_property("root_motion_modifier")
            bersaglio = str(mod.get_editor_property("warp_target_name"))
            inizio = unreal.AnimationLibrary.get_anim_notify_event_trigger_time(evento)
            fornitore = mod.get_editor_property("warp_point_anim_provider")
            if fornitore not in (unreal.WarpPointAnimProvider.BONE, unreal.WarpPointAnimProvider.STATIC):
                scrivi("   %-10s %.2f s  non usa un osso: lasciato com'e'" % (bersaglio, inizio))
                continue
            # Cerca la voce di Epic con lo stesso bersaglio e lo stesso inizio
            scelta = None
            for i, voce in enumerate(voci):
                if i in usate or voce["bersaglio"] != bersaglio or VARIANTE not in voce:
                    continue
                if abs(voce["inizio"] - inizio) < 0.02:
                    scelta = i
                    break
            if scelta is None:
                scrivi("   %-10s %.2f s  NESSUN PUNTO corrispondente: non cambiato" % (bersaglio, inizio))
                continue
            usate.add(scelta)
            stato.modify()
            mod.modify()
            mod.set_editor_property("warp_point_anim_transform", dati_in_trasformazione(voci[scelta][VARIANTE]))
            mod.set_editor_property("warp_point_anim_provider", unreal.WarpPointAnimProvider.STATIC)
            cambiati += 1
            scrivi("   %-10s %.2f s  -> punto fisso (%s)" % (bersaglio, inizio, VARIANTE))
        if cambiati:
            unreal.EditorAssetLibrary.save_loaded_asset(montaggio, False)
        scrivi("%s: %d rettangoli cambiati, salvato" % (nome, cambiati))
        cambiati_totale += cambiati

    scrivi("Fatto: %d rettangoli cambiati in tutto (attesi 12)" % cambiati_totale)


ripara()
