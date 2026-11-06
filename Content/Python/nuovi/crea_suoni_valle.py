# Valdorso - I suoni della valle e le campane del tempio (v0.1.3). Scritto da Claude il 06/11/2026.
# Si lancia in L_Valle (Strumenti -> Esegui script Python -> questo file), dopo crea_villaggio.py. Si può rilanciare.
#
# 1. Importa i suoni di V:/Texture/valdorso/suoni (fatti da Tools/Generatori/suoni_valle.py, da zero, senza licenze)
#    in /Game/Valle/Suoni; quelli che girano li mette "in giro" (Looping). Il vento, gli uccelli, i grilli, il fiume,
#    la riva e le rane li suona AValdorsoAmbiente (C++) intorno al colono: qui basta importarli.
# 2. Mette nella valle i suoni fermi (AValdorsoSorgenteSuono): la cascata, la fucina di Marta (in orario di lavoro),
#    la locanda di Bruno (piena di sera), il mulino, le rane della palude (di notte).
# 3. Mette la campana in cima al campanile del Tempio del Cuore (AValdorsoCampane): suona le ore canoniche.
# Le posizioni vengono dagli stessi dati del villaggio e della valle (Villaggio_3000.json, Luoghi_3000.json).
# Serve aver compilato Valdorso (le classi ValdorsoSorgenteSuono e ValdorsoCampane).

import json
import math
import os

import unreal

CARTELLA = "V:/Texture/valdorso/suoni"
DEST = "/Game/Valle/Suoni"
FILE_VILLAGGIO = "V:/Texture/valdorso/villaggio/Villaggio_3000.json"
FILE_LUOGHI = "V:/Texture/valdorso/valle/Luoghi_3000.json"
ETICHETTA = "ValdorsoSuoniValle"
QUOTA_MAX_CM = 80000.0
IN_GIRO = ["S_Vento", "S_Uccelli", "S_Grilli", "S_Rane", "S_Fiume", "S_Lago", "S_Cascata", "S_Fucina", "S_Locanda", "S_Mulino"]
UN_COLPO = ["S_CampanaTempio"]

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
avvisi = []


def avviso(testo):
    avvisi.append(testo)
    unreal.log_warning("[Valdorso] Suoni: " + testo)


def importa(file):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", file)
    c.set_editor_property("destination_path", DEST)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    strumenti.import_asset_tasks([c])


def importa_suoni():
    fatti = 0
    for nome in IN_GIRO + UN_COLPO:
        f = os.path.join(CARTELLA, nome + ".wav")
        if not os.path.exists(f):
            avviso("manca %s" % f)
            continue
        importa(f)
        s = unreal.load_asset("%s/%s" % (DEST, nome))
        if s is None:
            avviso("non importato: %s" % nome)
            continue
        s.set_editor_property("looping", nome in IN_GIRO)
        libreria.save_loaded_asset(s)
        fatti += 1
    unreal.log("[Valdorso] Suoni: importati %d suoni in %s" % (fatti, DEST))


def esegui():
    classe_sorgente = getattr(unreal, "ValdorsoSorgenteSuono", None)
    classe_campane = getattr(unreal, "ValdorsoCampane", None)
    if classe_sorgente is None or classe_campane is None:
        unreal.log_error("[Valdorso] Suoni: mancano le classi C++ ValdorsoSorgenteSuono / ValdorsoCampane (compilare Valdorso)")
        return
    importa_suoni()

    tutti = attori.get_all_level_actors()
    paesaggi = [a for a in tutti if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Suoni: nel livello aperto non c'è un Landscape. Apri L_Valle.")
        return
    angolo = paesaggi[0].get_actor_location()
    scala = paesaggi[0].get_actor_scale3d()

    def nel_mondo(x, y, quota_cm):
        valore = quota_cm / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + x, angolo.y + y, z)

    for a in tutti:
        if a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)

    with open(FILE_VILLAGGIO, "r", encoding="utf-8") as f:
        villaggio = {c["nome"]: c for c in json.load(f)["cose"]}
    with open(FILE_LUOGHI, "r", encoding="utf-8") as f:
        luoghi = json.load(f)

    def suono(nome):
        return unreal.load_asset("%s/%s" % (DEST, nome))

    def metti_sorgente(etichetta, dove, nome_suono, quando, vicino, lontano, volume=1.0, tace_vicino=False):
        s = suono(nome_suono)
        if s is None:
            avviso("%s: manca il suono %s" % (etichetta, nome_suono))
            return
        a = attori.spawn_actor_from_class(classe_sorgente, dove, unreal.Rotator(0, 0, 0))
        a.set_actor_label(etichetta)
        a.set_folder_path("Valle/Suoni")
        a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
        a.set_editor_property("suono", s)
        a.set_editor_property("quando", quando)
        a.set_editor_property("vicino", float(vicino))
        a.set_editor_property("lontano", float(lontano))
        a.set_editor_property("volume", float(volume))
        a.set_editor_property("tace_vicino", bool(tace_vicino))     # (06/11) le rane tacciono quando passi
        try:
            # con World Partition: sempre caricato (la cascata si sente da lontano)
            a.set_editor_property("is_spatially_loaded", False)
        except Exception:  # noqa: BLE001
            pass
        unreal.log("[Valdorso] Suoni: %s a (%.0f, %.0f, %.0f)" % (etichetta, dove.x, dove.y, dove.z))

    def da_villaggio(nome, alza=0.0):
        c = villaggio.get(nome)
        if c is None or "pos" not in c:
            avviso("nel villaggio non trovo %s" % nome)
            return None
        p = c["pos"]
        return nel_mondo(p[0], p[1], p[2] + alza)

    Q = unreal.ValdorsoQuando
    # la cascata: la pozza ai piedi del salto
    cas = luoghi.get("cascata")
    if cas:
        px, py = cas["pozza_m"]
        metti_sorgente("Suono della cascata", nel_mondo(px * 100.0, py * 100.0, cas.get("pelo_pozza_m", 0.0) * 100.0 + 800.0),
                       "S_Cascata", Q.SEMPRE, 2000, 30000)
    else:
        avviso("in Luoghi_3000.json non c'è la cascata")
    dove = da_villaggio("Luce della forgia", 0.0)
    if dove:
        metti_sorgente("Suono della fucina di Marta", dove, "S_Fucina", Q.LAVORO, 400, 4000)
    dove = da_villaggio("Locanda_di_Bruno", 200.0)
    if dove:
        metti_sorgente("Suono della locanda di Bruno", dove, "S_Locanda", Q.SERA, 600, 2500, 0.8)
    dove = da_villaggio("Mulino", 100.0)
    if dove:
        metti_sorgente("Suono del mulino", dove, "S_Mulino", Q.SEMPRE, 500, 4000)
    # le rane della palude: in mezzo alle cose della palude
    palude = [c["pos"] for c in villaggio.values() if c.get("sezione") == "Palude" and "pos" in c]
    if palude:
        mx = sum(p[0] for p in palude) / len(palude)
        my = sum(p[1] for p in palude) / len(palude)
        mz = min(p[2] for p in palude)
        metti_sorgente("Rane della palude", nel_mondo(mx, my, mz + 50.0), "S_Rane", Q.NOTTE, 1500, 6000, tace_vicino=True)
    else:
        avviso("nel villaggio non trovo la palude")

    # la campana: in cima al campanile (villaggio_edifici.tempio: campanile a (W/2 + 205, D/2 - 325) con W 1200, D 2200,
    # campana a 1550 + 120 cm dal pavimento), girata come il tempio
    t = villaggio.get("Tempio_del_Cuore")
    campana = suono("S_CampanaTempio")
    if t and campana:
        g = math.radians(t["gradi"])
        lx, ly = 1200.0 / 2 + 450.0 / 2 - 20.0, 2200.0 / 2 - 450.0 / 2 - 100.0
        x = t["pos"][0] + lx * math.cos(g) - ly * math.sin(g)
        y = t["pos"][1] + lx * math.sin(g) + ly * math.cos(g)
        dove = nel_mondo(x, y, t["pos"][2] + 1690.0)
        a = attori.spawn_actor_from_class(classe_campane, dove, unreal.Rotator(0, 0, 0))
        a.set_actor_label("Campane del Tempio del Cuore")
        a.set_folder_path("Valle/Suoni")
        a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
        a.set_editor_property("suono", campana)
        try:
            a.set_editor_property("is_spatially_loaded", False)
        except Exception:  # noqa: BLE001
            pass
        unreal.log("[Valdorso] Suoni: campane a (%.0f, %.0f, %.0f)" % (dove.x, dove.y, dove.z))
    else:
        avviso("non metto le campane (manca il tempio nel villaggio o il suono)")

    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    except Exception:  # noqa: BLE001
        pass
    unreal.log("[Valdorso] Suoni: FATTO (%d avvisi). In gioco: Valdorso.Campane.Prova, Valdorso.Ora 11.99 per mezzogiorno"
               % len(avvisi))


esegui()
