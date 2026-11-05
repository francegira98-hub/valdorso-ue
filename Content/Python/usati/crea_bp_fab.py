# Valdorso - Mette insieme i modelli di Fab arrivati a pezzi in un Blueprint per pacchetto. Scritto da Claude il 05/11/2026.
# Al posto di sistema_fab.py, che ha fatto chiudere Unreal: l'unione dei pezzi in una mesh sola (Merge Actors) va in
# crash con questi modelli. Qui invece ogni pacchetto diventa BP_<Nome>: un attore con dentro tutti i suoi pezzi, ognuno
# già al suo posto (le posizioni sono nei pezzi stessi), e una scala unica per i pacchetti con la misura sbagliata.
# Si trascina BP_<Nome> nel livello come un modello qualsiasi. Si può rilanciare: rifà i Blueprint (quelli ancora nel
# livello non si possono cancellare e restano come sono).
# Per controllare le misure, ogni Blueprint si mette un attimo nel livello e si toglie: il livello non va salvato.
# Resoconto in Saved/Claude/crea_bp_fab.txt.

import os
import re
import unreal

# (cartella in Content/Fab, scala)
PACCHETTI = [
    ("Casa_Camino", 1.0),
    ("Mercato_Tenda1", 1.0),
    ("Letto", 0.007),          # il file era in metri letti come centimetri; 0,007 scelto a occhio accanto al colono (05/11)
    ("Case_Medievali", 10.0),  # 10 volte troppo piccola
    # il letto rosso a baldacchino, per le case dei nobili: più grande e allungato (Y è la lunghezza del modello, 05/11)
    ("Letto2", (1.2, 1.6, 1.2)),
]

# (05/11) I letti sono figli di AValdorsoLetto (C++, ValdorsoLetto.h): ci si dorme con E. Valori di partenza, da
# sistemare a occhio con i comandi Valdorso.Letto.* e poi da scrivere qui.
# (05/11) Il verso e l'altezza del corpo sdraiato stanno nel C++ (ValdorsoLetto.cpp): valgono per tutti i letti.
LETTI = {
    "Letto": dict(posti=[0.0], altezza_materasso=42.0, distanza_uscita=115.0),
    "Letto2": dict(posti=[-55.0, 55.0], altezza_materasso=55.0, distanza_uscita=170.0),
}

righe = []
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
oggetti_sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
mesh_sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
attori_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
dati = unreal.SubobjectDataBlueprintFunctionLibrary


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Fab: " + testo)


def pezzi_di(cartella):
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    trovati = []
    for dato in registro.get_assets_by_path(cartella, recursive=True):
        if str(dato.asset_class_path.asset_name) != "StaticMesh":
            continue
        nome = str(dato.asset_name)
        if nome.endswith("_Intera"):
            continue
        trovati.append(unreal.load_asset(str(dato.package_name)))
    return trovati


def scala_di_costruzione_a_uno(mesh):
    """sistema_fab.py aveva provato a cambiare la scala dentro la mesh: si rimette a 1 (la scala ora è nel Blueprint)."""
    impostazioni = mesh_sub.get_lod_build_settings(mesh, 0)
    attuale = impostazioni.get_editor_property("build_scale3d")
    if abs(attuale.x - 1.0) > 1e-6 or abs(attuale.y - 1.0) > 1e-6 or abs(attuale.z - 1.0) > 1e-6:
        impostazioni.set_editor_property("build_scale3d", unreal.Vector(1.0, 1.0, 1.0))
        mesh_sub.set_lod_build_settings(mesh, 0, impostazioni)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        scrivi("   %s: scala di costruzione rimessa a 1" % mesh.get_name())


def aggiungi(blueprint, genitore, classe, nome):
    parametri = unreal.AddNewSubobjectParams(parent_handle=genitore, new_class=classe, blueprint_context=blueprint)
    maniglia, motivo = oggetti_sub.add_new_subobject(parametri)
    if str(motivo):
        scrivi("   non riesco ad aggiungere %s: %s" % (nome, motivo))
        return None, None
    oggetti_sub.rename_subobject(maniglia, unreal.Text(nome))
    return maniglia, dati.get_object(dati.get_data(maniglia))


def crea(nome, scala):
    # La scala può essere un numero (uguale sui tre assi) o tre numeri (X, Y, Z).
    sx, sy, sz = (scala, scala, scala) if isinstance(scala, (int, float)) else scala
    cartella = "/Game/Fab/" + nome
    percorso = "%s/BP_%s" % (cartella, nome)
    if unreal.EditorAssetLibrary.does_asset_exist(percorso):
        # Seconda versione (05/11): i Blueprint si rifanno, centrati. Prima togliere gli esemplari dal livello.
        if not unreal.EditorAssetLibrary.delete_asset(percorso):
            scrivi("%s: non riesco a cancellare il vecchio BP_%s (è ancora nel livello?)" % (nome, nome))
            return None
    pezzi = pezzi_di(cartella)
    if not pezzi:
        scrivi("%s: nessun pezzo trovato in %s" % (nome, cartella))
        return None
    for mesh in pezzi:
        scala_di_costruzione_a_uno(mesh)

    fabbrica = unreal.BlueprintFactory()
    classe_letto = getattr(unreal, "ValdorsoLetto", None)
    if nome in LETTI and classe_letto is None:
        scrivi("%s: manca ValdorsoLetto nel C++ (compilare prima): il letto nasce come attore semplice" % nome)
    fabbrica.set_editor_property("parent_class", classe_letto if (nome in LETTI and classe_letto) else unreal.Actor)
    blueprint = strumenti.create_asset("BP_" + nome, cartella, unreal.Blueprint, fabbrica)
    radice = oggetti_sub.k2_gather_subobject_data_for_blueprint(blueprint)[0]
    maniglia_scala, componente_scala = aggiungi(blueprint, radice, unreal.SceneComponent, "Scala")
    if componente_scala is None:
        return None
    componente_scala.set_editor_property("relative_scale3d", unreal.Vector(sx, sy, sz))
    # (05/11) I pezzi hanno le posizioni del file originale, anche a decine di metri dal punto 0: il Blueprint si
    # centra, con il centro in pianta sul suo perno e la parte più bassa a terra.
    minimo, massimo = None, None
    for mesh in pezzi:
        scatola = mesh.get_bounding_box()
        if minimo is None:
            minimo, massimo = scatola.min, scatola.max
        else:
            minimo = unreal.Vector(min(minimo.x, scatola.min.x), min(minimo.y, scatola.min.y), min(minimo.z, scatola.min.z))
            massimo = unreal.Vector(max(massimo.x, scatola.max.x), max(massimo.y, scatola.max.y), max(massimo.z, scatola.max.z))
    spostamento = unreal.Vector(-(minimo.x + massimo.x) / 2 * sx, -(minimo.y + massimo.y) / 2 * sy, -minimo.z * sz)
    componente_scala.set_editor_property("relative_location", spostamento)
    scrivi("%s: pezzi da (%.0f, %.0f, %.0f) a (%.0f, %.0f, %.0f) cm, spostati di (%.0f, %.0f, %.0f)" % (
        nome, minimo.x, minimo.y, minimo.z, massimo.x, massimo.y, massimo.z, spostamento.x, spostamento.y, spostamento.z))
    usati = set()
    for mesh in pezzi:
        base = re.sub(r"[^A-Za-z0-9_]", "_", mesh.get_name())[:60]
        nome_pezzo, n = base, 1
        while nome_pezzo in usati:
            n += 1
            nome_pezzo = "%s_%d" % (base, n)
        usati.add(nome_pezzo)
        _, componente = aggiungi(blueprint, maniglia_scala, unreal.StaticMeshComponent, nome_pezzo)
        if componente is not None:
            componente.set_editor_property("static_mesh", mesh)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if nome in LETTI and classe_letto:
        predefinito = unreal.get_default_object(blueprint.generated_class())
        for chiave, valore in LETTI[nome].items():
            predefinito.set_editor_property(chiave, valore)
        scrivi("%s: letto con %s" % (nome, {k: str(v) for k, v in LETTI[nome].items()}))
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    scrivi("%s: creato BP_%s con %d pezzi, scala %s" % (nome, nome, len(pezzi), scala))
    return blueprint


def misura(nome, blueprint):
    classe = unreal.EditorAssetLibrary.load_blueprint_class(blueprint.get_path_name())
    attore = attori_sub.spawn_actor_from_class(classe, unreal.Vector(0, 0, 100000), unreal.Rotator(0, 0, 0))
    if attore is None:
        return
    origine, mezzo = attore.get_actor_bounds(False)
    scrivi("   %s nel livello: %.0f x %.0f x %.0f cm" % (nome, mezzo.x * 2, mezzo.y * 2, mezzo.z * 2))
    attori_sub.destroy_actor(attore)


def principale():
    for nome, scala in PACCHETTI:
        blueprint = crea(nome, scala)
        if blueprint is not None:
            misura(nome, blueprint)
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "crea_bp_fab.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))
    scrivi("Fatto: resoconto in Saved/Claude/crea_bp_fab.txt (il livello non va salvato)")


principale()
