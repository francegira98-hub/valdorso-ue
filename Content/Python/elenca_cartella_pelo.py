# Valdorso - Lettura (non cambia niente): cosa c'è nella cartella di un taglio della libreria MetaHuman e su quale
# testa è fatto. Scrive in Saved/Claude/cartella_pelo.txt. Scritto da Claude il 29/10/2026.

import os
import unreal

USCITA = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude", "cartella_pelo.txt")
CARTELLE = ["/MetaHumanCharacter/Optional/Grooms/GroomAssets/Hair/Hair_S_Updo",
            "/MetaHumanCharacter/Optional/Grooms/GroomAssets/Hair/Hair_S_Casual",
            "/MetaHumanCharacter/Optional/Grooms/GroomAssets/Beards/Beard_L_Full"]
SALTA = ("Texture", "StaticMesh", "Material")
righe = []


def valore(v):
    if hasattr(v, "get_path_name"):
        return v.get_path_name()
    testo = str(v)
    return testo if len(testo) < 300 else testo[:300] + "..."


for cartella in CARTELLE:
    righe.append("")
    righe.append("=== " + cartella)
    for percorso in unreal.EditorAssetLibrary.list_assets(cartella, recursive=True):
        dati = unreal.EditorAssetLibrary.find_asset_data(percorso)
        classe = str(dati.asset_class_path.asset_name)
        righe.append("  [%s] %s" % (classe, percorso))
        if any(s in classe for s in SALTA):
            continue
        oggetto = unreal.EditorAssetLibrary.load_asset(percorso)
        if oggetto is None:
            continue
        for nome in sorted(dir(oggetto)):
            if nome.startswith("_"):
                continue
            basso = nome.lower()
            if not any(k in basso for k in ("mesh", "bind", "source", "head", "fit", "skel", "body", "groom", "lod", "rig")):
                continue
            try:
                v = oggetto.get_editor_property(nome)
            except Exception:
                continue
            righe.append("      %s = %s" % (nome, valore(v)))

os.makedirs(os.path.dirname(USCITA), exist_ok=True)
with open(USCITA, "w", encoding="utf-8") as f:
    f.write("\n".join(righe))
unreal.log("[Valdorso] Cartella pelo: scritto " + USCITA)
