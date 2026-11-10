# Valdorso - I banchi delle botteghe nel villaggio (v0.1.4, 10/11/2026). Scritto da Claude.
# Si lancia in L_Valle dal menu Valdorso -> Villaggio -> "Botteghe: metti i banchi" (o Strumenti -> Esegui script),
# dopo crea_villaggio.py e metti_png.py. Si puo' rilanciare: toglie i banchi messi prima (etichetta "ValdorsoBotteghe").
#
# Mette un AValdorsoBanco (C++, ValdorsoBotteghe.h) invisibile in quattro posti: E vicino apre la bottega.
#   la fucina di Marta  -> alla forgia (la luce "Luce della forgia", un metro e mezzo piu' in basso)
#   la conceria         -> ai telai della conceria ("Telai della conceria")
#   il falegname        -> alle cataste ("Cataste del falegname")
#   la locanda di Bruno -> dove sta Bruno, al bancone (il PNG "Bruno"; se non c'e', il centro della locanda)
# Il fuoco non serve metterlo: ogni falo' acceso e' gia' un banco.
# Resoconto in Saved/Claude/botteghe.txt. Alla fine salva.

import os

import unreal

ETICHETTA = "ValdorsoBotteghe"
CARTELLA = "Valle/Botteghe"

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Botteghe: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Botteghe: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "botteghe.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))


def esegui():
    classe = getattr(unreal, "ValdorsoBanco", None)
    tipi = getattr(unreal, "ValdorsoTipoBanco", None)
    if classe is None or tipi is None:
        unreal.log_error("[Valdorso] Botteghe: manca la classe C++ ValdorsoBanco (compilare Valdorso)")
        return
    tutti = attori.get_all_level_actors()
    tolti = 0
    for a in tutti:
        if a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            tolti += 1
    if tolti:
        scrivi("tolti %d banchi messi prima" % tolti)
    tutti = attori.get_all_level_actors()
    per_nome = {}
    for a in tutti:
        per_nome.setdefault(a.get_actor_label(), a)

    def trova(*nomi):
        for n in nomi:
            if n in per_nome:
                return per_nome[n]
        # anche chi inizia cosi' ("Bruno, il locandiere")
        for n in nomi:
            for etichetta, a in per_nome.items():
                if etichetta.startswith(n):
                    return a
        return None

    posti = [
        ("La fucina di Marta", tipi.FUCINA, ("Luce della forgia",), -150.0),
        ("La conceria", tipi.CONCERIA, ("Telai della conceria",), 0.0),
        ("Il falegname", tipi.FALEGNAME, ("Cataste del falegname", "Falegname"), 0.0),
        ("La locanda di Bruno", tipi.LOCANDA, ("Bruno", "Locanda_di_Bruno"), 0.0),
    ]
    messi = 0
    for etichetta, tipo, nomi, alza in posti:
        dove = trova(*nomi)
        if dove is None:
            avviso("%s: non trovo %s nel livello (crea_villaggio.py e metti_png.py sono stati lanciati?)" % (etichetta, " / ".join(nomi)))
            continue
        p = dove.get_actor_location()
        if tipo == tipi.LOCANDA and dove.get_actor_label().startswith("Bruno"):
            # non addosso a Bruno (anche lui si parla con E): un metro e mezzo verso il centro della locanda
            centro = per_nome.get("Locanda_di_Bruno")
            dx, dy = (centro.get_actor_location().x - p.x, centro.get_actor_location().y - p.y) if centro else (150.0, 0.0)
            lung = max(1.0, (dx * dx + dy * dy) ** 0.5)
            p = unreal.Vector(p.x + dx / lung * 150.0, p.y + dy / lung * 150.0, p.z)
        p = unreal.Vector(p.x, p.y, p.z + alza + 90.0)
        a = attori.spawn_actor_from_class(classe, p, unreal.Rotator(0, 0, 0))
        a.set_actor_label("Banco - " + etichetta)
        a.set_folder_path(CARTELLA)
        a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
        a.set_editor_property("banco", tipo)
        try:
            a.set_editor_property("is_spatially_loaded", False)
        except Exception:  # noqa: BLE001
            pass
        scrivi("%s a (%.0f, %.0f, %.0f), vicino a %s" % (etichetta, p.x, p.y, p.z, dove.get_actor_label()))
        messi += 1
    scrivi("banchi messi: %d su %d" % (messi, len(posti)))
    salva_resoconto()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


esegui()
