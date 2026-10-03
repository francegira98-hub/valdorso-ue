# Valdorso - Ritocco della nebbia del menu (Lvl_Menu), scritto da Claude il 03/10/2026.
# Strumenti -> Esegui script Python -> questo file (a gioco fermo).
# Prima scrive nel Registro output i valori di adesso (per Claude), poi schiarisce il velo lilla:
# la luce del cielo e la luna illuminavano troppo la nebbia volumetrica, che copriva il cielo.

import unreal

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def scrivi(testo):
    unreal.log("[Valdorso] Nebbia: " + testo)


def leggi(oggetto, nomi):
    for nome in nomi:
        try:
            scrivi("  " + nome + " = " + str(oggetto.get_editor_property(nome)))
        except Exception:
            scrivi("  " + nome + " = (non trovato)")


def imposta(oggetto, nome, valore):
    try:
        oggetto.set_editor_property(nome, valore)
    except Exception as errore:
        unreal.log_warning("[Valdorso] Nebbia: " + nome + " non impostato (" + str(errore) + ")")


def trova(etichetta):
    for a in attori.get_all_level_actors():
        if a.get_actor_label() == etichetta:
            return a
    return None


try:
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
        unreal.log_warning("[Valdorso] Nebbia: il gioco e' acceso, premi Stop e rilancia")
        raise SystemExit
except SystemExit:
    raise
except Exception:
    pass

nebbia = trova("Nebbia")
cielo = trova("LuceDelCielo")
luna = trova("Luna")

scrivi("--- valori di adesso ---")
if nebbia:
    leggi(nebbia.component, ["fog_density", "fog_height_falloff", "fog_cutoff_distance", "start_distance",
                             "volumetric_fog", "volumetric_fog_distance", "volumetric_fog_albedo",
                             "volumetric_fog_extinction_scale", "volumetric_fog_scattering_distribution"])
if cielo:
    leggi(cielo.light_component, ["intensity", "source_type", "volumetric_scattering_intensity"])
if luna:
    leggi(luna.light_component, ["intensity", "volumetric_scattering_intensity"])
    scrivi("  rotazione luna = " + str(luna.get_actor_rotation()))
for a in attori.get_all_level_actors():
    if isinstance(a, unreal.ValdorsoBraciere):
        scrivi("  " + a.get_actor_label() + " fuoco = " + str(a.get_editor_property("materiale_fuoco")))
    if isinstance(a, unreal.ValdorsoCerchioRune):
        scrivi("  CerchioRune materiale = " + str(a.get_editor_property("materiale")) +
               " posizione = " + str(a.get_actor_location()))

scrivi("--- ritocco ---")
if cielo:
    # La foto del cielo di prima e' molto chiara: illuminava tutta la nebbia di lilla.
    imposta(cielo.light_component, "volumetric_scattering_intensity", 0.1)
if luna:
    imposta(luna.light_component, "volumetric_scattering_intensity", 1.2)
if nebbia:
    nc = nebbia.component
    # Nebbia volumetrica solo vicino (40 m), piu' scura: resta il fumo intorno alle pietre, il cielo si vede.
    imposta(nc, "volumetric_fog_distance", 4000.0)
    imposta(nc, "volumetric_fog_albedo", unreal.Color(r=90, g=95, b=110, a=255))
    imposta(nc, "fog_cutoff_distance", 100000.0)

livelli.save_current_level()
scrivi("fatto")
