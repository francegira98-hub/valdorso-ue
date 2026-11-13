# Valdorso - I materiali pronti per gli oggetti ripetuti (13/11/2026, scritto da Claude dopo la prova della 0.2.1).
#
# Perché: nel registro del gioco pacchettizzato (Saved/Logs/Valdorso.log del launcher) Unreal scrive, per molti materiali
# del villaggio (MI_Villaggio_Coppi, _Pietra, _Trave, _Vetro, _Acqua...) e per M_Luminoso del menu:
#   "missing usage flag InstancedStaticMeshes! Default Material will be used in game"
# cioè: quando quel materiale sta su un oggetto ripetuto (istanze: pietre, coppi, assi messi tante volte), nel gioco
# pacchettizzato al suo posto si vede il materiale grigio di serie. Nell'editor non si nota, perché lì Unreal lo
# sistema da solo al volo.
#
# Cosa fa: in /Game/Valle e /Game/Menu cerca i materiali "veri" (Material) e, per ogni istanza (MI_), il materiale da
# cui viene (anche se sta altrove, per esempio un materiale di Fab scelto con le texture vere): le istanze prendono le
# spunte da lui. A chi non l'ha accende "Used with Instanced Static Meshes"; poi li salva.
# Scrive cosa ha fatto in Saved/Claude/sistema_materiali.txt.
# Si lancia dal menu Valdorso -> Strumenti -> "Pacchetto: sistema i materiali (oggetti ripetuti)", prima di Pacchettizza.
# Si può rilanciare: la seconda volta non cambia niente.

import os
import unreal

CARTELLE = ["/Game/Valle", "/Game/Menu"]
SPUNTE = ["used_with_instanced_static_meshes"]

RIGHE = []


def scrivi(testo):
    RIGHE.append(testo)
    unreal.log("[Valdorso] materiali: " + testo)


def principale():
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    libreria = unreal.EditorAssetLibrary
    mel = unreal.MaterialEditingLibrary
    trovati, cambiati, errori = 0, [], []
    da_guardare = {}
    for cartella in CARTELLE:
        filtro = unreal.ARFilter(package_paths=[cartella], recursive_paths=True,
                                 class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "Material"),
                                              unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant")])
        for dati in registro.get_assets(filtro):
            try:
                a = unreal.load_asset(str(dati.package_name))
                base = a if isinstance(a, unreal.Material) else (a.get_base_material() if a is not None else None)
                if base is not None:
                    da_guardare[base.get_path_name()] = base
            except Exception as e:  # noqa: BLE001
                errori.append("%s (%s)" % (dati.package_name, e))
    for percorso, m in sorted(da_guardare.items()):
        trovati += 1
        # i materiali del motore e dei plugin non si toccano (non si possono salvare): solo i nostri e quelli di Fab
        if not percorso.startswith("/Game/"):
            continue
        try:
            da_accendere = [s for s in SPUNTE if not m.get_editor_property(s)]
            if not da_accendere:
                continue
            for s in da_accendere:
                m.set_editor_property(s, True)
            mel.recompile_material(m)
            libreria.save_loaded_asset(m, only_if_is_dirty=False)
            cambiati.append(percorso)
        except Exception as e:  # noqa: BLE001
            errori.append("%s (%s)" % (percorso, e))
    scrivi("materiali guardati: %d (in %s)" % (trovati, ", ".join(CARTELLE)))
    scrivi("sistemati: %d" % len(cambiati))
    for p in cambiati:
        scrivi("  " + p)
    for e in errori:
        scrivi("ATTENZIONE: " + e)
    scrivi("FATTO. Poi: Tools\\Pacchettizza Valdorso.cmd (i materiali nuovi entrano nel pacchetto)")


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "sistema_materiali.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(RIGHE) + "\n")


try:
    principale()
finally:
    salva_resoconto()
