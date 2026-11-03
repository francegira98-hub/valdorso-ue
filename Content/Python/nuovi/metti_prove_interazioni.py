# Valdorso - Mette una bacheca e un'imposta di prova (v0.1.3) nel livello aperto, accanto alla porta di prova
# (o davanti al Player Start). Scritto da Claude il 03/11/2026. Si può rilanciare: non mette doppioni.
# Dopo: Play, E davanti alla bacheca per leggerla, E sotto l'imposta per aprirla.

import unreal

CARTELLA = "Valdorso/Prove"


def principale():
    attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    bacheca_classe = getattr(unreal, "ValdorsoBacheca", None)
    imposta_classe = getattr(unreal, "ValdorsoImposta", None)
    if bacheca_classe is None or imposta_classe is None:
        unreal.log_error("[Valdorso] Prove: mancano ValdorsoBacheca o ValdorsoImposta nel C++ (compila Valdorso prima)")
        return
    tutti = attori.get_all_level_actors()
    etichette = set(a.get_actor_label() for a in tutti)

    # Il punto di partenza: la porta di prova, se c'è; altrimenti 4 m davanti al Player Start, a terra.
    base = None
    for attore in tutti:
        if attore.get_actor_label() == "Porta di prova":
            base = attore
            break
    if base is not None:
        punto = base.get_actor_location()
        rotazione = base.get_actor_rotation()
        destra = base.get_actor_right_vector()
    else:
        punto = unreal.Vector(400, 0, 0)
        rotazione = unreal.Rotator(0, 0, 0)
        destra = unreal.Vector(0, 1, 0)
        for attore in tutti:
            if isinstance(attore, unreal.PlayerStart):
                p = attore.get_actor_location()
                avanti = attore.get_actor_forward_vector()
                punto = unreal.Vector(p.x + avanti.x * 400, p.y + avanti.y * 400, p.z - 90)
                rotazione = attore.get_actor_rotation()
                destra = attore.get_actor_right_vector()
                break

    def accanto(metri, alto=0.0):
        return unreal.Vector(punto.x + destra.x * metri * 100, punto.y + destra.y * metri * 100, punto.z + alto)

    messi = []
    if "Bacheca di prova" not in etichette:
        bacheca = attori.spawn_actor_from_class(bacheca_classe, accanto(3.0), rotazione)
        bacheca.set_actor_label("Bacheca di prova")
        bacheca.set_folder_path(CARTELLA)
        messi.append("bacheca")

    if "Imposta di prova" not in etichette:
        # Un pezzo di muro (cubo) con l'imposta davanti, all'altezza di una finestra.
        cubo = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
        avanti = unreal.MathLibrary.get_forward_vector(rotazione)
        muro_posto = accanto(-3.0, 125)
        muro = attori.spawn_actor_from_object(cubo, unreal.Vector(muro_posto.x - avanti.x * 15, muro_posto.y - avanti.y * 15, muro_posto.z), rotazione)
        muro.set_actor_scale3d(unreal.Vector(0.2, 2.0, 2.5))
        muro.set_actor_label("Muro della finestra di prova")
        muro.set_folder_path(CARTELLA)
        imposta = attori.spawn_actor_from_class(imposta_classe, accanto(-3.0, 110), rotazione)
        imposta.set_actor_label("Imposta di prova")
        imposta.set_folder_path(CARTELLA)
        messi.append("imposta")

    if messi:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[Valdorso] Prove: messi %s; livello salvato. Play e prova con E." % (", ".join(messi) if messi else "niente (c'erano già)"))


principale()
