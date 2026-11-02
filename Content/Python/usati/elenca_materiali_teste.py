# Valdorso - Lettura (non cambia niente): i pezzi e i materiali delle teste MetaHuman montate, per sapere quali
# parametri cambiare per pelle, occhi, capelli e barba. Scrive in Saved/Claude/materiali_teste.txt.
# Scritto da Claude il 29/10/2026.

import os
import unreal

USCITA = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude", "materiali_teste.txt")
DETTAGLIO = ["Lorenzo", "Grace"]   # di queste si scrivono anche tutti i parametri dei materiali
righe = []
visti = set()


def prop(oggetto, nome):
    try:
        return oggetto.get_editor_property(nome)
    except Exception:
        return None


def parametri(materiale, rientro):
    if materiale is None:
        return
    percorso = materiale.get_path_name()
    catena = []
    m = materiale
    while isinstance(m, unreal.MaterialInstance):
        m = prop(m, "parent")
        if m is not None:
            catena.append(m.get_path_name())
    righe.append(rientro + "materiale " + percorso)
    if catena:
        righe.append(rientro + "  genitori: " + " <- ".join(catena))
    if percorso in visti or not isinstance(materiale, unreal.MaterialInstance):
        return
    visti.add(percorso)
    mel = unreal.MaterialEditingLibrary
    try:
        for n in mel.get_scalar_parameter_names(materiale):
            righe.append(rientro + "  S %s = %s" % (n, mel.get_material_instance_scalar_parameter_value(materiale, n)))
        for n in mel.get_vector_parameter_names(materiale):
            righe.append(rientro + "  V %s = %s" % (n, mel.get_material_instance_vector_parameter_value(materiale, n)))
        for n in mel.get_texture_parameter_names(materiale):
            t = mel.get_material_instance_texture_parameter_value(materiale, n)
            righe.append(rientro + "  T %s = %s" % (n, t.get_path_name() if t else None))
        for n in mel.get_static_switch_parameter_names(materiale):
            righe.append(rientro + "  B %s = %s" % (n, mel.get_material_instance_static_switch_parameter_value(materiale, n)))
    except Exception as e:
        righe.append(rientro + "  (parametri non letti: %s)" % e)


def testa(nome):
    bp_percorso = "/Game/Unpacked/MH_%s/BP_MH_%s" % (nome, nome)
    righe.append("")
    righe.append("=== " + nome)
    bp = unreal.EditorAssetLibrary.load_asset(bp_percorso)
    if bp is None:
        righe.append("  (non trovata)")
        return
    dettaglio = nome in DETTAGLIO
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    for h in sub.k2_gather_subobject_data_for_blueprint(bp):
        dato = lib.get_data(h)
        obj = lib.get_object(dato)
        if obj is None or not isinstance(obj, unreal.ActorComponent):
            continue
        righe.append("  [%s] %s" % (obj.get_class().get_name(), obj.get_name()))
        if isinstance(obj, unreal.SkeletalMeshComponent):
            mesh = prop(obj, "skeletal_mesh_asset") or prop(obj, "skeletal_mesh")
            righe.append("    mesh " + (mesh.get_path_name() if mesh else "nessuna"))
            for i, m in enumerate(prop(obj, "override_materials") or []):
                if m:
                    righe.append("    override %d:" % i)
                    if dettaglio:
                        parametri(m, "      ")
            if mesh and dettaglio:
                for slot in prop(mesh, "materials") or []:
                    righe.append("    slot " + str(prop(slot, "material_slot_name")))
                    parametri(prop(slot, "material_interface"), "      ")
        elif obj.get_class().get_name() == "GroomComponent":
            groom = prop(obj, "groom_asset")
            righe.append("    groom " + (groom.get_path_name() if groom else "nessuno"))
            for i, m in enumerate(prop(obj, "override_materials") or []):
                if m and dettaglio:
                    righe.append("    override %d:" % i)
                    parametri(m, "      ")
            if groom and dettaglio:
                for gm in prop(groom, "hair_groups_materials") or []:
                    righe.append("    slot " + str(prop(gm, "slot_name")))
                    parametri(prop(gm, "material"), "      ")


cartelle = unreal.EditorAssetLibrary.list_assets("/Game/Unpacked", recursive=False, include_folder=True)
nomi = sorted(set(c.rstrip("/").split("/")[-1][3:] for c in cartelle if c.rstrip("/").split("/")[-1].startswith("MH_")))
righe.append("Teste: " + ", ".join(nomi))
for n in nomi:
    try:
        testa(n)
    except Exception as e:
        righe.append("  ERRORE %s: %s" % (n, e))

os.makedirs(os.path.dirname(USCITA), exist_ok=True)
with open(USCITA, "w", encoding="utf-8") as f:
    f.write("\n".join(righe))
unreal.log("[Valdorso] Materiali teste: scritto " + USCITA)
