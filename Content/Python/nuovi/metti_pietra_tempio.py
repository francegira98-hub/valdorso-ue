# Valdorso - Mette la pietra di Val d'Orso sull'altare del Tempio del Cuore, senza rifare il villaggio.
# Scritto da Claude il 07/11/2026 (v0.1.3). Si lancia in L_Valle (dopo aver compilato: serve la classe C++
# ValdorsoPietraVillaggio): Strumenti -> Esegui script Python -> questo file, oppure dal menu Valdorso ->
# "Tempio: metti la pietra di Val d'Orso".
#
# Cerca la luce "Altare del Cuore (luce)" messa da crea_villaggio.py (sta 260 cm sopra il pavimento del tempio, l'altare
# è alto 118 cm), toglie una pietra messa prima e ne mette una nuova sull'altare, con l'etichetta del villaggio: così il
# prossimo crea_villaggio.py la toglie e la rimette da solo. Alla fine salva il livello.

import unreal

ETICHETTA = "ValdorsoVillaggio"
NOME_LUCE = "Altare del Cuore (luce)"
SOPRA_ALTARE = 142.0

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def scrivi(testo):
    unreal.log("[Valdorso] Pietra del tempio: " + testo)


def principale():
    classe = getattr(unreal, "ValdorsoPietraVillaggio", None)
    if classe is None:
        unreal.log_error("[Valdorso] Pietra del tempio: manca la classe C++ ValdorsoPietraVillaggio. Compila Valdorso e riapri l'editor.")
        return
    tutti = attori.get_all_level_actors()
    luce = None
    for a in tutti:
        if a.get_actor_label() == NOME_LUCE:
            luce = a
            break
    if luce is None:
        unreal.log_error("[Valdorso] Pietra del tempio: non trovo la luce \"%s\". Sei in L_Valle? Il villaggio c'è (crea_villaggio.py)?" % NOME_LUCE)
        return
    tolte = 0
    for a in tutti:
        if isinstance(a, classe):
            attori.destroy_actor(a)
            tolte += 1
    if tolte:
        scrivi("tolte %d pietre messe prima" % tolte)
    p = luce.get_actor_location()
    pietra = attori.spawn_actor_from_class(classe, unreal.Vector(p.x, p.y, p.z - SOPRA_ALTARE), luce.get_actor_rotation())
    if pietra is None:
        unreal.log_error("[Valdorso] Pietra del tempio: non sono riuscito a metterla")
        return
    pietra.set_actor_label("La pietra di Val d'Orso")
    cartella = str(luce.get_folder_path()) or "Valle/Villaggio/Villaggio"
    pietra.set_folder_path(cartella)
    pietra.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    scrivi("messa sull'altare in (%.0f, %.0f, %.0f), cartella %s" % (p.x, p.y, p.z - SOPRA_ALTARE, cartella))
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("[Valdorso] Pietra del tempio: salvataggio non riuscito (%s): Ctrl+Maiusc+S" % e)
    scrivi("FATTO. In Play: entra nel Tempio del Cuore (a nord della piazza), avvicinati all'altare, E.")


principale()
