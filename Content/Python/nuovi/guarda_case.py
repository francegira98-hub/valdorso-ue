# Valdorso - Guarda le case (15/11/2026, scritto da Claude). NON CAMBIA NIENTE: scrive solo un resoconto.
# Primo passo della "pelle nuova" delle case (v0.2.5, decisa il 15/11): prima di cambiare i materiali delle nostre
# case bisogna sapere quali usano. Per ogni casa della pianta (Villaggio) cerca il suo attore nel livello e scrive:
# i modelli (mesh), i materiali di ogni slot e le texture che quei materiali usano.
# Menu Valdorso -> Villaggio -> "Case: guarda i materiali (non cambia niente)".
# Resoconto in Saved/Claude/case_materiali.txt: prima l'elenco dei materiali con quante case li usano, poi casa per casa.

import json
import os

import unreal

PIANTA = "V:/Texture/valdorso/villaggio/Pianta_3000.json"
righe = []


def scrivi(t):
    righe.append(t)
    unreal.log("[Valdorso] Guarda case: " + t)


def texture_di(mat):
    try:
        return sorted(set(t.get_name() for t in unreal.MaterialEditingLibrary.get_used_textures(mat) if t))
    except Exception:  # noqa: BLE001
        return []


def principale():
    with open(PIANTA, "r", encoding="utf-8") as f:
        pianta = json.load(f)
    nomi = [c["nome"] for c in pianta if c.get("tipo") == "edificio"]
    attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    per_nome = {}
    for a in attori:
        if a:
            per_nome.setdefault(a.get_actor_label(), a)
    usi = {}          # materiale -> set(case)
    dettagli = []
    mancano = []
    for nome in nomi:
        a = per_nome.get(nome)
        if a is None:
            mancano.append(nome)
            continue
        righe_casa = []
        for comp in a.get_components_by_class(unreal.StaticMeshComponent):
            m = comp.get_editor_property("static_mesh")
            if m is None:
                continue
            slot = []
            for k in range(comp.get_num_materials()):
                mat = comp.get_material(k)
                if mat is None:
                    continue
                p = mat.get_path_name().split(".")[0]
                usi.setdefault(p, set()).add(nome)
                slot.append(p.rsplit("/", 1)[-1])
            righe_casa.append("    %s: %s" % (m.get_name(), ", ".join(slot)))
        dettagli.append((nome, a.get_class().get_name(), righe_casa))
    scrivi("case nella pianta: %d; trovate nel livello: %d; non trovate: %d" % (len(nomi), len(dettagli), len(mancano)))
    scrivi("")
    scrivi("MATERIALI (quante case li usano, poi le texture):")
    for p, case in sorted(usi.items(), key=lambda kv: -len(kv[1])):
        mat = unreal.EditorAssetLibrary.load_asset(p)
        tex = texture_di(mat) if mat else []
        genitore = ""
        try:
            if isinstance(mat, unreal.MaterialInstance) and mat.get_editor_property("parent"):
                genitore = " (figlio di %s)" % mat.get_editor_property("parent").get_name()
        except Exception:  # noqa: BLE001
            pass
        scrivi("  %3d  %s%s" % (len(case), p, genitore))
        if tex:
            scrivi("         texture: %s" % ", ".join(tex))
    scrivi("")
    scrivi("CASA PER CASA:")
    for nome, classe, rr in dettagli:
        scrivi("  %s  [%s]" % (nome, classe))
        for r in rr:
            scrivi(r)
    if mancano:
        scrivi("")
        scrivi("non trovate nel livello: %s" % ", ".join(mancano))
    scrivi("FATTO (non e' cambiato niente)")


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "case_materiali.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
