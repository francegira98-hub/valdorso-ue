# Valdorso - Lettura (non cambia niente): come chiedere da Python il rig e le texture di un MetaHuman.
# Scrive in Saved/Claude/api_rig.txt. Scritto da Claude il 28/10/2026.

import os
import unreal

USCITA = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude", "api_rig.txt")
righe = []


def doc(oggetto, nome):
    righe.append("")
    righe.append("== " + nome)
    if oggetto is None:
        righe.append("   (non esiste)")
        return
    testo = (getattr(oggetto, "__doc__", "") or "").strip()
    righe.extend("   " + r for r in testo.splitlines()[:60])


sotto = getattr(unreal, "MetaHumanCharacterEditorSubsystem", None)
for funzione in ("request_auto_rigging", "request_texture_sources", "can_build_meta_human", "remove_face_rig",
                 "build_meta_human", "commit_face_state"):
    doc(getattr(sotto, funzione, None), "MetaHumanCharacterEditorSubsystem." + funzione)

for nome in dir(unreal):
    basso = nome.lower()
    if ("rig" in basso or "texture" in basso or "request" in basso) and "metahuman" in basso:
        classe = getattr(unreal, nome)
        righe.append("")
        righe.append("== " + nome)
        for membro in sorted(m for m in dir(classe) if not m.startswith("_")):
            if membro in ("assign", "cast", "copy", "export_text", "import_text", "get_editor_property",
                          "set_editor_property", "set_editor_properties", "static_struct", "to_dict", "to_tuple",
                          "static_enum", "get_display_name"):
                continue
            valore = getattr(classe, membro, None)
            primo = ((getattr(valore, "__doc__", "") or "").strip().splitlines() or [""])[0]
            righe.append("   .%s  %s" % (membro, primo[:160]))

# Lo stato dei personaggi che ci sono (rig e texture), per capire cosa si legge.
registro = unreal.AssetRegistryHelpers.get_asset_registry()
filtro = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/MetaHumanCharacter", "MetaHumanCharacter")], recursive_classes=True)
righe.append("")
righe.append("== Personaggi nel progetto")
for dato in registro.get_assets(filtro):
    righe.append("   " + str(dato.package_name))

os.makedirs(os.path.dirname(USCITA), exist_ok=True)
with open(USCITA, "w", encoding="utf-8") as f:
    f.write("\n".join(righe))
unreal.log("[Valdorso] API rig: scritto " + USCITA)
