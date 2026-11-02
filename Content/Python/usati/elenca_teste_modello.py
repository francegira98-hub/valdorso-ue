# Valdorso - Lettura (non cambia niente): le mesh dei volti nel plugin MetaHuman (la testa "modello" su cui sono fatti
# i capelli del Creator). Scrive in Saved/Claude/teste_modello.txt. Scritto da Claude il 29/10/2026.

import os
import unreal

USCITA = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude", "teste_modello.txt")
registro = unreal.AssetRegistryHelpers.get_asset_registry()
righe = []
for classe in ("SkeletalMesh", "StaticMesh"):
    filtro = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", classe)],
                             package_paths=["/MetaHumanCharacter", "/MetaHuman"], recursive_paths=True)
    for d in registro.get_assets(filtro):
        nome = str(d.package_name)
        if any(p in nome.lower() for p in ("face", "head", "archetype", "template", "base")):
            righe.append("%s  %s" % (classe, nome))
righe.sort()
os.makedirs(os.path.dirname(USCITA), exist_ok=True)
with open(USCITA, "w", encoding="utf-8") as f:
    f.write("\n".join(righe) or "(nessuna)")
unreal.log("[Valdorso] Teste modello: scritto " + USCITA)
