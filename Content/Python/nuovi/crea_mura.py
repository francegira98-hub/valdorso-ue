# Valdorso - Le mura da castello e il tempio che cresce (16/11/2026, scritto da Claude; v0.2.6 "Le mura" e v0.2.7
# "Il tempio", approvate da Fra il 15/11). Mette nel livello i pezzi fatti dalla chat 3D (V:/Texture/valdorso/mura:
# 36 .glb, Valdorso_mura_tempio.json con le schede, posizioni_mura_valdorso.json con le 197 posizioni delle mura).
#
# Voci nel menu Valdorso -> Villaggio:
#   "Mura: 1. prova degli assi"           importa solo SM_Prova_Assi e lo mette davanti alla telecamera: la freccia deve
#                                          guardare +Y, il cubetto di ferro +X. Lo script lo controlla da solo e lo scrive.
#   "Mura: 2. costruisci le mura"          importa tutti i pezzi, spegne le mura vecchie della fase 3 (il muro di cinta,
#                                          le due torri di ogni porta, gli archi, le torri d'angolo 8 e 9: non si
#                                          cancellano, si spengono) e mette le mura nuove: tratti, snodi, 17 torri, 4 torri
#                                          della porta con le ante (AValdorsoPorta), la grata (AValdorsoGrata), la leva con
#                                          il lucchetto (AValdorsoLeva, Chiave_Porta) e l'argano; in ogni torre le
#                                          porticine (quella a terra chiusa con la Chiave_Torri, quelle sul cammino aperte).
#                                          E la stalla del mulo al posto di guardia del passo.
#   "Mura: 3. il tempio che cresce"        il Tempio del Cuore: fase 1 la cappella di oggi, fase 2 la chiesa con il
#                                          campanile, fase 3 la pieve (nello stesso punto, la porta verso la piazza).
#   "Mura: torna come prima"               toglie tutto il nuovo e riaccende il vecchio (mura, torri, cappella).
#   "Mura: 5. il recinto della caserma"    (16/11 sera, deciso da Fra) il muretto di pietra intorno alla caserma della
#                                          milizia (fase 3): portale a ovest con la grata piccola (giorno e notte), lo
#                                          stemma, due lanterne, il corpo di guardia con la leva (porta con la
#                                          Chiave_Caserma), campanella, garitta, rastrelliera e bersagli.
#   "Mura: 4. strade alle porte"           (16/11 sera) SM_Fase2_Strade_Porte (fasi 2-0) e la maschera senza erba.
# (16/11 sera) Con la terza e quarta consegna della chat 3D: le Z di ogni pezzo da posizioni_mura_valdorso_Z.json
# (cammino continuo, tratti con la scala, torri di raccordo), schede_pezzi.json, i gradini davanti alle porticine e le
# rampe delle porte (posizioni_gradini_e_soglie.json), le lanterne con la luce (Valdorso_lanterne.json).
# Tutti i pezzi nuovi delle mura sono della fase 3 (etichetta "ValdorsoFase 3-0"); il tempio ha le sue fasi.
# Se Unreal specchia la Y (come con la grotta del nord), lo si vede dalla prova degli assi e si corregge da solo
# (scala Y = -1 sui modelli). Le Z: dal terreno, con il pezzo affondato di 40 cm (la scarpa copre i salti).
# Resoconto in Saved/Claude/mura_rapporto_<voce>.txt (assi, mura, tempio, indietro). Dopo: "PNG: panche, letti e porte" (i PNG imparano le porte nuove).

import json
import math
import os
import re

import unreal

MODO = str(globals().get("MURA_MODO", "assi"))

CARTELLA = "V:/Texture/valdorso/mura"
PEZZI = "/Game/Valle/Mura/Pezzi"
MATERIALI = "/Game/Valle/Villaggio/Materiali"
ETICHETTA = "ValdorsoMuraNuove"
SPENTA = "ValdorsoMuraVecchiaSpenta"
TEMPIO_CAMBIATO = "ValdorsoTempioCambiato"
CONTORNO = "Valle/Mura"
AFFONDA = 40.0
FASE_MURA = "ValdorsoFase 3-0"
# le cose vecchie della fase 3 che le mura nuove sostituiscono (per nome nel livello)
# (16/11 sera, decise da Fra) le mura BASSE della fase 2 (chat 3D: Valdorso_mura_fase2.json, stessi posti e stesse Z
# della fase 3): al posto della palizzata larga, dei suoi portali e delle lanterne di legno delle porte larghe
MURA2_DATI = "Valdorso_mura_fase2.json"
FASE_MURA2 = "ValdorsoFase 2-2"
FASE_DA_2 = "ValdorsoFase 2-0"          # le ante e le rampe delle porte: uguali nelle fasi 2 e 3
VECCHIE_FASE2 = ("Palizzata larga", "Portale largo", "Lanterne delle porte larghe", "Luce lanterna larga")
VECCHIE = ("Muro di cinta", "Torre_della_porta_", "Porta di pietra", "Torre_d_angolo_8", "Torre_d_angolo_9")
# i materiali del villaggio per gli slot dei pezzi
SLOT = {"pietra": "Pietra", "legno": "Trave", "ferro": "Ferro", "coppi": "Coppi", "paglia": "Paglia",
        "intonaco": "Intonaco"}
# (16/11 sera) la lanterna da muro ha anche il vetro (trasparente) e la candela (che fa luce): tengono il loro materiale
SLOT_PROPRI = ("vetro", "luce")
# (16/11 sera) la terza e la quarta consegna della chat 3D (mura sul pendio): se ci sono, si usano queste
POSIZIONI_Z = "posizioni_mura_valdorso_Z.json"      # le Z vere di ogni pezzo (cammino continuo, scale, raccordi)
SCHEDE_KIT = "schede_pezzi.json"                     # le schede di tutti i pezzi del kit (raccordi, gradini...)
GRADINI_SOGLIE = "posizioni_gradini_e_soglie.json"   # i gradini davanti alle porticine e le rampe delle 4 porte
LANTERNE = "Valdorso_lanterne.json"                  # i punti delle lanterne di ogni torre
# le luci delle lanterne (330 nelle mura): senza ombre e corte, per non pesare; quelle dentro le torri piu' corte
LUCE_COLORE = (1.0, 0.62, 0.32)
LUCE_INTENSITA = 3.0          # candele
LUCE_RAGGIO_FUORI = 700.0
LUCE_RAGGIO_DENTRO = 400.0
LANTERNE_CON_LUCE = True      # False: solo le lanterne, senza le luci (se la sera il gioco rallenta)

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(t):
    righe.append(t)
    unreal.log("[Valdorso] Mura: " + t)


def avviso(t):
    righe.append("ATTENZIONE: " + t)
    unreal.log_warning("[Valdorso] Mura: " + t)


# ------------------------------------------------------------------------------------------------ importare

def compito(f, dest):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", f)
    c.set_editor_property("destination_path", dest)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    return c


def cerca_e_metti_a_posto(nome, cartella=None, importato=True, sposta=True):
    """L'importazione dei .glb (5.8) mette il modello in <cartella>/<nome>/StaticMeshes/<nome>: lo si sposta in
    <cartella>/<nome> (come per la grotta del nord).
    (16/11 sera, errore di Claude corretto) Se <cartella>/<nome> c'era gia' (un'importazione di prima), il modello appena
    importato restava nella sottocartella e lo script usava quello VECCHIO. Ora il nuovo prende il posto del vecchio:
    tutto cio' che usava il vecchio passa al nuovo (consolidate), poi il nuovo si sposta al posto giusto."""
    # (secondo controllo, 16/11 sera) solo se il .glb e' stato importato davvero in questo giro: altrimenti una copia
    # vecchia rimasta in una sottocartella prenderebbe il posto di quella buona. sposta=False: come crea_villaggio.py,
    # il modello resta nella sottocartella dell'importazione (cosi' anche crea_villaggio.py lo ritrova uguale).
    cartella = cartella or PEZZI
    giusto = "%s/%s" % (cartella, nome)
    vecchio = None
    if libreria.does_asset_exist(giusto):
        v = unreal.load_asset("%s.%s" % (giusto, nome))
        if isinstance(v, unreal.StaticMesh):
            vecchio = v
    nuovo = None
    for percorso in libreria.list_assets(cartella, recursive=True, include_folder=False):
        pacchetto = percorso.split(".")[0]
        if pacchetto.rsplit("/", 1)[-1] != nome or pacchetto == giusto:
            continue
        x = unreal.load_asset(percorso)
        if isinstance(x, unreal.StaticMesh):
            nuovo = (pacchetto, x)
            break
    if nuovo is None:
        return vecchio
    pacchetto, x = nuovo
    if not importato and vecchio is not None:
        return vecchio
    if not sposta:
        return x
    if vecchio is not None and vecchio.get_path_name() != x.get_path_name():
        ok = False
        try:
            ok = bool(libreria.consolidate_assets(x, [vecchio]))
        except Exception as e:  # noqa: BLE001
            avviso("%s: errore nel sostituire il modello vecchio (%s)" % (nome, e))
        if not ok:
            avviso("%s: non riesco a sostituire il modello vecchio: uso quello nuovo dalla sottocartella" % nome)
            return x
        # resta un rimando (redirector) al posto del vecchio: lo si toglie, cosi' il nuovo puo' andare al suo posto
        if libreria.does_asset_exist(giusto):
            try:
                libreria.delete_asset(giusto)
            except Exception:  # noqa: BLE001
                pass
    if not libreria.does_asset_exist(giusto) and libreria.rename_asset(pacchetto, giusto):
        x = unreal.load_asset("%s.%s" % (giusto, nome))
    return x


SLOT_ESATTI = {}
# (16/11 notte, foto di Fra: dentro i muri erano "bruttini") i materiali degli interni: figli di quelli del villaggio, con
# la texture piu' fitta (Scala) e un'altra tinta. slot del modello: (nome, materiale del villaggio, Scala, Tinta)
INTERNI = {"intonaco_dentro": ("MI_Interno_Intonaco", "Intonaco", 1.5, (1.0, 0.97, 0.90)),
           "pietra_dentro": ("MI_Interno_Pietra", "Pietra", 1.25, (0.92, 0.90, 0.86)),
           "pavimento_legno": ("MI_Interno_PavimentoLegno", "Assi", 1.0, (0.62, 0.55, 0.48)),
           "pavimento_pietra": ("MI_Interno_PavimentoPietra", "Pietra", 0.5, (0.62, 0.60, 0.58))}


def materiali_interni():
    """Crea (o aggiorna) i materiali degli interni e li segna in SLOT_ESATTI per veste()."""
    mel = unreal.MaterialEditingLibrary
    for slot, (nome, genitore, scala, tinta) in INTERNI.items():
        g = unreal.load_asset("%s/MI_Villaggio_%s" % (MATERIALI, genitore))
        if g is None:
            avviso("manca MI_Villaggio_%s: lo slot %s prende il materiale di fuori" % (genitore, slot))
            continue
        try:
            percorso = "%s/%s" % (MATERIALI, nome)
            mi = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
            if mi is None:
                mi = strumenti.create_asset(nome, MATERIALI, unreal.MaterialInstanceConstant,
                                            unreal.MaterialInstanceConstantFactoryNew())
            mel.set_material_instance_parent(mi, g)
            mel.set_material_instance_scalar_parameter_value(mi, "Scala", scala)
            mel.set_material_instance_vector_parameter_value(mi, "Tinta",
                                                             unreal.LinearColor(tinta[0], tinta[1], tinta[2], 1.0))
            mel.update_material_instance(mi)
            libreria.save_loaded_asset(mi)
            SLOT_ESATTI[slot] = mi
        except Exception as e:  # noqa: BLE001
            avviso("%s: materiale non fatto (%s): lo slot %s prende quello di fuori" % (nome, e, slot))
    scrivi("materiali degli interni: %s" % (", ".join(sorted(m.get_name() for m in SLOT_ESATTI.values())) or "nessuno"))


def veste(mesh, mat):
    """I materiali del villaggio negli slot (pietra, legno, ferro, coppi), la collisione vera (le scale a chiocciola, i
    vani delle porte e il cammino di ronda devono restare percorribili), Nanite acceso."""
    for i, sm in enumerate(mesh.get_editor_property("static_materials")):
        nome = str(sm.get_editor_property("material_slot_name")).lower()
        # (16/11 notte) gli slot "di dentro" (intonaco_dentro, pavimento_legno...) hanno un materiale loro: prima il nome
        # esatto, altrimenti "pietra_dentro" prenderebbe la pietra di fuori
        if nome in SLOT_ESATTI:
            mesh.set_material(i, SLOT_ESATTI[nome])
            continue
        mi = sm.get_editor_property("material_interface")
        if mi is not None:
            nome += " " + mi.get_name().lower()
        chiave = next((v for k, v in SLOT.items() if k in nome), None)
        if chiave and chiave in mat:
            mesh.set_material(i, mat[chiave])
        elif any(k in nome for k in SLOT_PROPRI):
            pass
        else:
            avviso("%s: lo slot %s resta con il suo colore" % (mesh.get_name(), nome))
    try:
        bs = mesh.get_editor_property("body_setup")
        if bs is not None:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            mesh.set_editor_property("body_setup", bs)
    except Exception as e:  # noqa: BLE001
        avviso("%s: collisione non impostata (%s)" % (mesh.get_name(), e))
    try:
        # (secondo controllo, 16/11 sera) Nanite non disegna i materiali trasparenti (il vetro della lanterna): li' spento
        trasparente = False
        for sm in mesh.get_editor_property("static_materials"):
            mi = sm.get_editor_property("material_interface")
            base = mi.get_base_material() if mi is not None else None
            if base is not None and base.get_editor_property("blend_mode") not in (
                    unreal.BlendMode.BLEND_OPAQUE, unreal.BlendMode.BLEND_MASKED):
                trasparente = True
        ns = mesh.get_editor_property("nanite_settings")
        if ns.get_editor_property("enabled") == trasparente:
            ns.set_editor_property("enabled", not trasparente)
            mesh.set_editor_property("nanite_settings", ns)
    except Exception as e:  # noqa: BLE001
        avviso("%s: Nanite non controllato (%s)" % (mesh.get_name(), e))
    libreria.save_loaded_asset(mesh)


def importa(nomi, mat):
    compiti = []
    importati = set()
    for n in nomi:
        f = "%s/glb/%s.glb" % (CARTELLA, n)
        if os.path.exists(f):
            compiti.append(compito(f, PEZZI))
            importati.add(n)
        else:
            avviso("manca il file %s" % f)
    scrivi("importo %d pezzi..." % len(compiti))
    strumenti.import_asset_tasks(compiti)
    pezzi = {}
    for n in nomi:
        a = cerca_e_metti_a_posto(n, importato=n in importati)
        if a is None:
            avviso("pezzo non importato: %s" % n)
            continue
        if n != "SM_Prova_Assi" and mat:
            veste(a, mat)
        pezzi[n] = a
    scrivi("pezzi pronti: %d su %d" % (len(pezzi), len(nomi)))
    return pezzi


def materiali():
    mat = {}
    for n in set(SLOT.values()):
        a = unreal.load_asset("%s/MI_Villaggio_%s" % (MATERIALI, n))
        if a is None:
            avviso("manca MI_Villaggio_%s (prima il villaggio)" % n)
        else:
            mat[n] = a
    return mat


def specchio_y(pezzi):
    """SM_Prova_Assi: x da -30 a 100, y da -10 a 120, z da 0 a 100. Se la y arriva al contrario, Unreal ha specchiato:
    -1 (si rimette con la scala Y = -1 sui modelli)."""
    m = pezzi.get("SM_Prova_Assi")
    if m is None:
        avviso("manca SM_Prova_Assi: non so se Unreal ha specchiato la Y (penso di no)")
        return 1.0
    b = m.get_bounding_box()
    scrivi("prova assi: x da %.0f a %.0f, y da %.0f a %.0f, z da %.0f a %.0f"
           % (b.min.x, b.max.x, b.min.y, b.max.y, b.min.z, b.max.z))
    if b.max.y > 80 and b.max.x > 80 and b.max.z > 80:
        scrivi("prova assi: GIUSTA (freccia verso +Y, ferro verso +X, paletto in alto)")
        return 1.0
    if b.min.y < -80 and b.max.x > 80 and b.max.z > 80:
        scrivi("prova assi: Unreal ha SPECCHIATO la Y (come con la grotta): la correggo con la scala Y = -1")
        return -1.0
    avviso("prova assi: orientamento strano (girato?): guardare a occhio e dirlo alla chat 3D")
    return 1.0


# ------------------------------------------------------------------------------------------------ nel livello

def angolo_paesaggio():
    for a in attori.get_all_level_actors():
        if isinstance(a, unreal.Landscape):
            return a.get_actor_location()
    return None


# (16/11 sera, secondo giro) chi copriva il terreno nei punti dove non lo trovavo: nome -> volte
COPERTO = {}


def terreno(mondo, x, y, vicino):
    """La quota del paesaggio in (x, y), da +2 km a -2 km di quota; se il raggio tocca il paesaggio piu' volte
    (montagne, grotte) prende il punto piu' vicino alla quota 'vicino'.
    (16/11 sera, secondo giro) Il raggio si ferma alla prima cosa "piena" che tocca (un pezzo di mura appena messo,
    l'acqua del fiume, un albero, una roccia...): allora si rifa' il raggio ignorando quella cosa, fino a 12 volte,
    e si segna chi copriva (COPERTO) per il resoconto."""
    ignora = []
    for _ in range(12):
        trovate = []
        fermato = None
        try:
            r = unreal.SystemLibrary.line_trace_multi(mondo, unreal.Vector(x, y, 200000.0), unreal.Vector(x, y, -200000.0),
                                                      unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora,
                                                      unreal.DrawDebugTrace.NONE, True)
            colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
        except Exception:  # noqa: BLE001
            colpi = []
        for hit in colpi or []:
            t = hit.to_tuple()
            if t[9] is None:
                continue
            if "Landscape" in t[9].get_class().get_name():
                trovate.append(t[4].z)
            elif t[0]:
                fermato = t[9]
        if trovate:
            return min(trovate, key=lambda z: abs(z - vicino))
        if fermato is None or fermato in ignora:
            return None
        ignora.append(fermato)
        if not fermato.actor_has_tag(ETICHETTA):
            n = fermato.get_actor_label()
            COPERTO[n] = COPERTO.get(n, 0) + 1
    return None


def camera_editor():
    """(16/11 sera) In Unreal 5.8 get_level_viewport_camera_info da' due valori (posizione, rotazione), non tre."""
    r = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_level_viewport_camera_info()
    if isinstance(r, tuple) and len(r) == 3:
        return r
    if isinstance(r, tuple) and len(r) == 2:
        return (r[0] is not None, r[0], r[1])
    return (False, None, None)


def al_mondo(o, yaw, locale):
    """Un punto nel sistema di un pezzo (x, y, z) nel mondo (o = origine del pezzo, yaw in gradi)."""
    a = math.radians(yaw)
    x, y, z = locale
    return unreal.Vector(o.x + x * math.cos(a) - y * math.sin(a), o.y + x * math.sin(a) + y * math.cos(a), o.z + z)


def finisci(a, nome, fase=FASE_MURA, cartella=CONTORNO):
    a.set_actor_label(nome)
    a.set_folder_path(cartella)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)] + ([unreal.Name(fase)] if fase else []))
    return a


def metti_mesh(mesh, dove, yaw, nome, sy, fase=FASE_MURA, cartella=CONTORNO):
    a = attori.spawn_actor_from_object(mesh, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    if a is None:
        avviso("non riesco a mettere %s" % nome)
        return None
    if sy < 0:
        a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
    return finisci(a, nome, fase, cartella)


def mesh_su(componente, mesh, sy, yaw_relativo=0.0):
    """Mette un modello vero su un componente di un attore C++ (l'anta, la grata, l'asta della leva)."""
    componente.set_editor_property("static_mesh", mesh)
    componente.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
    componente.set_relative_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw_relativo), False, False)
    componente.set_relative_scale3d(unreal.Vector(1.0, sy, 1.0))
    # (16/11 sera) il C++ da' all'anta, all'asta e alla grata un materiale suo (bianco): si rimettono quelli del modello
    # (legno, ferro), slot per slot, altrimenti porte e leva restano bianche
    try:
        for i in range(componente.get_num_materials()):
            componente.set_material(i, mesh.get_material(i))
    except Exception as e:  # noqa: BLE001
        avviso("%s: materiali del modello non rimessi (%s)" % (mesh.get_name(), e))


def porta_vera(classe, mesh, dove, yaw, verso, chiave, nome, sy):
    """Un'anta o una porticina: AValdorsoPorta con il modello vero sulla cerniera (l'origine del modello e' il cardine)."""
    p = attori.spawn_actor_from_class(classe, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    if p is None:
        return None
    cerniera = p.get_editor_property("cerniera")
    anta = p.get_editor_property("anta")
    cerniera.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
    mesh_su(anta, mesh, sy)
    p.set_editor_property("angolo_apertura", 90.0)
    p.set_editor_property("verso_fisso", int(verso))
    if chiave:
        p.set_editor_property("chiusa_a_chiave", True)
        p.set_editor_property("chiave", unreal.Name(chiave))
    return finisci(p, nome)


def tutti_per_nome():
    d = {}
    for a in attori.get_all_level_actors():
        if a:
            d.setdefault(a.get_actor_label(), a)
    return d


MAI = "ValdorsoFase 99-99"            # (16/11 sera) una fase che non arriva mai: il C++ la nasconde e le toglie la collisione
PRIMA = "ValdorsoFasePrima "           # la fase che aveva prima di essere spenta (per "torna come prima")


def spegni(a, si):
    """Spegne (o riaccende) una cosa vecchia senza cancellarla.
    (16/11 sera) Prima si spegnevano solo i suoi pezzi (invisibili, senza collisione), ma in Play tornavano: le torri
    vecchie delle porte si vedevano e i loro muri invisibili chiudevano le porticine. Ora le si da' anche la fase 99,
    che non arriva mai: cosi' la nasconde il sistema delle fasi, nel gioco e in Play, con la collisione spenta."""
    try:
        a.modify()
    except Exception:  # noqa: BLE001
        pass
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        try:
            c.modify()
        except Exception:  # noqa: BLE001
            pass
        c.set_visibility(not si, True)
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION if si else unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    vecchie = [str(t) for t in a.get_editor_property("tags")]
    tags = [t for t in vecchie if t != SPENTA and t != MAI]
    if si:
        if not any(t.startswith(PRIMA) for t in tags):
            fase = [t for t in tags if t.startswith("ValdorsoFase ")]
            tags.append(PRIMA + (fase[0][len("ValdorsoFase "):] if fase else "nessuna"))
        tags = [t for t in tags if not t.startswith("ValdorsoFase ")] + [SPENTA, MAI]
    else:
        prima = [t for t in tags if t.startswith(PRIMA)]
        tags = [t for t in tags if not t.startswith(PRIMA)]
        if prima and prima[0][len(PRIMA):] != "nessuna":
            tags.append("ValdorsoFase " + prima[0][len(PRIMA):])
    a.set_editor_property("tags", [unreal.Name(t) for t in tags])


# (16/11 sera, foto di Fra) le 8 lanterne di legno delle porte larghe della palizzata (fase 2) restano davanti alle
# torri delle mura: nella fase 3 non servono piu' (le mura hanno le loro lanterne). Diventano "solo fase 2".
SOLO_FASE2 = ("Lanterne delle porte larghe", "Luce lanterna larga")
CAMBIATA = "ValdorsoMuraFaseCambiata"


def fase_solo_2(a, si):
    """Mette (si=True) o toglie (si=False) la fase 2-2 a una cosa della fase 2, ricordando quella di prima."""
    tags = [str(t) for t in a.get_editor_property("tags")]
    try:
        a.modify()
    except Exception:  # noqa: BLE001
        pass
    if si:
        if CAMBIATA in tags or SPENTA in tags:
            return False
        fase = [t for t in tags if t.startswith("ValdorsoFase ")]
        tags = [t for t in tags if not t.startswith("ValdorsoFase ") and not t.startswith(PRIMA)]
        # (16/11 sera) resta dalla fase in cui c'era gia' (1 se c'era sempre) fino alla fase 2
        da = 1
        if fase:
            try:
                da, fino = [int(v) for v in fase[0][len("ValdorsoFase "):].split("-")]
            except ValueError:
                return False
            # (secondo controllo) una cosa che nasce dopo la fase 2, o che finisce gia' prima, non si tocca
            if da > 2 or fino in (1, 2):
                return False
        tags += ["ValdorsoFase %d-2" % da, CAMBIATA, PRIMA + (fase[0][len("ValdorsoFase "):] if fase else "nessuna")]
    else:
        if CAMBIATA not in tags:
            return False
        prima = [t for t in tags if t.startswith(PRIMA)]
        if not prima:
            # (non dovrebbe capitare) senza il ricordo della fase di prima si toglie solo il segno
            a.set_editor_property("tags", [unreal.Name(t) for t in tags if t != CAMBIATA])
            return True
        tags = [t for t in tags if not t.startswith("ValdorsoFase ") and not t.startswith(PRIMA) and t != CAMBIATA]
        if prima[0][len(PRIMA):] != "nessuna":
            tags.append("ValdorsoFase " + prima[0][len(PRIMA):])
    a.set_editor_property("tags", [unreal.Name(t) for t in tags])
    return True


def togli_nuovi(solo=None):
    via = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(ETICHETTA) and (solo is None or solo(a)):
            attori.destroy_actor(a)
            via += 1
    return via


def leggi(nome, se_manca=None):
    f = CARTELLA + "/" + nome
    if not os.path.exists(f):
        return se_manca
    with open(f, "r", encoding="utf-8") as g:
        return json.load(g)


def apri_dati():
    schede = leggi("Valdorso_mura_tempio.json", {})
    # (16/11 sera) le schede del kit nuovo (con raccordi, scale, fondazioni, gradini) al posto di quelle di prima
    kit = leggi(SCHEDE_KIT)
    if kit:
        schede["pezzi"] = kit
    posizioni = leggi(POSIZIONI_Z)
    if posizioni:
        scrivi("posizioni: %s (le Z della chat 3D)" % POSIZIONI_Z)
    else:
        posizioni = leggi("posizioni_mura_valdorso.json")
        scrivi("posizioni: posizioni_mura_valdorso.json (Z dal terreno)")
    return schede, posizioni


def metti_lanterne(lanterna, punti, o, yaw, etichetta, sy, conti):
    """(16/11 sera) Le lanterne di ferro di una torre (punti della chat 3D nel sistema della torre) con la loro luce."""
    luce = (LANTERNE_DATI.get("SM_Lanterna_Muro", {}) or {}).get("luce_cm", [0.0, 42.0, -24.0])
    for j, pt in enumerate(punti):
        dove = al_mondo(o, yaw, pt["posizione_cm"])
        gira = yaw + float(pt.get("yaw", 0))
        a = metti_mesh(lanterna, dove, gira, "%s lanterna %d" % (etichetta, j + 1), sy, cartella=CONTORNO + "/Lanterne")
        if a is None:
            continue
        try:
            a.get_component_by_class(unreal.StaticMeshComponent).set_collision_profile_name("NoCollision")
        except Exception:  # noqa: BLE001
            pass
        conti["lanterne"] = conti.get("lanterne", 0) + 1
        if not LANTERNE_CON_LUCE:
            continue
        lp = al_mondo(dove, gira, luce)
        l = attori.spawn_actor_from_class(unreal.PointLight, lp, unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
        if l is None:
            continue
        lc = l.get_component_by_class(unreal.PointLightComponent)
        lc.set_mobility(unreal.ComponentMobility.MOVABLE)
        try:
            lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
        except Exception:  # noqa: BLE001
            pass
        lc.set_intensity(LUCE_INTENSITA)
        dentro = any(k in str(pt.get("dove", "")) for k in ("dentro", "stanza", "passaggio"))
        lc.set_attenuation_radius(LUCE_RAGGIO_DENTRO if dentro else LUCE_RAGGIO_FUORI)
        lc.set_light_color(unreal.LinearColor(LUCE_COLORE[0], LUCE_COLORE[1], LUCE_COLORE[2], 1.0))
        lc.set_cast_shadows(False)
        finisci(l, "%s luce %d" % (etichetta, j + 1), cartella=CONTORNO + "/Lanterne")
        conti["luci"] = conti.get("luci", 0) + 1


LANTERNE_DATI = {}


def numero_specchio():
    """Il risultato della prova degli assi, scritto da "Mura: 1." (se non c'e', la si rifa' qui)."""
    f = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude", "mura_assi.txt")
    if os.path.exists(f):
        with open(f, "r", encoding="utf-8") as g:
            return -1.0 if g.read().strip() == "-1" else 1.0
    return None


def salva_specchio(sy):
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "mura_assi.txt"), "w", encoding="utf-8") as g:
        g.write("-1" if sy < 0 else "1")


# ------------------------------------------------------------------------------------------------ i modi

def fai_assi():
    pezzi = importa(["SM_Prova_Assi"], None)
    sy = specchio_y(pezzi)
    salva_specchio(sy)
    m = pezzi.get("SM_Prova_Assi")
    if m is None:
        return
    togli_nuovi(lambda a: a.get_actor_label() == "Prova degli assi (mura)")
    ok, dove, gira = camera_editor()
    if ok:
        yaw = gira.yaw
        p = unreal.Vector(dove.x + math.cos(math.radians(yaw)) * 400.0, dove.y + math.sin(math.radians(yaw)) * 400.0,
                          dove.z - 150.0)
        a = metti_mesh(m, p, 0.0, "Prova degli assi (mura)", 1.0, fase=None)
        if a:
            scrivi("messa davanti alla telecamera con yaw 0 e SENZA correzione: guardala con la freccia degli assi in "
                   "basso a sinistra (la freccia del pezzo deve andare come la Y verde, il cubetto come la X rossa)")
    scrivi("FATTO: ora 'Mura: 2. costruisci le mura'")


def fai_mura():
    schede, posizioni = apri_dati()
    angolo = angolo_paesaggio()
    if angolo is None:
        avviso("nel livello non c'e' il paesaggio (apri L_Valle)")
        return
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    mat = materiali()
    # (16/11 sera) i gradini davanti alle porticine, le rampe delle porte e le lanterne (se la chat 3D li ha mandati)
    gs = leggi(GRADINI_SOGLIE, {}) or {}
    LANTERNE_DATI.clear()
    LANTERNE_DATI.update(leggi(LANTERNE, {}) or {})
    punti_lanterne = LANTERNE_DATI.get("lanterne", {})
    extra = [g["pezzo"] for g in gs.get("gradini", []) + gs.get("soglie", [])]
    if punti_lanterne:
        extra.append("SM_Lanterna_Muro")
    mura2 = leggi(MURA2_DATI)
    if mura2:
        extra += [q["pezzo"] for q in mura2["pezzi"] + mura2.get("scale_al_cammino", [])]
    scrivi("mura basse della fase 2: %s" % ("si'" if mura2 else "no (manca %s)" % MURA2_DATI))
    scrivi("gradini %d, rampe delle porte %d, lanterne: %s" % (len(gs.get("gradini", [])), len(gs.get("soglie", [])),
                                                              "si'" if punti_lanterne else "no (manca %s)" % LANTERNE))
    nomi = sorted(set([p["pezzo"] for p in posizioni["pezzi"]] + extra + ["SM_Porta_Anta_Sinistra", "SM_Porta_Anta_Destra",
                                                                          "SM_Grata", "SM_Leva", "SM_Argano",
                                                                          "SM_Torre_Porticina", "SM_Stalla_Mulo",
                                                                          "SM_Prova_Assi"]))
    pezzi = importa(nomi, mat)
    if mura2:
        # (secondo controllo) se manca anche un solo pezzo delle mura basse non si fa niente della fase 2:
        # la palizzata larga resta com'e' (meglio la palizzata che un buco)
        senza = sorted(set(q["pezzo"] for q in mura2["pezzi"] + mura2.get("scale_al_cammino", [])
                           if q["pezzo"] not in pezzi))
        if senza:
            avviso("mura basse: mancano %s: la fase 2 resta con la palizzata" % ", ".join(senza))
            mura2 = None
    punti2 = [q for q in punti_lanterne.get("SM_Torre_Porta", []) if "ai lati del portone" in str(q.get("dove", ""))]
    if not pezzi.get("SM_Lanterna_Muro"):
        punti2 = []
    sy = numero_specchio()
    if sy is None:
        sy = specchio_y(pezzi)
        salva_specchio(sy)
    scrivi("correzione della Y: %s" % ("scala -1 (Unreal ha specchiato)" if sy < 0 else "nessuna"))
    via = togli_nuovi(lambda a: not a.actor_has_tag(TEMPIO_CAMBIATO) and "Tempio" not in a.get_actor_label())
    if via:
        scrivi("tolti %d pezzi delle mura di prima" % via)

    # le mura vecchie della fase 3 si spengono
    spente = 0
    for a in attori.get_all_level_actors():
        if a and a.get_actor_label().startswith(VECCHIE) and not a.actor_has_tag(ETICHETTA):
            spegni(a, True)
            spente += 1
    scrivi("spente %d cose vecchie (muro di cinta, torri delle porte, archi, torri d'angolo 8 e 9)" % spente)
    solo2 = 0
    for a in attori.get_all_level_actors():
        if a and a.get_actor_label().startswith(SOLO_FASE2) and fase_solo_2(a, True):
            solo2 += 1
    scrivi("lanterne delle porte larghe della palizzata: solo nella fase 2 (%d cose cambiate ora)" % solo2)

    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    classe_grata = getattr(unreal, "ValdorsoGrata", None)
    classe_leva = getattr(unreal, "ValdorsoLeva", None)
    if not (classe_porta and classe_grata and classe_leva):
        avviso("mancano le classi del C++ della v0.2.6 (ValdorsoGrata, ValdorsoLeva): compila; metto solo i muri")
    sch = schede["pezzi"]
    conti = {}
    zeta = None
    for k, p in enumerate(posizioni["pezzi"]):
        nome = p["pezzo"]
        m = pezzi.get(nome)
        if m is None:
            continue
        x, y = p["posizione_cm"][0] + angolo.x, p["posizione_cm"][1] + angolo.y
        yaw = float(p["yaw"])
        if len(p["posizione_cm"]) >= 3:
            # (16/11 sera) la Z la da' la chat 3D (Z del mondo, calcolata sulle quote vere: cammino continuo)
            z = float(p["posizione_cm"][2])
        else:
            # la quota: il punto piu' basso del terreno sotto il pezzo (all'inizio, al centro, alla fine), meno 40 cm
            ing = sch.get(nome, {}).get("ingombro_cm", {"x": [0, 0]})
            quote = []
            vicino = zeta if zeta is not None else angolo.z + 5000.0
            for lx in (ing["x"][0], (ing["x"][0] + ing["x"][1]) / 2.0, ing["x"][1]):
                w = al_mondo(unreal.Vector(x, y, 0.0), yaw, (lx, 0.0, 0.0))
                q = terreno(mondo, w.x, w.y, vicino)
                if q is not None:
                    quote.append(q)
            if not quote:
                avviso("%s %d: non trovo il terreno, lo salto" % (nome, k))
                continue
            # (le torri meno: le porte a terra e il passaggio della porta devono restare alti abbastanza)
            z = min(quote) - (10.0 if nome in ("SM_Torre", "SM_Torre_Porta") else AFFONDA)
        zeta = z
        o = unreal.Vector(x, y, z)
        etichetta = "Mura ~ %03d %s" % (k + 1, nome.replace("SM_", ""))
        a = metti_mesh(m, o, yaw, etichetta, sy)
        if a is None:
            continue
        conti[nome] = conti.get(nome, 0) + 1
        # (16/11 sera) le lanterne della torre (anche le torri di raccordo hanno i loro punti)
        if nome in punti_lanterne and pezzi.get("SM_Lanterna_Muro"):
            metti_lanterne(pezzi["SM_Lanterna_Muro"], punti_lanterne[nome], o, yaw, etichetta, sy, conti)
        # (16/11 sera) torri e torri della porta si riconoscono dalla scheda (porticine, battenti), raccordi compresi
        s = sch.get(nome, {})
        if "porticine" not in s or not classe_porta:
            continue
        # le porticine: quella a terra chiusa con la chiave delle torri, quelle sul cammino aperte
        porticina = pezzi.get("SM_Torre_Porticina")
        for j, pc in enumerate(s.get("porticine", [])):
            if porticina is None:
                break
            apre = pc.get("apre", "")
            verso = 1 if "+90" in apre else -1
            a_terra = pc["cardine"][2] < 100
            porta_vera(classe_porta, porticina, al_mondo(o, yaw, pc["cardine"]), yaw + float(pc.get("yaw_chiusa", 0)),
                       verso, "Chiave_Torri" if a_terra else None, "%s porticina %d" % (etichetta, j + 1), sy)
            conti["porticine"] = conti.get("porticine", 0) + 1
        if "battenti" not in s:
            continue
        # la porta della citta': le due ante, la grata, la leva con il lucchetto, l'argano
        for lato, verso in (("SM_Porta_Anta_Sinistra", -1), ("SM_Porta_Anta_Destra", 1)):
            b = s["battenti"][lato]
            if pezzi.get(lato):
                anta = porta_vera(classe_porta, pezzi[lato], al_mondo(o, yaw, b["cardine"]),
                                  yaw + float(b.get("yaw_chiusa", 0)), verso, None,
                                  "%s %s" % (etichetta, "anta sinistra" if verso < 0 else "anta destra"), sy)
                if anta and mura2:
                    # le stesse ante valgono anche per la torre bassa della fase 2 (stessi cardini)
                    anta.set_editor_property("tags", [unreal.Name(ETICHETTA), unreal.Name(FASE_DA_2)])
        grata = None
        if pezzi.get("SM_Grata"):
            g = s["grata"]
            # la grata del C++ sta nel piano Y-Z (si passa lungo la sua X): girata di 90 gradi rispetto alla torre
            grata = attori.spawn_actor_from_class(classe_grata, al_mondo(o, yaw, g["posizione_chiusa"]),
                                                  unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 90.0))
            if grata:
                grata.set_editor_property("modello", pezzi["SM_Grata"])
                grata.set_editor_property("larghezza", 440.0)
                grata.set_editor_property("altezza", 540.0)
                grata.set_editor_property("corsa", float(g.get("corsa_cm", 540)))
                mesh_su(grata.get_editor_property("ferro"), pezzi["SM_Grata"], sy, -90.0)
                finisci(grata, "%s grata" % etichetta)
        if pezzi.get("SM_Leva") and grata:
            lv = s["leva"]
            # la leva del C++ gira intorno alla sua Y: girata di 90 gradi, gira intorno alla X della torre
            leva = attori.spawn_actor_from_class(classe_leva, al_mondo(o, yaw, lv["perno"]),
                                                 unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 90.0))
            if leva:
                leva.set_editor_property("grata", grata)
                perno = leva.get_editor_property("perno")
                perno.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
                mesh_su(leva.get_editor_property("asta"), pezzi["SM_Leva"], sy, -90.0)
                base = leva.get_editor_property("base")
                base.set_relative_location(unreal.Vector(0.0, 0.0, -25.0), False, False)
                base.set_relative_scale3d(unreal.Vector(0.4, 0.3, 0.5))
                base.set_visibility(False, True)
                finisci(leva, "%s leva della grata" % etichetta)
        if pezzi.get("SM_Argano"):
            metti_mesh(pezzi["SM_Argano"], al_mondo(o, yaw, s["argano"]["asse"]), yaw, "%s argano" % etichetta, sy)
    # (16/11 sera) i gradini davanti alle porticine a terra e le rampe di pietra delle 4 porte (posizioni gia' nel
    # sistema della Pianta, Z del mondo; fase 3 come le mura)
    for tipo in ("gradini", "soglie"):
        for g in gs.get(tipo, []):
            m = pezzi.get(g["pezzo"])
            if m is None:
                continue
            gp = g["posizione_cm"]
            dove = unreal.Vector(gp[0] + angolo.x, gp[1] + angolo.y, float(gp[2]))
            nome_g = "Mura ~ %03d %s" % (int(g.get("torre_indice", -1)) + 1, g["pezzo"].replace("SM_", ""))
            # le rampe delle porte servono uguali nella fase 2 (stessi pavimenti); i gradini delle porticine solo nella 3
            fase_g = FASE_DA_2 if (tipo == "soglie" and mura2) else FASE_MURA
            if metti_mesh(m, dove, float(g["yaw"]), nome_g, sy, fase=fase_g):
                conti[tipo] = conti.get(tipo, 0) + 1
    # (16/11 sera) le mura basse della fase 2: stessi posti, pezzi SM_Mura2_*; le scale al cammino al posto delle torri;
    # alle porte quattro lanterne di ferro (i punti "ai lati del portone" della torre alta: il passaggio e' identico)
    if mura2:
        n2 = 0
        for p in mura2["pezzi"] + mura2.get("scale_al_cammino", []):
            m = pezzi.get(p["pezzo"])
            if m is None:
                continue
            gp = p["posizione_cm"]
            o2 = unreal.Vector(gp[0] + angolo.x, gp[1] + angolo.y, float(gp[2]))
            nome2 = "Mura basse ~ %03d %s" % (int(p.get("indice", 0)) + 1, p["pezzo"].replace("SM_Mura2_", ""))
            if metti_mesh(m, o2, float(p["yaw"]), nome2, sy, fase=FASE_MURA2, cartella=CONTORNO + "/Fase 2") is None:
                continue
            n2 += 1
            if "battenti" in mura2.get("schede", {}).get(p["pezzo"], {}) and punti2:
                c2 = {}
                metti_lanterne(pezzi["SM_Lanterna_Muro"], punti2, o2, float(p["yaw"]), nome2, sy, c2)
                for a in attori.get_all_level_actors():
                    if a and a.get_actor_label().startswith(nome2 + " l") and a.actor_has_tag(ETICHETTA):
                        a.set_editor_property("tags", [unreal.Name(ETICHETTA), unreal.Name(FASE_MURA2)])
                conti["lanterne fase 2"] = conti.get("lanterne fase 2", 0) + c2.get("lanterne", 0)
        conti["mura basse (fase 2)"] = n2
        da_spegnere = VECCHIE_FASE2
        if not punti2:
            # (secondo controllo) senza le lanterne di ferro le porte resterebbero al buio: quelle di legno restano
            avviso("niente lanterne di ferro per le porte basse: restano quelle di legno")
            da_spegnere = tuple(v for v in VECCHIE_FASE2 if v not in SOLO_FASE2)
        spente2 = 0
        for a in attori.get_all_level_actors():
            if a and a.get_actor_label().startswith(da_spegnere) and not a.actor_has_tag(ETICHETTA) \
                    and not a.actor_has_tag(SPENTA):
                fase_solo_2(a, False)      # (le lanterne di legno erano "solo fase 2": prima tornano com'erano)
                spegni(a, True)
                spente2 += 1
        scrivi("fase 2: spente %d cose (palizzata larga, portali larghi, lanterne di legno delle porte)" % spente2)
    scrivi("messi: " + ", ".join("%s %d" % (n.replace("SM_", ""), c) for n, c in sorted(conti.items())))
    if COPERTO:
        scrivi("sopra il terreno c'era (ignorato per trovare la quota): " +
               ", ".join("%s %d" % (n, c) for n, c in sorted(COPERTO.items(), key=lambda v: -v[1])[:15]))

    # la stalla del mulo al posto di guardia del passo (la proposta della chat 3D)
    st = schede.get("posto_di_guardia", {}).get("SM_Stalla_Mulo", {}).get("proposta_nel_mondo")
    # (18/11 sera, secondo controllo) con il posto di guardia della Guardia Reale ("Mura: 11") la stalla sta a ovest
    # della strada, girata verso la strada: rimessa dov'era toccherebbe la terrazza del quartiere
    passo_nuovo = next((a for a in attori.get_all_level_actors() if a and a.actor_has_tag(PASSO_REALE)), None)
    porta_passo = tutti_per_nome().get(PASSO_PORTA) if passo_nuovo is not None else None
    if st and pezzi.get("SM_Stalla_Mulo") and porta_passo is not None:
        g0 = porta_passo.get_actor_location()
        x = g0.x + PASSO_D[0] * PASSO_STALLA[0] + PASSO_N[0] * PASSO_STALLA[1]
        y = g0.y + PASSO_D[1] * PASSO_STALLA[0] + PASSO_N[1] * PASSO_STALLA[1]
        q = terreno(mondo, x, y, g0.z)
        if q is not None:
            metti_mesh(pezzi["SM_Stalla_Mulo"], unreal.Vector(x, y, q - 10.0), float(st.get("yaw", 0)) + 180.0,
                       "Stalla del mulo", sy, fase=None, cartella="Valle/Posto di guardia")
            scrivi("la stalla del mulo a ovest della strada del passo (posto di guardia della Guardia Reale)")
    elif st and pezzi.get("SM_Stalla_Mulo"):
        x, y = st["posizione"][0] + angolo.x, st["posizione"][1] + angolo.y
        q = terreno(mondo, x, y, angolo.z + 10000.0)
        if q is not None:
            metti_mesh(pezzi["SM_Stalla_Mulo"], unreal.Vector(x, y, q - 10.0), float(st.get("yaw", 0)),
                       "Stalla del mulo", sy, fase=None, cartella="Valle/Posto di guardia")
            scrivi("la stalla del mulo accanto alla casermetta del passo")
    mostra_fase(3)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("FATTO e salvato. Ora: 'PNG: panche, letti e porte'; per guardarle: 'Villaggio: mostra la fase 3'")


def fai_tempio():
    schede, _ = apri_dati()
    mat = materiali()
    # (17/11 sera, v0.2.7) gli interni e i sagrati della chat 3D: se i loro file ci sono, si importano e si mettono
    # nell'origine dei gusci, con la stessa fase. I materiali degli interni (pietra_dentro...) vanno pronti prima.
    materiali_interni()
    in_piu = [n for n in TEMPIO_IN_PIU if os.path.exists("%s/glb/%s.glb" % (CARTELLA, n))]
    pezzi = importa(["SM_Tempio_Fase2", "SM_Tempio_Fase3", "SM_Prova_Assi"] + in_piu, mat)
    sy = numero_specchio()
    if sy is None:
        sy = specchio_y(pezzi)
        salva_specchio(sy)
    per_nome = tutti_per_nome()
    cappella = per_nome.get("Tempio_del_Cuore")
    if cappella is None:
        avviso("nel livello non c'e' 'Tempio_del_Cuore'")
        return
    togli_nuovi(lambda a: a.get_actor_label().startswith("Tempio del Cuore, fase"))
    o = cappella.get_actor_location()
    # la cappella ha yaw 180 e la porta a (0, -1100) suoi; i pezzi nuovi hanno la porta a (0, +1100): yaw 0 la mette
    # nello stesso punto del mondo, verso la piazza (lo controllo con il vettore porta)
    yaw_cappella = cappella.get_actor_rotation().yaw
    yaw = yaw_cappella + 180.0
    porta_vecchia = al_mondo(o, yaw_cappella, (0.0, -1100.0, 0.0))
    porta_nuova = al_mondo(o, yaw, (0.0, 1100.0, 0.0))
    scrivi("porta della cappella (%.0f, %.0f), porta delle fasi nuove (%.0f, %.0f)"
           % (porta_vecchia.x, porta_vecchia.y, porta_nuova.x, porta_nuova.y))
    for fase, nome in ((2, "SM_Tempio_Fase2"), (3, "SM_Tempio_Fase3")):
        m = pezzi.get(nome)
        if m is None:
            continue
        etichetta_fase = "ValdorsoFase %d-%d" % (fase, 0 if fase == 3 else 2)
        metti_mesh(m, unreal.Vector(o.x, o.y, o.z), yaw, "Tempio del Cuore, fase %d" % fase, sy,
                   fase=etichetta_fase, cartella="Valle/Tempio")
        for coda, titolo in (("_Dentro", "dentro"), ("_Sagrato", "sagrato")):
            m2 = pezzi.get(nome + coda)
            if m2 is not None:
                metti_mesh(m2, unreal.Vector(o.x, o.y, o.z), yaw, "Tempio del Cuore, fase %d, %s" % (fase, titolo), sy,
                           fase=etichetta_fase, cartella="Valle/Tempio")
                scrivi("fase %d: messo %s" % (fase, nome + coda))
        # una luce calda piccola sui due candelieri grandi davanti all'altare (i primi due punti di luci_cm)
        dentro = (leggi(TEMPIO_DENTRO_DATI) or {}).get("pezzi", {}).get(nome + "_Dentro")
        # (19/11) il posto a sedere di Maestro Elric sulla panca contro il muro (dati della chat 3D): i banchi della
        # chiesa sono un modello solo, quindi sopra ci va una panca del C++ che non si vede, con un posto
        seduto = ((dentro or {}).get("posti", {}).get("maestro_elric", {}) or {}).get("seduto_cm")
        classe_panca_t = getattr(unreal, "ValdorsoPanca", None)
        if seduto and classe_panca_t is not None and pezzi.get(nome + "_Dentro") is not None:
            try:
                g_s = float(dentro["posti"]["maestro_elric"].get("yaw_seduto", 0.0))
                # il pavimento e' a 6 cm (i piedi di chi sta in piedi li' davanti)
                pavimento = float((dentro["posti"]["maestro_elric"].get("in_piedi_davanti_cm") or [0, 0, 6.0])[2])
                pa = attori.spawn_actor_from_class(classe_panca_t, al_mondo(o, yaw, (float(seduto[0]), float(seduto[1]), pavimento)),
                                                   unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + g_s))
                if pa:
                    pa.set_editor_property("posti", [0.0])
                    for comp in pa.get_components_by_class(unreal.StaticMeshComponent):
                        comp.set_visibility(False, True)
                        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
                    finisci(pa, "Tempio del Cuore, fase %d, panca del muro (posto a sedere)" % fase, etichetta_fase, "Valle/Tempio")
                    scrivi("fase %d: posto a sedere sulla panca del muro" % fase)
            except Exception as ex:  # noqa: BLE001
                avviso("tempio, fase %d: il posto a sedere di Elric: %s" % (fase, ex))
        if dentro and pezzi.get(nome + "_Dentro") is not None:
            for i, pt in enumerate(dentro.get("luci_cm", [])[:2]):
                l = attori.spawn_actor_from_class(unreal.PointLight, al_mondo(o, yaw, (pt[0], pt[1], pt[2] + 12.0)),
                                                  unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
                if l is None:
                    continue
                lc = l.get_component_by_class(unreal.PointLightComponent)
                lc.set_mobility(unreal.ComponentMobility.MOVABLE)
                try:
                    lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
                except Exception:  # noqa: BLE001
                    pass
                lc.set_intensity(LUCE_INTENSITA)
                lc.set_attenuation_radius(280.0)
                lc.set_light_color(unreal.LinearColor(LUCE_COLORE[0], LUCE_COLORE[1], LUCE_COLORE[2], 1.0))
                lc.set_cast_shadows(False)
                finisci(l, "Tempio del Cuore, fase %d, candela %d" % (fase, i), etichetta_fase, "Valle/Tempio")
    # (17/11 sera) le dodici panche della cappella restano solo nella fase 1: nelle fasi 2 e 3 ci sono i banchi degli
    # interni nuovi, negli stessi posti (prima si vedevano in tutte le fasi, in mezzo alla navata)
    if any(pezzi.get(n) is not None for n in TEMPIO_IN_PIU):
        panche = 0
        for a in attori.get_all_level_actors():
            if a and a.get_actor_label() == "Tempio_del_Cuore panca":
                t2 = [t for t in a.get_editor_property("tags") if not str(t).startswith("ValdorsoFase")
                      and str(t) != TEMPIO_CAMBIATO]
                a.modify()
                # (con TEMPIO_CAMBIATO "Mura: torna come prima" le rimette in tutte le fasi, come la cappella)
                a.set_editor_property("tags", t2 + [unreal.Name("ValdorsoFase 1-1"), unreal.Name(TEMPIO_CAMBIATO)])
                panche += 1
        scrivi("panche della cappella lasciate solo alla fase 1: %d" % panche)
    # (19/11, v0.2.7) la pietra di Val d'Orso segue l'altare: nella chiesa e nella pieve l'altare e' piu' in fondo, e la
    # pietra restava a mezz'aria fra i banchi. Il C++ (AValdorsoPietraVillaggio::PostoPerFase) la sposta quando cambia la
    # fase; qui si scrivono i posti: fase 1 dov'e' adesso (zero = resta), fasi 2 e 3 dai dati della chat 3D
    # (posti.pietra_val_d_orso.centro_cm: il centro del piano dell'altare).
    classe_pietra = getattr(unreal, "ValdorsoPietraVillaggio", None)
    pietre = [a for a in attori.get_all_level_actors() if a and classe_pietra and isinstance(a, classe_pietra)]
    dati_dentro = (leggi(TEMPIO_DENTRO_DATI) or {}).get("pezzi", {})
    if len(pietre) == 1:
        # (secondo controllo) la fase 1 e' scritta per intero (dov'e' adesso, sull'altare della cappella): cosi' ci torna
        # anche se il villaggio scende di fase. Se i posti c'erano gia', quello della fase 1 resta quello di prima.
        gia = list(pietre[0].get_editor_property("posto_per_fase") or [])
        qui = pietre[0].get_actor_location()
        primo = gia[0] if gia and (abs(gia[0].x) + abs(gia[0].y) + abs(gia[0].z)) > 1.0 else qui
        posti_pietra = [unreal.Vector(primo.x, primo.y, primo.z)]
        for nome in ("SM_Tempio_Fase2_Dentro", "SM_Tempio_Fase3_Dentro"):
            c = dati_dentro.get(nome, {}).get("posti", {}).get("pietra_val_d_orso", {}).get("centro_cm")
            if c and pezzi.get(nome) is not None:
                posti_pietra.append(al_mondo(o, yaw, (float(c[0]), float(c[1]), float(c[2]))))
            else:
                posti_pietra.append(unreal.Vector(0.0, 0.0, 0.0))
        try:
            pietre[0].modify()
            pietre[0].set_editor_property("posto_per_fase", posti_pietra)
            qp = pietre[0].get_actor_location()
            scrivi("pietra di Val d'Orso: fase 1 dov'e' (%.0f, %.0f, %.0f); fase 2 (%.0f, %.0f, %.0f); fase 3 (%.0f, %.0f, %.0f)"
                   % (qp.x, qp.y, qp.z, posti_pietra[1].x, posti_pietra[1].y, posti_pietra[1].z,
                      posti_pietra[2].x, posti_pietra[2].y, posti_pietra[2].z))
        except Exception as ex:  # noqa: BLE001
            avviso("pietra di Val d'Orso: i posti per fase non si scrivono (%s): serve compilare il C++ del 19/11" % ex)
    else:
        avviso("pietra di Val d'Orso: nel livello ce ne sono %d (ne serve una): i posti per fase non si scrivono" % len(pietre))
    # la cappella resta solo nella fase 1
    tags = [t for t in cappella.get_editor_property("tags") if not str(t).startswith("ValdorsoFase")
            and str(t) != TEMPIO_CAMBIATO]
    cappella.set_editor_property("tags", tags + [unreal.Name("ValdorsoFase 1-1"), unreal.Name(TEMPIO_CAMBIATO)])
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    mostra_fase(3)
    scrivi("FATTO: fase 1 la cappella, fase 2 la chiesa, fase 3 la pieve (Villaggio: mostra la fase 2 / 3 per guardarle)")


def fai_indietro():
    # (le strade e i lampioni messi dalla voce 4 restano: i loro modelli di base sono gia' quelli nuovi, senza i tratti
    # e senza il palo staccati; per tornare a prima servono i file ..._prima_16-11 in V:/Texture/valdorso/villaggio)
    via = togli_nuovi()
    for a in attori.get_all_level_actors():
        if a and (a.actor_has_tag(STRADE_NUOVE) or a.actor_has_tag(CASERMA)):
            attori.destroy_actor(a)
            via += 1
    accese = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(SPENTA):
            # (18/11 sera, secondo controllo) le cose di prima del passo restano spente: il posto di guardia della Guardia
            # Reale ("Mura: 11") non si toglie da qui, e riaccese farebbero un posto di guardia doppio
            na = a.get_actor_label()
            if na in PASSO_VECCHIE or na.startswith(PASSO_VECCHIE_INIZI):
                continue
            spegni(a, False)
            accese += 1
        if a and a.actor_has_tag(CAMBIATA):
            fase_solo_2(a, False)
        if a and a.actor_has_tag(TEMPIO_CAMBIATO):
            tags = [t for t in a.get_editor_property("tags") if not str(t).startswith("ValdorsoFase")
                    and str(t) != TEMPIO_CAMBIATO]
            a.set_editor_property("tags", tags + [unreal.Name("ValdorsoFase 1-0")])
            scrivi("la cappella torna in tutte le fasi")
    # (19/11, secondo controllo) con la cappella in tutte le fasi la pietra di Val d'Orso non si sposta piu'
    classe_pietra = getattr(unreal, "ValdorsoPietraVillaggio", None)
    for a in attori.get_all_level_actors():
        if a and classe_pietra and isinstance(a, classe_pietra):
            try:
                a.modify()
                a.set_editor_property("posto_per_fase", [])
                scrivi("pietra di Val d'Orso: resta sull'altare della cappella in tutte le fasi")
            except Exception:  # noqa: BLE001
                pass
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("FATTO: tolti %d pezzi nuovi, riaccese %d cose vecchie" % (via, accese))


STRADE = "V:/Texture/valdorso/villaggio"
STRADE_PEZZI = "/Game/Valle/Villaggio/Pezzi"


# (16/11 sera, terza consegna delle strade) Le strade e le case spostate si leggono da Villaggio_3000.json, cosi' a
# ogni consegna della chat 3D basta aggiornare quel file:
# - strade: le voci "mesh" con questi inizi di nome del modello (la Via dell'anello SM_Fase2_Strade non si tocca);
#   "Strade del villaggio" c'e' gia' nel livello: le si cambia solo il modello; le altre si mettono nuove.
#   Tutte sono fatte come le Strade del villaggio (colori dei vertici, bordi sfumati) e ne prendono le impostazioni;
#   calpestabili solo i tratti staccati dalla strada della fase 1 (..._Fase12), le altre senza collisione.
# - cose spostate dalla fase 3: ogni voce con " (fase 3)" nel nome e' la copia spostata della voce con lo stesso nome
#   senza " (fase 3)" (case, le loro porte, le loro "Vita", luci): l'originale resta fino alla fase 2.
STRADE_INIZI = ("SM_Villaggio_Strade", "SM_Fase3_Strada", "SM_Fase3_Strade_", "SM_Fase3_Raccordi", "SM_Fase2_Strade_Porte")
STRADA_BASE = "SM_Villaggio_Strade"
# (16/11 sera) i lampioni del villaggio: il palo della lanterna 7 (dietro la pieve) e' un pezzo a parte, uno per le
# fasi 1-2 e uno spostato per la fase 3; il modello "Lanterne" si aggiorna (senza quel palo). Stessa origine.
# gruppi: (nome dell'attore di base, modello di base, inizi dei nomi dei modelli, chi e' calpestabile/con collisione)
GRUPPI = (("Strade del villaggio", STRADA_BASE, STRADE_INIZI, lambda c: c["asset"].endswith("_Fase12")),
          ("Lanterne", "SM_Villaggio_Lanterne", ("SM_Villaggio_Lanterne", "SM_Lanterna7_"), lambda c: True))
TUTTI_GLI_INIZI = tuple(x for g in GRUPPI for x in g[2])
SEGNO_FASE3 = " (fase 3)"
STRADE_NUOVE = "ValdorsoStradeNuove"


def impostazioni(modello):
    """Materiale, collisione e Nanite di un modello di strada che c'e' gia' (per darli uguali ai pezzi nuovi)."""
    d = {"materiale": modello.get_material(0), "nanite": None, "traccia": None, "due_facce": None, "per_slot": {}}
    # (16/11 sera) i materiali per nome dello slot (i lampioni ne hanno tre: ferro, fiamma, trave)
    for i, sm in enumerate(modello.get_editor_property("static_materials")):
        d["per_slot"][str(sm.get_editor_property("material_slot_name"))] = modello.get_material(i)
    try:
        d["nanite"] = modello.get_editor_property("nanite_settings")
    except Exception:  # noqa: BLE001
        pass
    try:
        bs = modello.get_editor_property("body_setup")
        d["traccia"] = bs.get_editor_property("collision_trace_flag")
        d["due_facce"] = bs.get_editor_property("double_sided_geometry")
    except Exception:  # noqa: BLE001
        pass
    return d


def applica(m, d):
    # ogni slot prende il materiale dello slot con lo stesso nome nel modello di prima (se non c'e', il primo)
    for i, sm in enumerate(m.get_editor_property("static_materials")):
        nome = str(sm.get_editor_property("material_slot_name"))
        m.set_material(i, d["per_slot"].get(nome) or d["materiale"])
    try:
        if d["nanite"] is not None:
            m.set_editor_property("nanite_settings", d["nanite"])
        bs = m.get_editor_property("body_setup")
        if bs is not None and d["traccia"] is not None:
            bs.set_editor_property("collision_trace_flag", d["traccia"])
            bs.set_editor_property("double_sided_geometry", bool(d["due_facce"]))
            m.set_editor_property("body_setup", bs)
    except Exception as e:  # noqa: BLE001
        avviso("%s: impostazioni non copiate (%s)" % (m.get_name(), e))
    libreria.save_loaded_asset(m)


def come(a, modello_attore, nome, fase):
    """Mette nome, cartella, etichette e collisione come l'attore modello."""
    c = a.get_component_by_class(unreal.StaticMeshComponent)
    cm = modello_attore.get_component_by_class(unreal.StaticMeshComponent)
    a.set_actor_scale3d(modello_attore.get_actor_scale3d())
    if cm:
        c.set_collision_profile_name(cm.get_collision_profile_name())
        c.set_collision_enabled(cm.get_collision_enabled())
        c.set_editor_property("cast_shadow", cm.get_editor_property("cast_shadow"))
    a.set_actor_label(nome)
    a.set_folder_path(str(modello_attore.get_folder_path()))
    a.set_editor_property("tags", [unreal.Name("ValdorsoVillaggio"), unreal.Name("ValdorsoFase " + fase)])
    try:
        c.set_editor_property("enable_auto_lod_generation", False)
    except Exception:  # noqa: BLE001
        pass


CASERMA = "ValdorsoCasermaNuova"
CASERMA_DATI = "Valdorso_caserma.json"


def fai_caserma():
    """(16/11 sera, v0.2.6, deciso da Fra) Il recinto della caserma della milizia, fase 3. Pezzi e posizioni della chat 3D
    (V:/Texture/valdorso/mura/Valdorso_caserma.json: posizioni gia' nel mondo, cm della Pianta, Z del mondo).
    La grata e' una AValdorsoGrata (si chiude la sera e si apre la mattina), la leva una AValdorsoLeva senza lucchetto
    dentro il corpo di guardia, che ha la porta chiusa con la Chiave_Caserma. Si puo' rilanciare: toglie e rimette."""
    global CARTELLA
    dati = leggi(CASERMA_DATI)
    if not dati:
        avviso("manca %s/%s" % (CARTELLA, CASERMA_DATI))
        return
    angolo = angolo_paesaggio()
    if angolo is None:
        avviso("nel livello non c'e' il paesaggio (apri L_Valle)")
        return
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    classe_grata = getattr(unreal, "ValdorsoGrata", None)
    classe_leva = getattr(unreal, "ValdorsoLeva", None)
    if not (classe_porta and classe_grata and classe_leva):
        avviso("mancano le classi del C++ delle mura (ValdorsoGrata, ValdorsoLeva, ValdorsoPorta): compila")
        return
    LANTERNE_DATI.clear()
    LANTERNE_DATI.update(leggi(LANTERNE, {}) or {})
    luce = (LANTERNE_DATI.get("SM_Lanterna_Muro", {}) or {}).get("luce_cm", [0.0, 42.0, -24.0])
    posizioni = dati["posizioni"]
    classe_campanella_presto = getattr(unreal, "ValdorsoCampanella", None)
    pezzi = importa(sorted(set(p["pezzo"] for p in posizioni)), materiali())
    sy = numero_specchio() or 1.0
    via = 0
    # (20/11, deciso da Fra con la foto) la campanella sta dove l'ha spostata lui a mano (a destra della porta del corpo
    # di guardia: dov'era nei dati, la corda pendeva davanti alla porta). Se nel livello c'e' gia', il suo posto si tiene.
    campanella_tenuta = None
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(CASERMA):
            if classe_campanella_presto is not None and isinstance(a, classe_campanella_presto):
                campanella_tenuta = (a.get_actor_location(), a.get_actor_rotation())
            attori.destroy_actor(a)
            via += 1
    if via:
        scrivi("tolti %d pezzi del recinto di prima" % via)

    def mio(a, nome):
        a.set_actor_label("Caserma ~ " + nome)
        a.set_folder_path("Valle/Caserma")
        a.set_editor_property("tags", [unreal.Name(CASERMA), unreal.Name(FASE_MURA)])
        return a

    # (20/11, v0.2.7.1) la campanella nuova della chat 3D (V:/Texture/valdorso/campanella: bronzo, con il battaglio e la
    # corda) al posto di quella di ferro: e' un attore del C++ (AValdorsoCampanella), le guardie la suonano con E. Porta
    # con se' il posto della guardia nella garitta (voce "garitta", guardia_in_piedi): li' si fa il turno in garitta.
    classe_campanella = getattr(unreal, "ValdorsoCampanella", None)
    pezzi_campanella = {}
    if classe_campanella is not None and os.path.exists(CAMPANELLA + "/glb/SM_Campanella_Caserma.glb"):
        vecchia_cartella = CARTELLA
        CARTELLA = CAMPANELLA
        try:
            pezzi_campanella = importa(["SM_Campanella_Caserma", "SM_Campanella_Battaglio"], materiali())
        finally:
            CARTELLA = vecchia_cartella
    elif classe_campanella is None:
        avviso("caserma: manca la classe del C++ ValdorsoCampanella (compila): resta la campanella di prima, che non suona")
    garitta_guardia = None
    for p in posizioni:
        gi = p.get("guardia_in_piedi") if p.get("pezzo") == "SM_Garitta" else None
        if gi and gi.get("mondo_cm"):
            garitta_guardia = unreal.Vector(gi["mondo_cm"][0] + angolo.x, gi["mondo_cm"][1] + angolo.y, float(gi["mondo_cm"][2]))

    conti = {}
    grata = None
    da_fare_dopo = []
    for p in posizioni:
        m = pezzi.get(p["pezzo"])
        # (secondo controllo) la campanella nuova non ha bisogno del modello di quella vecchia
        if m is None and not (p["pezzo"] == "SM_Campanella" and pezzi_campanella.get("SM_Campanella_Caserma") is not None):
            continue
        gp = p["posizione_cm"]
        dove = unreal.Vector(gp[0] + angolo.x, gp[1] + angolo.y, float(gp[2]))
        yaw = float(p["yaw"])
        nome, pezzo = p["nome"], p["pezzo"]
        if pezzo == "SM_Campanella" and pezzi_campanella.get("SM_Campanella_Caserma") is not None:
            # stessa posizione e quota, girata di 180 (la vecchia sporgeva verso +Y, la nuova verso -Y)
            dove_c, gira_c = dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 180.0)
            if campanella_tenuta is not None:
                dove_c, gira_c = campanella_tenuta
                scrivi("campanella: tengo il posto di adesso (%.1f, %.1f, %.1f), yaw %.1f (nei dati era %.1f, %.1f, %.1f)"
                       % (dove_c.x, dove_c.y, dove_c.z, gira_c.yaw, dove.x, dove.y, dove.z))
            ca = attori.spawn_actor_from_class(classe_campanella, dove_c, gira_c)
            if ca:
                # (il modello e' la radice dell'attore: solo il modello e la scala, senza toccarne il posto)
                cm = ca.get_editor_property("modello")
                cm.set_editor_property("static_mesh", pezzi_campanella["SM_Campanella_Caserma"])
                ca.set_actor_scale3d(unreal.Vector(1.0, sy, 1.0))
                bt = ca.get_editor_property("battaglio")
                if pezzi_campanella.get("SM_Campanella_Battaglio") is not None:
                    mesh_su(bt, pezzi_campanella["SM_Campanella_Battaglio"], 1.0)
                    # (la radice ha la scala Y = sy: il posto del battaglio va scritto nello stesso verso)
                    bt.set_relative_location(unreal.Vector(0.0, -45.0 * sy, -7.0), False, False)
                if garitta_guardia is not None:
                    ca.set_editor_property("garitta", garitta_guardia)
                    scrivi("campanella nuova messa; la guardia in garitta sta in (%.0f, %.0f, %.0f)"
                           % (garitta_guardia.x, garitta_guardia.y, garitta_guardia.z))
                else:
                    avviso("caserma: nei dati manca guardia_in_piedi della garitta: niente turno in garitta")
                mio(ca, "campanella")
                conti["SM_Campanella_Caserma"] = 1
                continue
            avviso("caserma: la campanella nuova non si e' messa: resta quella di prima, se c'e'")
            if m is None:
                continue
        if pezzo == "SM_Recinto_Grata":
            # come nelle mura: la grata del C++ sta nel piano Y-Z, quindi l'attore e' girato di 90 gradi e il modello di -90
            grata = attori.spawn_actor_from_class(classe_grata, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 90.0))
            if grata:
                grata.set_editor_property("modello", m)
                grata.set_editor_property("larghezza", 258.0)
                grata.set_editor_property("altezza", 330.0)
                grata.set_editor_property("corsa", float(p.get("corsa_cm", 335)))
                mesh_su(grata.get_editor_property("ferro"), m, sy, -90.0)
                mio(grata, "grata")
        elif pezzo == "SM_Leva":
            da_fare_dopo.append((p, m, dove, yaw))
        elif pezzo == "SM_Torre_Porticina":
            # "yaw da 90 a 180": se il secondo numero e' piu' grande del primo gira nel verso +1
            num = [float(v) for v in re.findall(r"-?\d+(?:\.\d+)?", str(p.get("apre", "")))]
            verso = 1 if len(num) < 2 or num[1] > num[0] else -1
            porta = porta_vera(classe_porta, m, dove, yaw, verso, "Chiave_Caserma", nome, sy)
            if porta:
                mio(porta, "porta del corpo di guardia")
        elif pezzo == "SM_Lanterna_Muro":
            a = metti_mesh(m, dove, yaw, nome, sy)
            if a:
                mio(a, nome.replace("_", " "))
                try:
                    a.get_component_by_class(unreal.StaticMeshComponent).set_collision_profile_name("NoCollision")
                except Exception:  # noqa: BLE001
                    pass
                l = attori.spawn_actor_from_class(unreal.PointLight, al_mondo(dove, yaw, luce),
                                                  unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0)) if LANTERNE_CON_LUCE else None
                if l:
                    lc = l.get_component_by_class(unreal.PointLightComponent)
                    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
                    try:
                        lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
                    except Exception:  # noqa: BLE001
                        pass
                    lc.set_intensity(LUCE_INTENSITA)
                    lc.set_attenuation_radius(LUCE_RAGGIO_FUORI)
                    lc.set_light_color(unreal.LinearColor(LUCE_COLORE[0], LUCE_COLORE[1], LUCE_COLORE[2], 1.0))
                    lc.set_cast_shadows(False)
                    mio(l, nome.replace("_", " ") + " luce")
        else:
            a = metti_mesh(m, dove, yaw, nome, sy)
            if a:
                mio(a, nome.replace("_", " "))
        conti[pezzo] = conti.get(pezzo, 0) + 1
    # la leva dopo la grata (deve conoscerla). Senza lucchetto: e' la porta del corpo di guardia a essere chiusa a chiave
    for p, m, dove, yaw in da_fare_dopo:
        if grata is None:
            avviso("la leva non ha la grata: non la metto")
            continue
        leva = attori.spawn_actor_from_class(classe_leva, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 90.0))
        if leva:
            leva.set_editor_property("grata", grata)
            leva.set_editor_property("chiave", unreal.Name("None"))
            leva.get_editor_property("perno").set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
            mesh_su(leva.get_editor_property("asta"), m, sy, -90.0)
            base = leva.get_editor_property("base")
            base.set_relative_location(unreal.Vector(0.0, 0.0, -25.0), False, False)
            base.set_relative_scale3d(unreal.Vector(0.4, 0.3, 0.5))
            base.set_visibility(False, True)
            mio(leva, "leva della grata")
    scrivi("messi: " + ", ".join("%s %d" % (n.replace("SM_", ""), c) for n, c in sorted(conti.items())))
    # le cose di oggi che finiscono sotto il muretto (entro 60 cm dalla sua linea): si scrivono, non si toccano
    sotto = set()
    muri = [a for a in attori.get_all_level_actors() if a and a.actor_has_tag(CASERMA)
            and a.get_actor_label().startswith(("Caserma ~ muretto", "Caserma ~ portale"))]
    for b in attori.get_all_level_actors():
        if not b or b.actor_has_tag(CASERMA) or b.is_temporarily_hidden_in_editor():
            continue
        o, e = b.get_actor_bounds(False)
        if max(e.x, e.y) > 1500 or max(e.x, e.y) < 5:
            continue
        for mu in muri:
            mo, me = mu.get_actor_bounds(False)
            if abs(o.x - mo.x) < e.x + me.x and abs(o.y - mo.y) < e.y + me.y and abs(o.z - mo.z) < e.z + me.z + 100:
                sotto.add(b.get_actor_label())
                break
    scrivi("cose che toccano il muretto (da guardare): " + (", ".join(sorted(sotto)[:30]) or "niente"))
    mostra_fase(3)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("FATTO e salvato. Poi 'Rete: le cose che si usano sempre caricate' e 'PNG: panche, letti e porte'. "
           "In Play: Valdorso.Villaggio.Fase 3, Valdorso.Zaino.Dai Chiave_Caserma")


ALLOGGI = "ValdorsoAlloggiNuovi"
ALLOGGI_DATI = "Valdorso_caserma_milizia.json"
# le cose di oggi degli alloggi e della torre che vanno via (messe da crea_villaggio.py): i letti al piano terra, le
# luci, le porte senza modello e la scala a pioli della torre
ALLOGGI_VIA = ("Alloggi_della_milizia letto", "Alloggi_della_milizia luce", "Alloggi_della_milizia porta",
               "Torre_di_guardia_della_milizia porta", "Torre_di_guardia_della_milizia scala")
COPERCHI = {"SM_Baule_Piccolo": ("SM_Baule_Piccolo_Coperchio", (0.0, 22.0, 42.0)),
            "SM_Baule_Grande": ("SM_Baule_Grande_Coperchio", (0.0, 30.0, 50.0))}


# (16/11 notte, foto di Fra) le panche dei tavoli messe dal villaggio erano in fila sullo stesso lato: la panca e' lunga
# lungo la sua Y come il tavolo, quindi va una per lato lungo (X del tavolo -+85). (tavolo, etichetta delle sue panche)
TAVOLI_E_PANCHE = (("Tavolo della mensa 0", "Mensa_della_milizia panca"),
                   ("Tavolo della mensa 1", "Mensa_della_milizia panca"),
                   ("Tavolo della caserma 0", "Caserma_della_milizia panca"))


def sistema_panche():
    per_nome = tutti_per_nome()
    usate = set()
    fatte = 0
    for tavolo, etichetta in TAVOLI_E_PANCHE:
        t = per_nome.get(tavolo)
        if t is None:
            continue
        c, _ = t.get_actor_bounds(False)
        yaw_t = t.get_actor_rotation().yaw
        modello = re.compile(r"^%s\d*$" % re.escape(etichetta))
        panche = [a for a in attori.get_all_level_actors()
                  if a and a not in usate and modello.match(a.get_actor_label()) and a.actor_has_tag("ValdorsoVillaggio")]
        panche.sort(key=lambda a: (a.get_actor_location().x - c.x) ** 2 + (a.get_actor_location().y - c.y) ** 2)
        for a, lato in zip(panche[:2], (-85.0, 85.0)):
            dove = al_mondo(unreal.Vector(c.x, c.y, a.get_actor_location().z), yaw_t, (lato, 0.0, 0.0))
            a.modify()
            a.set_actor_location(dove, False, False)
            a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw_t + (0.0 if lato < 0 else 180.0)), False)
            usate.add(a)
            fatte += 1
    scrivi("panche rimesse ai due lati lunghi dei tavoli (mensa, caserma della fase 2): %d" % fatte)


CELLE_FASI = "ValdorsoCelleFasi"
CELLE_FASI_DATI = "Valdorso_celle_fasi.json"
# (17/11, v0.2.7, deciso da Fra: la prigione in tutte e tre le fasi, sempre nella zona delle guardie) chi fa da ancora:
# la cella della fase 2 sta dentro la caserma della fase 2 (attore del villaggio); la gabbia della fase 1 e' all'aperto
CELLE_ANCORE = {"SM_Cella_Caserma_Fase2": "Caserma_della_milizia"}


def telecamera_passa(a):
    """(17/11, foto di Fra: nella gabbia la telecamera finiva addosso al colono) Le sbarre non fermano la telecamera: resta
    alla sua distanza, dietro e sopra, e guarda dentro attraverso le sbarre. Il colono le urta come prima."""
    if a is None:
        return
    for comp in a.get_components_by_class(unreal.PrimitiveComponent):
        try:
            comp.set_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA, unreal.CollisionResponseType.ECR_IGNORE)
        except Exception as ex:  # noqa: BLE001
            avviso("%s: la telecamera non passa (%s)" % (a.get_actor_label(), ex))
            return


def fai_celle_fasi():
    """(17/11, v0.2.7) Le celle delle fasi 1 e 2 (dati della chat 3D, V:/Texture/valdorso/mura/Valdorso_celle_fasi.json):
    la gabbia di legno della milizia (fase 1) e la cella nella caserma (fase 2). Per ognuna: il modello, la porta vera
    con la Chiave_Prigione e una AValdorsoCella (il C++ ci porta chi e' arrestato). Nella fase 3 ci sono le celle degli
    alloggi (fai_alloggi). Si puo' rilanciare: toglie e rimette le sue cose."""
    dati = leggi(CELLE_FASI_DATI)
    if not dati:
        scrivi("celle delle fasi 1 e 2: niente (manca %s)" % CELLE_FASI_DATI)
        return
    angolo = angolo_paesaggio()
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    classe_cella = getattr(unreal, "ValdorsoCella", None)
    if angolo is None or not classe_porta or not classe_cella:
        avviso("celle delle fasi 1 e 2: manca il paesaggio o il C++ (ValdorsoPorta, ValdorsoCella): compila")
        return
    celle = dati["celle"]
    nomi = set()
    for c in celle.values():
        nomi.add(c["pezzo"])
        nomi.add(c["porta"]["pezzo"])
    pezzi = importa(sorted(nomi), materiali())
    sy = numero_specchio() or 1.0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(CELLE_FASI):
            attori.destroy_actor(a)
    per_nome = tutti_per_nome()
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    fatte = 0
    for chiave, c in celle.items():
        fase = "ValdorsoFase %d-%d" % (int(c["fase"][0]), int(c["fase"][1]))
        m = pezzi.get(c["pezzo"])
        mp = pezzi.get(c["porta"]["pezzo"])
        if m is None or mp is None:
            avviso("%s: manca il modello (%s o %s)" % (chiave, c["pezzo"], c["porta"]["pezzo"]))
            continue
        ancora = CELLE_ANCORE.get(c["pezzo"])
        if ancora:
            # dentro un edificio del villaggio: stesso posto e stessi gradi dell'attore (scala 1: e' un pezzo come le mura)
            b = per_nome.get(ancora)
            if b is None:
                avviso("%s: nel livello manca \"%s\"" % (chiave, ancora))
                continue
            o = b.get_actor_location()
            yaw0 = b.get_actor_rotation().yaw
        else:
            # all'aperto: X e Y dalla Pianta; il pavimento 8 cm sopra il terreno piu' alto sotto i quattro angoli
            pm = c["posizione_mondo_cm"]
            yaw0 = float(c.get("gradi", 0))
            base = unreal.Vector(pm[0] + angolo.x, pm[1] + angolo.y, float(pm[2]))
            sc = c["scatola_cm"]
            quote = []
            for cx, cy in ((sc["min"][0], sc["min"][1]), (sc["min"][0], sc["max"][1]), (sc["max"][0], sc["min"][1]),
                           (sc["max"][0], sc["max"][1]), (0.0, 0.0)):
                w = al_mondo(base, yaw0, (cx, cy, 0.0))
                q = terreno(mondo, w.x, w.y, base.z)
                if q is not None:
                    quote.append(q)
            z = (max(quote) + 8.0) if quote else base.z
            if quote and abs(z - base.z) > 15.0:
                scrivi("%s: pavimento a %.1f (i dati dicevano %.1f): seguo il terreno vero" % (chiave, z, base.z))
            o = unreal.Vector(base.x, base.y, z)

        def mio(a, nome, fase=fase):
            a.set_actor_label(nome)
            a.set_folder_path("Valle/Caserma/Celle")
            a.set_editor_property("tags", [unreal.Name(CELLE_FASI), unreal.Name(fase)])
            return a

        etichetta = "Cella ~ %s" % chiave
        prima = set(attori.get_all_level_actors())
        try:
            a = metti_mesh(m, o, yaw0, etichetta, sy)
            if a is None:
                continue
            mio(a, etichetta)
            telecamera_passa(a)
            p = c["porta"]
            num = re.search(r"da\s*([+-]?\d+(?:\.\d+)?)\s*a\s*([+-]?\d+(?:\.\d+)?)", str(p.get("apre", "")))
            verso = 1 if not num or float(num.group(2)) > float(num.group(1)) else -1
            porta = porta_vera(classe_porta, mp, al_mondo(o, yaw0, p["cardine_cm"]), yaw0 + float(p.get("yaw", 0)), verso,
                               "Chiave_Prigione", etichetta + " porta", sy)
            if porta:
                mio(porta, etichetta + " porta")
                telecamera_passa(porta)
            pp = c["prigioniero_cm"]
            sc = c["scatola_cm"]
            ca = attori.spawn_actor_from_class(classe_cella, al_mondo(o, yaw0, pp),
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw0))
            if ca:
                mio(ca, etichetta + " segnaposto")
                ca.set_editor_property("numero", 1)
                ca.set_editor_property("centro_scatola", unreal.Vector(
                    (sc["min"][0] + sc["max"][0]) / 2.0 - pp[0], (sc["min"][1] + sc["max"][1]) / 2.0 - pp[1],
                    (sc["min"][2] + sc["max"][2]) / 2.0 - pp[2]))
                ca.set_editor_property("mezza_scatola", unreal.Vector(
                    (sc["max"][0] - sc["min"][0]) / 2.0, (sc["max"][1] - sc["min"][1]) / 2.0,
                    (sc["max"][2] - sc["min"][2]) / 2.0))
                # l'uscita all'aperto, alla quota del terreno vero
                u = al_mondo(o, yaw0, c.get("uscita_cm", (0.0, 0.0, 0.0)))
                q = terreno(mondo, u.x, u.y, o.z)
                ca.set_editor_property("uscita", unreal.Vector(u.x, u.y, q if q is not None else o.z))
                fatte += 1
        except Exception as ex:  # noqa: BLE001
            avviso("%s: non messa (%s)" % (chiave, ex))
            # (secondo controllo) niente pezzi a meta' nel livello: via tutto quello che e' nato per questa cella
            for x in attori.get_all_level_actors():
                if x and x not in prima:
                    try:
                        attori.destroy_actor(x)
                    except Exception:  # noqa: BLE001
                        pass
    scrivi("celle delle fasi 1 e 2 (gabbia della milizia, cella della caserma): %d su %d" % (fatte, len(celle)))


def fai_alloggi():
    """(16/11 sera, v0.2.6, deciso da Fra) La caserma della milizia dentro, fase 3: gli Alloggi rifatti (sala, ufficio
    della capitana, prigione con tre celle; sopra la camerata) e la Torre di guardia a piani con la scala a chiocciola
    e la porta verso la camerata. Dati della chat 3D: V:/Texture/valdorso/mura/Valdorso_caserma_milizia.json (arredi nel
    sistema di ogni edificio). I due edifici restano gli attori del villaggio (stesso posto): gli si cambia il modello.
    Si puo' rilanciare: toglie e rimette le sue cose. "Torna come prima" NON rifa' l'interno vecchio."""
    dati = leggi(ALLOGGI_DATI)
    if not dati:
        avviso("manca %s/%s" % (CARTELLA, ALLOGGI_DATI))
        return
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    classe_panca = getattr(unreal, "ValdorsoPanca", None)
    if not classe_porta:
        avviso("manca la classe del C++ ValdorsoPorta: compila")
        return
    letto = unreal.load_asset("/Game/Fab/Letto/BP_Letto")
    classe_letto = letto.generated_class() if letto else None
    mat = materiali()
    materiali_interni()
    nomi = set(dati["edifici"].keys())
    for e in dati["edifici"].values():
        for ar in e["arredi"]:
            pz = str(ar.get("pezzo") or "")
            if pz.startswith("SM_"):
                nomi.add(pz)
                if pz in COPERCHI:
                    nomi.add(COPERCHI[pz][0])
    pezzi = importa(sorted(nomi), mat)
    mancano = [n for n in dati["edifici"] if n not in pezzi]
    if mancano:
        avviso("non importati: %s: non tocco niente" % ", ".join(mancano))
        return
    sy = numero_specchio() or 1.0
    per_nome = tutti_per_nome()
    edifici = {}
    for nome_pezzo in dati["edifici"]:
        b = per_nome.get(nome_pezzo[3:])
        cb = b.get_component_by_class(unreal.StaticMeshComponent) if b else None
        if cb is None:
            avviso("nel livello manca \"%s\" (apri L_Valle): non tocco niente" % nome_pezzo[3:])
            return
        edifici[nome_pezzo] = (b, cb)
    via = 0
    vecchie = re.compile(r"^(%s)\d*$" % "|".join(re.escape(v) for v in ALLOGGI_VIA))
    for a in attori.get_all_level_actors():
        if not a:
            continue
        if a.actor_has_tag(ALLOGGI) or (vecchie.match(a.get_actor_label()) and a.actor_has_tag("ValdorsoVillaggio")):
            attori.destroy_actor(a)
            via += 1
    scrivi("tolte %d cose di prima (letti, luci, porte, scala a pioli, o il lancio di prima)" % via)

    def mio(a, nome):
        a.set_actor_label(nome)
        a.set_folder_path("Valle/Caserma/Dentro")
        a.set_editor_property("tags", [unreal.Name(ALLOGGI), unreal.Name(FASE_MURA)])
        return a

    conti = {}
    for nome_pezzo, e in dati["edifici"].items():
        b, cb = edifici[nome_pezzo]
        nome_b = nome_pezzo[3:]
        if cb.get_editor_property("static_mesh") != pezzi[nome_pezzo]:
            cb.set_editor_property("static_mesh", pezzi[nome_pezzo])
        # (i materiali messi a mano sull'attore di prima non valgono per il modello nuovo)
        try:
            cb.empty_override_materials()
        except Exception:  # noqa: BLE001
            pass
        # (16/11 notte, errore di Claude corretto, foto di Fra) gli attori del villaggio hanno la scala Y = -1 (i modelli
        # del villaggio arrivano specchiati); i modelli nuovi sono come quelli delle mura (correzione sy): con la scala
        # vecchia l'edificio usciva specchiato (portico dalla parte sbagliata, porta della torre contro il muro)
        b.set_actor_scale3d(unreal.Vector(1.0, sy, 1.0))
        scrivi("%s: modello nuovo (scala Y %+.0f)" % (nome_b, sy))
        o = b.get_actor_location()
        yaw0 = b.get_actor_rotation().yaw
        for ar in e["arredi"]:
            ruolo = ar.get("ruolo")
            nome = "%s %s" % (nome_b, ar["nome"])
            dove = al_mondo(o, yaw0, ar["posizione_cm"])
            yaw = yaw0 + float(ar.get("yaw", 0))
            rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)
            a = None
            try:
                if ruolo in ("porta", "porta_cella"):
                    m = pezzi.get(ar.get("pezzo"))
                    if m is None:
                        avviso("%s: manca il modello %s" % (nome, ar.get("pezzo")))
                        continue
                    # "yaw da 0 a +90": se il secondo numero e' piu' grande del primo gira nel verso +1
                    num = re.search(r"da\s*([+-]?\d+(?:\.\d+)?)\s*a\s*([+-]?\d+(?:\.\d+)?)", str(ar.get("apre", "")))
                    verso = 1 if not num or float(num.group(2)) > float(num.group(1)) else -1
                    # le celle con la chiave della prigione, l'ufficio con quella della caserma
                    chiave = "Chiave_Prigione" if ruolo == "porta_cella" else ("Chiave_Caserma" if ar.get("a_chiave") else None)
                    # (16/11 notte) la chat 3D scrive anche quale chiave: "caserma" o "prigione"
                    chiave = {"caserma": "Chiave_Caserma", "prigione": "Chiave_Prigione"}.get(str(ar.get("chiave")), chiave)
                    a = porta_vera(classe_porta, m, dove, yaw, verso, chiave, nome, sy)
                elif ruolo == "letto":
                    if classe_letto is None:
                        avviso("manca /Game/Fab/Letto/BP_Letto: niente letti")
                        continue
                    a = attori.spawn_actor_from_class(classe_letto, dove, rot)
                elif ruolo == "panca":
                    if classe_panca is None:
                        continue
                    a = attori.spawn_actor_from_class(classe_panca, dove, rot)
                    if a and "Trave" in mat:
                        for comp in a.get_components_by_class(unreal.StaticMeshComponent):
                            comp.set_material(0, mat["Trave"])
                elif ruolo == "tavolo":
                    asset = unreal.load_asset(str(ar.get("pezzo")))
                    if not isinstance(asset, unreal.StaticMesh):
                        avviso("%s: manca %s" % (nome, ar.get("pezzo")))
                        continue
                    # come crea_villaggio.py ("modo scatola"): la posizione e' il centro in basso della scatola del modello
                    bb = asset.get_bounding_box()
                    cx, cy = (bb.min.x + bb.max.x) / 2.0, (bb.min.y + bb.max.y) / 2.0
                    a = attori.spawn_actor_from_object(asset, al_mondo(dove, yaw, (-cx, -cy, -bb.min.z)), rot)
                elif ruolo in ("scrivania", "baule") or (ruolo == "sedia" and ar.get("pezzo")):
                    m = pezzi.get(ar.get("pezzo"))
                    if m is None:
                        avviso("%s: manca il modello %s" % (nome, ar.get("pezzo")))
                        continue
                    a = metti_mesh(m, dove, yaw, nome, sy)
                    cop = COPERCHI.get(ar.get("pezzo"))
                    if a and cop and pezzi.get(cop[0]):
                        c2 = metti_mesh(pezzi[cop[0]], al_mondo(dove, yaw, cop[1]), yaw, nome + " coperchio", sy)
                        if c2:
                            mio(c2, nome + " coperchio")
                elif ruolo == "luce":
                    a = attori.spawn_actor_from_class(unreal.PointLight, dove, rot)
                    if a:
                        lc = a.get_component_by_class(unreal.PointLightComponent)
                        lc.set_mobility(unreal.ComponentMobility.MOVABLE)
                        try:
                            lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
                        except Exception:  # noqa: BLE001
                            pass
                        lc.set_intensity(float(ar.get("intensita", 4.0)))
                        # corte e senza ombre: ogni stanza ha la sua, e non devono passare troppo dai solai
                        lc.set_attenuation_radius(float(ar.get("raggio", 600.0)))
                        lc.set_light_color(unreal.LinearColor(1.0, 0.62, 0.32, 1.0))
                        lc.set_cast_shadows(False)
                else:
                    # (un posto senza modello)
                    conti["solo il posto (%s)" % ruolo] = conti.get("solo il posto (%s)" % ruolo, 0) + 1
                    continue
            except Exception as ex:  # noqa: BLE001
                avviso("%s (%s): %s" % (nome, ruolo, ex))
                # (secondo controllo) una cosa messa a meta' non resta nel livello senza la sua etichetta
                if a is not None:
                    try:
                        attori.destroy_actor(a)
                    except Exception:  # noqa: BLE001
                        pass
                continue
            if a is None:
                avviso("%s (%s): non messo" % (nome, ruolo))
                continue
            mio(a, nome)
            conti[ruolo] = conti.get(ruolo, 0) + 1
        # (16/11 notte, v0.2.6) le celle vere: un attore AValdorsoCella nel punto del prigioniero, girato come
        # l'edificio, con la scatola dell'interno nel suo sistema e il punto dove si esce a fine pena (sotto il portico,
        # davanti all'ingresso). Il C++ (AValdorsoPlayerState) ci porta chi e' arrestato e ce lo tiene.
        classe_cella = getattr(unreal, "ValdorsoCella", None)
        if e.get("celle") and classe_cella is None:
            avviso("manca la classe del C++ ValdorsoCella (compila): le celle restano solo stanze")
        ingresso = e.get("ingresso", {}).get("vano_cm", [-715, -595])
        uscita = al_mondo(o, yaw0, ((ingresso[0] + ingresso[1]) / 2.0, -560.0, 0.0))
        for c in e.get("celle", []):
            sc = c["scatola_cm"]
            a1, a2 = al_mondo(o, yaw0, sc["min"]), al_mondo(o, yaw0, sc["max"])
            pr = al_mondo(o, yaw0, c["prigioniero_cm"])
            if classe_cella is not None:
                try:
                    ca = attori.spawn_actor_from_class(classe_cella, pr, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw0))
                    if ca:
                        mio(ca, "%s cella %s" % (nome_b, c["cella"]))
                        pp = c["prigioniero_cm"]
                        ca.set_editor_property("numero", int(c["cella"]))
                        ca.set_editor_property("centro_scatola", unreal.Vector(
                            (sc["min"][0] + sc["max"][0]) / 2.0 - pp[0], (sc["min"][1] + sc["max"][1]) / 2.0 - pp[1],
                            (sc["min"][2] + sc["max"][2]) / 2.0 - pp[2]))
                        ca.set_editor_property("mezza_scatola", unreal.Vector(
                            (sc["max"][0] - sc["min"][0]) / 2.0, (sc["max"][1] - sc["min"][1]) / 2.0,
                            (sc["max"][2] - sc["min"][2]) / 2.0))
                        ca.set_editor_property("uscita", uscita)
                        conti["celle del C++"] = conti.get("celle del C++", 0) + 1
                except Exception as ex:  # noqa: BLE001
                    avviso("cella %s: non messa (%s)" % (c["cella"], ex))
            scrivi("cella %s: scatola (%.0f, %.0f, %.0f)-(%.0f, %.0f, %.0f), prigioniero (%.0f, %.0f, %.0f)"
                   % (c["cella"], min(a1.x, a2.x), min(a1.y, a2.y), a1.z, max(a1.x, a2.x), max(a1.y, a2.y), a2.z,
                      pr.x, pr.y, pr.z))
    scrivi("messi: " + ", ".join("%s %d" % (n, c) for n, c in sorted(conti.items())))
    # le altre cose della fase 3 che si trovano dentro i due edifici (da guardare: i muri dentro sono cambiati)
    dentro = set()
    for b, _ in edifici.values():
        bo, be = b.get_actor_bounds(False)
        for x in attori.get_all_level_actors():
            if not x or x == b or x.actor_has_tag(ALLOGGI) or x.is_temporarily_hidden_in_editor():
                continue
            fasi = [str(t) for t in x.get_editor_property("tags") if str(t).startswith("ValdorsoFase ")]
            if fasi:
                try:
                    da, fino = [int(v) for v in fasi[0][len("ValdorsoFase "):].split("-")]
                except ValueError:
                    continue
                if not (da <= 3 and (fino == 0 or fino >= 3)):
                    continue
            xo, xe = x.get_actor_bounds(False)
            if max(xe.x, xe.y) > 1500 or max(xe.x, xe.y, xe.z) < 5:
                continue
            if abs(xo.x - bo.x) < be.x and abs(xo.y - bo.y) < be.y and abs(xo.z - bo.z) < be.z:
                dentro.add(x.get_actor_label())
    scrivi("altre cose della fase 3 dentro alloggi e torre (da guardare): " + (", ".join(sorted(dentro)[:40]) or "niente"))
    try:
        sistema_panche()
    except Exception as ex:  # noqa: BLE001
        avviso("panche della mensa non sistemate (%s)" % ex)
    try:
        fai_celle_fasi()
    except Exception as ex:  # noqa: BLE001
        avviso("celle delle fasi 1 e 2 non messe (%s)" % ex)
    mostra_fase(3)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("FATTO e salvato. Poi 'Rete: le cose che si usano sempre caricate' e 'PNG: panche, letti e porte'. "
           "In Play: Valdorso.Villaggio.Fase 3, Valdorso.Zaino.Dai Chiave_Caserma (ufficio), Chiave_Prigione (celle); "
           "Valdorso.Cella.Secondi 30 e Valdorso.Colpa.Cella per provare la prigione")


def mostra_fase(fase):
    """Nell'editor fa vedere solo le cose di una fase (come "Villaggio: mostra la fase N"): dopo aver messo cose nuove
    l'editor le mostrava tutte insieme e sembrava che le strade vecchie ci fossero ancora (foto di Fra, 16/11)."""
    for a in attori.get_all_level_actors():
        if not a:
            continue
        for t in a.get_editor_property("tags"):
            x = str(t)
            if x.startswith("ValdorsoFase "):
                try:
                    da, fino = [int(v) for v in x[len("ValdorsoFase "):].split("-")]
                except ValueError:
                    break
                a.set_is_temporarily_hidden_in_editor(not (da <= fase and (fino == 0 or fase <= fino)))
                break
    scrivi("nell'editor ora si vede la fase %d (per cambiare: 'Villaggio: mostra la fase N')" % fase)


def fai_strade():
    """(16/11 sera, v0.2.6) Le strade della chat 3D, le cose spostate dalla fase 3 e le maschere del terreno, lette da
    Villaggio_3000.json. Non si rifa' il villaggio intero (rifarebbe anche la cappella e le mura vecchie): i pezzi di
    strada hanno tutti la stessa origine delle Strade del villaggio e ne prendono posizione, materiale e impostazioni."""
    f = STRADE + "/Villaggio_3000.json"
    if not os.path.exists(f):
        avviso("manca %s" % f)
        return
    with open(f, "r", encoding="utf-8") as g:
        cose = json.load(g)["cose"]
    solo2 = sum(1 for a in attori.get_all_level_actors()
                if a and a.get_actor_label().startswith(SOLO_FASE2) and fase_solo_2(a, True))
    scrivi("lanterne di legno delle porte larghe: solo nella fase 2 (%d cose cambiate ora)" % solo2)
    saltati = []
    for nome_base, asset_base, inizi, con_collisione in GRUPPI:
        saltati.append(nome_base)
        per_nome = tutti_per_nome()
        base = per_nome.get(nome_base)
        cb = base.get_component_by_class(unreal.StaticMeshComponent) if base else None
        if cb is None or cb.get_editor_property("static_mesh") is None:
            avviso("nel livello manca \"%s\" (apri L_Valle): salto il suo gruppo" % nome_base)
            continue
        gruppo = [c for c in cose if c.get("tipo") == "mesh" and str(c.get("asset", "")).startswith(inizi)]
        mancanti = [c["asset"] for c in gruppo if not os.path.exists("%s/%s.glb" % (STRADE, c["asset"]))]
        if mancanti:
            avviso("mancano in %s: %s (salto il gruppo di %s)" % (STRADE, ", ".join(mancanti), nome_base))
            continue
        saltati.pop()
        imp = impostazioni(cb.get_editor_property("static_mesh"))   # prima di reimportare
        strumenti.import_asset_tasks([compito("%s/%s.glb" % (STRADE, c["asset"]), STRADE_PEZZI) for c in gruppo])
        nomi = [c["nome"] for c in gruppo if c["asset"] != asset_base]
        via = 0
        for a in attori.get_all_level_actors():
            if a and a.get_actor_label() in nomi:
                attori.destroy_actor(a)
                via += 1
        for c in gruppo:
            m = cerca_e_metti_a_posto(c["asset"], STRADE_PEZZI, sposta=False)
            if m is None:
                avviso("%s non importato" % c["asset"])
                continue
            applica(m, imp)
            if c["asset"] == asset_base:
                if cb.get_editor_property("static_mesh") != m:
                    cb.set_editor_property("static_mesh", m)
                scrivi("%s: modello aggiornato" % c["nome"])
                continue
            a = attori.spawn_actor_from_object(m, base.get_actor_location(), base.get_actor_rotation())
            if a is None:
                avviso("non riesco a mettere %s" % c["nome"])
                continue
            fase = c.get("fase", [1, 0])
            come(a, base, c["nome"], "%d-%d" % (fase[0], fase[1]))
            piena = con_collisione(c)
            if not piena:
                a.get_component_by_class(unreal.StaticMeshComponent).set_collision_profile_name("NoCollision")
            scrivi("%s: messa (fasi %d-%d%s)" % (c["nome"], fase[0], fase[1], "" if piena else ", senza collisione"))
        if via:
            scrivi("%s: tolti %d pezzi del lancio di prima" % (nome_base, via))

    # le cose spostate dalla fase 3: l'originale fino alla fase 2, una copia spostata dalla fase 3
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(STRADE_NUOVE):
            attori.destroy_actor(a)
    per_nome = tutti_per_nome()
    voci = {}
    for c in cose:
        voci.setdefault(c["nome"], c)
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    spostate = 0
    for c in cose:
        if SEGNO_FASE3 not in c["nome"] or str(c.get("asset", "")).startswith(TUTTI_GLI_INIZI):
            continue
        vecchia = c["nome"].replace(SEGNO_FASE3, "")
        if "Lanterne" in saltati and vecchia.startswith("Luce lanterna "):
            # (secondo controllo) il palo non si e' spostato: la sua luce resta dov'e'
            avviso("%s: non spostata (i lampioni non sono stati aggiornati)" % vecchia)
            continue
        prima, a = voci.get(vecchia), per_nome.get(vecchia)
        if prima is None or a is None:
            avviso("non trovo %s (per %s)" % (vecchia, c["nome"]))
            continue
        if per_nome.get(c["nome"]) is not None:
            # (secondo controllo) c'e' gia' (messa da crea_villaggio.py, che legge lo stesso json): niente doppioni
            fase_solo_2(a, True)
            scrivi("%s c'e' gia': non la copio" % c["nome"])
            continue
        if a.actor_has_tag(SPENTA):
            avviso("%s e' spenta: non la copio" % vecchia)
            continue
        d = [c["pos"][i] - prima["pos"][i] for i in range(3)]
        fase_solo_2(a, True)
        copia = attori.duplicate_actor(a, mondo, unreal.Vector(d[0], d[1], d[2]))
        if copia is None:
            avviso("non riesco a copiare %s" % vecchia)
            continue
        tags = [str(t) for t in copia.get_editor_property("tags")]
        tags = [t for t in tags if not t.startswith("ValdorsoFase ") and not t.startswith(PRIMA) and t != CAMBIATA]
        copia.set_editor_property("tags", [unreal.Name(t) for t in tags + ["ValdorsoFase 3-0", STRADE_NUOVE]])
        copia.set_actor_label(c["nome"])
        # (secondo controllo) fuori dai modelli semplici da lontano (HLOD), o da lontano si vedrebbero tutte e due
        for x in (a, copia):
            for cmp_ in x.get_components_by_class(unreal.StaticMeshComponent):
                try:
                    cmp_.set_editor_property("enable_auto_lod_generation", False)
                except Exception:  # noqa: BLE001
                    pass
        spostate += 1
        scrivi("%s: dalla fase 3 spostata di (%+.0f, %+.0f, %+.0f) cm; quella di prima fino alla fase 2"
               % (vecchia, d[0], d[1], d[2]))
        # che cosa c'e' gia' dove va una casa (alberi, lanterne...): lo si scrive, non si tocca
        if c.get("tipo") == "mesh" and c.get("sezione") == "Villaggio":
            l = copia.get_actor_location()
            vicine = []
            for b in attori.get_all_level_actors():
                if not b or b == copia or b == a or b.actor_has_tag(STRADE_NUOVE):
                    continue
                o, e = b.get_actor_bounds(False)
                if max(e.x, e.y) > 3000:
                    continue
                if abs(o.x - l.x) < 600 + e.x and abs(o.y - l.y) < 600 + e.y:
                    vicine.append(b.get_actor_label())
            scrivi("   vicino a %s (entro 6 m): %s" % (c["nome"], ", ".join(sorted(set(vicine))[:25]) or "niente"))
    scrivi("cose spostate dalla fase 3: %d" % spostate)

    # le maschere del terreno: le rilegge il materiale della valle (come alla fine di crea_villaggio.py)
    valle = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()),
                         "Python", "usati", "crea_materiale_valle.py")
    try:
        with open(valle, "r", encoding="utf-8") as g:
            exec(compile(g.read(), valle, "exec"), {"__name__": "__main__"})
        scrivi("materiale della valle rifatto (T_ValleMaschere e T_SenzaErba nuove)")
    except Exception as e:  # noqa: BLE001
        avviso("materiale della valle non rifatto (%s): lanciare a mano usati/crea_materiale_valle.py" % e)
    mostra_fase(3)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    scrivi("FATTO e salvato. Poi 'PNG: panche, letti e porte' (le porte delle case spostate).")


# ------------------------------------------------------------------------------------------------ gradini da soglia
# (17/11, v0.2.7) Il collaudo dei passaggi (nuovi/collauda_passaggi.py) ha trovato una cinquantina di case con il
# pavimento che finisce a 80-90 cm dalla porta e scende di 20-96 cm sul terreno, senza gradino. La chat 3D ha fatto un
# kit: SM_Gradino_Soglia_1..6 (alzate di 18 cm, pedata 30, larghi 140; origine a terra al centro del lato che tocca la
# casa, scendono verso +X; Z dell'origine = Z del pavimento - N x 18). "Mura: 7" legge le porte segnate dal collaudo
# (Saved/Claude/collaudo_passaggi.json), rimisura da se' il bordo del pavimento e il salto fuori da ogni porta e mette
# il gradino giusto, con la fase della porta. Si puo' rilanciare: toglie e rimette i suoi gradini.

GRADINI = "ValdorsoGradiniSoglia"
GRADINI_CARTELLA = "Valle/Villaggio/Gradini"
GRADINO_ALZATA = 18.0
GRADINO_MINIMO = 20.0          # sotto questo salto non serve un gradino
GRADINO_TERRA = ("Landscape", "Strade", "Piazza", "Strada")   # su che cosa puo' appoggiare un gradino


def fase_di_attore(a):
    for t in a.get_editor_property("tags"):
        x = str(t)
        if x.startswith("ValdorsoFase "):
            try:
                da, fino = [int(v) for v in x[len("ValdorsoFase "):].split("-")]
                return da, fino, x
            except ValueError:
                return None
    return None


def linea_colono(mondo, a, b, ignora):
    """Una linea con il profilo del colono (Pawn): la tupla del colpo o None."""
    try:
        r = unreal.SystemLibrary.line_trace_single_by_profile(mondo, a, b, "Pawn", False, ignora,
                                                              unreal.DrawDebugTrace.NONE, True)
    except Exception:  # noqa: BLE001
        r = unreal.SystemLibrary.line_trace_single(mondo, a, b, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora,
                                                   unreal.DrawDebugTrace.NONE, True)
    if r is None:
        return None
    if isinstance(r, tuple):
        if len(r) == 2 and isinstance(r[0], bool):
            if not r[0] or r[1] is None:
                return None
            r = r[1]
        else:
            return r if r and r[0] else None
    t = r.to_tuple()
    return t if t and t[0] else None


def chi_e(t):
    a = t[9] if len(t) > 9 else None
    if a is None:
        return ""
    return a.get_class().get_name() + " " + a.get_actor_label()


def misura_soglia(mondo, cx, cy, base, dx, dy, ignora):
    """Dal centro della porta verso (dx, dy): dove finisce il pavimento e quanto si scende.
    Ritorna (distanza del bordo, quota del pavimento, salto, su che cosa si scende) oppure None se non c'e' un salto
    verso il terreno entro 1,5 m."""
    t = linea_colono(mondo, unreal.Vector(cx, cy, base + 120.0), unreal.Vector(cx, cy, base - 150.0), ignora)
    if t is None:
        return None
    z = t[4].z
    s = 10.0
    while s <= 150.0:
        x, y = cx + dx * s, cy + dy * s
        t = linea_colono(mondo, unreal.Vector(x, y, z + 60.0), unreal.Vector(x, y, z - 250.0), ignora)
        if t is None or t[3] < 1.0:
            return None                      # il vuoto, o un muro: qui un gradino non serve
        zn = t[4].z
        if zn - z > 45.0:
            return None
        if z - zn > 12.0:
            # il bordo: si guarda il terreno 15 cm piu' in la' (dove appoggia il primo gradino in basso)
            sotto = chi_e(t)
            t2 = linea_colono(mondo, unreal.Vector(x + dx * 15.0, y + dy * 15.0, z + 60.0),
                              unreal.Vector(x + dx * 15.0, y + dy * 15.0, z - 250.0), ignora)
            if t2 is not None and t2[3] >= 1.0 and (t2[4].z <= zn or not any(k in sotto for k in GRADINO_TERRA)):
                zn = min(zn, t2[4].z)
                sotto = chi_e(t2) or sotto
            if any(k in sotto for k in GRADINO_TERRA):
                return s, z, z - zn, sotto
            # scende su un pezzo della casa (uno zoccolo, un suo scalino): si va avanti da li'
            zn = t[4].z
        z = zn
        s += 2.0
    return None


def fai_gradini():
    """(17/11, v0.2.7) "Mura: 7. i gradini davanti alle porte": vedi sopra."""
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    f = os.path.join(cartella, "collaudo_passaggi.json")
    if not os.path.exists(f):
        avviso("manca %s: lancia prima 'Per Claude: collauda i passaggi'" % f)
        return
    with open(f, "r", encoding="utf-8") as g:
        collaudo = json.load(g).get("porte", [])
    # (secondo controllo) tutte le porte del collaudo, non solo quelle segnate: dopo il primo giro le porte sistemate
    # tornano OK, e rilanciando si toglierebbero i loro gradini. Chi non ha un salto sul terreno resta senza gradino.
    segnate = {}
    for r in collaudo:
        segnate.setdefault(r["porta"], r)
    scrivi("porte del collaudo: %d" % len(segnate))
    pezzi = importa(["SM_Gradino_Soglia_%d" % n for n in range(1, 7)], materiali())
    if len(pezzi) < 6:
        avviso("gradini da soglia: mancano dei pezzi del kit (%d su 6): niente" % len(pezzi))
        return
    tutti = [a for a in attori.get_all_level_actors() if a]
    tolti = 0
    for a in tutti:
        if a.actor_has_tag(GRADINI):
            attori.destroy_actor(a)
            tolti += 1
    if tolti:
        scrivi("tolti %d gradini messi prima" % tolti)
        tutti = [a for a in attori.get_all_level_actors() if a]
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    if mondo is None or classe_porta is None:
        avviso("gradini: manca il mondo o la classe C++ ValdorsoPorta (compila e riapri l'editor)")
        return
    per_nome = {}
    porte = []
    con_fase = []
    for a in tutti:
        per_nome.setdefault(a.get_actor_label(), a)
        if isinstance(a, classe_porta) or a.get_class().get_name() in ("ValdorsoGrata", "ValdorsoCella"):
            porte.append(a)
        fa = fase_di_attore(a)
        if fa is not None:
            try:
                o, e = a.get_actor_bounds(False)
                con_fase.append((a, fa, o.x, o.y, e.x, e.y))
            except Exception:  # noqa: BLE001
                con_fase.append((a, fa, 0.0, 0.0, 1e9, 1e9))
    messi = 0
    conti = {}
    nuovi = []
    for nome in sorted(segnate):
        r = segnate[nome]
        porta = per_nome.get(nome)
        if porta is None:
            avviso("gradini: nel livello non trovo la porta '%s'" % nome)
            continue
        # le porte delle mura, delle celle e degli alloggi hanno gia' i loro gradini; le capanne si guardano a parte
        if (porta.actor_has_tag(ETICHETTA) or porta.actor_has_tag(CELLE_FASI) or porta.actor_has_tag(ALLOGGI)
                or porta.actor_has_tag(CASERMA) or porta.actor_has_tag(PASSO_REALE) or nome.startswith("Capanna")):
            continue
        cx, cy, base = [float(v) for v in r["mondo_cm"]]
        yaw = math.radians(float(r["passaggio_yaw"]))
        fp = fase_di_attore(porta)
        fase = fp[0] if fp else 3
        ignora = list(porte) + nuovi
        for a, fa, ox, oy, ex, ey in con_fase:
            if not (fa[0] <= fase and (fa[1] == 0 or fase <= fa[1])):
                if abs(ox - cx) - ex <= 600.0 and abs(oy - cy) - ey <= 600.0 and a not in ignora:
                    ignora.append(a)
        trovato = None
        for verso in (1.0, -1.0):
            dx, dy = math.cos(yaw) * verso, math.sin(yaw) * verso
            m = misura_soglia(mondo, cx, cy, base, dx, dy, ignora)
            if m is not None and m[2] > GRADINO_MINIMO and (trovato is None or m[2] > trovato[0][2]):
                trovato = (m, dx, dy)
        if trovato is None:
            continue
        (bordo, pavimento, salto, sotto), dx, dy = trovato
        n = int(math.ceil(salto / GRADINO_ALZATA))
        # (secondo controllo) se il terreno scende ancora, al piede del gradino il salto e' piu' grande: si rimisura
        # li' (una volta) e, se serve, si prende il pezzo con un'alzata in piu'
        piede = bordo + 30.0 * min(n, 6) - 10.0
        tp = linea_colono(mondo, unreal.Vector(cx + dx * piede, cy + dy * piede, pavimento + 60.0),
                          unreal.Vector(cx + dx * piede, cy + dy * piede, pavimento - 300.0), ignora)
        if tp is not None and tp[3] >= 1.0 and any(k in chi_e(tp) for k in GRADINO_TERRA):
            salto = max(salto, pavimento - tp[4].z)
            n = int(math.ceil(salto / GRADINO_ALZATA))
        if n > 6:
            avviso("gradini: %s ha un salto di %.0f cm, piu' dei 108 del kit: metto il _6, da guardare" % (nome, salto))
            n = 6
        lungo = 5.0 + 30.0 * n
        x, y = cx + dx * (bordo - 2.0), cy + dy * (bordo - 2.0)
        # c'e' posto davanti? una linea a 40 cm sopra il pavimento, lunga come il gradino piu' mezzo metro
        t = linea_colono(mondo, unreal.Vector(x + dx * 5.0, y + dy * 5.0, pavimento + 40.0),
                         unreal.Vector(x + dx * (lungo + 50.0), y + dy * (lungo + 50.0), pavimento + 40.0), ignora)
        davanti = ""
        if t is not None and "Landscape" not in chi_e(t):
            davanti = "; ATTENZIONE: davanti a %.0f cm c'e' %s" % (t[3], chi_e(t))
        a = attori.spawn_actor_from_object(pezzi["SM_Gradino_Soglia_%d" % n],
                                           unreal.Vector(x, y, pavimento - n * GRADINO_ALZATA),
                                           unreal.Rotator(roll=0.0, pitch=0.0, yaw=math.degrees(math.atan2(dy, dx))))
        if a is None:
            avviso("gradini: non riesco a mettere il gradino di %s" % nome)
            continue
        a.set_actor_label("Gradino ~ %s" % nome)
        a.set_folder_path(GRADINI_CARTELLA)
        a.set_editor_property("tags", [unreal.Name(GRADINI)] + ([unreal.Name(fp[2])] if fp else []))
        messi += 1
        nuovi.append(a)
        conti[n] = conti.get(n, 0) + 1
        scrivi("  %s: salto %.0f cm su %s, bordo a %.0f cm dalla porta -> SM_Gradino_Soglia_%d%s%s"
               % (nome, salto, sotto, bordo, n, " (%s)" % fp[2] if fp else "", davanti))
    scrivi("gradini messi: %d (%s)" % (messi, ", ".join("_%d: %d" % (k, conti[k]) for k in sorted(conti))))
    # (17/11) la lanterna della caserma della fase 2 stava davanti alla porta, ora anche davanti al gradino: nei dati
    # del villaggio e' stata spostata di 1,5 m a est; qui si portano li' anche le due cose gia' nel livello
    try:
        with open(STRADE + "/Villaggio_3000.json", "r", encoding="utf-8-sig") as g:
            cose = json.load(g).get("cose", [])
        angolo = angolo_paesaggio()
        for c in cose:
            if c.get("nome") in ("Lanterna della caserma", "Lanterna della caserma luce 0") and angolo is not None:
                a = per_nome.get(c["nome"])
                if a is None:
                    continue
                dove = a.get_actor_location()
                nx, ny = angolo.x + float(c["pos"][0]), angolo.y + float(c["pos"][1])
                if abs(dove.x - nx) > 1.0 or abs(dove.y - ny) > 1.0:
                    a.modify()       # (17/11 sera) senza, lo spostamento non si salvava e alla riapertura tornava com'era
                    a.set_actor_location(unreal.Vector(nx, ny, dove.z), False, False)
                    scrivi("  %s spostata di (%+.0f, %+.0f) cm, come nei dati del villaggio" % (c["nome"], nx - dove.x, ny - dove.y))
    except Exception as e:  # noqa: BLE001
        avviso("lanterna della caserma non spostata (%s)" % e)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    scrivi("FATTO e salvato. Ora rilancia 'Per Claude: collauda i passaggi' per vedere che cosa resta.")


# ------------------------------------------------------------------------------------------------ scale di fuori
# (17/11, v0.2.7) Tre porte nostre (Mulino, Casermetta_della_Guardia, Torre_del_passo) stanno 3-4 m sopra il terreno
# senza scala: il collaudo dei passaggi le dava BLOCCO. Tools/Generatori/scale_esterne.py fa per ognuna una scala di
# legno (ballatoio + rampa lungo la facciata fino al terreno vero) e scrive V:/Texture/valdorso/mura/
# Valdorso_scale_esterne.json. "Mura: 8" le importa e le mette: l'origine sul bordo del pavimento davanti alla porta
# (rimisurato qui), +X verso fuori. Si puo' rilanciare: toglie e rimette le sue scale.

TEMPIO_IN_PIU = ("SM_Tempio_Fase2_Dentro", "SM_Tempio_Fase3_Dentro", "SM_Tempio_Fase2_Sagrato", "SM_Tempio_Fase3_Sagrato")
TEMPIO_DENTRO_DATI = "Valdorso_tempio_dentro.json"
PONTI_DATI = "Valdorso_ponti.json"


def fai_ponti():
    """(17/11 sera, v0.2.7) "Mura: 9. i ponti veri sul fiume": i due ponti della chat 3D (SM_Ponte_Mulino, SM_Ponte_Nord)
    al posto dei nostri, sugli stessi attori del villaggio: stessa origine e stesso yaw. Gli attori del villaggio hanno
    la scala Y = -1, i pezzi della chat 3D no: la scala diventa (1, sy, 1), come per gli alloggi."""
    dati = leggi(PONTI_DATI)
    if not dati or not dati.get("pezzi"):
        scrivi("ponti: niente (manca %s)" % PONTI_DATI)
        return
    angolo = angolo_paesaggio()
    pezzi = importa(sorted(dati["pezzi"]), materiali())
    sy = numero_specchio() or 1.0
    per_nome = tutti_per_nome()
    fatti = 0
    for asset, d in sorted(dati["pezzi"].items()):
        m = pezzi.get(asset)
        b = per_nome.get(d.get("nome"))
        if m is None or b is None:
            avviso("ponti: manca il modello %s o nel livello non c'e' '%s'" % (asset, d.get("nome")))
            continue
        cb = b.get_component_by_class(unreal.StaticMeshComponent)
        if cb is None:
            avviso("ponti: '%s' non ha un modello" % d.get("nome"))
            continue
        dove = b.get_actor_location()
        yaw = b.get_actor_rotation().yaw
        o = d.get("origine_mondo_cm")
        if angolo is not None and o:
            scarto = math.hypot(dove.x - (angolo.x + o[0]), dove.y - (angolo.y + o[1]))
            dy = (yaw - float(d.get("yaw", yaw)) + 180.0) % 360.0 - 180.0
            scrivi("  %s: scarto dall'origine dei dati %.1f cm in pianta, %.1f cm in quota, %.1f gradi di yaw"
                   % (d["nome"], scarto, dove.z - o[2], dy))
            if scarto > 5.0 or abs(dove.z - o[2]) > 5.0 or abs(dy) > 0.5:
                avviso("ponti: %s non sta dove dicono i dati: lascio il ponte di prima" % d["nome"])
                continue
        b.modify()
        cb.set_editor_property("static_mesh", m)
        b.set_actor_scale3d(unreal.Vector(1.0, sy, 1.0))
        for i in range(len(m.get_editor_property("static_materials"))):
            cb.set_material(i, m.get_material(i))
        fatti += 1
        scrivi("  %s: ora e' %s" % (d["nome"], asset))
    scrivi("ponti cambiati: %d su %d" % (fatti, len(dati["pezzi"])))
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    scrivi("FATTO e salvato.")


SCALE_FUORI = "ValdorsoScaleFuori"
SCALE_FUORI_DATI = "Valdorso_scale_esterne.json"
CAPANNE_RIFATTE = (("Capanna di Nonna Edda", "SM_Palude_CapannaEdda"),
                   ("Capanna di Orlando il Muto", "SM_Villaggio_CapannaOrlando"))


def fai_scale():
    dati = leggi(SCALE_FUORI_DATI)
    if not dati or not dati.get("scale"):
        scrivi("scale di fuori: niente (manca %s)" % SCALE_FUORI_DATI)
        return
    angolo = angolo_paesaggio()
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    if angolo is None or mondo is None or classe_porta is None:
        avviso("scale di fuori: manca il paesaggio o la classe C++ ValdorsoPorta")
        return
    pezzi = importa(sorted(set(sc["pezzo"] for sc in dati["scale"])), materiali())
    sy = numero_specchio() or 1.0
    tutti = [a for a in attori.get_all_level_actors() if a]
    tolte = 0
    for a in tutti:
        if a.actor_has_tag(SCALE_FUORI):
            attori.destroy_actor(a)
            tolte += 1
    if tolte:
        scrivi("tolte %d scale messe prima" % tolte)
        tutti = [a for a in attori.get_all_level_actors() if a]
    per_nome = {}
    porte = []
    for a in tutti:
        per_nome.setdefault(a.get_actor_label(), a)
        if isinstance(a, classe_porta):
            porte.append(a)
    messe = 0
    for sc in dati["scale"]:
        m = pezzi.get(sc["pezzo"])
        porta = per_nome.get(sc["porta"])
        if m is None or porta is None:
            avviso("scale di fuori: manca il modello %s o la porta '%s'" % (sc["pezzo"], sc["porta"]))
            continue
        if porta.actor_has_tag(SPENTA):
            # (18/11 sera) la casermetta e la torre del passo di prima sono spente ("Mura: 11"): niente scala
            scrivi("scale di fuori: '%s' e' spenta, la sua scala non si mette" % sc["porta"])
            continue
        yaw = float(sc["yaw"])
        dx, dy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        ox, oy = angolo.x + float(sc["origine_pianta_cm"][0]), angolo.y + float(sc["origine_pianta_cm"][1])
        pav = float(sc["origine_pianta_cm"][2])
        # si rimisura: il pavimento 40 cm prima del bordo dei dati, poi in fuori a passi di 2 cm finche' finisce
        bordo = None
        t = linea_colono(mondo, unreal.Vector(ox - dx * 40.0, oy - dy * 40.0, pav + 100.0),
                         unreal.Vector(ox - dx * 40.0, oy - dy * 40.0, pav - 100.0), porte)
        if t is not None and t[3] >= 1.0 and abs(t[4].z - pav) <= 30.0:
            z = t[4].z
            d = -40.0
            while d <= 60.0:
                t2 = linea_colono(mondo, unreal.Vector(ox + dx * d, oy + dy * d, z + 40.0),
                                  unreal.Vector(ox + dx * d, oy + dy * d, z - 40.0), porte)
                if t2 is None or t2[3] < 1.0 or abs(t2[4].z - z) > 12.0:
                    bordo = d - 2.0
                    break
                d += 2.0
            if bordo is not None:
                scrivi("  %s: pavimento misurato a z %.1f (dati %.1f), bordo a %+.0f cm da quello dei dati"
                       % (sc["porta"], z, pav, bordo))
                pav = z
        if bordo is None:
            avviso("scale di fuori: %s: non riesco a rimisurare il bordo del pavimento, uso i dati" % sc["porta"])
            bordo = 0.0
        dove = unreal.Vector(ox + dx * bordo, oy + dy * bordo, pav)
        a = attori.spawn_actor_from_object(m, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
        if a is None:
            avviso("scale di fuori: non riesco a mettere la scala di %s" % sc["porta"])
            continue
        if sy < 0:
            a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
        a.set_actor_label("Scala ~ %s" % sc["porta"])
        a.set_folder_path("Valle/Villaggio/Scale di fuori")
        fp = fase_di_attore(porta)
        a.set_editor_property("tags", [unreal.Name(SCALE_FUORI)] + ([unreal.Name(fp[2])] if fp else []))
        messe += 1
        scrivi("  %s: %s, %d gradini verso %+d, scende %.0f cm" % (sc["porta"], sc["pezzo"], int(sc["gradini"]),
                                                                  int(sc["verso_rampa"]), float(sc["scende_cm"])))
    scrivi("scale di fuori messe: %d su %d" % (messe, len(dati["scale"])))
    # (17/11) le due capanne su pali: il generatore le ha rifatte con la gronda piu' alta davanti alla porta
    # (Tools/Generatori/villaggio_edifici.py, capanna_palafitta); qui si reimportano tenendo materiali e impostazioni
    for nome, asset in CAPANNE_RIFATTE:
        try:
            f = "%s/%s.glb" % (STRADE, asset)
            b = per_nome.get(nome)
            cb = b.get_component_by_class(unreal.StaticMeshComponent) if b else None
            vecchio = cb.get_editor_property("static_mesh") if cb else None
            if not os.path.exists(f) or vecchio is None:
                avviso("capanne: manca %s o nel livello non c'e' '%s'" % (f, nome))
                continue
            imp = impostazioni(vecchio)
            strumenti.import_asset_tasks([compito(f, STRADE_PEZZI)])
            m = cerca_e_metti_a_posto(asset, STRADE_PEZZI, sposta=False)
            if m is None:
                avviso("capanne: %s non importato" % asset)
                continue
            applica(m, imp)
            if cb.get_editor_property("static_mesh") != m:
                cb.set_editor_property("static_mesh", m)
            scrivi("  %s: modello reimportato (%s)" % (nome, asset))
        except Exception as e:  # noqa: BLE001
            avviso("capanne: %s non rifatta (%s)" % (nome, e))
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    scrivi("FATTO e salvato. Ora rilancia 'Per Claude: collauda i passaggi'.")


# ------------------------------------------------------------------------------------------------ la locanda nuova
# (17/11 notte, v0.2.7) "Mura: 10. la locanda nuova di Bruno". Il modello della chat 3D (SM_Locanda_Livia: sala con
# bancone e camino, cucina, retro dell'oste, cinque camere, la stanza grande con la vasca) va sull'attore del villaggio
# "Locanda_di_Bruno", stesso posto e stesso yaw: la porta cade dov'era (locale (-150, -500), prima (-150, -480)).
# - le cose vecchie dentro (letti, panche, tavoli, botti, "Vita Locanda_di_Bruno") si spengono con la fase 99;
# - la porta "Locanda_di_Bruno porta" resta la stessa (la usano i cammini dei PNG), spostata sulla soglia nuova e
#   allargata al vano di 140 x 240;
# - porte delle camere, letti, bauli, targhette, vasca, scrittoio, cassaforte dai dati (Valdorso_locande.json, arredi);
# - la sala: quattro tavoli con due panche ciascuno (sala_arredi);
# - il baule del colono va al suo posto nuovo. Bruno, il gatto e i posti della sera li rimettono i loro script
#   (metti_png.py, crea_volti.py, crea_gatto.py, giornate_vive.py), che hanno gia' i punti nuovi: vanno rilanciati dopo.
# Si puo' rilanciare: toglie e rimette le sue cose (etichetta LOCANDA_NUOVA).

LOCANDE = "V:/Texture/valdorso/locande"
LOCANDA_NUOVA = "ValdorsoLocandaNuova"
LOCANDA_VECCHIE = ("Locanda_di_Bruno letto", "Locanda_di_Bruno panca", "Tavolo della locanda 0", "Tavolo della locanda 1",
                   "Botte della locanda 0", "Botte della locanda 1", "Botte della locanda 2", "Vita Locanda_di_Bruno")
LOCANDA_TAVOLO = "/Game/Fab/Mercato_Tenda1/table_002"
TAVOLO_LUNGO = 140.0          # cm: il tavolo della sala in pianta (18/11: piu' piccolo)
TAVOLO_LARGO = 70.0
PANCA_DAL_TAVOLO = 65.0       # cm dal centro del tavolo al centro della panca
ZOCCOLO_FONDO = 400.0         # cm di zoccolo sotto la fondazione della locanda
# (18/11 notte) la chat 3D ha riempito di pietra la fascia sotto i muri nel modello (Valdorso-locanda-correzioni-18-11):
# lo zoccolo provvisorio non serve piu'. Quello messo prima lo toglie il passo 0 (ha l'etichetta della locanda nuova).
ZOCCOLO_SERVE = False
LETTO_CONTATO = 220.0         # cm: la lunghezza del letto con cui la chat 3D ha contato le camere (18/11)


def ingombro_mesh(a):
    """Centro e mezzo ingombro (nel mondo) dei soli modelli di un attore: senza le scatole invisibili (dove ci si sdraia,
    dove si clicca), che get_actor_bounds conterebbe."""
    lo = hi = None
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        if c.get_editor_property("static_mesh") is None:
            continue
        r = unreal.SystemLibrary.get_component_bounds(c)
        ce, ex = r[0], r[1]
        p0 = (ce.x - ex.x, ce.y - ex.y, ce.z - ex.z)
        p1 = (ce.x + ex.x, ce.y + ex.y, ce.z + ex.z)
        lo = p0 if lo is None else tuple(min(u, v) for u, v in zip(lo, p0))
        hi = p1 if hi is None else tuple(max(u, v) for u, v in zip(hi, p1))
    if lo is None:
        return a.get_actor_bounds(False)
    return (unreal.Vector((lo[0] + hi[0]) / 2.0, (lo[1] + hi[1]) / 2.0, (lo[2] + hi[2]) / 2.0),
            unreal.Vector((hi[0] - lo[0]) / 2.0, (hi[1] - lo[1]) / 2.0, (hi[2] - lo[2]) / 2.0))


MISURE_LETTO = {}             # lungo e largo (cm) del letto vero, misurati da metti_letto


def metti_letto(classe_letto, mondo, dove, yaw, nome):
    """Un letto vero (BP_Letto) dove i dati della chat 3D mettono il loro (centro 'dove', testa a +Y del pezzo girato
    di 'yaw'). Rende l'attore (o None)."""
    # (18/11 notte, Fra: "letti troppo nel muro") il nostro letto (BP_Letto) non ha l'origine nel mezzo, e' piu'
    # lungo di quello dei dati e ha il cuscino dalla parte -Y (nei dati la testa e' a +Y, contro il muro).
    # 1) Si prova girato di 180 (cuscino al muro) con il MEZZO del modello dove dicono i dati; ma dal letto si
    #    scende sempre dalla sua +X (AValdorsoLetto::TrasformaUscita, 110 cm): se li' c'e' un muro si tiene il
    #    verso dei dati (secondo controllo: nelle camere 1 e 5 e da Bruno si scenderebbe dentro il muro).
    # 2) Se e' piu' lungo dei 220 cm contati dalla chat 3D scivola verso i piedi, fin dove c'e' posto.
    # (18/11 sera) Le misure si prendono con il letto dritto (yaw 0) e poi lo si gira: cosi' vale anche per i letti messi
    # di sbieco (il quartiere del passo e' girato di 37 gradi, e li' la scatola del mondo sarebbe piu' grande del letto).
    a = attori.spawn_actor_from_class(classe_letto, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
    if a:
        c0, e0 = ingombro_mesh(a)
        q0 = a.get_actor_location()
        fx, fy = c0.x - q0.x, c0.y - q0.y          # dal punto dell'attore al mezzo del modello, nel sistema del letto
        lungo_letto = 2.0 * e0.y
        MISURE_LETTO["lungo"], MISURE_LETTO["largo"] = max(2.0 * e0.x, 2.0 * e0.y), min(2.0 * e0.x, 2.0 * e0.y)
        if e0.x > e0.y:
            avviso("%s: il modello del letto e' lungo lungo la sua X (%.0f x %.0f): il verso potrebbe essere sbagliato"
                   % (nome, 2.0 * e0.x, 2.0 * e0.y))
            lungo_letto = 2.0 * e0.x

        def metti_girato(gradi, scivola=0.0, sx=0.0, sy_=0.0):
            r = math.radians(gradi)
            a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=gradi), False)
            a.set_actor_location(unreal.Vector(dove.x - (fx * math.cos(r) - fy * math.sin(r)) + sx * scivola,
                                               dove.y - (fx * math.sin(r) + fy * math.cos(r)) + sy_ * scivola, q0.z),
                                 False, False)

        def libero_verso(dx, dy, quanto):
            """Quanti cm liberi dal mezzo del letto in quella direzione (al massimo 'quanto'), a 100 cm da terra."""
            da = unreal.Vector(dove.x, dove.y, dove.z + 100.0)
            t = linea_colono(mondo, da, unreal.Vector(dove.x + dx * quanto, dove.y + dy * quanto, dove.z + 100.0), [a])
            return quanto if t is None else float(t[3])

        verso = yaw + 180.0
        metti_girato(verso)
        girato = True
        ga = math.radians(verso)
        if libero_verso(math.cos(ga), math.sin(ga), 150.0) < 150.0:
            # dalla parte dove si scende c'e' qualcosa: il verso dei dati
            verso = yaw
            metti_girato(verso)
            girato = False
            ga = math.radians(verso)
            if libero_verso(math.cos(ga), math.sin(ga), 150.0) < 150.0:
                avviso("%s: non c'e' posto per scendere ne' da una parte ne' dall'altra (da guardare)" % nome)
        ang = math.radians(yaw)
        tx, ty = -math.sin(ang), math.cos(ang)          # dove sta la testa nei dati (il muro), nel mondo
        in_piu = max(0.0, (lungo_letto - LETTO_CONTATO) / 2.0) + 3.0
        # (secondo controllo) nelle camere 4 e 5 il letto sta di traverso fra due muri: non oltre il posto che c'e'
        posto_ai_piedi = libero_verso(-tx, -ty, lungo_letto / 2.0 + 60.0) - lungo_letto / 2.0
        in_piu = max(0.0, min(in_piu, posto_ai_piedi - 2.0))
        posto_in_testa = libero_verso(tx, ty, lungo_letto / 2.0 + 60.0) - lungo_letto / 2.0
        if posto_in_testa + in_piu < 0.0:
            avviso("%s: il letto (%.0f cm) non ci sta: entra nel muro di %.0f cm (da dire alla chat 3D)"
                   % (nome, lungo_letto, -(posto_in_testa + in_piu)))
        metti_girato(verso, in_piu, -tx, -ty)
        scrivi("  %s: lungo %.0f cm, %s, verso i piedi di %.0f cm (posto: %.0f in testa, %.0f ai piedi)"
               % (nome, lungo_letto, "cuscino al muro" if girato else "verso dei dati (si scende dall'altra parte)",
                  in_piu, posto_in_testa, posto_ai_piedi))
    return a


def fai_locanda():
    global CARTELLA
    f = LOCANDE + "/Valdorso_locande.json"
    if not os.path.exists(f):
        avviso("locanda: manca %s" % f)
        return
    with open(f, "r", encoding="utf-8-sig") as g:
        dati = json.load(g)
    e = dati.get("edifici", {}).get("SM_Locanda_Livia")
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    classe_panca = getattr(unreal, "ValdorsoPanca", None)
    if not e or classe_porta is None or classe_panca is None:
        avviso("locanda: mancano i dati di SM_Locanda_Livia o le classi C++ ValdorsoPorta / ValdorsoPanca")
        return
    letto = unreal.load_asset("/Game/Fab/Letto/BP_Letto")
    classe_letto = letto.generated_class() if letto else None
    mat = materiali()
    materiali_interni()
    nomi = {"SM_Locanda_Livia"}
    for ar in e["arredi"]:
        if ar.get("pezzo"):
            nomi.add(ar["pezzo"])
            if ar["pezzo"] in COPERCHI:
                nomi.add(COPERCHI[ar["pezzo"]][0])
        if ar.get("coperchio"):
            nomi.add(ar["coperchio"])
    # i pezzi che ci sono gia' (porte, bauli e scrittoio della caserma) non si reimportano: si usano quelli. Il modello
    # della locanda si reimporta sempre (la chat 3D lo ha cambiato il 17/11).
    gia = {}
    da_importare = []
    for n in sorted(nomi):
        m = None if n == "SM_Locanda_Livia" else unreal.load_asset("%s/%s" % (PEZZI, n))
        if isinstance(m, unreal.StaticMesh):
            gia[n] = m
        else:
            da_importare.append(n)
    vecchia = CARTELLA
    CARTELLA = LOCANDE
    try:
        pezzi = importa(da_importare, mat)
    finally:
        CARTELLA = vecchia
    pezzi.update(gia)
    if pezzi.get("SM_Locanda_Livia") is None:
        avviso("locanda: il modello SM_Locanda_Livia non c'e': niente")
        return
    sy = numero_specchio() or 1.0
    per_nome = tutti_per_nome()
    b = per_nome.get("Locanda_di_Bruno")
    cb = b.get_component_by_class(unreal.StaticMeshComponent) if b else None
    if cb is None:
        avviso("locanda: nel livello non c'e' 'Locanda_di_Bruno'")
        return

    def mia(a, nome):
        a.set_actor_label(nome)
        a.set_folder_path("Valle/Villaggio/Locanda nuova")
        a.set_editor_property("tags", [unreal.Name(LOCANDA_NUOVA)])
        return a

    # 0. via le cose messe da un lancio di prima
    tolte = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(LOCANDA_NUOVA):
            attori.destroy_actor(a)
            tolte += 1
    if tolte:
        scrivi("tolte %d cose messe prima" % tolte)
    # 1. il modello nuovo sull'attore del villaggio (scala (1, sy, 1), come gli alloggi)
    m = pezzi["SM_Locanda_Livia"]
    b.modify()
    cb.set_editor_property("static_mesh", m)
    for i in range(len(m.get_editor_property("static_materials"))):
        cb.set_material(i, m.get_material(i))
    b.set_actor_scale3d(unreal.Vector(1.0, sy, 1.0))
    o = b.get_actor_location()
    yaw0 = b.get_actor_rotation().yaw
    # (18/11 notte, Fra: "e' ancora sollevata da terra") il pavimento stava a 15-45 cm sopra il prato. Si abbassa fin dove
    # il terreno lo permette: il punto piu' alto del prato sotto i muri, piu' 4 cm (piu' giu' il prato entrerebbe in sala).
    z_prima = o.z
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    fuori0 = re.findall(r"\d+(?:\.\d+)?", str(e.get("misure_cm", {}).get("fuori", "")))
    mx0, my0 = (float(fuori0[0]) / 2.0, float(fuori0[1]) / 2.0) if len(fuori0) >= 2 else (750.0, 500.0)
    quote = []
    for i in range(9):
        for j in range(7):
            w = al_mondo(o, yaw0, (-mx0 + 2.0 * mx0 * i / 8.0, -my0 + 2.0 * my0 * j / 6.0, 0.0))
            q = terreno(mondo, w.x, w.y, o.z)
            if q is not None:
                quote.append(q)
    if len(quote) >= 40:
        nuova_z = max(quote) + 4.0
        scrivi("terreno sotto la locanda: da %.0f a %.0f; pavimento da %.0f a %.0f (%+.0f cm)"
               % (min(quote), max(quote), o.z, nuova_z, nuova_z - o.z))
        if abs(nuova_z - o.z) >= 80.0:
            avviso("locanda: il terreno e' a %+.0f cm dal pavimento, troppo: non la sposto (da guardare)" % (nuova_z - o.z))
        elif abs(nuova_z - o.z) > 1.0:
            if nuova_z - o.z < -20.0:
                avviso("locanda: scende di %.0f cm: guardare che la piazza davanti alla porta non resti piu' alta della soglia"
                       % (o.z - nuova_z))
            b.set_actor_location(unreal.Vector(o.x, o.y, nuova_z), False, False)
            o = b.get_actor_location()
            # (secondo controllo) i gradini davanti alla porta erano misurati sul pavimento di prima: si tolgono, e
            # "Mura: 7" li rifa' dopo il collaudo
            sg = e.get("ingresso", {}).get("centro_soglia_cm", [-150, -500, 0])
            ws = al_mondo(o, yaw0, sg)
            for g in attori.get_all_level_actors():
                if g and g.actor_has_tag(GRADINI):
                    pg = g.get_actor_location()
                    if math.hypot(pg.x - ws.x, pg.y - ws.y) < 300.0:
                        scrivi("tolto %s (era per il pavimento di prima)" % g.get_actor_label())
                        attori.destroy_actor(g)
    else:
        avviso("locanda: non riesco a misurare il terreno sotto (%d punti): resta alla quota di prima" % len(quote))
    scrivi("Locanda_di_Bruno: modello nuovo SM_Locanda_Livia a (%.0f, %.0f, %.0f), yaw %.0f" % (o.x, o.y, o.z, yaw0))
    # 1b. (18/11, Fra: "locanda sollevata") il terreno scende verso est piu' del metro di fondazione del modello: sotto
    # ci va uno zoccolo di pietra largo come l'edificio, che scende altri 4 metri (sotto terra non si vede)
    cubo = unreal.load_asset("/Engine/BasicShapes/Cube")
    # (secondo controllo) largo come i MURI, non come l'ingombro del modello (che comprende le gronde: 790 x 550 contro
    # 750 x 500): "fuori": "1500 x 1000", con l'origine al centro. La z e' il fondo della fondazione.
    ing_p = dati.get("pezzi", {}).get("SM_Locanda_Livia", {}).get("ingombro_cm", {})
    fuori = re.findall(r"\d+(?:\.\d+)?", str(e.get("misure_cm", {}).get("fuori", "")))
    mx, my = (float(fuori[0]) / 2.0, float(fuori[1]) / 2.0) if len(fuori) >= 2 else (750.0, 500.0)
    ix, iy, iz = [-mx, mx], [-my, my], ing_p.get("z", [-100, 1328])
    if ZOCCOLO_SERVE and cubo is not None:
        # (18/11 notte, dalla foto di Fra) la fondazione del modello e' piu' stretta dei muri: sotto il filo dei muri si
        # vedeva il vuoto. Lo zoccolo parte quindi da 1 cm sotto il pavimento (non dal fondo della fondazione) e sporge
        # 3 cm dai muri, come un basamento.
        ix, iy = [ix[0] - 3.0, ix[1] + 3.0], [iy[0] - 3.0, iy[1] + 3.0]
        centro = ((ix[0] + ix[1]) / 2.0, (iy[0] + iy[1]) / 2.0, -1.0 - ZOCCOLO_FONDO / 2.0)
        zc = attori.spawn_actor_from_object(cubo, al_mondo(o, yaw0, centro), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw0))
        if zc:
            zc.set_actor_scale3d(unreal.Vector((ix[1] - ix[0]) / 100.0, (iy[1] - iy[0]) / 100.0, ZOCCOLO_FONDO / 100.0))
            pietra = mat.get("Pietra") or mat.get("Muro")
            if pietra is not None:
                zc.get_component_by_class(unreal.StaticMeshComponent).set_material(0, pietra)
            mia(zc, "Locanda_di_Bruno zoccolo")
            scrivi("zoccolo di pietra sotto la locanda: da 1 a %.0f cm sotto il pavimento, 3 cm fuori dai muri" % (1.0 + ZOCCOLO_FONDO))
    # 2. le cose vecchie dentro si spengono
    # (secondo controllo) i letti e le panche vecchi si tolgono del tutto, come negli alloggi: spenti resterebbero nel
    # livello, e le giornate dei PNG (giornate_vive.py) li sceglierebbero ancora come panca e letto piu' vicini
    spente = 0
    via = 0
    for a in attori.get_all_level_actors():
        if not a or a.actor_has_tag(LOCANDA_NUOVA):
            continue
        n = a.get_actor_label()
        if n in ("Locanda_di_Bruno letto", "Locanda_di_Bruno panca"):
            attori.destroy_actor(a)
            via += 1
        elif n in LOCANDA_VECCHIE and not a.actor_has_tag(MAI):
            spegni(a, True)
            spente += 1
    scrivi("cose vecchie della locanda: %d letti e panche tolti, %d spente (fase 99)" % (via, spente))
    # 3. la porta d'ingresso: la stessa di prima, sulla soglia nuova, larga e alta come il vano
    porta = per_nome.get("Locanda_di_Bruno porta")
    ing = e.get("ingresso", {})
    soglia = ing.get("centro_soglia_cm", [-150, -500, 0])
    if porta is not None:
        porta.modify()
        # nel mezzo del muro (il muro va da y -500 a -440): la soglia dei dati e' sul filo di fuori
        dove = al_mondo(o, yaw0, (soglia[0], soglia[1] + 30.0, soglia[2]))
        porta.set_actor_location(dove, False, False)
        # (secondo controllo) non si scala l'attore (con l'anta che gira si storcerebbe): si allarga l'anta e si sposta
        # la cerniera. Di serie: cerniera a (0, -50, 0), anta cubo 100 x 6 x 210 con il centro a (0, 50, 105).
        largo = float(ing.get("larghezza_cm", 140))
        alto = float(ing.get("altezza_cm", 240))
        porta.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
        cerniera = porta.get_editor_property("cerniera")
        anta = porta.get_editor_property("anta")
        cerniera.modify()
        anta.modify()
        cerniera.set_relative_location(unreal.Vector(0.0, -largo / 2.0, 0.0), False, False)
        anta.set_relative_scale3d(unreal.Vector(0.06, largo / 100.0, alto / 100.0))
        anta.set_relative_location(unreal.Vector(0.0, largo / 2.0, alto / 2.0), False, False)
        scrivi("porta d'ingresso sulla soglia nuova (%.0f, %.0f), %s x %s cm" % (dove.x, dove.y, ing.get("larghezza_cm", 140),
                                                                              ing.get("altezza_cm", 240)))
    else:
        avviso("locanda: non trovo 'Locanda_di_Bruno porta'")
    # 4. gli arredi dai dati
    conti = {}
    letti_messi = []
    bauli_messi = []
    for ar in e["arredi"]:
        ruolo = ar.get("ruolo")
        nome = "Locanda_di_Bruno %s" % ar["nome"]
        dove = al_mondo(o, yaw0, ar["posizione_cm"])
        yaw = yaw0 + float(ar.get("yaw", 0))
        a = None
        coperchio_messo = None
        try:
            if ruolo == "porta":
                if ar["nome"] == "porta_ingresso":
                    continue          # resta la porta di prima (vedi 3)
                mp = pezzi.get(ar.get("pezzo"))
                if mp is None:
                    avviso("%s: manca il modello %s" % (nome, ar.get("pezzo")))
                    continue
                num = re.search(r"da\s*([+-]?\d+(?:\.\d+)?)\s*a\s*([+-]?\d+(?:\.\d+)?)", str(ar.get("apre", "")))
                verso = 1 if not num or float(num.group(2)) > float(num.group(1)) else -1
                # (20/11, v0.2.7.1) le cinque camere hanno la chiave (ValdorsoLocanda.h: Bruno la da' per una notte)
                cam = re.match(r"porta_camera_(\d+)$", str(ar["nome"]))
                chiave_camera = "Chiave_Camera_%d" % int(cam.group(1)) if cam and 1 <= int(cam.group(1)) <= 5 else None
                a = porta_vera(classe_porta, mp, dove, yaw, verso, chiave_camera, nome, sy)
                if chiave_camera and a is not None:
                    conti["porte a chiave"] = conti.get("porte a chiave", 0) + 1
                    # da dentro si esce sempre: il punto della stanza e' il suo letto (dai dati)
                    letto_c = next((x for x in e["arredi"] if x.get("nome") == "letto_camera_%d" % int(cam.group(1))), None)
                    if letto_c is not None:
                        try:
                            a.set_editor_property("punto_dentro", al_mondo(o, yaw0, letto_c["posizione_cm"]))
                        except Exception:  # noqa: BLE001
                            avviso("%s: manca PuntoDentro nel C++ (compila e rilancia): da dentro servirebbe la chiave" % nome)
                    else:
                        avviso("%s: non trovo il suo letto nei dati: da dentro servirebbe la chiave" % nome)
            elif ruolo == "letto":
                if classe_letto is None:
                    avviso("manca /Game/Fab/Letto/BP_Letto: niente letti")
                    continue
                a = metti_letto(classe_letto, mondo, dove, yaw, nome)
            elif ar.get("pezzo"):
                mp = pezzi.get(ar["pezzo"])
                if mp is None:
                    avviso("%s: manca il modello %s" % (nome, ar["pezzo"]))
                    continue
                a = metti_mesh(mp, dove, yaw, nome, sy)
                cop = COPERCHI.get(ar["pezzo"])
                if cop is None and ar.get("coperchio"):
                    # (secondo controllo) dove sta il coperchio sulla cassa lo dicono i dati del pezzo
                    su = dati.get("pezzi", {}).get(ar["coperchio"], {}).get("posizione_sulla_cassa_cm", [0.0, 0.0, 0.0])
                    cop = (ar["coperchio"], (float(su[0]), float(su[1]), float(su[2])))
                if a and cop and pezzi.get(cop[0]):
                    c2 = metti_mesh(pezzi[cop[0]], al_mondo(dove, yaw, cop[1]), yaw, nome + " coperchio", sy)
                    if c2:
                        mia(c2, nome + " coperchio")
                        coperchio_messo = c2
            else:
                conti["solo il posto (%s)" % ruolo] = conti.get("solo il posto (%s)" % ruolo, 0) + 1
                continue
        except Exception as ex:  # noqa: BLE001
            avviso("%s (%s): %s" % (nome, ruolo, ex))
            if a is not None:
                try:
                    attori.destroy_actor(a)
                except Exception:  # noqa: BLE001
                    pass
            continue
        if a is None:
            avviso("non riesco a mettere %s" % nome)
            continue
        mia(a, nome)
        conti[ruolo] = conti.get(ruolo, 0) + 1
        if ruolo == "letto":
            letti_messi.append(a)
        elif ruolo == "baule":
            bauli_messi.append((a, coperchio_messo))
    # 4b. (18/11 notte, Fra: "letti e bauli troppo attaccati") il nostro letto vero (BP_Letto) e' piu' grande di quello dei
    # dati: dove un baule entra nel letto si sposta quanto basta (piu' 6 cm), dal lato dove deve muoversi di meno
    scostati = 0
    for cassa, cop in bauli_messi:
        try:
            co, ce = ingombro_mesh(cassa)
            for lt in letti_messi:
                lo, le = ingombro_mesh(lt)
                if abs(co.z - lo.z) > 150.0:
                    continue
                px = (ce.x + le.x) - abs(co.x - lo.x)
                py = (ce.y + le.y) - abs(co.y - lo.y)
                if px <= -6.0 or py <= -6.0:
                    continue          # gia' staccati di almeno 6 cm
                if min(px, py) > 80.0:
                    avviso("%s e' tutto dentro un letto (%.0f cm): lo lascio, da guardare" % (cassa.get_actor_label(), min(px, py)))
                    continue
                if px < py:
                    sposta = unreal.Vector((px + 6.0) * (1.0 if co.x >= lo.x else -1.0), 0.0, 0.0)
                else:
                    sposta = unreal.Vector(0.0, (py + 6.0) * (1.0 if co.y >= lo.y else -1.0), 0.0)
                for pezzo in (cassa, cop):
                    if pezzo is not None:
                        q = pezzo.get_actor_location()
                        pezzo.set_actor_location(unreal.Vector(q.x + sposta.x, q.y + sposta.y, q.z), False, False)
                scostati += 1
                scrivi("  %s staccato dal letto di (%+.0f, %+.0f) cm" % (cassa.get_actor_label(), sposta.x, sposta.y))
                break
        except Exception as ex:  # noqa: BLE001
            avviso("baule e letto: %s" % ex)
    if scostati:
        scrivi("bauli staccati dai letti: %d" % scostati)
    # 5. la sala: i tavoli e le panche.
    # (18/11, Fra: "tavoli piu' piccoli, sembra troppo ingombrante") il tavolo si stringe a 140 x 70 in pianta (alto come
    # prima), la panca si accorcia a 140 e si avvicina (a 65 dal centro del tavolo invece di 85), e la fila davanti
    # all'ingresso sta 10 cm piu' verso il fondo (nei dati): davanti alla porta restano piu' di 150 cm liberi.
    tavolo = unreal.load_asset(LOCANDA_TAVOLO)
    sala = e.get("sala_arredi", {})
    misure_tavolo = sala.get("tavolo_cm") or [TAVOLO_LARGO, TAVOLO_LUNGO]
    tavolo_largo, tavolo_lungo = float(min(misure_tavolo)), float(max(misure_tavolo))
    panca_dal_tavolo = float(sala.get("panche_a_cm", PANCA_DAL_TAVOLO))
    for t in sala.get("tavoli", []):
        yaw = yaw0 + float(t.get("yaw", 0))
        # (18/11 notte) la chat 3D ha rifatto i dati con queste misure (fila davanti all'ingresso gia' a y -200): i centri
        # si prendono dai dati cosi' come sono
        ct = list(t["centro_cm"])
        dove = al_mondo(o, yaw0, ct)
        if isinstance(tavolo, unreal.StaticMesh):
            bb = tavolo.get_bounding_box()
            lx, ly = max(bb.max.x - bb.min.x, 1.0), max(bb.max.y - bb.min.y, 1.0)
            # il lato lungo del tavolo corre lungo le panche (Y): se il modello e' lungo in X si gira di 90
            gira_tavolo = 90.0 if lx > ly else 0.0
            lungo, corto = max(lx, ly), min(lx, ly)
            s_lungo, s_corto = min(1.0, tavolo_lungo / lungo), min(1.0, tavolo_largo / corto)
            scala = unreal.Vector(s_lungo, s_corto, 1.0) if lx > ly else unreal.Vector(s_corto, s_lungo, 1.0)
            cx, cy = (bb.min.x + bb.max.x) / 2.0 * scala.x, (bb.min.y + bb.max.y) / 2.0 * scala.y
            a = attori.spawn_actor_from_object(tavolo, al_mondo(dove, yaw + gira_tavolo, (-cx, -cy, -bb.min.z)),
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + gira_tavolo))
            if a:
                a.set_actor_scale3d(scala)
                mia(a, "Locanda_di_Bruno %s" % t["nome"])
                conti["tavolo"] = conti.get("tavolo", 0) + 1
                if conti["tavolo"] == 1:
                    scrivi("tavolo: il modello e' %.0f x %.0f cm, messo a %.0f x %.0f" % (lx, ly, lx * scala.x, ly * scala.y))
        for pa in t.get("panche", []):
            # (secondo controllo) chi siede guarda il tavolo: il davanti della panca e' la sua +X, quindi quella a est si gira
            dxp, dyp = pa["centro_cm"][0] - t["centro_cm"][0], pa["centro_cm"][1] - t["centro_cm"][1]
            if abs(dxp) < abs(dyp):
                # (secondo controllo) una panca in testa al tavolo (oggi non ce ne sono): resta dove dicono i dati
                avviso("locanda: la panca %s di %s non e' sul lato lungo: la lascio dove dicono i dati" % (pa.get("lato", ""), t["nome"]))
                a = attori.spawn_actor_from_class(classe_panca, al_mondo(o, yaw0, pa["centro_cm"]),
                                                  unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + float(pa.get("yaw", 0))))
                if a:
                    mia(a, "Locanda_di_Bruno %s panca %s" % (t["nome"], pa.get("lato", "")))
                    conti["panca"] = conti.get("panca", 0) + 1
                continue
            est = dxp > 0
            gira = 180.0 if est else 0.0
            cp = (ct[0] + (panca_dal_tavolo if est else -panca_dal_tavolo), ct[1], pa["centro_cm"][2])
            a = attori.spawn_actor_from_class(classe_panca, al_mondo(o, yaw0, cp),
                                              unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + gira))
            if a:
                a.set_actor_scale3d(unreal.Vector(1.0, tavolo_lungo / 180.0, 1.0))
                if "Trave" in mat:
                    for comp in a.get_components_by_class(unreal.StaticMeshComponent):
                        comp.set_material(0, mat["Trave"])
                mia(a, "Locanda_di_Bruno %s panca %s" % (t["nome"], pa.get("lato", "")))
                conti["panca"] = conti.get("panca", 0) + 1
    if not isinstance(tavolo, unreal.StaticMesh):
        avviso("locanda: manca %s: la sala ha le panche ma non i tavoli" % LOCANDA_TAVOLO)
    # 6. il baule del colono al posto nuovo (con i suoi pezzi, che lo seguono)
    baule = per_nome.get("Il baule del colono")
    bc = sala.get("baule_colono", {}).get("cm")
    if baule is not None and bc:
        prima = baule.get_actor_location()
        nuovo_posto = al_mondo(o, yaw0, (bc[0], bc[1] - 5.0, 0.0))
        # (secondo controllo) in z si muove solo di quanto e' scesa la locanda: il baule sta a 40 cm dal pavimento
        # (crea_volti.py), non a 0
        dx, dy, dz = nuovo_posto.x - prima.x, nuovo_posto.y - prima.y, o.z - z_prima
        if abs(dx) > 1.0 or abs(dy) > 1.0 or abs(dz) > 1.0:
            mossi = 0
            for a in attori.get_all_level_actors():
                if a and a.get_actor_label().startswith("Il baule del colono"):
                    q = a.get_actor_location()
                    a.modify()
                    a.set_actor_location(unreal.Vector(q.x + dx, q.y + dy, q.z + dz), False, False)
                    mossi += 1
            scrivi("baule del colono spostato di (%+.0f, %+.0f) cm, con %d pezzi" % (dx, dy, mossi - 1))
    scrivi("messi: " + ", ".join("%s %d" % (k, conti[k]) for k in sorted(conti)))
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    scrivi("FATTO e salvato. ORA, in questo ordine: 'PNG: metti Gaspare... Bruno...', 'Volti: ...', 'Gatto della locanda', "
           "'Rete', 'PNG: panche, letti e porte', 'Per Claude: collauda i passaggi', e alla fine 'Mura: 7' (i gradini).")


# ---------------------------------------------------------------------------------------------------------------------
# (18/11 sera, v0.2.7) IL POSTO DI GUARDIA DEL PASSO PER LA GUARDIA REALE (deciso da Fra il 18/11; pezzi della chat 3D,
# Valdorso-passo-guardia-reale-18-11.zip in V:/Texture/valdorso/passo). Voce "Mura: 11".
#   - il portale di pietra (SM_Passo_Portale) prende il posto del portale di legno SULLO STESSO ATTORE "Porta del passo"
#     (cosi' gli script delle persone, che lo cercano per nome, continuano a valere);
#   - la GRATA al posto della sbarra (Fra, 18/11; come nelle Cronache): AValdorsoGrata, alzata dall'alba al tramonto;
#     la leva ha un lucchetto con una chiave che i coloni non hanno (Chiave_Passo): la muovono solo le guardie del Re;
#   - muri merlati, torretta di vedetta, quartiere (corpo di guardia e alloggio della capitana) con porte, letti, tavolo,
#     baule, scrittoio e sedia; braciere, panche, rastrelliera, lanterne;
#   - le cose di prima (palizzata, torre, casermetta, fuoco, lanterne, brande) si SPENGONO (fase 99), non si cancellano;
#   - la stalla del mulo e le botti si spostano a ovest della strada.
# Si puo' rilanciare: toglie e rimette le sue cose (etichetta PASSO_REALE).
PASSO = "V:/Texture/valdorso/passo"
PASSO_DATI = "Valdorso_passo_guardia_reale.json"
PASSO_REALE = "ValdorsoPassoReale"
PASSO_CARTELLA = "Valle/Passo/Guardia Reale"
PASSO_PORTA = "Porta del passo"
PASSO_VECCHIE = ("Palizzata del passo 1", "Palizzata del passo 2", "Torre_del_passo", "Torre_del_passo porta",
                 "Torre_del_passo scala", "Scala ~ Torre_del_passo porta", "Casermetta_della_Guardia",
                 "Casermetta_della_Guardia porta", "Scala ~ Casermetta_della_Guardia porta", "Campo della Guardia",
                 "Luce del fuoco della Guardia", "Lanterne del passo")
PASSO_VECCHIE_INIZI = ("Luce lanterna del passo", "Gradino ~ Torre_del_passo", "Gradino ~ Casermetta_della_Guardia")
PASSO_VIA = ("Branda della Guardia", "Panca della Guardia")     # queste si tolgono (vedi fai_passo, passo 2)
PASSO_ORA_MATTINA = 6.0        # la grata si alza
PASSO_ORA_SERA = 20.0          # e si abbassa ("aperto dall'alba al tramonto", la bacheca)
PASSO_STALLA = (1700.0, -750.0)                 # lungo, traverso (cm): dove va la stalla del mulo
PASSO_STALLA_GIRA = 180.0                       # la stalla si gira: il lato aperto verso la strada, non verso la roccia
# dietro la stalla (a ovest), non davanti al suo lato aperto; sono cinque (villaggio.py, guardia())
PASSO_BOTTI = ((1560.0, -1000.0), (1660.0, -1030.0), (1760.0, -1000.0), (1860.0, -1030.0), (1960.0, -1000.0))
PASSO_D = (-0.80105, -0.59860)                  # lungo il passo, verso la valle (villaggio.py, guardia())
PASSO_N = (0.59860, -0.80105)                   # di traverso


def luce_calda(dove, nome, intensita, raggio):
    l = attori.spawn_actor_from_class(unreal.PointLight, dove, unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
    if l is None:
        return None
    lc = l.get_component_by_class(unreal.PointLightComponent)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    try:
        lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    except Exception:  # noqa: BLE001
        pass
    lc.set_intensity(intensita)
    lc.set_attenuation_radius(raggio)
    lc.set_light_color(unreal.LinearColor(LUCE_COLORE[0], LUCE_COLORE[1], LUCE_COLORE[2], 1.0))
    lc.set_cast_shadows(False)
    l.set_actor_label(nome)
    return l


def fai_passo():
    global CARTELLA
    f = PASSO + "/" + PASSO_DATI
    if not os.path.exists(f):
        avviso("passo: manca %s" % f)
        return
    with open(f, "r", encoding="utf-8-sig") as g:
        dati = json.load(g)
    pz = dati["pezzi"]
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    classe_grata = getattr(unreal, "ValdorsoGrata", None)
    classe_leva = getattr(unreal, "ValdorsoLeva", None)
    classe_panca = getattr(unreal, "ValdorsoPanca", None)
    if classe_porta is None or classe_grata is None or classe_leva is None:
        avviso("passo: mancano le classi del C++ (ValdorsoPorta, ValdorsoGrata, ValdorsoLeva): compila")
        return
    letto = unreal.load_asset("/Game/Fab/Letto/BP_Letto")
    classe_letto = letto.generated_class() if letto else None
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    per_nome = tutti_per_nome()
    porta = per_nome.get(PASSO_PORTA)
    cporta = porta.get_component_by_class(unreal.StaticMeshComponent) if porta else None
    if cporta is None:
        avviso("passo: nel livello non c'e' '%s' (sei in L_Valle?)" % PASSO_PORTA)
        return
    mat = materiali()
    materiali_interni()
    # i pezzi del passo si reimportano sempre; quelli del kit che ci sono gia' (porte, baule, scrittoio, sedia, lanterna,
    # leva) si usano cosi' come sono
    del_passo = sorted(n for n in pz if n.startswith("SM_Passo_"))
    vecchia = CARTELLA
    CARTELLA = PASSO
    try:
        pezzi = importa(del_passo, mat)
    finally:
        CARTELLA = vecchia
    for n in ("SM_Porta_Esterna", "SM_Porta_Camera", "SM_Baule_Grande", "SM_Baule_Grande_Coperchio", "SM_Scrittoio",
              "SM_Sedia", "SM_Lanterna_Muro", "SM_Leva"):
        m = unreal.load_asset("%s/%s" % (PEZZI, n))
        if isinstance(m, unreal.StaticMesh):
            pezzi[n] = m
        else:
            avviso("passo: manca il pezzo %s (lo mette 'Mura: 10' o 'Mura: 5'): quella cosa si salta" % n)
    if any(pezzi.get(n) is None for n in ("SM_Passo_Portale", "SM_Passo_Grata")):
        avviso("passo: mancano il portale o la grata: niente")
        return
    sy = numero_specchio() or 1.0
    conti = {}

    def mio(a, nome, fase=None):
        a.set_actor_label(nome)
        a.set_folder_path(PASSO_CARTELLA)
        a.set_editor_property("tags", [unreal.Name(PASSO_REALE)] + ([unreal.Name(fase)] if fase else []))
        return a

    def v(p):
        return unreal.Vector(float(p[0]), float(p[1]), float(p[2]))

    def pezzo_intero(nome_pezzo, etichetta):
        d = pz.get(nome_pezzo, {})
        m = pezzi.get(nome_pezzo)
        if m is None or "origine_mondo_cm" not in d:
            avviso("passo: %s non messo (manca il modello o l'origine)" % nome_pezzo)
            return None
        a = metti_mesh(m, v(d["origine_mondo_cm"]), float(d["yaw"]), etichetta, sy)
        if a:
            mio(a, etichetta)
            conti["pezzi grandi"] = conti.get("pezzi grandi", 0) + 1
        return a

    # 0. via le cose messe da un lancio di prima
    tolte = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(PASSO_REALE):
            attori.destroy_actor(a)
            tolte += 1
    if tolte:
        scrivi("tolte %d cose messe prima" % tolte)

    # 1. il portale sull'attore di sempre
    dp = pz["SM_Passo_Portale"]
    o_dati = v(dp["origine_mondo_cm"])
    o_ora = porta.get_actor_location()
    scarto = math.hypot(o_ora.x - o_dati.x, o_ora.y - o_dati.y)
    if scarto > 50.0 or abs(o_ora.z - o_dati.z) > 50.0:
        avviso("passo: '%s' e' a %.0f cm (e %.0f in altezza) dall'origine dei dati: i pezzi nuovi non combacerebbero. Niente."
               % (PASSO_PORTA, scarto, o_ora.z - o_dati.z))
        return
    porta.modify()
    cporta.modify()
    cporta.set_editor_property("static_mesh", pezzi["SM_Passo_Portale"])
    for i in range(cporta.get_num_materials()):
        cporta.set_material(i, pezzi["SM_Passo_Portale"].get_material(i))
    porta.set_actor_location(o_dati, False, False)
    porta.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(dp["yaw"])), False)
    porta.set_actor_scale3d(unreal.Vector(1.0, sy, 1.0))
    scrivi("'%s': ora e' SM_Passo_Portale, a (%.0f, %.0f, %.0f), yaw %.2f (era a %.1f cm da li')"
           % (PASSO_PORTA, o_dati.x, o_dati.y, o_dati.z, float(dp["yaw"]), scarto))

    # 2. le cose di prima si spengono (fase 99: nel gioco spariscono, con la collisione)
    spente = []
    tolte_vecchie = []
    for a in attori.get_all_level_actors():
        if not a or a.actor_has_tag(PASSO_REALE):
            continue
        n = a.get_actor_label()
        if n.startswith(PASSO_VIA):
            # (secondo controllo) le brande e la panca di prima si TOLGONO: spente resterebbero scelte dagli abitanti
            # per dormire e sedersi (giornate_vive.py), come i letti vecchi della locanda
            attori.destroy_actor(a)
            tolte_vecchie.append(n)
            continue
        if n in PASSO_VECCHIE or n.startswith(PASSO_VECCHIE_INIZI):
            spegni(a, True)
            try:
                a.set_editor_property("enable_auto_lod_generation", False)
            except Exception:  # noqa: BLE001
                pass
            # (18/11 sera, foto di Fra) la scala a pioli della torre di prima restava in vista nell'editor: non e' fatta
            # di modelli normali. Si nascondono tutti i pezzi che si vedono, non solo i modelli
            for pc in a.get_components_by_class(unreal.PrimitiveComponent):
                try:
                    pc.modify()
                    pc.set_visibility(False, True)
                    pc.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
                except Exception:  # noqa: BLE001
                    pass
            for lc in a.get_components_by_class(unreal.LightComponent):
                lc.modify()
                lc.set_visibility(False, True)
            spente.append(n)
    scrivi("cose di prima spente (%d): %s" % (len(spente), ", ".join(sorted(spente)) or "nessuna"))
    if tolte_vecchie:
        scrivi("brande e panca di prima tolte: %s" % ", ".join(sorted(tolte_vecchie)))
    mancano = [n for n in PASSO_VECCHIE if n not in spente]
    if mancano:
        scrivi("  (non trovate, forse gia' tolte: %s)" % ", ".join(mancano))

    # 3. muri, torretta, quartiere
    pezzo_intero("SM_Passo_Muro_Ovest", "Passo ~ muro ovest")
    pezzo_intero("SM_Passo_Muro_Est", "Passo ~ muro est")
    pezzo_intero("SM_Passo_Torretta", "Passo ~ torretta di vedetta")
    pezzo_intero("SM_Passo_Quartiere", "Passo ~ quartiere della Guardia Reale")

    # 4. la grata (al posto della sbarra) e la sua leva
    gr = dp.get("grata", {})
    grata = None
    try:
        ch = gr["chiusa"]
        # come nelle mura: la grata del C++ sta nel piano Y-Z, quindi l'attore e' girato di 90 gradi e il modello di -90
        grata = attori.spawn_actor_from_class(classe_grata, v(ch["mondo_cm"]),
                                              unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(ch["yaw_mondo"]) + 90.0))
        if grata:
            grata.set_editor_property("modello", pezzi["SM_Passo_Grata"])
            grata.set_editor_property("larghezza", 520.0)
            grata.set_editor_property("altezza", 595.0)
            grata.set_editor_property("corsa", float(gr.get("corsa_cm", 595)))
            grata.set_editor_property("giorno_e_notte", True)
            grata.set_editor_property("ora_mattina", PASSO_ORA_MATTINA)
            grata.set_editor_property("ora_sera", PASSO_ORA_SERA)
            mesh_su(grata.get_editor_property("ferro"), pezzi["SM_Passo_Grata"], sy, -90.0)
            mio(grata, "Passo ~ grata del portale")
            conti["grata"] = 1
    except Exception as ex:  # noqa: BLE001
        avviso("passo: la grata: %s" % ex)
    lv = gr.get("leva", {}).get("perno")
    if grata and lv and pezzi.get("SM_Leva"):
        try:
            leva = attori.spawn_actor_from_class(classe_leva, v(lv["mondo_cm"]),
                                                 unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(lv["yaw_mondo"]) + 90.0))
            if leva:
                leva.set_editor_property("grata", grata)
                # il lucchetto: una chiave che nessun colono ha (la grata la muovono le guardie del Re, con l'ora)
                leva.set_editor_property("chiave", unreal.Name("Chiave_Passo"))
                leva.get_editor_property("perno").set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
                mesh_su(leva.get_editor_property("asta"), pezzi["SM_Leva"], sy, -90.0)
                base = leva.get_editor_property("base")
                base.set_relative_location(unreal.Vector(0.0, 0.0, -25.0), False, False)
                base.set_relative_scale3d(unreal.Vector(0.4, 0.3, 0.5))
                base.set_visibility(False, True)
                mio(leva, "Passo ~ leva della grata")
                conti["leva"] = 1
        except Exception as ex:  # noqa: BLE001
            avviso("passo: la leva: %s" % ex)

    # 5. le lanterne di ferro con la loro luce, e le luci dentro la torretta
    lanterna = pezzi.get("SM_Lanterna_Muro")
    for j, lt in enumerate(dati.get("lanterne_SM_Lanterna_Muro", [])):
        if lanterna is None:
            break
        a = metti_mesh(lanterna, v(lt["mondo_cm"]), float(lt["yaw_mondo"]), "Passo ~ lanterna %d" % (j + 1), sy)
        if a is None:
            continue
        mio(a, "Passo ~ lanterna %d" % (j + 1))
        try:
            a.get_component_by_class(unreal.StaticMeshComponent).set_collision_profile_name("NoCollision")
        except Exception:  # noqa: BLE001
            pass
        conti["lanterne"] = conti.get("lanterne", 0) + 1
        if LANTERNE_CON_LUCE and lt.get("luce"):
            l = luce_calda(v(lt["luce"]["mondo_cm"]), "Passo ~ luce lanterna %d" % (j + 1), LUCE_INTENSITA, LUCE_RAGGIO_FUORI)
            if l:
                mio(l, "Passo ~ luce lanterna %d" % (j + 1))
    for j, ld in enumerate(pz.get("SM_Passo_Torretta", {}).get("luci_dentro", [])):
        l = luce_calda(v(ld["mondo_cm"]), "Passo ~ luce della torretta %d" % (j + 1), LUCE_INTENSITA, LUCE_RAGGIO_DENTRO)
        if l:
            mio(l, "Passo ~ luce della torretta %d" % (j + 1))

    # 6. il cortile: braciere (con il suo fuoco), panche, rastrelliera
    for nome_pezzo, etichetta in (("SM_Passo_Braciere", "Passo ~ braciere"), ("SM_Passo_Panca", "Passo ~ panca"),
                                  ("SM_Passo_Rastrelliera", "Passo ~ rastrelliera"),
                                  # (19/11) il banco da campo di Livia: registro dei turni e cassetta del soldo
                                  ("SM_Passo_Banco_Livia", "Passo ~ banco di Livia")):
        m = pezzi.get(nome_pezzo)
        for j, ps in enumerate(pz.get(nome_pezzo, {}).get("posa", [])):
            if m is None:
                break
            nome = etichetta if len(pz[nome_pezzo]["posa"]) == 1 else "%s %d" % (etichetta, j + 1)
            a = metti_mesh(m, v(ps["origine_mondo_cm"]), float(ps["yaw"]), nome, sy)
            if a:
                mio(a, nome)
                conti["arredi del cortile"] = conti.get("arredi del cortile", 0) + 1
            # (18/11 sera) i posti a sedere delle panche di pietra (dati della chat 3D): sopra il modello va una panca del
            # C++ (AValdorsoPanca) che non si vede, con i posti dei dati. La panca del C++ e' lunga lungo la sua Y e ci si
            # siede guardando la sua +X; quella di pietra e' lunga lungo la sua X.
            posti = ps.get("posti") or []
            if a and posti and classe_panca is not None:
                try:
                    guarda = float(posti[0]["yaw_mondo"])
                    pa = attori.spawn_actor_from_class(classe_panca, v(ps["origine_mondo_cm"]),
                                                       unreal.Rotator(roll=0.0, pitch=0.0, yaw=guarda))
                    if pa:
                        # dove sta ogni posto lungo la Y della panca del C++ (dal suo centro)
                        rg = math.radians(guarda)
                        yx, yy = -math.sin(rg), math.cos(rg)
                        o_p = ps["origine_mondo_cm"]
                        lungo_y = sorted((float(q["mondo_cm"][0]) - o_p[0]) * yx + (float(q["mondo_cm"][1]) - o_p[1]) * yy
                                         for q in posti)
                        pa.set_editor_property("posti", lungo_y)
                        for comp in pa.get_components_by_class(unreal.StaticMeshComponent):
                            comp.set_visibility(False, True)
                        mio(pa, nome + " (posti a sedere)")
                        conti["panche da sedersi"] = conti.get("panche da sedersi", 0) + 1
                except Exception as ex:  # noqa: BLE001
                    avviso("passo: i posti di %s: %s" % (nome, ex))
    fuoco = pz.get("SM_Passo_Braciere", {}).get("luce")
    if fuoco:
        l = luce_calda(v(fuoco["mondo_cm"]), "Passo ~ fuoco del braciere", LUCE_INTENSITA * 4.0, 1100.0)
        if l:
            mio(l, "Passo ~ fuoco del braciere")

    # 7. le porte della torretta e del quartiere, e gli arredi del quartiere
    def porta_dai_dati(d, nome, chiave):
        m = pezzi.get(str(d.get("pezzo")))
        if m is None:
            avviso("passo: %s: manca il modello %s" % (nome, d.get("pezzo")))
            return
        # "yaw da 0 a +90": se il secondo numero e' piu' grande del primo gira nel verso +1
        num = [float(x) for x in re.findall(r"-?\d+(?:\.\d+)?", str(d.get("apre", "")).split("yaw")[-1])]
        verso = 1 if len(num) < 2 or num[1] > num[0] else -1
        p = porta_vera(classe_porta, m, v(d["mondo_cm"]), float(d["yaw_mondo"]), verso, chiave, nome, sy)
        if p:
            mio(p, nome)
            conti["porte"] = conti.get("porte", 0) + 1

    tp = pz.get("SM_Passo_Torretta", {}).get("porta")
    if tp:
        porta_dai_dati(tp, "Passo ~ torretta porta", "Chiave_Passo" if tp.get("a_chiave") else None)
    ar = pz.get("SM_Passo_Quartiere", {}).get("arredi", {})
    letti_messi, bauli_messi = [], []
    for chiave_dati, d in ar.items():
        if not isinstance(d, dict):
            continue
        nome = "Passo ~ quartiere %s" % chiave_dati.replace("_", " ")
        pezzo = str(d.get("pezzo", ""))
        a = None
        try:
            if chiave_dati.startswith("porta_"):
                # (secondo controllo) a chiave solo dove lo dicono i dati: l'alloggio della capitana (una chiave che i
                # coloni non hanno). Nel corpo di guardia e sulla torretta si entra, come nella torre di prima.
                porta_dai_dati(d, nome, "Chiave_Passo" if d.get("a_chiave") else None)
                continue
            dove, yaw = v(d["mondo_cm"]), float(d["yaw_mondo"])
            if pezzo.startswith("letto"):
                if classe_letto is None:
                    avviso("passo: manca /Game/Fab/Letto/BP_Letto: niente %s" % nome)
                    continue
                a = metti_letto(classe_letto, mondo, dove, yaw, nome)
                if a:
                    letti_messi.append((a, yaw))
            elif pezzo.startswith("tavolo"):
                tavolo = unreal.load_asset(LOCANDA_TAVOLO)
                if not isinstance(tavolo, unreal.StaticMesh):
                    avviso("passo: manca %s: niente tavolo" % LOCANDA_TAVOLO)
                    continue
                bb = tavolo.get_bounding_box()
                lx, ly = max(bb.max.x - bb.min.x, 1.0), max(bb.max.y - bb.min.y, 1.0)
                # nei dati il tavolo e' lungo lungo la sua Y (come in sala), girato di 'yaw'
                gira_tavolo = 90.0 if lx > ly else 0.0
                s_lungo, s_corto = min(1.0, TAVOLO_LUNGO / max(lx, ly)), min(1.0, TAVOLO_LARGO / min(lx, ly))
                scala = unreal.Vector(s_lungo, s_corto, 1.0) if lx > ly else unreal.Vector(s_corto, s_lungo, 1.0)
                cx, cy = (bb.min.x + bb.max.x) / 2.0 * scala.x, (bb.min.y + bb.max.y) / 2.0 * scala.y
                a = attori.spawn_actor_from_object(tavolo, al_mondo(dove, yaw + gira_tavolo, (-cx, -cy, -bb.min.z)),
                                                   unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + gira_tavolo))
                if a:
                    a.set_actor_scale3d(scala)
                # le due panche ai posti dei dati, lunghe come il tavolo, con il davanti (+X) verso il tavolo
                for j, po in enumerate(d.get("posti", [])):
                    if classe_panca is None:
                        break
                    pp = v(po["mondo_cm"])
                    verso_tavolo = math.degrees(math.atan2(dove.y - pp.y, dove.x - pp.x))
                    pa = attori.spawn_actor_from_class(classe_panca, pp, unreal.Rotator(roll=0.0, pitch=0.0, yaw=verso_tavolo))
                    if pa:
                        pa.set_actor_scale3d(unreal.Vector(1.0, TAVOLO_LUNGO / 180.0, 1.0))
                        if "Trave" in mat:
                            for comp in pa.get_components_by_class(unreal.StaticMeshComponent):
                                comp.set_material(0, mat["Trave"])
                        mio(pa, "%s panca %d" % (nome, j + 1))
                        conti["panche"] = conti.get("panche", 0) + 1
            elif pezzi.get(pezzo) is not None:
                a = metti_mesh(pezzi[pezzo], dove, yaw, nome, sy)
                cop = COPERCHI.get(pezzo)
                if a and cop and pezzi.get(cop[0]):
                    c2 = metti_mesh(pezzi[cop[0]], al_mondo(dove, yaw, cop[1]), yaw, nome + " coperchio", sy)
                    if c2:
                        mio(c2, nome + " coperchio")
                    bauli_messi.append((a, c2, pezzi[pezzo], yaw))
            else:
                avviso("passo: %s: non so mettere '%s'" % (nome, pezzo))
                continue
        except Exception as ex:  # noqa: BLE001
            avviso("passo: %s: %s" % (nome, ex))
            if a is not None:
                try:
                    attori.destroy_actor(a)
                except Exception:  # noqa: BLE001
                    pass
            continue
        if a is None:
            avviso("passo: non riesco a mettere %s" % nome)
            continue
        mio(a, nome)
        conti["arredi del quartiere"] = conti.get("arredi del quartiere", 0) + 1

    # 7b. (secondo controllo) il nostro letto e' piu' grande di quello dei dati: un baule che ci entra si stacca di 6 cm.
    # Si conta negli assi del letto (il quartiere e' girato di 37 gradi: le scatole del mondo qui non valgono).
    for cassa, cop, mesh_cassa, yaw_cassa in bauli_messi:
        try:
            bbc = mesh_cassa.get_bounding_box()
            hx, hy = (bbc.max.x - bbc.min.x) / 2.0, (bbc.max.y - bbc.min.y) / 2.0
            cc = ingombro_mesh(cassa)[0]
            for lt, yaw_letto in letti_messi:
                cb = ingombro_mesh(lt)[0]
                rb = math.radians(yaw_letto)
                ux, uy = -math.sin(rb), math.cos(rb)          # lungo il letto
                wx, wy = math.cos(rb), math.sin(rb)           # di traverso
                th = math.radians(yaw_cassa - yaw_letto)
                eu = hx * abs(math.sin(th)) + hy * abs(math.cos(th))
                ew = hx * abs(math.cos(th)) + hy * abs(math.sin(th))
                du = (cc.x - cb.x) * ux + (cc.y - cb.y) * uy
                dw = (cc.x - cb.x) * wx + (cc.y - cb.y) * wy
                pu = MISURE_LETTO.get("lungo", 220.0) / 2.0 + eu - abs(du)
                pw = MISURE_LETTO.get("largo", 110.0) / 2.0 + ew - abs(dw)
                if pu <= -6.0 or pw <= -6.0 or min(pu, pw) > 80.0:
                    continue
                if pu < pw:
                    k = (pu + 6.0) * (1.0 if du >= 0 else -1.0)
                    mx, my = ux * k, uy * k
                else:
                    k = (pw + 6.0) * (1.0 if dw >= 0 else -1.0)
                    mx, my = wx * k, wy * k
                for pezzo_mosso in (cassa, cop):
                    if pezzo_mosso is not None:
                        q = pezzo_mosso.get_actor_location()
                        pezzo_mosso.set_actor_location(unreal.Vector(q.x + mx, q.y + my, q.z), False, False)
                scrivi("  %s staccato dal letto di %.0f cm" % (cassa.get_actor_label(), math.hypot(mx, my)))
                break
        except Exception as ex:  # noqa: BLE001
            avviso("passo: baule e letto: %s" % ex)

    # 8. la stalla del mulo e le botti: a ovest della strada (dove stavano tocca la terrazza del quartiere)
    def al_passo(lungo, traverso):
        return (o_dati.x + PASSO_D[0] * lungo + PASSO_N[0] * traverso, o_dati.y + PASSO_D[1] * lungo + PASSO_N[1] * traverso)

    def sposta(a, lungo, traverso):
        q = a.get_actor_location()
        t0 = terreno(mondo, q.x, q.y, q.z)
        x, y = al_passo(lungo, traverso)
        t1 = terreno(mondo, x, y, o_dati.z)
        if t1 is None:
            avviso("passo: %s: non trovo il terreno nel posto nuovo: resta dov'e'" % a.get_actor_label())
            return False
        a.modify()
        a.set_actor_location(unreal.Vector(x, y, t1 + ((q.z - t0) if t0 is not None else 0.0)), False, False)
        return True

    per_nome = tutti_per_nome()
    stalla = per_nome.get("Stalla del mulo")
    if stalla is not None:
        qs = stalla.get_actor_location()
        if sposta(stalla, PASSO_STALLA[0], PASSO_STALLA[1]):
            if not stalla.actor_has_tag("ValdorsoStallaGirata"):
                # (secondo controllo) una volta sola: rilanciando non rigira
                rs = stalla.get_actor_rotation()
                stalla.set_actor_rotation(unreal.Rotator(roll=rs.roll, pitch=rs.pitch, yaw=rs.yaw + PASSO_STALLA_GIRA), False)
                stalla.set_editor_property("tags", list(stalla.get_editor_property("tags")) + [unreal.Name("ValdorsoStallaGirata")])
            ns = stalla.get_actor_location()
            dx, dy, dz = ns.x - qs.x, ns.y - qs.y, ns.z - qs.z
            # con la stalla vanno le sue cose (il mulo, la mangiatoia: tutto cio' che comincia con il suo nome)
            con = 0
            for a in attori.get_all_level_actors():
                if a and a != stalla and a.get_actor_label().startswith(("Stalla del mulo", "Mulo")) \
                        and not a.actor_has_tag(PASSO_REALE):
                    q = a.get_actor_location()
                    if math.hypot(q.x - qs.x, q.y - qs.y) < 600.0:
                        a.modify()
                        a.set_actor_location(unreal.Vector(q.x + dx, q.y + dy, q.z + dz), False, False)
                        con += 1
            scrivi("stalla del mulo spostata a ovest della strada (lungo %.0f, traverso %.0f), con %d sue cose"
                   % (PASSO_STALLA[0], PASSO_STALLA[1], con))
    botti = sorted((a for a in attori.get_all_level_actors() if a and a.get_actor_label().startswith("Botte della Guardia")),
                   key=lambda a: a.get_actor_label())
    mosse = 0
    for a, (lu, tr) in zip(botti, PASSO_BOTTI):
        if sposta(a, lu, tr):
            mosse += 1
    if len(botti) > len(PASSO_BOTTI):
        avviso("passo: le botti sono %d ma i posti nuovi %d: le ultime restano dove sono (da spostare a mano)"
               % (len(botti), len(PASSO_BOTTI)))
    if botti:
        scrivi("botti della Guardia spostate accanto alla stalla: %d su %d" % (mosse, len(botti)))

    # 9. (18/11 sera) niente erba sotto il portale e lungo la strada del cortile: la maschera T_SenzaErba (in
    # V:/Texture/valdorso/villaggio) ha il rettangolo nuovo; il materiale della valle la rilegge e l'erba si ricalcola
    valle = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()),
                         "Python", "usati", "crea_materiale_valle.py")
    try:
        with open(valle, "r", encoding="utf-8") as g:
            exec(compile(g.read(), valle, "exec"), {"__name__": "__main__"})
        unreal.SystemLibrary.execute_console_command(mondo, "grass.FlushCache")
        scrivi("materiale della valle rifatto (T_SenzaErba con il passo) ed erba ricalcolata")
    except Exception as ex:  # noqa: BLE001
        avviso("passo: materiale della valle non rifatto (%s): lanciare a mano usati/crea_materiale_valle.py" % ex)

    scrivi("messi: " + ", ".join("%s %d" % (k, n) for k, n in sorted(conti.items())))
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    scrivi("FATTO e salvato. ORA, in questo ordine: 'PNG: metti Gaspare, le guardie del passo...', 'Volti: ...', 'Rete', "
           "'PNG: panche, letti e porte', 'Per Claude: collauda i passaggi', 'Mura: 7' (i gradini). "
           "Se un giorno si rilancia 'crea il villaggio' (o la sua parte 'Guardia'), dopo va rilanciata questa voce.")


# ------------------------------------------------------------------------------------------------------------------
# (19/11, v0.2.7) "Mura: 12. i registri della ronda". I pezzi della chat 3D (V:/Texture/valdorso/ronda): una cassetta
# di ferro con il libro, su un palo. Quattro nel villaggio: locanda, forgia, mulino (accanto alla porta, fuori dai
# 120 cm davanti) e tempio (accanto alla pietra dei segni, che c'e' in tutte e tre le fasi). Sono attori del C++
# (AValdorsoRegistroRonda): la guardia di ronda li firma con E. Si puo' rilanciare: toglie quelli di prima.
# Chi firma sta a -Y del pezzo (75 cm), quindi il pezzo si gira con la sua -Y verso fuori.
RONDA = "V:/Texture/valdorso/ronda"
CAMPANELLA = "V:/Texture/valdorso/campanella"
RONDA_ETICHETTA = "ValdorsoRegistroRonda"
RONDA_CARTELLA = "Valle/Villaggio/Ronda"
# (tappa, come si chiama negli avvisi, edificio, porta, di lato preferito: +1 a destra di chi esce, -1 a sinistra)
RONDA_PORTE = (
    ("Locanda", "la locanda di Bruno", "Locanda_di_Bruno", "Locanda_di_Bruno porta", 1),
    ("Forgia", "la forgia di Marta", "Fucina_di_Marta", "Fucina_di_Marta porta", 1),
    ("Mulino", "il mulino", "Mulino", "Mulino porta", 1),
)
RONDA_FUORI = 75.0          # dal filo della porta verso fuori (il pezzo e' profondo 48)
RONDA_DI_LATO = 140.0       # dal bordo dell'anta
# il tempio: 170 cm a ovest della pietra dei segni (14820, 40298), girato verso la piazza (fuori = +Y del mondo)
RONDA_TEMPIO = ("Tempio", "il tempio", 14650.0, 40298.0, 5509.0, 0.0, 1.0)


def per_nome_vuoto():
    return "Locanda_di_Bruno" not in tutti_per_nome()


def fai_ronda():
    global CARTELLA
    classe = getattr(unreal, "ValdorsoRegistroRonda", None)
    if classe is None:
        avviso("ronda: manca la classe del C++ ValdorsoRegistroRonda: compila, poi rilancia")
        return
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    mat = materiali()
    if not mat:
        # (secondo controllo) senza i materiali il pezzo resta anche senza collisione, e con E non si troverebbe
        avviso("ronda: mancano i materiali del villaggio (MI_Villaggio_...): niente")
        return
    if per_nome_vuoto():
        avviso("ronda: nel livello non vedo il villaggio (sei in L_Valle, con il villaggio caricato?): niente")
        return
    vecchia = CARTELLA
    CARTELLA = RONDA
    try:
        pezzi = importa(["SM_Registro_Ronda_Palo"], mat)
    finally:
        CARTELLA = vecchia
    palo = pezzi.get("SM_Registro_Ronda_Palo")
    if palo is None:
        avviso("ronda: manca SM_Registro_Ronda_Palo in %s/glb: niente" % RONDA)
        return
    tolti = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(RONDA_ETICHETTA):
            attori.destroy_actor(a)
            tolti += 1
    if tolti:
        scrivi("tolti %d registri messi prima" % tolti)
    per_nome = tutti_per_nome()

    def libero(x, y, z, pende=25.0):
        """Vero se in quel punto, e a 35 cm tutto intorno, sotto c'e' solo il prato o il selciato (niente gradini,
        panche, muri). (19/11, dopo il primo lancio) la piazza e le strade sono modelli, non prato: vanno bene, purche'
        piane (entro 25 cm dalla quota del punto)."""
        for dx, dy in ((0, 0), (35, 0), (-35, 0), (0, 35), (0, -35)):
            c = linea_colono(mondo, unreal.Vector(x + dx, y + dy, z + 260.0), unreal.Vector(x + dx, y + dy, z - 300.0), [])
            if c is None:
                return False, "sotto non c'e' niente"
            chi = c[9]
            nome = chi.get_actor_label() if chi else ""
            prato = chi is not None and "Landscape" in chi.get_class().get_name()
            selciato = nome.startswith(("Piazza", "Strada", "Via ", "Sagrato"))
            if not (prato or selciato):
                return False, "c'e' %s" % (nome or "qualcosa")
            if abs(c[4].z - z) > pende:
                return False, "non e' piano (%.0f cm)" % (c[4].z - z)
        return True, ""

    def suolo(x, y, z_vicino, da=300.0):
        """La quota di dove si cammina in quel punto (prato o selciato), cercata da 3 m sopra a 8 m sotto."""
        c = linea_colono(mondo, unreal.Vector(x, y, z_vicino + da), unreal.Vector(x, y, z_vicino - 800.0), [])
        return c[4].z if c is not None else terreno(mondo, x, y, z_vicino)

    def metti(tappa, luogo, x, y, z_vicino, fx, fy):
        """Mette il registro in (x, y) con il davanti (dove sta chi firma) verso (fx, fy)."""
        z = suolo(x, y, z_vicino, 100.0)
        if z is None:
            avviso("ronda: %s: non trovo il terreno in (%.0f, %.0f): non messo" % (tappa, x, y))
            return None
        yaw = math.degrees(math.atan2(fx, -fy))      # la -Y del pezzo verso (fx, fy)
        a = attori.spawn_actor_from_class(classe, unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
        if a is None:
            avviso("ronda: %s: non riesco a metterlo" % tappa)
            return None
        c = a.get_editor_property("modello")
        c.set_editor_property("static_mesh", palo)
        a.set_editor_property("tappa", unreal.Name(tappa))
        a.set_editor_property("luogo", luogo)
        a.set_actor_label("Registro della ronda ~ %s" % tappa)
        a.set_folder_path(RONDA_CARTELLA)
        a.set_editor_property("tags", [unreal.Name(RONDA_ETICHETTA)])
        scrivi("%s: registro in (%.0f, %.0f, %.0f), yaw %.1f; chi firma sta in (%.0f, %.0f)"
               % (tappa, x, y, z, yaw, x + fx * 75.0, y + fy * 75.0))
        return a

    messi = 0
    for tappa, luogo, n_casa, n_porta, lato_preferito in RONDA_PORTE:
        casa, porta = per_nome.get(n_casa), per_nome.get(n_porta)
        if casa is None or porta is None:
            avviso("ronda: %s: non trovo '%s' o '%s' nel livello: saltato" % (tappa, n_casa, n_porta))
            continue
        pc, pp = casa.get_actor_location(), porta.get_actor_location()
        # il verso "fuori": dal centro della casa alla porta, raddrizzato sull'asse della casa piu' vicino
        yc = math.radians(casa.get_actor_rotation().yaw)
        assi = [(math.cos(yc + k * math.pi / 2.0), math.sin(yc + k * math.pi / 2.0)) for k in range(4)]
        vx, vy = pp.x - pc.x, pp.y - pc.y
        fx, fy = max(assi, key=lambda t: t[0] * vx + t[1] * vy)
        lx, ly = -fy, fx
        # quanto e' larga l'anta (lungo il muro)
        larga = 100.0
        try:
            o, e = porta.get_actor_bounds(True)
            larga = max(80.0, min(240.0, 2.0 * (abs(e.x * lx) + abs(e.y * ly))))
            # il centro vero dell'anta (l'attore sta sulla cerniera)
            cx, cy = o.x, o.y
        except Exception:  # noqa: BLE001
            cx, cy = pp.x, pp.y
        fatto = False
        prove = []
        # (19/11, dopo il secondo lancio) la porta del mulino e' in alto, in cima alla scala di fuori ("Mura: 8"), e
        # intorno il terreno pende. Li' il registro va a terra contro il muro, sotto la porta e di lato (dalla parte
        # senza la scala: sotto i gradini c'e' la scala e il posto viene scartato): il terreno si cerca da sotto il
        # pianerottolo, e puo' pendere di piu' (lo zoccolo scende 30 cm sotto terra).
        scala = per_nome.get("Scala ~ " + n_porta)
        fuori_base, lato_base, salto, da, pende = RONDA_FUORI, larga / 2.0 + RONDA_DI_LATO, 120.0, 300.0, 25.0
        fuori_prove = (fuori_base, fuori_base + 40.0)
        ripiego = None
        if scala is not None:
            salto, da, pende = 900.0, -80.0, 45.0
            # (19/11, dopo il terzo lancio) contro il muro del mulino il terreno scende di 70 cm in 75: si cerca anche
            # piu' in fuori, dove spiana (li' stanno Teodoro e Agnese). Se proprio niente va bene, si mette nel posto
            # libero che pende meno (ripiego), e lo si dice.
            fuori_prove = (fuori_base, 150.0, 230.0, 310.0, 400.0, 500.0)
            scrivi("%s: la porta ha la scala di fuori: il registro va a terra, sotto, di lato" % tappa)
        for lato in (lato_preferito, -lato_preferito):
            for di_lato in (0.0, 60.0, 120.0, 200.0, 300.0):
                for fuori in fuori_prove:
                    d = lato_base + di_lato
                    x = cx + fx * fuori + lx * d * lato
                    y = cy + fy * fuori + ly * d * lato
                    z = suolo(x, y, pp.z, da)
                    if z is None:
                        prove.append("(%.0f, %.0f): niente terreno" % (x, y))
                        continue
                    if abs(z - pp.z) > salto:
                        prove.append("(%.0f, %.0f): il terreno e' a %.0f cm dalla soglia" % (x, y, z - pp.z))
                        continue
                    ok, perche = libero(x, y, z, pende)
                    if not ok:
                        prove.append("(%.0f, %.0f): %s" % (x, y, perche))
                        continue
                    # anche il posto di chi firma deve essere libero
                    ok, perche = libero(x + fx * 75.0, y + fy * 75.0, z, pende + 15.0)
                    if not ok:
                        prove.append("(%.0f, %.0f): davanti %s" % (x, y, perche))
                        if scala is not None and "piano" in perche:
                            zd = suolo(x + fx * 75.0, y + fy * 75.0, z, 100.0)
                            scende = abs((zd if zd is not None else z - 999.0) - z)
                            if ripiego is None or scende < ripiego[0]:
                                ripiego = (scende, x, y, z, d, fuori)
                        continue
                    if metti(tappa, luogo, x, y, z, fx, fy):
                        messi += 1
                        scrivi("  (a %.0f cm dal centro della porta, di lato; %.0f cm fuori dal muro; anta larga %.0f)"
                               % (d, fuori, larga))
                    fatto = True
                    break
                if fatto:
                    break
            if fatto:
                break
        if not fatto and ripiego is not None:
            scende, x, y, z, d, fuori = ripiego
            if metti(tappa, luogo, x, y, z, fx, fy):
                messi += 1
                fatto = True
                avviso("ronda: %s: nessun posto piano: messo dove pende meno (davanti scende di %.0f cm), a %.0f cm di lato "
                       "e %.0f fuori dal muro: guarda la foto" % (tappa, scende, d, fuori))
        if not fatto:
            avviso("ronda: %s: nessun posto libero accanto alla porta. Provati: %s" % (tappa, "; ".join(prove)))

    tappa, luogo, x, y, zv, fx, fy = RONDA_TEMPIO
    z = suolo(x, y, zv)
    if z is not None:
        ok, perche = libero(x, y, z)
        if not ok:
            avviso("ronda: Tempio: nel posto scelto %s: lo metto lo stesso, guarda la foto" % perche)
    if metti(tappa, luogo, x, y, zv, fx, fy):
        messi += 1

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    scrivi("FATTO e salvato: %d registri su 4. Il tempio va guardato in tutte e tre le fasi ('Villaggio: mostra la fase N')."
           % messi)



try:
    if MODO == "assi":
        fai_assi()
    elif MODO == "mura":
        fai_mura()
    elif MODO == "tempio":
        fai_tempio()
    elif MODO == "indietro":
        fai_indietro()
    elif MODO == "strade":
        fai_strade()
    elif MODO == "caserma":
        fai_caserma()
    elif MODO == "alloggi":
        fai_alloggi()
    elif MODO == "gradini":
        fai_gradini()
    elif MODO == "scale":
        fai_scale()
    elif MODO == "ponti":
        fai_ponti()
    elif MODO == "locanda":
        fai_locanda()
    elif MODO == "passo":
        fai_passo()
    elif MODO == "ronda":
        fai_ronda()
    else:
        avviso("modo sconosciuto: %s" % MODO)
except Exception as e:  # noqa: BLE001
    avviso("errore: %s" % e)
    raise
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    # (16/11 sera) un resoconto per ogni voce: prima il tempio copriva quello delle mura
    with open(os.path.join(cartella, "mura_rapporto_%s.txt" % MODO), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
