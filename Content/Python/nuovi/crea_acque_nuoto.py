# Valdorso - Le acque per il nuoto (v0.1.3). Scritto da Claude il 06/11/2026, preparato mentre Fra dormiva.
# Si lancia in L_Valle: Strumenti -> Esegui script Python -> questo file. Si può rilanciare (toglie quello di prima).
#
# Mette nel livello un solo attore, "Acque per il nuoto" (AValdorsoAcque, C++), con la forma dell'acqua presa da
# V:/Texture/valdorso/valle/Acqua_3000.json (gli stessi dati di lago e fiumi, vedi usati/crea_acqua_valle.py):
#   - gli specchi: il lago (riva e pelo) e la pozza della cascata;
#   - i corsi: il fiume, l'emissario e il torrente, punto per punto con il pelo (quota dell'acqua) e la larghezza.
# Il movimento del colono (UValdorsoMovementComponent) lo usa per nuotare. Serve aver compilato Valdorso.

import json

import unreal

FILE_ACQUA = "V:/Texture/valdorso/valle/Acqua_3000.json"
ETICHETTA = "ValdorsoAcque"
QUOTA_MAX_CM = 80000.0          # i 16 bit delle altezze coprono 0-800 m (valle.py)

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def esegui():
    classe = getattr(unreal, "ValdorsoAcque", None)
    if classe is None:
        unreal.log_error("[Valdorso] Acque: manca la classe C++ ValdorsoAcque (compilare Valdorso)")
        return
    with open(FILE_ACQUA, "r", encoding="utf-8") as f:
        dati = json.load(f)
    tutti = attori.get_all_level_actors()
    paesaggi = [a for a in tutti if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Acque: nel livello aperto non c'è un Landscape. Apri L_Valle.")
        return
    angolo = paesaggi[0].get_actor_location()
    scala = paesaggi[0].get_actor_scale3d()

    def nel_mondo(p):
        valore = p[2] / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + p[0], angolo.y + p[1], z)

    for a in tutti:
        if a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)

    specchi = []
    for chiave, nome in (("lago", "Lago"), ("pozza", "Pozza della cascata")):
        d = dati.get(chiave)
        if not d or not d.get("riva"):
            continue
        riva = [nel_mondo(p) for p in d["riva"]]
        pelo = nel_mondo([0.0, 0.0, d.get("pelo_cm", d["riva"][0][2])]).z
        s = unreal.ValdorsoSpecchioAcqua()
        s.set_editor_property("nome", nome)
        s.set_editor_property("riva", [unreal.Vector2D(v.x, v.y) for v in riva])
        s.set_editor_property("pelo", float(pelo))
        specchi.append(s)
        unreal.log("[Valdorso] Acque: %s, %d punti di riva, pelo a %.0f cm" % (nome, len(riva), pelo))

    corsi = []
    for chiave, nome in (("fiume", "Fiume (la Vena)"), ("emissario", "Emissario"), ("torrente", "Torrente")):
        d = dati.get(chiave)
        if not d or not d.get("punti"):
            continue
        c = unreal.ValdorsoCorsoAcqua()
        c.set_editor_property("nome", nome)
        c.set_editor_property("punti", [nel_mondo(p["posizione"]) for p in d["punti"]])
        c.set_editor_property("larghezze", [float(p.get("larghezza_cm", 1000.0)) for p in d["punti"]])
        corsi.append(c)
        unreal.log("[Valdorso] Acque: %s, %d punti" % (nome, len(d["punti"])))

    a = attori.spawn_actor_from_class(classe, angolo, unreal.Rotator(0, 0, 0))
    a.set_actor_label("Acque per il nuoto")
    a.set_folder_path("Valle/Acqua")
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    a.set_editor_property("specchi", specchi)
    a.set_editor_property("corsi", corsi)
    try:
        # con World Partition: sempre caricato (il nuoto serve ovunque, anche lontano)
        a.set_editor_property("is_spatially_loaded", False)
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("[Valdorso] Acque: Is Spatially Loaded non spento (%s): spegnerlo a mano nei Dettagli" % e)
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    except Exception:  # noqa: BLE001
        pass
    unreal.log("[Valdorso] Acque: FATTO, %d specchi e %d corsi d'acqua per il nuoto" % (len(specchi), len(corsi)))


esegui()
