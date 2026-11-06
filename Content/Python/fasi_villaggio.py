# Valdorso - Nell'editor: mostra le cose del villaggio di una fase sola (06/11/2026, scritto da Claude).
# Voci del menu Valdorso: "Villaggio: mostra la fase 1 / 2 / 3". Nasconde le altre solo per questa sessione
# dell'editor (non cambia il livello e non serve salvare). In gioco la fase la decide il server
# (AValdorsoFasiVillaggio, comando di console Valdorso.Villaggio.Fase N).

import unreal

PREFISSO = "ValdorsoFase "


def fase_di(attore):
    """(da, fino) dall'etichetta "ValdorsoFase da-fino"; None se l'attore c'è sempre."""
    for t in attore.get_editor_property("tags"):
        testo = str(t)
        if testo.startswith(PREFISSO):
            try:
                da, fino = testo[len(PREFISSO):].split("-")
                return int(da), int(fino)
            except ValueError:
                return None
    return None


def mostra(fase):
    attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    mostrate = nascoste = 0
    for a in attori.get_all_level_actors():
        f = fase_di(a)
        if f is None:
            continue
        visibile = f[0] <= fase and (f[1] == 0 or fase <= f[1])
        a.set_is_temporarily_hidden_in_editor(not visibile)
        if visibile:
            mostrate += 1
        else:
            nascoste += 1
    unreal.log("[Valdorso] Villaggio nell'editor: fase %d, %d cose mostrate, %d nascoste" % (fase, mostrate, nascoste))
