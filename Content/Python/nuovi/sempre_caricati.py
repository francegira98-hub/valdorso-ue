# Valdorso - Le cose che si usano non stanno nei pezzi della valle (13/11/2026, scritto da Claude dopo la grande prova).
#
# Perché: alla prova del 12/11 il server girava dall'editor e il gioco dal pacchetto. I pezzi della valle (World
# Partition) hanno nomi diversi nei due casi, quindi quando un giocatore premeva E su una porta, un PNG, il pozzo o la
# pietra dei segni, il server non trovava l'oggetto e non faceva niente. Gli attori "sempre caricati" (Is Spatially
# Loaded = falso) stanno invece nel livello principale, con lo stesso nome nell'editor e nel pacchetto.
#
# Cosa fa: carica tutti gli attori di L_Valle, e a quelli delle nostre classi C++ (AValdorso..., anche i Blueprint
# figli), ai Player Start e alle acque (WaterBody) toglie "Is Spatially Loaded". Poi salva.
# Scrive cosa ha fatto in Saved/Claude/sempre_caricati.txt.
# Si lancia dal menu Valdorso -> Strumenti -> "Rete: le cose che si usano sempre caricate", e da solo alla fine di
# crea_villaggio.py e metti_png.py (che rifanno gli attori).
#
# Dopo: si rifà il pacchetto (Tools\Pacchettizza Valdorso.cmd), perché anche il gioco deve avere la valle nuova.

import os
import unreal

RIGHE = []


def scrivi(testo):
    RIGHE.append(testo)
    unreal.log("[Valdorso] sempre caricati: " + testo)


def classi_valdorso():
    """Tutte le classi C++ del modulo Valdorso che sono attori (AValdorsoPorta, AValdorsoPNG, ...)."""
    trovate = []
    for nome in dir(unreal):
        if not nome.startswith("Valdorso"):
            continue
        c = getattr(unreal, nome, None)
        try:
            if isinstance(c, type) and issubclass(c, unreal.Actor):
                trovate.append(c)
        except TypeError:
            pass
    return tuple(trovate)


def carica_tutto():
    """Nel World Partition dell'editor non tutti gli attori sono caricati: si caricano tutti, se si riesce."""
    try:
        lib = unreal.WorldPartitionBlueprintLibrary
        descrizioni = lib.get_actor_descs()
        guid = [d.guid for d in descrizioni]
        if guid:
            lib.load_actors(guid)
        scrivi("attori del World Partition caricati: %d" % len(guid))
    except Exception as e:  # noqa: BLE001
        scrivi("non riesco a caricare tutto (%s): uso gli attori già caricati" % e)


def da_tenere(attore, nostre):
    if nostre and isinstance(attore, nostre):
        return True
    for classe in ("PlayerStart", "WaterBody", "WaterZone", "PhysicsVolume"):
        c = getattr(unreal, classe, None)
        if c is not None and isinstance(attore, c):
            return True
    return False


def principale():
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not mondo or "L_Valle" not in mondo.get_path_name():
        scrivi("ATTENZIONE: il livello aperto non è L_Valle (%s). Apri /Game/Prove/L_Valle e rilancia."
               % (mondo.get_path_name() if mondo else "nessuno"))
        return
    carica_tutto()
    nostre = classi_valdorso()
    scrivi("classi Valdorso trovate: %d" % len(nostre))
    attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    cambiati, gia, per_classe = 0, 0, {}
    for a in attori:
        if not da_tenere(a, nostre):
            continue
        try:
            if not a.get_editor_property("is_spatially_loaded"):
                gia += 1
                continue
            a.modify()
            a.set_editor_property("is_spatially_loaded", False)
            cambiati += 1
            n = a.get_class().get_name()
            per_classe[n] = per_classe.get(n, 0) + 1
        except Exception as e:  # noqa: BLE001
            scrivi("non cambio %s (%s)" % (a.get_actor_label(), e))
    scrivi("cambiati: %d, erano già sempre caricati: %d" % (cambiati, gia))
    for n in sorted(per_classe, key=lambda k: -per_classe[k]):
        scrivi("  %s: %d" % (n, per_classe[n]))
    if cambiati:
        try:
            unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
            scrivi("salvato")
        except Exception as e:  # noqa: BLE001
            scrivi("ATTENZIONE: salvataggio non riuscito (%s): salvare a mano con Ctrl+Maiusc+S" % e)
    scrivi("FATTO. Ora: chiudere Unreal e lanciare Tools\\Pacchettizza Valdorso.cmd")


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "sempre_caricati.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(RIGHE) + "\n")


try:
    principale()
finally:
    salva_resoconto()
