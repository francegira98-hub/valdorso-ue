# Valdorso - Mette la nebbia della cascata (NS_CascataNebbia) nella pozza (v0.1.3). Scritto da Claude il 07/10/2026.
# Si lancia nell'editor, con L_Valle aperto, DOPO crea_cascata_valle.py: Strumenti -> Esegui script Python -> questo file.
# Si può rilanciare: rimpiazza la nebbia messa prima (etichetta "ValdorsoNebbia").
#
# Cosa fa: legge da V:/Texture/valdorso/valle/Acqua_3000.json dove l'acqua della cascata batte nella pozza
# e lì, un metro sopra il pelo dell'acqua, mette il sistema Niagara fatto da Fra (Content/Valle/Cascata/NS_CascataNebbia).

import json

import unreal

CARTELLA_PEZZI = "V:/Texture/valdorso/valle/cascata"
FILE_ACQUA = "V:/Texture/valdorso/valle/Acqua_3000.json"
SISTEMA = "/Game/Valle/Cascata/NS_CascataNebbia"
ETICHETTA = "ValdorsoNebbia"
QUOTA_MAX_CM = 80000.0
SOPRA_IL_PELO_CM = 100.0

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def esegui():
    with open(FILE_ACQUA, "r", encoding="utf-8") as f:
        c = json.load(f).get("cascata")
    if c is None:
        unreal.log_error("[Valdorso] Nebbia: Acqua_3000.json non ha la cascata.")
        return
    with open(CARTELLA_PEZZI + "/Cascata_3000.json", "r", encoding="utf-8") as f:
        info = json.load(f)
    sistema = unreal.load_asset(SISTEMA)
    if sistema is None:
        unreal.log_error("[Valdorso] Nebbia: non trovo " + SISTEMA + " (deve chiamarsi NS_CascataNebbia, in Content/Valle/Cascata).")
        return
    paesaggi = [a for a in attori.get_all_level_actors() if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Nebbia: nel livello aperto non c'è un Landscape. Apri L_Valle e rilancia.")
        return
    angolo = paesaggi[0].get_actor_location()
    scala = paesaggi[0].get_actor_scale3d()

    def nel_mondo(p):
        valore = p[2] / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + p[0], angolo.y + p[1], z)

    for a in attori.get_all_level_actors():
        if ETICHETTA in [str(t) for t in a.tags]:
            attori.destroy_actor(a)

    orlo = nel_mondo(c["orlo_cm"])
    pelo = nel_mondo(c["pozza_cm"]).z
    d = c["direzione"]
    avanti = info["batte_avanti_cm"]
    dove = unreal.Vector(orlo.x + d[0] * avanti, orlo.y + d[1] * avanti, pelo + SOPRA_IL_PELO_CM)
    a = attori.spawn_actor_from_object(sistema, dove, unreal.Rotator(0.0, 0.0, 0.0))
    a.set_actor_label("Cascata nebbia")
    a.tags = [unreal.Name(ETICHETTA)]
    try:
        a.set_folder_path("Valle/Cascata")
    except Exception:
        pass
    unreal.log("[Valdorso] Nebbia: messa dove batte la cascata, a X {:.0f}, Y {:.0f}, Z {:.0f}. Fatto.".format(
        dove.x, dove.y, dove.z))


esegui()
