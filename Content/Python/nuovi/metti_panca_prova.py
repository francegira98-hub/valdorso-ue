# Valdorso - Mette una panca di prova (v0.1.3) nel livello aperto, 3 m davanti alla porta di prova (o al Player
# Start). Scritto da Claude il 03/11/2026. Si può rilanciare: non mette doppioni.
# Dopo: Play, avvicinati da davanti, di lato o da dietro e premi E; per alzarti E di nuovo, o muoviti.

import unreal

CARTELLA = "Valdorso/Prove"


def principale():
    attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    panca_classe = getattr(unreal, "ValdorsoPanca", None)
    if panca_classe is None:
        unreal.log_error("[Valdorso] Panca: manca ValdorsoPanca nel C++ (rigenera i file di progetto e compila prima)")
        return
    tutti = attori.get_all_level_actors()
    if any(a.get_actor_label() == "Panca di prova" for a in tutti):
        unreal.log("[Valdorso] Panca: c'è già, niente da fare.")
        return

    punto = unreal.Vector(400, 0, 0)
    rotazione = unreal.Rotator(0, 0, 0)
    base = next((a for a in tutti if a.get_actor_label() == "Porta di prova"), None)
    if base is None:
        base = next((a for a in tutti if isinstance(a, unreal.PlayerStart)), None)
    if base is not None:
        p = base.get_actor_location()
        avanti = base.get_actor_forward_vector()
        alto = -90 if isinstance(base, unreal.PlayerStart) else 0
        punto = unreal.Vector(p.x + avanti.x * 300, p.y + avanti.y * 300, p.z + alto)
        r = base.get_actor_rotation()
        # La panca guarda verso la porta (o verso il Player Start): chi arriva da lì la trova di fronte.
        rotazione = unreal.Rotator(0, 0, r.yaw + 180)

    panca = attori.spawn_actor_from_class(panca_classe, punto, rotazione)
    panca.set_actor_label("Panca di prova")
    panca.set_folder_path(CARTELLA)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[Valdorso] Panca: messa e livello salvato. Play e prova con E da tutti i lati.")


principale()
