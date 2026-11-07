# Valdorso - Le rocce con la collisione vera (v0.1.3). Scritto da Claude il 07/11/2026.
# Fra (07/11): vicino al belvedere "non va avanti, come se ci fosse un muro invisibile": le rocce di Poly Haven e
# Megascans avevano la collisione semplice (un guscio largo intorno alla roccia). Questo script dice a Unreal di usare
# la forma vera della roccia ("Usa il complesso come semplice"): niente muri invisibili, e dove la roccia è abbastanza
# piatta ci si sale sopra e ci si cammina (dove è ripida ferma la parete che si vede). Scalare davvero (aggrapparsi)
# è della patch delle animazioni.
# Si lancia una volta in L_Valle (Strumenti -> Esegui script Python): cambia le mesh, non il livello, e le salva.
# Tocca solo le mesh di Poly Haven e Megascans usate nel livello (le rocce di piazza_rocce.py e i massi messi da
# crea_villaggio.py). Resoconto in Saved/Claude/rocce_collisione.txt.

import os

import unreal

libreria = unreal.EditorAssetLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Rocce: " + testo)


def esegui():
    # le mesh di roccia usate davvero nel livello: quelle delle rocce della valle e i massi del villaggio, delle
    # grotte e della palude (tutte le mesh di Poly Haven e Megascans). Le altre mesh delle cartelle non si toccano
    # (la scansione grezza da 84 MB, se non è usata, resta com'è).
    mesh = {}
    for a in attori.get_all_level_actors():
        for comp in a.get_components_by_class(unreal.StaticMeshComponent):
            sm = comp.get_editor_property("static_mesh")
            if sm and sm.get_path_name().startswith(("/Game/Fab/PolyHaven/", "/Game/Fab/Megascans/")):
                mesh[sm.get_path_name()] = sm
    fatte = 0
    for percorso, sm in sorted(mesh.items()):
        try:
            bs = sm.get_editor_property("body_setup")
            if bs is None:
                scrivi("senza body setup: %s" % percorso)
                continue
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            sm.set_editor_property("body_setup", bs)
            libreria.save_loaded_asset(sm)
            fatte += 1
            scrivi("collisione vera: %s" % percorso)
        except Exception as e:  # noqa: BLE001
            scrivi("ATTENZIONE %s: %s" % (percorso, e))
    scrivi("FATTO: %d mesh di roccia con la collisione vera" % fatte)


try:
    esegui()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "rocce_collisione.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe))
