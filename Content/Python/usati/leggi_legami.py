# Valdorso - Legge i "legami" (groom binding) delle sopracciglia: quelli fatti dal Creator per ogni testa (che funzionano)
# e i nostri in /Game/Peli. Scrive da quale testa modello partono e con quanti punti, per capire perche' alcune
# sopracciglia finiscono fuori posto. Solo lettura. Scritto da Claude il 02/11/2026.
# Risultato in Saved/Claude/legami_sopracciglia.txt (e nel Registro output).

import os
import unreal

CAMPI = ["groom", "source_skeletal_mesh", "target_skeletal_mesh", "num_interpolation_points", "matching_section",
         "groom_binding_type"]


def leggi(percorso):
    legame = unreal.EditorAssetLibrary.load_asset(percorso)
    if legame is None:
        return "  (non si apre)"
    righe = []
    for campo in CAMPI:
        try:
            valore = legame.get_editor_property(campo)
            if isinstance(valore, unreal.Object):
                valore = valore.get_path_name()
            righe.append("  %s = %s" % (campo, valore))
        except Exception as problema:
            righe.append("  %s = ? (%s)" % (campo, str(problema)[:60]))
    return "\n".join(righe)


def principale():
    righe = []
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    # 1. I legami del Creator (in /Game/Unpacked e /Game/MetaHumans): solo sopracciglia e capelli, per confronto.
    for radice in ("/Game/Unpacked", "/Game/MetaHumans"):
        for percorso in unreal.EditorAssetLibrary.list_assets(radice, recursive=True, include_folder=False):
            nome = percorso.split("/")[-1]
            if "Binding" in nome and ("Eyebrows" in nome or "Hair" in nome):
                righe.append("== CREATOR " + percorso.split(".")[0])
                righe.append(leggi(percorso))
    # 2. I nostri legami delle sopracciglia (alcune teste).
    for testa in ("Omari", "Prova", "Sunita", "Grace"):
        for percorso in unreal.EditorAssetLibrary.list_assets("/Game/Peli/" + testa, recursive=False, include_folder=False):
            if "Eyebrows" in percorso:
                righe.append("== NOSTRO " + percorso.split(".")[0])
                righe.append(leggi(percorso))
    testo = "\n".join(righe)
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "legami_sopracciglia.txt"), "w", encoding="utf-8") as file:
        file.write(testo)
    unreal.log("[Valdorso] Legami letti: %d righe in Saved/Claude/legami_sopracciglia.txt" % len(righe))


principale()
