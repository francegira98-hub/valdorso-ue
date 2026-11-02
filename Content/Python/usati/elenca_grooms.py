# Valdorso - Lettura (non cambia niente): tutti i capelli, barbe e sopracciglia (groom) che ci sono, i legami
# (binding) delle teste e le funzioni per creare legami nuovi. Scrive in Saved/Claude/grooms.txt.
# Scritto da Claude il 29/10/2026.

import os
import unreal

USCITA = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude", "grooms.txt")
righe = []
registro = unreal.AssetRegistryHelpers.get_asset_registry()


def prop(oggetto, nome):
    try:
        return oggetto.get_editor_property(nome)
    except Exception as e:
        return "(? %s)" % e


def elenca(classe, cartelle):
    filtro = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/HairStrandsCore", classe)],
                             package_paths=cartelle, recursive_paths=True)
    return sorted(str(d.package_name) for d in registro.get_assets(filtro))


righe.append("== Groom nel plugin MetaHuman e nel progetto")
for p in elenca("GroomAsset", ["/MetaHumanCharacter", "/Game"]):
    righe.append("   " + p)

righe.append("")
righe.append("== Legami (binding)")
for p in elenca("GroomBindingAsset", ["/Game"]):
    b = unreal.EditorAssetLibrary.load_asset(p)
    g = prop(b, "groom")
    t = prop(b, "target_skeletal_mesh")
    s = prop(b, "source_skeletal_mesh")
    nome = lambda x: x.get_path_name() if hasattr(x, "get_path_name") else str(x)
    righe.append("   %s\n      groom %s\n      su %s\n      da %s\n      tipo %s  punti %s" % (
        p, nome(g), nome(t), nome(s), prop(b, "group_infos") and "" or "", prop(b, "num_interpolation_points")))

righe.append("")
righe.append("== Funzioni per i groom (Python)")
for nome in dir(unreal):
    if "groom" in nome.lower():
        classe = getattr(unreal, nome)
        righe.append("   " + nome)
        for m in sorted(x for x in dir(classe) if not x.startswith("_")):
            if m.startswith(("create", "build", "set_", "get_")) or "binding" in m:
                doc = ((getattr(getattr(classe, m, None), "__doc__", "") or "").strip().splitlines() or [""])[0]
                righe.append("      .%s  %s" % (m, doc[:200]))

lib = getattr(unreal, "GroomBlueprintLibrary", None)
for m in ("create_new_groom_binding_asset_with_path", "create_new_groom_binding_asset"):
    f = getattr(lib, m, None) if lib else None
    righe.append("")
    righe.append("== GroomBlueprintLibrary." + m)
    righe.append((getattr(f, "__doc__", "") or "(non c'è)").strip())

os.makedirs(os.path.dirname(USCITA), exist_ok=True)
with open(USCITA, "w", encoding="utf-8") as f:
    f.write("\n".join(righe))
unreal.log("[Valdorso] Grooms: scritto " + USCITA)
