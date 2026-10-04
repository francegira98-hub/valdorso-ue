# Valdorso - Il volo del primo ingresso in Lvl_Menu (v0.1.2, passo 2.5), scritto da Claude il 04/10/2026.
# Strumenti -> Esegui script Python -> questo file (a gioco fermo, dopo aver compilato il codice del passo 2.5).
#
# Cosa fa:
#   - mette in Lvl_Menu la curva "VoloIngresso" (cartella Scena), con i punti di serie: tra i bracieri,
#     sopra l'altare e il cerchio di rune, poi verso le montagne con la luna davanti;
#   - salva il livello.
# Per ritoccare il volo: si seleziona VoloIngresso, si spostano i suoi punti (i quadratini sulla curva) e si salva.
# Si puo' rilanciare: se la curva c'e' gia', non la tocca (i ritocchi restano). Per ripartire dai punti di serie
# basta cancellare VoloIngresso e rilanciare. Alla fine: "[Valdorso] Volo d'ingresso: fatto".

import unreal

LIVELLO = "/Game/Menu/Lvl_Menu"
NOME = "VoloIngresso"

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def scrivi(testo):
    unreal.log("[Valdorso] Volo d'ingresso: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] Volo d'ingresso: " + testo)


def esegui():
    if not hasattr(unreal, "ValdorsoVoloIngresso"):
        avviso("nel codice non c'e' ancora ValdorsoVoloIngresso: compila il passo 2.5 e rilancia")
        return

    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if mondo is None or mondo.get_name() != "Lvl_Menu":
        livelli.load_level(LIVELLO)

    for a in attori.get_all_level_actors():
        if isinstance(a, unreal.ValdorsoVoloIngresso):
            scrivi("c'e' gia' (" + a.get_actor_label() + "): non lo tocco")
            return

    volo = attori.spawn_actor_from_class(unreal.ValdorsoVoloIngresso, unreal.Vector(0, 0, 0), unreal.Rotator())
    if volo is None:
        avviso("non riesco a creare la curva")
        return
    volo.set_actor_label(NOME)
    volo.set_folder_path("Scena")
    livelli.save_current_level()
    scrivi("messa la curva con i punti di serie")
    scrivi("fatto")


esegui()
