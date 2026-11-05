# Valdorso - Mette una porta di prova (AValdorsoPorta) nel livello aperto, 4 m davanti al Player Start, con due
# stipiti ai lati. Scritto da Claude il 03/11/2026. Si può rilanciare: se la porta c'è già, non ne mette un'altra.
# Dopo: Play, avvicinati, E per aprire e chiudere (in basso compare "E · Apri la porta").

import unreal

ETICHETTA = "Porta di prova"
CARTELLA = "Valdorso/Prove"


def principale():
    attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    classe = getattr(unreal, "ValdorsoPorta", None)
    if classe is None:
        unreal.log_error("[Valdorso] Porta: manca ValdorsoPorta nel C++ (compila Valdorso prima)")
        return
    tutti = attori.get_all_level_actors()
    for attore in tutti:
        if attore.get_actor_label() == ETICHETTA:
            unreal.log("[Valdorso] Porta: c'è già (%s), non ne metto un'altra." % attore.get_actor_location())
            return

    # Davanti al Player Start (o al centro del mondo, se non c'è).
    partenza = unreal.Vector(0, 0, 0)
    avanti = unreal.Vector(1, 0, 0)
    direzione = 0.0
    for attore in tutti:
        if isinstance(attore, unreal.PlayerStart):
            partenza = attore.get_actor_location()
            direzione = attore.get_actor_rotation().yaw
            avanti = attore.get_actor_forward_vector()
            break
    # Il Player Start sta a mezza altezza del colono: la porta va a terra (circa 90 cm più in basso).
    punto = unreal.Vector(partenza.x + avanti.x * 400, partenza.y + avanti.y * 400, partenza.z - 90)
    rotazione = unreal.Rotator(0, 0, direzione)

    porta = attori.spawn_actor_from_class(classe, punto, rotazione)
    porta.set_actor_label(ETICHETTA)
    porta.set_folder_path(CARTELLA)

    # Due stipiti di pietra ai lati (cubi), così si vede che è una porta.
    cubo = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    destra = porta.get_actor_right_vector()
    for lato, nome in ((-1, "Stipite sinistro"), (1, "Stipite destro")):
        posto = unreal.Vector(punto.x + destra.x * 70 * lato, punto.y + destra.y * 70 * lato, punto.z + 115)
        stipite = attori.spawn_actor_from_object(cubo, posto, rotazione)
        stipite.set_actor_scale3d(unreal.Vector(0.3, 0.4, 2.3))
        stipite.set_actor_label(nome)
        stipite.set_folder_path(CARTELLA)

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[Valdorso] Porta: messa in %s, livello salvato. Play, avvicinati e premi E." % punto)


principale()
