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
    if st and pezzi.get("SM_Stalla_Mulo"):
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
    pezzi = importa(["SM_Tempio_Fase2", "SM_Tempio_Fase3", "SM_Prova_Assi"], mat)
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
        metti_mesh(m, unreal.Vector(o.x, o.y, o.z), yaw, "Tempio del Cuore, fase %d" % fase, sy,
                   fase="ValdorsoFase %d-%d" % (fase, 0 if fase == 3 else 2), cartella="Valle/Tempio")
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
            spegni(a, False)
            accese += 1
        if a and a.actor_has_tag(CAMBIATA):
            fase_solo_2(a, False)
        if a and a.actor_has_tag(TEMPIO_CAMBIATO):
            tags = [t for t in a.get_editor_property("tags") if not str(t).startswith("ValdorsoFase")
                    and str(t) != TEMPIO_CAMBIATO]
            a.set_editor_property("tags", tags + [unreal.Name("ValdorsoFase 1-0")])
            scrivi("la cappella torna in tutte le fasi")
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
    pezzi = importa(sorted(set(p["pezzo"] for p in posizioni)), materiali())
    sy = numero_specchio() or 1.0
    via = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(CASERMA):
            attori.destroy_actor(a)
            via += 1
    if via:
        scrivi("tolti %d pezzi del recinto di prima" % via)

    def mio(a, nome):
        a.set_actor_label("Caserma ~ " + nome)
        a.set_folder_path("Valle/Caserma")
        a.set_editor_property("tags", [unreal.Name(CASERMA), unreal.Name(FASE_MURA)])
        return a

    conti = {}
    grata = None
    da_fare_dopo = []
    for p in posizioni:
        m = pezzi.get(p["pezzo"])
        if m is None:
            continue
        gp = p["posizione_cm"]
        dove = unreal.Vector(gp[0] + angolo.x, gp[1] + angolo.y, float(gp[2]))
        yaw = float(p["yaw"])
        nome, pezzo = p["nome"], p["pezzo"]
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
                or porta.actor_has_tag(CASERMA) or nome.startswith("Capanna")):
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
                    a.set_actor_location(unreal.Vector(nx, ny, dove.z), False, False)
                    scrivi("  %s spostata di (%+.0f, %+.0f) cm, come nei dati del villaggio" % (c["nome"], nx - dove.x, ny - dove.y))
    except Exception as e:  # noqa: BLE001
        avviso("lanterna della caserma non spostata (%s)" % e)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    scrivi("FATTO e salvato. Ora rilancia 'Per Claude: collauda i passaggi' per vedere che cosa resta.")


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
