# Inventario per Claude
# Scrive in un file di testo l'elenco delle risorse del progetto (cartella, nome, tipo),
# cosi' Claude usa i nomi veri, anche dei pacchetti che non stanno su GitHub.
# Il file finisce in V:\Valdorso\Saved\Claude (esclusa da GitHub) e la cartella si apre da sola.
# Uso: menu Valdorso -> Inventario per Claude, oppure da Registro output in modalita' Python.

import datetime
import os
from collections import Counter, defaultdict

import unreal


def _tipo(dati):
    """Nome del tipo di risorsa (StaticMesh, Material, World...)."""
    try:
        return str(dati.asset_class_path.asset_name)
    except AttributeError:
        return str(dati.asset_class)


def esegui(cartella="/Game", apri_cartella=True):
    """Fa l'inventario di 'cartella' (di serie tutto il progetto) e restituisce il percorso del file."""
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    filtro = unreal.ARFilter(package_paths=[cartella], recursive_paths=True)
    tutte = registro.get_assets(filtro)

    # Le cartelle __ExternalActors__ e __ExternalObjects__ sono i pezzi delle mappe
    # (un file per oggetto): sono migliaia di nomi in codice, a Claude non servono.
    risorse = [d for d in tutte if "/__External" not in str(d.package_name)]

    per_cartella = defaultdict(list)
    per_tipo = Counter()
    for d in risorse:
        tipo = _tipo(d)
        per_cartella[str(d.package_path)].append((str(d.asset_name), tipo))
        per_tipo[tipo] += 1

    adesso = datetime.datetime.now()
    salvati = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
    destinazione = os.path.join(salvati, "Claude")
    os.makedirs(destinazione, exist_ok=True)
    percorso = os.path.join(destinazione, "inventario_" + adesso.strftime("%Y%m%d_%H%M") + ".txt")

    righe = []
    righe.append("INVENTARIO PER CLAUDE - Valdorso")
    righe.append("Data: " + adesso.strftime("%d/%m/%Y %H:%M"))
    righe.append("Unreal Engine: " + unreal.SystemLibrary.get_engine_version())
    righe.append("Cartella esaminata: " + cartella)
    righe.append("Risorse: " + str(len(risorse)) + " in " + str(len(per_cartella)) + " cartelle")
    righe.append("")
    righe.append("== Quante per tipo ==")
    for tipo, quante in per_tipo.most_common():
        righe.append("  " + tipo + ": " + str(quante))
    righe.append("")
    righe.append("== Elenco per cartella ==")
    for nome_cartella in sorted(per_cartella):
        elementi = sorted(per_cartella[nome_cartella])
        righe.append("")
        righe.append(nome_cartella + "  (" + str(len(elementi)) + ")")
        for nome, tipo in elementi:
            righe.append("  " + nome + "  [" + tipo + "]")

    with open(percorso, "w", encoding="utf-8") as file:
        file.write("\n".join(righe) + "\n")

    unreal.log("Inventario per Claude: " + str(len(risorse)) + " risorse scritte in " + percorso)
    if apri_cartella:
        os.startfile(destinazione)
    return percorso