# Valdorso - La valle si vede anche da lontano (v0.1.4, 10/11/2026). Scritto da Claude, chiesto da Fra: "da play in
# lontananza si vede malissimo".
# Si lancia in L_Valle dal menu Valdorso -> Villaggio -> "Valle: si vede anche da lontano" (o Strumenti -> Esegui script).
# Si puo' rilanciare. Alla fine salva. Resoconto in Saved/Claude/lontananza.txt.
#
# Perche': la valle e' un livello con World Partition. In gioco si caricano solo i pezzi vicini al colono (la "distanza di
# caricamento" della griglia); quello che sta piu' in la' si vede solo se ci sono gli HLOD (i modelli semplici da lontano),
# e gli HLOD non sono mai stati costruiti. Cosi' da lontano mancavano pezzi di terreno, l'acqua, le rocce grandi e le case:
# restavano gli alberi (che sono sempre caricati) sospesi nel vuoto.
# Cosa fa:
#   1. Il terreno (Landscape e i suoi pezzi, LandscapeStreamingProxy) sempre caricato: i monti si vedono da ogni punto.
#   2. L'acqua (lago, fiumi, la zona dell'acqua) sempre caricata.
#   3. Le cose grandi (piu' di 25 m: rocce, torri, il tempio, i muri) sempre caricate: si vedono da lontano.
#   4. La distanza di caricamento della griglia principale portata ad almeno 512 m (se il livello la espone).
# PRIMA: nella finestra World Partition carica tutta la valle (seleziona tutta la griglia, tasto destro -> Carica regione
# dalla selezione): lo script cambia solo i pezzi caricati nell'editor.
# Dopo, per il resto (le case piccole, gli oggetti): menu Build -> Build HLODs (una volta; ci mette un po').
# Nel gioco c'e' anche Impostazioni -> Distanza visiva (di serie "Lontana").

import os

import unreal

GRANDE_CM = 2500.0          # oltre questa misura (larghezza o altezza) una cosa resta sempre caricata
RAGGIO_MINIMO = 51200       # cm: la distanza di caricamento della griglia principale, almeno

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Lontananza: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Lontananza: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "lontananza.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))


def sempre_caricato(a):
    """Vero se l'ha cambiato (era caricato a pezzi)."""
    try:
        if a.get_editor_property("is_spatially_loaded"):
            a.modify()
            a.set_editor_property("is_spatially_loaded", False)
            return True
    except Exception as e:  # noqa: BLE001
        avviso("%s: non riesco a cambiarlo (%s)" % (a.get_actor_label(), e))
    return False


def griglia(mondo):
    """La distanza di caricamento della griglia principale: almeno RAGGIO_MINIMO (se il livello la espone a Python)."""
    try:
        impostazioni = mondo.get_world_settings()
        wp = impostazioni.get_editor_property("world_partition")
        if wp is None:
            avviso("il livello non ha World Partition: niente da fare per la griglia")
            return
        hash_ = wp.get_editor_property("runtime_hash")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a leggere le impostazioni di World Partition (%s): cambia a mano in Impostazioni del mondo -> "
               "World Partition -> Runtime Settings -> Loading Range della griglia principale (almeno %d)" % (e, RAGGIO_MINIMO))
        return
    if hash_ is None:
        avviso("World Partition senza runtime hash")
        return
    nome_classe = hash_.get_class().get_name()
    cambiati = 0
    try:
        if "SpatialHash" in nome_classe:
            griglie = list(hash_.get_editor_property("grids"))
            for i, g in enumerate(griglie):
                vecchio = g.get_editor_property("loading_range")
                if i == 0 and vecchio < RAGGIO_MINIMO:
                    g.set_editor_property("loading_range", RAGGIO_MINIMO)
                    griglie[i] = g
                    cambiati += 1
                    scrivi("griglia %s: distanza di caricamento da %d a %d cm" % (g.get_editor_property("grid_name"), vecchio, RAGGIO_MINIMO))
                else:
                    scrivi("griglia %s: distanza di caricamento %d cm (resta)" % (g.get_editor_property("grid_name"), vecchio))
            if cambiati:
                hash_.modify()
                hash_.set_editor_property("grids", griglie)
        elif "HashSet" in nome_classe:
            parti = list(hash_.get_editor_property("runtime_partitions"))
            for i, p in enumerate(parti):
                strato = p.get_editor_property("main_layer")
                if strato is None:
                    continue
                vecchio = strato.get_editor_property("loading_range")
                if i == 0 and vecchio < RAGGIO_MINIMO:
                    strato.modify()
                    strato.set_editor_property("loading_range", RAGGIO_MINIMO)
                    cambiati += 1
                    scrivi("partizione %s: distanza di caricamento da %d a %d cm" % (p.get_editor_property("name"), vecchio, RAGGIO_MINIMO))
                else:
                    scrivi("partizione %s: distanza di caricamento %d cm (resta)" % (p.get_editor_property("name"), vecchio))
        else:
            avviso("runtime hash %s che non conosco: cambia a mano la distanza di caricamento (almeno %d)" % (nome_classe, RAGGIO_MINIMO))
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a cambiare la griglia (%s, %s): cambia a mano in Impostazioni del mondo -> World Partition -> "
               "Runtime Settings -> Loading Range (almeno %d)" % (nome_classe, e, RAGGIO_MINIMO))


def in_un_livello_istanza(a):
    """Vero se l'attore sta dentro un Level Instance (non e' del livello principale: non lo tocco)."""
    try:
        return "LevelInstance" in a.get_outer().get_path_name()
    except Exception:  # noqa: BLE001
        return False


def cambia_tutti(tutti):
    terreno = acqua = grandi = 0
    for a in tutti:
        if in_un_livello_istanza(a):
            continue
        nome = a.get_class().get_name()
        if isinstance(a, unreal.LandscapeProxy):
            terreno += 1 if sempre_caricato(a) else 0
        elif nome.startswith("WaterBody") or nome == "WaterZone":
            acqua += 1 if sempre_caricato(a) else 0
        elif isinstance(a, unreal.StaticMeshActor):
            try:
                _, misura = a.get_actor_bounds(False)
            except Exception:  # noqa: BLE001
                continue
            if max(misura.x, misura.y) * 2.0 > GRANDE_CM or misura.z * 2.0 > GRANDE_CM:
                if sempre_caricato(a):
                    grandi += 1
    return terreno, acqua, grandi


def esegui():
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    tutti = attori.get_all_level_actors()
    if not any(isinstance(a, unreal.LandscapeProxy) for a in tutti):
        unreal.log_error("[Valdorso] Lontananza: nel livello aperto non c'e' un Landscape. Apri L_Valle.")
        return
    with unreal.ScopedEditorTransaction("Valdorso: la valle si vede da lontano"):
        terreno, acqua, grandi = cambia_tutti(tutti)
        griglia(mondo)
    pezzi = sum(1 for a in tutti if isinstance(a, unreal.LandscapeProxy))
    scrivi("terreno: %d pezzi nel livello caricato, %d ora sempre caricati" % (pezzi, terreno))
    if pezzi < 4:
        avviso("pochi pezzi di terreno caricati: carica tutta la valle nella finestra World Partition e rilancia")
    scrivi("acqua: %d ora sempre caricate" % acqua)
    scrivi("cose grandi (oltre %d m): %d ora sempre caricate" % (int(GRANDE_CM / 100), grandi))
    scrivi("FATTO. Per le cose piccole lontane: Build -> Build HLODs (una volta, ci mette un po'). In gioco: Impostazioni -> "
           "Distanza visiva.")
    salva_resoconto()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


esegui()
