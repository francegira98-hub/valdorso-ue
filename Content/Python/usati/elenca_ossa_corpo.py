# Valdorso - Lettura (non cambia niente): tutte le ossa del CORPO MetaHuman, con genitore e posizione.
# Scrive Saved/Claude/ossa_corpo.txt e ossa_corpo.csv: Claude lo legge e sceglie le ossa di Corporatura e Muscoli.
# Scritto da Claude il 29/10/2026 (v0.1.2, passo 4.4, il corpo). Si lancia da Strumenti -> Esegui script Python.

import os
import unreal

CARTELLE = ["/Game/MetaHumans/MH_Prova", "/Game/MetaHumans", "/Game/Unpacked"]
USCITA = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")


def scrivi(testo):
    unreal.log("[Valdorso] Ossa corpo: " + testo)


def trova_corpo():
    """La prima SkeletalMesh del corpo (nome con 'body') nelle cartelle dei MetaHuman."""
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    for cartella in CARTELLE:
        filtro = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "SkeletalMesh")],
                                 package_paths=[cartella], recursive_paths=True)
        trovate = sorted(str(d.package_name) for d in registro.get_assets(filtro))
        for nome in trovate:
            if "body" in nome.lower().split("/")[-1]:
                mesh = unreal.EditorAssetLibrary.load_asset(nome)
                if mesh:
                    return mesh, trovate
    return None, []


def vettore(t):
    v = t.translation
    return "%.2f;%.2f;%.2f" % (v.x, v.y, v.z)


def principale():
    mesh, tutte = trova_corpo()
    if mesh is None:
        unreal.log_error("[Valdorso] Ossa corpo: non trovo la mesh del corpo in " + ", ".join(CARTELLE))
        return
    scheletro = mesh.get_editor_property("skeleton")
    scrivi("mesh " + mesh.get_path_name() + ", scheletro " + (scheletro.get_path_name() if scheletro else "?"))

    posa = unreal.AnimPoseExtensions.get_reference_pose(scheletro)
    nomi = unreal.AnimPoseExtensions.get_bone_names(posa)

    # I genitori: con lo strumento delle ossa, se c'è (plugin Skeletal Mesh Modeling Tools); se no restano vuoti.
    genitori = {}
    try:
        modifica = unreal.SkeletonModifier()
        modifica.set_skeletal_mesh(mesh)
        for n in nomi:
            genitori[str(n)] = str(modifica.get_parent_name(n))
    except Exception as errore:
        scrivi("genitori non disponibili (%s): Claude li ricava dalle posizioni" % errore)

    righe = ["osso,genitore,locale_xyz,mondo_xyz"]
    for n in nomi:
        locale = unreal.AnimPoseExtensions.get_bone_pose(posa, n, unreal.AnimPoseSpaces.LOCAL)
        mondo = unreal.AnimPoseExtensions.get_bone_pose(posa, n, unreal.AnimPoseSpaces.WORLD)
        righe.append("%s,%s,%s,%s" % (n, genitori.get(str(n), ""), vettore(locale), vettore(mondo)))

    os.makedirs(USCITA, exist_ok=True)
    with open(os.path.join(USCITA, "ossa_corpo.csv"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe))
    with open(os.path.join(USCITA, "ossa_corpo.txt"), "w", encoding="utf-8") as f:
        f.write("Mesh: %s\nScheletro: %s\nOssa: %d\n" % (mesh.get_path_name(),
                scheletro.get_path_name() if scheletro else "?", len(nomi)))
        f.write("Mesh 'body' trovate:\n  " + "\n  ".join(tutte) + "\n\n")
        f.write("\n".join(righe))
    scrivi("%d ossa, scritto %s" % (len(nomi), os.path.join(USCITA, "ossa_corpo.txt")))


principale()
