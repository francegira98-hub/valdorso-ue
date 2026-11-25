# Valdorso - Le Terre di Montalto, la mappa oltre il passo (PROVA). Scritto da Claude il 24/11/2026.
# E' una prova chiesta da Fra per vedere come viene: NON tocca L_Valle, NON toglie il sigillo del passo, non c'e'
# nessun viaggio dalla valle a qui. Il livello nuovo e' /Game/Prove/L_Montalto.
#
# Si lancia dal menu Valdorso, in tre passi:
#   «Montalto: 1. crea il livello»      crea (o riapre) L_Montalto con sole, cielo, nebbia e nuvole, e scrive nel
#                                       resoconto i numeri per importare il terreno.
#   (a mano, come per la valle)         Modalita' Paesaggio -> Importa da file -> Altezze_4000.png, con i numeri del resoconto.
#   «Montalto: 2. vesti il terreno»     materiale del terreno (lo stesso della valle, con le maschere di Montalto: erba,
#                                       terra, ghiaia, roccia, neve e l'erba vera) e i tre fiumi con il plugin Water:
#                                       l'Aure (navigabile, 80 m), la Brina e il torrente di Pietralta.
#   «Montalto: 4. la rocca del duca»     (25/11) importa i pezzi della rocca (Montalto_rocca.json) e la monta sul piano in
#                                       cima al colle: 4 torri, 22 cortine, la porta con la grata del passo, mastio,
#                                       palazzo, cappella, scuderie, pozzo.
#   «Montalto: 5. il porto»              (25/11) la riva dei magazzini della lana: banchina di 200 m a valle del ponte,
#                                       pontile, gru, magazzini, muro di sponda, rampa, scala e le due barche.
#   «Montalto: 6. il Granaio della Corona» (25/11) il granaio e il suo piano di carico, nella citta' bassa.
#   «Montalto: 7. le case di citta'»     (25/11) qualche centinaio di case a schiera lungo le strade della citta' (dove
#                                       vanno lo dice Case_4000.json, fatto da montalto.py).
#   «Montalto: 8. le mura e la porta nord» (25/11) il giro delle mura della citta' bassa, con torri, varchi e Porta nord.
#   «Montalto: 9. il cimitero vecchio»   (25/11) recinto, cancello, cappella e 48 tombe, fuori dalla Porta nord.
#   «Montalto: 3. il ponte sull'Aure»    (25/11) importa i 7 pezzi del ponte della chat 3D (V:/Texture/valdorso/montalto/glb,
#                                       dati in Montalto_ponte.json) e lo monta dove la Strada del Re passa l'Aure:
#                                       teste, arcate, pile, le due torri, il levatoio a due ali (chiuso) e le 4 catene.
#                                       Il levatoio per ora e' fermo: la leva e l'argano arrivano con il C++.
# Si puo' rilanciare: rimpiazza quello che ha fatto prima. Resoconto in Saved/Claude/montalto.txt.
#
# I file li fa Tools/Generatori/montalto.py e stanno in V:/Texture/valdorso/montalto.
# ATTENZIONE: in L_Montalto non premere Play. Il colono salvato ha la posizione della valle e qui finirebbe nel vuoto;
# la mappa si guarda volando con la telecamera dell'editor. Il viaggio dal passo e' un lavoro a parte.

import json
import os

import unreal

MODO = globals().get("MONTALTO_MODO", "livello")
LIVELLO = "/Game/Prove/L_Montalto"
NOME_LIVELLO = "L_Montalto"
CARTELLA = "V:/Texture/valdorso/montalto"
LATO = 4000
FILE_LUOGHI = CARTELLA + "/Luoghi_%d.json" % LATO
FILE_ACQUA = CARTELLA + "/Acqua_%d.json" % LATO
FILE_ALTEZZE = CARTELLA + "/Altezze_%d.png" % LATO
ETICHETTA = "MontaltoProva"
ETICHETTA_ACQUA = "MontaltoAcqua"
QUOTA_MAX_CM = 80000.0
RISOLUZIONE_ZONA = 2048
SEMPRE = unreal.PropertyAccessChangeNotifyMode.ALWAYS
CARTELLA_MATERIALI_ACQUA = "/Game/Valle/Acqua"

libreria = unreal.EditorAssetLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Montalto: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE " + testo)
    unreal.log_warning("[Valdorso] Montalto: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "montalto.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")


def nome_del_livello_aperto():
    try:
        mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        return mondo.get_name() if mondo is not None else ""
    except Exception:  # noqa: BLE001
        return ""


def paesaggi_del_livello():
    return [a for a in attori.get_all_level_actors() if isinstance(a, unreal.Landscape)]


def quota_del_paesaggio(x, y):
    """La quota del Paesaggio in (x, y) del mondo, con un raggio dall'alto; None se non lo trova."""
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ignora = []
    for _ in range(12):
        try:
            r = unreal.SystemLibrary.line_trace_multi(mondo, unreal.Vector(x, y, 200000.0), unreal.Vector(x, y, -200000.0),
                                                      unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora,
                                                      unreal.DrawDebugTrace.NONE, True)
            colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
        except Exception:  # noqa: BLE001
            return None
        fermato = None
        for hit in colpi or []:
            t = hit.to_tuple()
            if t[9] is None:
                continue
            if "Landscape" in t[9].get_class().get_name():
                return t[4].z
            if t[0]:
                fermato = t[9]
        if fermato is None or fermato in ignora:
            return None
        ignora.append(fermato)
    return None


def terreno_aggiornato(titolo, blocca=True):
    """(25/11 sera) Controlla che il Paesaggio in Unreal sia quello dell'ultimo Altezze_4000.png: misura la quota in
    alcuni punti che Luoghi_4000.json conosce (prove_terreno) e la confronta. Se non torna, lo dice chiaro e NON monta:
    vuol dire che il Paesaggio va cancellato e reimportato."""
    luoghi = leggi_json(FILE_LUOGHI)
    if luoghi is None or not paesaggi_del_livello():
        return False
    if len(paesaggi_del_livello()) > 1:
        # (25/11 sera) e' successo davvero: tre terreni uno sopra l'altro, e il vecchio copriva la banchina del porto
        avviso("%s: NEL LIVELLO CI SONO %d TERRENI UNO SOPRA L'ALTRO. Nell'Outliner cerca «Landscape», cancellali tutti "
               "(anche i «WaterBrushManager»), rifai «Importa da file» UNA volta sola con i numeri del passo 1, poi "
               "«Montalto: 2» e le altre voci." % (titolo, len(paesaggi_del_livello())))
        return False
    prove = luoghi.get("prove_terreno")
    if not prove:
        return True
    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()
    sbagliate = []
    misurate = 0
    sotto_nuove = sotto_vecchie = 0
    for pr in prove:
        x, y, z = pr["unreal_cm"]
        attesa = angolo.z + (z / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
        vera = quota_del_paesaggio(angolo.x + x, angolo.y + y)
        if vera is None:
            continue
        misurate += 1
        base = pr.get("base_cm")
        if base is not None and abs(base - z) >= 20.0:
            # qui l'ultimo giro ha abbassato il terreno (sotto una casa): a quale delle due quote somiglia?
            attesa_base = angolo.z + (base / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
            if abs(vera - attesa_base) < abs(vera - attesa):
                sotto_vecchie += 1
            else:
                sotto_nuove += 1
        if abs(vera - attesa) > 120.0:
            sbagliate.append("%s: in Unreal %.1f m, nel file %.1f m" % (pr["cosa"], vera / 100.0, attesa / 100.0))
    if misurate == 0:
        scrivi("(non riesco a misurare il terreno: vado avanti senza il controllo)")
        return True
    if not sbagliate and sotto_vecchie > sotto_nuove:
        sbagliate.append("sotto le case il terreno non e' ancora abbassato (%d punti su %d)"
                         % (sotto_vecchie, sotto_vecchie + sotto_nuove))
    if sbagliate and not blocca:
        avviso("IL TERRENO IN UNREAL SEMBRA VECCHIO (%s): cancella l'attore Landscape, rifai «Importa da file» con i numeri "
               "del passo 1 e rilancia questa voce." % "; ".join(sbagliate[:4]))
        return False
    if sbagliate:
        avviso("%s: IL TERRENO IN UNREAL E' VECCHIO, non lo monto. In %d punti su %d la quota non e' quella dell'ultimo "
               "Altezze_%d.png (%s). Cancella l'attore Landscape, rifai «Importa da file» con i numeri del passo 1, poi "
               "«Montalto: 2» e di nuovo questa voce." % (titolo, len(sbagliate), misurate, LATO, "; ".join(sbagliate[:4])))
        return False
    scrivi("terreno controllato in %d punti: e' quello giusto" % misurate)
    return True


def leggi_json(percorso):
    if not os.path.exists(percorso):
        avviso("manca il file " + percorso)
        return None
    with open(percorso, "r", encoding="utf-8") as f:
        return json.load(f)


# ====================================================================================== 1. il livello
def uno_di(classe, nome, posto=None, giro=None):
    """Un attore di quella classe con la nostra etichetta: se c'e' gia' (di qualunque nome) lo lascia."""
    for a in attori.get_all_level_actors():
        if isinstance(a, classe):
            return a, False
    a = attori.spawn_actor_from_class(classe, posto or unreal.Vector(0, 0, 60000), giro or unreal.Rotator(0, 0, 0))
    if a is None:
        avviso("non riesco a creare " + nome)
        return None, False
    a.set_actor_label(nome)
    a.tags = [unreal.Name(ETICHETTA)]
    return a, True


def crea_livello():
    aperto = nome_del_livello_aperto()
    if aperto != NOME_LIVELLO:
        # (secondo controllo, 24/11) questo script non salva MAI un altro livello: se nel livello aperto c'e' qualcosa
        # di non salvato ci si ferma, e lo salva Fra a mano (File -> Salva tutto)
        try:
            sporchi = list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()) \
                + list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
        except Exception as e:  # noqa: BLE001
            avviso("non riesco a sapere se c'e' qualcosa da salvare (%s): salva tutto a mano (Ctrl+Maiusc+S) e rilancia" % e)
            return
        if sporchi:
            avviso("nel livello aperto (%s) ci sono %d cose non salvate: salva tutto (File -> Salva tutto, "
                   "Ctrl+Maiusc+S) e rilancia. Non ho toccato niente." % (aperto, len(sporchi)))
            return
        for f in (FILE_ALTEZZE, FILE_LUOGHI):
            if not os.path.exists(f):
                avviso("manca " + f + ": non creo il livello. Dillo a Claude.")
                return
        if libreria.does_asset_exist(LIVELLO):
            if not livelli.load_level(LIVELLO):
                avviso("non riesco ad aprire " + LIVELLO)
                return
            scrivi("aperto il livello che c'era: " + LIVELLO)
        else:
            if not livelli.new_level(LIVELLO):
                avviso("non riesco a creare " + LIVELLO)
                return
            scrivi("creato il livello nuovo: " + LIVELLO)
    else:
        scrivi("L_Montalto e' gia' aperto")
    if nome_del_livello_aperto() != NOME_LIVELLO:
        avviso("il livello aperto non e' L_Montalto: mi fermo senza toccare niente")
        return

    centro = unreal.Vector(LATO * 50.0, LATO * 50.0, 60000.0)
    sole, nuovo = uno_di(unreal.DirectionalLight, "Sole di Montalto", centro, unreal.Rotator(0.0, -42.0, -125.0))
    if sole is not None and nuovo:
        comp = sole.get_component_by_class(unreal.DirectionalLightComponent)
        for nome, valore in (("mobility", unreal.ComponentMobility.MOVABLE), ("intensity", 8.0),
                             ("atmosphere_sun_light", True)):
            try:
                comp.set_editor_property(nome, valore)
            except Exception as e:  # noqa: BLE001
                avviso("sole: %s non impostato (%s)" % (nome, e))
        scrivi("messo il sole")
    for nome_classe, nome in (("SkyAtmosphere", "Cielo di Montalto"), ("ExponentialHeightFog", "Nebbia di Montalto"),
                              ("VolumetricCloud", "Nuvole di Montalto")):
        classe = getattr(unreal, nome_classe, None)
        if classe is None:
            avviso("manca la classe " + nome_classe)
            continue
        a, nuovo = uno_di(classe, nome, centro)
        if a is not None and nuovo:
            scrivi("messo: " + nome)
    nebbia_a_posto()
    luce, nuovo = uno_di(unreal.SkyLight, "Luce del cielo di Montalto", centro)
    if luce is not None and nuovo:
        comp = luce.get_component_by_class(unreal.SkyLightComponent)
        for nome, valore in (("mobility", unreal.ComponentMobility.MOVABLE), ("real_time_capture", True)):
            try:
                comp.set_editor_property(nome, valore)
            except Exception as e:  # noqa: BLE001
                avviso("luce del cielo: %s non impostato (%s)" % (nome, e))
        scrivi("messa la luce del cielo")

    dati = leggi_json(FILE_LUOGHI)
    scrivi("")
    scrivi("ADESSO, A MANO (una volta sola), IL TERRENO:")
    scrivi("  1. in alto a sinistra: «Modalita' Selezione» -> «Paesaggio»")
    scrivi("  2. a sinistra, scheda «Gestisci» -> «Nuovo» -> «Importa da file»")
    scrivi("  3. File della mappa di altezza: " + FILE_ALTEZZE)
    if dati is not None:
        scrivi("  4. Posizione:  X 0   Y 0   Z %s" % dati["posizione_z_cm"])
        scrivi("     Scala:      X %s   Y %s   Z %s" % (dati["scala_xy_cm"], dati["scala_xy_cm"], dati["scala_z"]))
        scrivi("     (il terreno ha %d x %d punti: Unreal deve leggere da solo questa misura dal file)"
               % (dati["vertici"], dati["vertici"]))
    scrivi("  5. «Importa», aspetta, poi torna in «Modalita' Selezione» e salva (Ctrl+S)")
    scrivi("  6. menu Valdorso -> «Montalto: 2. vesti il terreno»")
    if paesaggi_del_livello():
        scrivi("(nel livello c'e' gia' un Paesaggio: se e' quello giusto, salta al punto 6)")
    try:
        livelli.save_current_level()
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a salvare il livello (%s)" % e)


# ====================================================================================== 2a. il materiale
def materiale_del_terreno():
    """Lo script della valle (usati/crea_materiale_valle.py), tale e quale, con i file e le misure di Montalto.
    Cosi' Montalto ha lo stesso terreno della valle e ogni miglioria fatta li' arriva anche qui."""
    sorgente = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()),
                            "Python", "usati", "crea_materiale_valle.py")
    if not os.path.exists(sorgente):
        avviso("manca " + sorgente + ": il materiale non si fa")
        return False
    with open(sorgente, "r", encoding="utf-8") as f:
        codice = f.read()
    cambi = (
        ('MASCHERE = "V:/Texture/valdorso/valle/T_ValleMaschere_3000.png"',
         'MASCHERE = "%s/T_MontaltoMaschere_%d.png"' % (CARTELLA, LATO), 1),
        ('MASCHERE2 = "V:/Texture/valdorso/valle/T_ValleMaschere2_3000.png"',
         'MASCHERE2 = "%s/T_MontaltoMaschere2_%d.png"' % (CARTELLA, LATO), 1),
        ('SENZA_ERBA = "V:/Texture/valdorso/villaggio/T_SenzaErba.png"',
         'SENZA_ERBA = "%s/T_MontaltoSenzaErba.png"' % CARTELLA, 1),
        ('DESTINAZIONE = "/Game/Valle/Terreno"', 'DESTINAZIONE = "/Game/Montalto/Terreno"', 1),
        ('LATO_VALLE_CM = 300000.0', 'LATO_VALLE_CM = %.1f' % (LATO * 100.0), 1),
        ('DESTINAZIONE + "/M_Valle"', 'DESTINAZIONE + "/M_Montalto"', 1),
        ('strumenti.create_asset("M_Valle", DESTINAZIONE', 'strumenti.create_asset("M_Montalto", DESTINAZIONE', 1),
    )
    for vecchio, nuovo, quante in cambi:
        if codice.count(vecchio) != quante:
            avviso("lo script del materiale della valle e' cambiato (non trovo: %s): il materiale non si fa, "
                   "dillo a Claude" % vecchio)
            return False
        codice = codice.replace(vecchio, nuovo)
    for f in ("T_MontaltoMaschere_%d.png" % LATO, "T_MontaltoMaschere2_%d.png" % LATO):
        if not os.path.exists(CARTELLA + "/" + f):
            avviso("manca " + CARTELLA + "/" + f + ": il materiale non si fa")
            return False
    exec(compile(codice, sorgente, "exec"), {"__name__": "__main__"})   # noqa: S102
    materiale = libreria.load_asset("/Game/Montalto/Terreno/M_Montalto") \
        if libreria.does_asset_exist("/Game/Montalto/Terreno/M_Montalto") else None
    if materiale is None:
        avviso("M_Montalto non e' stato creato: guarda le righe «[Valdorso] Valle:» nel Registro output")
        return False
    sul_paesaggio = paesaggi_del_livello()[0].get_editor_property("landscape_material")
    if sul_paesaggio is None or sul_paesaggio.get_name() != "M_Montalto":
        avviso("M_Montalto esiste ma non e' sul paesaggio: lo script della valle si e' fermato prima "
               "(guarda le righe «[Valdorso] Valle:» nel Registro output)")
        return False
    scrivi("materiale del terreno: /Game/Montalto/Terreno/M_Montalto, messo sul paesaggio")
    return True


# ====================================================================================== 2b. i fiumi
def fiumi():
    """Come crea_acqua_valle.py, ma solo fiumi: l'Aure, la Brina e il torrente di Pietralta."""
    Fiume = getattr(unreal, "WaterBodyRiver", None)
    Zona = getattr(unreal, "WaterZone", None)
    if Fiume is None or Zona is None:
        avviso("il plugin Water non e' acceso: niente fiumi")
        return
    dati = leggi_json(FILE_ACQUA)
    if dati is None:
        return
    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()

    def nel_mondo(p):
        valore = p[2] / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + p[0], angolo.y + p[1], z)

    tutti = attori.get_all_level_actors()
    for a in tutti:
        if ETICHETTA_ACQUA in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    zone = [a for a in attori.get_all_level_actors() if isinstance(a, Zona)]
    if zone:
        zona = zone[0]
    else:
        zona = attori.spawn_actor_from_class(
            Zona, unreal.Vector(angolo.x + dati["lato_cm"] / 2, angolo.y + dati["lato_cm"] / 2, 0))
        scrivi("creata la zona d'acqua")
    estensione = unreal.Vector2D(dati["lato_cm"] + 20000, dati["lato_cm"] + 20000)

    def colore_nostro(componente, nome):
        coppia = (("MI_AcquaFiume", "water_material"), ("MI_AcquaFiume_LOD", "water_static_mesh_material"),
                  ("MI_AcquaLontana", "water_hlod_material"))
        for asset, proprieta in coppia:
            percorso = CARTELLA_MATERIALI_ACQUA + "/" + asset
            if not libreria.does_asset_exist(percorso):
                continue
            try:
                componente.set_editor_property(proprieta, libreria.load_asset(percorso))
            except Exception as e:  # noqa: BLE001
                avviso("%s: non riesco a dare %s (%s)" % (nome, asset, e))

    fatti = []
    for chiave in ("fiume", "brina", "torrente"):
        if chiave not in dati:
            continue
        nome = dati[chiave].get("nome", chiave)
        punti = dati[chiave]["punti"]
        posizioni = [nel_mondo(p["posizione"]) for p in punti]
        fiume = attori.spawn_actor_from_class(Fiume, posizioni[0])
        if fiume is None:
            avviso("non riesco a creare il fiume " + nome)
            continue
        fiume.set_actor_label(nome)
        fiume.tags = [unreal.Name(ETICHETTA_ACQUA)]
        componente = fiume.get_water_body_component()
        try:
            componente.set_editor_property("affects_landscape", False)
        except Exception as e:  # noqa: BLE001
            avviso("%s: non riesco a spegnere «Affects Landscape» (%s)" % (nome, e))
        colore_nostro(componente, nome)
        spline = fiume.get_water_spline()
        spline.set_spline_points(posizioni, unreal.SplineCoordinateSpace.WORLD, True)
        try:
            for i, p in enumerate(punti):
                componente.set_river_width_at_spline_input_key(float(i), float(p["larghezza_cm"]))
                componente.set_river_depth_at_spline_input_key(float(i), float(p["profondita_cm"]))
            try:
                base = componente.get_water_velocity_at_spline_input_key(0.0) or 1.0
                for i, p in enumerate(punti):
                    componente.set_water_velocity_at_spline_input_key(float(i), float(base * p.get("velocita", 1.0)))
            except Exception as e:  # noqa: BLE001
                avviso("%s: corrente punto per punto non riuscita (%s)" % (nome, e))
        except Exception as e:  # noqa: BLE001
            avviso("%s: larghezza punto per punto non riuscita, resta quella di Unreal (%s)" % (nome, e))
        scrivi("%s: %d punti, largo da %.0f a %.0f m, pelo da %.1f a %.1f m" % (
            nome, len(punti), min(p["larghezza_cm"] for p in punti) / 100.0,
            max(p["larghezza_cm"] for p in punti) / 100.0, punti[0]["posizione"][2] / 100.0,
            punti[-1]["posizione"][2] / 100.0))
        fatti.append((fiume, spline))

    # il «ridisegno» che serve a Unreal per far comparire l'acqua (lo stesso della valle)
    for fiume, spline in fatti:
        try:
            spline.set_editor_property("closed_loop", False, SEMPRE)
        except Exception:  # noqa: BLE001
            pass
        componente = fiume.get_water_body_component()
        try:
            componente.set_editor_property("water_material", componente.get_editor_property("water_material"), SEMPRE)
        except Exception as e:  # noqa: BLE001
            avviso("%s: ridisegno non riuscito (%s)" % (fiume.get_actor_label(), e))
        posto = fiume.get_actor_location()
        fiume.set_actor_location(posto + unreal.Vector(0, 0, 1), False, False)
        fiume.set_actor_location(posto, False, False)
    for nome, valore in (("render_target_resolution", unreal.IntPoint(RISOLUZIONE_ZONA, RISOLUZIONE_ZONA)),
                         ("zone_extent", estensione)):
        try:
            zona.set_editor_property(nome, valore, SEMPRE)
        except Exception as e:  # noqa: BLE001
            avviso("zona d'acqua: %s non impostato (%s)" % (nome, e))


# ====================================================================================== 2c. i segnaposto
def segnaposto():
    """Una nota per ogni luogo (si vedono solo nell'editor), per orientarsi volando sopra la mappa."""
    dati = leggi_json(FILE_LUOGHI)
    if dati is None:
        return
    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()
    for a in attori.get_all_level_actors():
        if "MontaltoLuogo" in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    classe = getattr(unreal, "Note", None) or unreal.TargetPoint
    quanti = 0
    for nome, luogo in dati["luoghi"].items():
        x, y, z = luogo["unreal_cm"]
        z_mondo = angolo.z + (z / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
        a = attori.spawn_actor_from_class(classe, unreal.Vector(angolo.x + x, angolo.y + y, z_mondo + 600.0))
        if a is None:
            continue
        a.set_actor_label("Luogo - " + nome)
        a.tags = [unreal.Name("MontaltoLuogo")]
        try:
            a.set_editor_property("text", nome)
        except Exception:  # noqa: BLE001
            pass
        quanti += 1
    scrivi("segnaposto dei luoghi: %d (nell'Outliner cercali con «Luogo»)" % quanti)


def nebbia_a_posto():
    """(25/11) La nebbia era stata messa a 600 m di quota: la nebbia di Unreal si infittisce sotto la sua quota, e a
    valle era un muro azzurro che nascondeva tutto il terreno (foto di Fra). Ora sta al livello del fiume ed e' leggera."""
    classe = getattr(unreal, "ExponentialHeightFog", None)
    if classe is None:
        return
    for a in attori.get_all_level_actors():
        if not isinstance(a, classe):
            continue
        posto = a.get_actor_location()
        a.set_actor_location(unreal.Vector(posto.x, posto.y, 5000.0), False, False)
        comp = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        for nome, valore in (("fog_density", 0.004), ("fog_height_falloff", 0.05)):
            try:
                comp.set_editor_property(nome, valore)
            except Exception as e:  # noqa: BLE001
                avviso("nebbia: %s non impostato (%s)" % (nome, e))
        scrivi("nebbia abbassata al livello del fiume e resa leggera")


def vesti():
    aperto = nome_del_livello_aperto()
    if aperto != NOME_LIVELLO:
        avviso("il livello aperto e' «%s», non L_Montalto: non tocco niente. Lancia prima «Montalto: 1. crea il livello»."
               % aperto)
        return
    if not paesaggi_del_livello():
        avviso("in L_Montalto non c'e' ancora il Paesaggio: importalo a mano (i passi sono nel resoconto del punto 1)")
        return
    p = paesaggi_del_livello()[0]
    s, l = p.get_actor_scale3d(), p.get_actor_location()
    scrivi("paesaggio: posizione %.0f %.0f %.0f, scala %.3f %.3f %.3f" % (l.x, l.y, l.z, s.x, s.y, s.z))
    dati = leggi_json(FILE_LUOGHI)
    if dati is not None and (abs(s.x - dati["scala_xy_cm"]) > 0.05 or abs(s.z - dati["scala_z"]) > 0.05
                             or abs(l.z - dati["posizione_z_cm"]) > 1.0):
        avviso("la scala o la quota del paesaggio non sono quelle del resoconto (X/Y %s, Z %s, posizione Z %s): "
               "fiumi e luoghi potrebbero non combaciare" % (dati["scala_xy_cm"], dati["scala_z"], dati["posizione_z_cm"]))
    terreno_aggiornato("attenzione", blocca=False)
    for nome, passo in (("nebbia", nebbia_a_posto), ("materiale", materiale_del_terreno), ("fiumi", fiumi),
                        ("segnaposto", segnaposto)):
        try:
            passo()
        except Exception as e:  # noqa: BLE001
            avviso("%s: errore (%s)" % (nome, e))
    try:
        livelli.save_current_level()
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a salvare il livello (%s)" % e)


# ====================================================================================== 3. il ponte sull'Aure
FILE_PONTE = CARTELLA + "/Montalto_ponte.json"
DEST_PONTE = "/Game/Montalto/Ponte"
ETICHETTA_PONTE = "MontaltoPonte"
MATERIALI_VILLAGGIO = "/Game/Valle/Villaggio/Materiali"
# slot del modello -> materiali da provare, in ordine (il primo che esiste)
SLOT_PONTE = (("pavimento_legno", ("MI_Interno_PavimentoLegno", "MI_Villaggio_Assi")),
              ("pavimento_pietra", ("MI_Interno_PavimentoPietra", "MI_Villaggio_Selciato", "MI_Villaggio_Pietra")),
              ("paglia", ("MI_Villaggio_Paglia",)),
              ("intonaco_dentro", ("MI_Interno_Intonaco", "MI_Villaggio_Intonaco")),
              ("intonaco", ("MI_Villaggio_Intonaco",)),
              ("pietra", ("MI_Mura_PietraCastello", "MI_Villaggio_Pietra")),
              ("legno", ("MI_Villaggio_Trave",)), ("ferro", ("MI_Villaggio_Ferro",)), ("coppi", ("MI_Villaggio_Coppi",)))


def modello_importato(nome, dest=None):
    for percorso in libreria.list_assets(dest or DEST_PONTE, recursive=True, include_folder=False):
        if percorso.split(".")[0].rsplit("/", 1)[-1] == nome:
            x = unreal.load_asset(percorso)
            if isinstance(x, unreal.StaticMesh):
                return x
    return None


TERRA_MI = [None, False]          # il materiale della terra smossa, e se si e' gia' provato a farlo


def materiale_terra():
    """(25/11) MI_Montalto_TerraSmossa per lo slot «terra» (tumuli e fossa del cimitero): la texture della chat 3D in
    V:/Texture/valdorso/montalto/texture (1 m per riquadro), sullo stesso materiale di base della pietra del villaggio."""
    if TERRA_MI[1]:
        return TERRA_MI[0]
    TERRA_MI[1] = True
    dest = "/Game/Montalto/Materiali"
    try:
        base = unreal.load_asset(MATERIALI_VILLAGGIO + "/MI_Villaggio_Pietra") \
            if libreria.does_asset_exist(MATERIALI_VILLAGGIO + "/MI_Villaggio_Pietra") else None
        file_c, file_n = CARTELLA + "/texture/T_Terra_Smossa_C.png", CARTELLA + "/texture/T_Terra_Smossa_N.png"
        if base is None or not os.path.exists(file_c) or not os.path.exists(file_n):
            avviso("terra smossa: manca MI_Villaggio_Pietra o la texture in %s/texture: la terra tiene il colore del modello"
                   % CARTELLA)
            return None
        compiti = []
        for f in (file_c, file_n):
            c = unreal.AssetImportTask()
            c.set_editor_property("filename", f)
            c.set_editor_property("destination_path", dest)
            c.set_editor_property("automated", True)
            c.set_editor_property("replace_existing", True)
            c.set_editor_property("save", True)
            compiti.append(c)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(compiti)
        tc = unreal.load_asset(dest + "/T_Terra_Smossa_C") if libreria.does_asset_exist(dest + "/T_Terra_Smossa_C") else None
        tn = unreal.load_asset(dest + "/T_Terra_Smossa_N") if libreria.does_asset_exist(dest + "/T_Terra_Smossa_N") else None
        if not isinstance(tc, unreal.Texture2D) or not isinstance(tn, unreal.Texture2D):
            avviso("la texture della terra smossa non e' arrivata in %s: la terra tiene il colore del modello" % dest)
            return None
        tn.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tn.set_editor_property("srgb", False)
        libreria.save_loaded_asset(tn)
        mel = unreal.MaterialEditingLibrary
        percorso = dest + "/MI_Montalto_TerraSmossa"
        mi = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
        if mi is None:
            mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                "MI_Montalto_TerraSmossa", dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, base)
        mel.set_material_instance_texture_parameter_value(mi, "Colore", tc)
        mel.set_material_instance_texture_parameter_value(mi, "Rilievo", tn)
        mel.set_material_instance_scalar_parameter_value(mi, "Scala", 1.0)
        mel.set_material_instance_scalar_parameter_value(mi, "Ruvidezza", 0.91)
        mel.set_material_instance_vector_parameter_value(mi, "Tinta", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        mel.update_material_instance(mi)
        libreria.save_loaded_asset(mi)
        TERRA_MI[0] = mi
        scrivi("materiale della terra smossa fatto: " + percorso)
    except Exception as e:  # noqa: BLE001
        avviso("materiale della terra smossa non fatto (%s): la terra tiene il colore del modello" % e)
    return TERRA_MI[0]


def vesti_modello(mesh):
    for i, sm in enumerate(mesh.get_editor_property("static_materials")):
        nome_slot = str(sm.get_editor_property("material_slot_name")).lower()
        nome = nome_slot
        mi = sm.get_editor_property("material_interface")
        if mi is not None:
            nome += " " + mi.get_name().lower()
        if nome_slot == "terra" or nome_slot.startswith("terra"):
            terra = materiale_terra()
            if terra is not None:
                mesh.set_material(i, terra)
            continue
        fatto = False
        for chiave, candidati in SLOT_PONTE:
            if chiave not in nome:
                continue
            for c in candidati:
                if libreria.does_asset_exist(MATERIALI_VILLAGGIO + "/" + c):
                    mesh.set_material(i, unreal.load_asset(MATERIALI_VILLAGGIO + "/" + c))
                    fatto = True
                    break
            break
        if not fatto and not any(c in nome for c in ("azzurro", "oro", "bronzo", "corda", "vetro", "terra", "luce")):
            avviso("%s: lo slot «%s» resta con il suo colore" % (mesh.get_name(), nome))
    try:
        bs = mesh.get_editor_property("body_setup")
        if bs is not None:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            mesh.set_editor_property("body_setup", bs)
    except Exception as e:  # noqa: BLE001
        avviso("%s: collisione non impostata (%s)" % (mesh.get_name(), e))
    libreria.save_loaded_asset(mesh)


LUCE_COLORE = (1.0, 0.62, 0.32)       # come le lanterne di Valdorso


def luce_calda(dove, nome, cartella, etichetta, intensita=3.0, raggio=450.0):
    """(26/11) Una luce calda senza ombre (costa poco), per bracieri, candele e lanterne dei pezzi della chat 3D."""
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
    l.set_folder_path(cartella)
    l.tags = [unreal.Name(etichetta)]
    return l


def ponte():
    import math
    aperto = nome_del_livello_aperto()
    if aperto != NOME_LIVELLO:
        avviso("il livello aperto e' «%s», non L_Montalto: non tocco niente." % aperto)
        return
    if not paesaggi_del_livello():
        avviso("in L_Montalto non c'e' il Paesaggio: prima i passi 1 e 2")
        return
    if not terreno_aggiornato("il ponte"):
        return
    dati = leggi_json(FILE_PONTE)
    luoghi = leggi_json(FILE_LUOGHI)
    if dati is None or luoghi is None:
        return
    if "ponte" not in luoghi:
        avviso("Luoghi_%d.json e' quello vecchio (senza il posto del ponte): serve il terreno del 25/11, reimportato" % LATO)
        return
    nomi = sorted(set(v["pezzo"] for v in dati["montaggio_cm"]) | {dati["catene_di_fuori"]["pezzo"]})
    compiti = []
    for n in nomi:
        f = "%s/glb/%s.glb" % (CARTELLA, n)
        if not os.path.exists(f):
            avviso("manca il file " + f)
            continue
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", DEST_PONTE)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", True)
        compiti.append(c)
    scrivi("importo %d pezzi del ponte..." % len(compiti))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(compiti)
    pezzi = {}
    for n in nomi:
        m = modello_importato(n)
        if m is None:
            avviso("pezzo non importato: " + n)
            continue
        vesti_modello(m)
        pezzi[n] = m
    scrivi("pezzi pronti: %d su %d" % (len(pezzi), len(nomi)))
    if len(pezzi) < len(nomi):
        avviso("mancano dei pezzi: non monto il ponte a meta'")
        return

    # Unreal ha specchiato la Y dei modelli? (secondo controllo) L'ala del levatoio va da y -13 a +998 (parte dal perno):
    # se arriva da -998 a +13, si'. La testa dice se il «su» e' giusto (z da -2500 a 1200).
    b = pezzi["SM_Montalto_Ponte_Levatoio"].get_bounding_box()
    bt = pezzi["SM_Montalto_Ponte_Testa"].get_bounding_box()
    scrivi("prova assi: ala y da %.0f a %.0f (attesi -13 e 998); testa z da %.0f a %.0f (attesi -2500 e 1200)"
           % (b.min.y, b.max.y, bt.min.z, bt.max.z))
    su_giusto = bt.max.z > 1000 and bt.min.z < -2000
    if b.max.y > 900 and b.min.y > -100 and su_giusto:
        sy = 1.0
    elif b.min.y < -900 and b.max.y < 100 and su_giusto:
        sy = -1.0
        scrivi("Unreal ha specchiato la Y: la correggo con la scala Y = -1, come per le mura")
    else:
        sy = 1.0
        avviso("orientamento dei modelli strano: guardare il ponte a occhio e dirlo a Claude")

    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()
    lp = luoghi["ponte"]
    cx, cy = angolo.x + lp["centro_unreal_cm"][0], angolo.y + lp["centro_unreal_cm"][1]
    z0 = angolo.z + (lp["pelo_m"] * 100.0 / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
    # il sistema del ponte: la sua Y (da «sud» a «nord» nei dati) e' l'asse da ovest a est; yaw da X verso Y
    ax, ay = lp["asse_unreal"]
    giro = math.degrees(math.atan2(ay, ax)) - 90.0
    ca, sa = math.cos(math.radians(giro)), math.sin(math.radians(giro))

    def mondo(p):
        return unreal.Vector(cx + p[0] * ca - p[1] * sa, cy + p[0] * sa + p[1] * ca, z0 + p[2])

    for a in attori.get_all_level_actors():
        if ETICHETTA_PONTE in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    messi = 0
    for i, v in enumerate(dati["montaggio_cm"]):
        a = attori.spawn_actor_from_object(pezzi[v["pezzo"]], mondo(v["pos"]),
                                           unreal.Rotator(roll=0.0, pitch=0.0, yaw=giro + v["yaw"]))
        if a is None:
            avviso("non riesco a mettere " + v["cosa"])
            continue
        if sy < 0:
            a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
        a.set_actor_label("Ponte %02d - %s" % (i + 1, v["cosa"]))
        a.set_folder_path("Montalto/Ponte")
        a.tags = [unreal.Name(ETICHETTA_PONTE)]
        messi += 1
    scrivi("ponte: %d pezzi su %d, centro a %.0f %.0f, strada a quota %.0f cm, girato di %.1f gradi"
           % (messi, len(dati["montaggio_cm"]), cx, cy, z0 + 1200.0, giro))

    # le 4 catene di fuori, con il levatoio chiuso: dal buco della torre all'occhiello sulla punta dell'ala
    catena = pezzi[dati["catene_di_fuori"]["pezzo"]]
    catene = 0
    # (secondo controllo) nei dati le catene partono da Y ∓1220, ma il buco sulla facciata della torre e' a ∓1120
    # (torre a ∓1400, facciata a 280): parto dal buco vero, altrimenti la catena buca il muro. Da dire alla chat 3D.
    facciata = float(dati["pezzi"]["SM_Montalto_Ponte_Torre"].get("facciata_y_cm", 280))
    for buco in dati["catene_di_fuori"]["dal_buco_cm"]:
        segno = -1.0 if buco[1] < 0 else 1.0
        buco = [buco[0], segno * (1400.0 - facciata), buco[2]]
        occhiello = [buco[0], segno * 60.0, 1212.0]
        da, a_ = mondo(buco), mondo(occhiello)
        lungo = unreal.Vector(a_.x - da.x, a_.y - da.y, a_.z - da.z)
        misura = math.sqrt(lungo.x ** 2 + lungo.y ** 2 + lungo.z ** 2)
        try:
            rot = unreal.MathLibrary.make_rot_from_y(lungo)
        except Exception as e:  # noqa: BLE001
            avviso("catene: non riesco a orientarle (%s): le salto" % e)
            break
        # tanti pezzi da un metro (appena stirati per arrivare giusti), non un pezzo solo stirato 18 volte
        anelli = max(int(round(misura / 100.0)), 1)
        passo_c = misura / anelli
        for k in range(anelli):
            posto = unreal.Vector(da.x + lungo.x * k / anelli, da.y + lungo.y * k / anelli, da.z + lungo.z * k / anelli)
            c = attori.spawn_actor_from_object(catena, posto, rot)
            if c is None:
                continue
            c.set_actor_scale3d(unreal.Vector(1.0, passo_c / 100.0, 1.0))
            c.set_actor_label("Ponte - catena %d.%02d" % (catene + 1, k + 1))
            c.set_folder_path("Montalto/Ponte/Catene")
            c.tags = [unreal.Name(ETICHETTA_PONTE)]
        catene += 1
    scrivi("catene: %d su 4 (circa 18 m l'una con il levatoio chiuso, a pezzi da un metro)" % catene)
    scrivi("Da guardare: le teste a filo con le due rive e con la strada; le pile fino al fondo; le ali che si toccano in "
           "mezzo; le torrette in diagonale. Il levatoio e' fermo (chiuso): leva e argano arrivano con il C++.")
    try:
        livelli.save_current_level()
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a salvare il livello (%s)" % e)


# ====================================================================================== 4. la rocca del duca
FILE_ROCCA = CARTELLA + "/Montalto_rocca.json"
DEST_ROCCA = "/Game/Montalto/Rocca"
ETICHETTA_ROCCA = "MontaltoRocca"


def trova_modello_per_nome(nome):
    """Un modello gia' nel progetto (la grata del passo), cercato per nome in tutto /Game."""
    try:
        registro = unreal.AssetRegistryHelpers.get_asset_registry()
        for d in registro.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")):
            if str(d.asset_name) == nome and str(d.package_path).startswith("/Game/"):
                x = d.get_asset()
                if isinstance(x, unreal.StaticMesh):
                    return x
    except Exception as e:  # noqa: BLE001
        avviso("ricerca di %s non riuscita (%s)" % (nome, e))
    return None


def rocca():
    import math
    aperto = nome_del_livello_aperto()
    if aperto != NOME_LIVELLO:
        avviso("il livello aperto e' «%s», non L_Montalto: non tocco niente." % aperto)
        return
    if not paesaggi_del_livello():
        avviso("in L_Montalto non c'e' il Paesaggio: prima i passi 1 e 2")
        return
    if not terreno_aggiornato("la rocca"):
        return
    dati = leggi_json(FILE_ROCCA)
    luoghi = leggi_json(FILE_LUOGHI)
    if dati is None or luoghi is None:
        return
    if "rocca" not in luoghi:
        avviso("Luoghi_%d.json e' quello vecchio (senza il posto della rocca): serve il terreno nuovo, reimportato" % LATO)
        return
    # (26/11) gli interni: gli alloggi delle guardie e l'appartamento del duca nel mastio (Montalto_rocca_interni.json,
    # stesso sistema della rocca). Si aggiungono al montaggio solo se il json e tutti i suoi modelli ci sono.
    interni = leggi_json(CARTELLA + "/Montalto_rocca_interni.json") if os.path.exists(CARTELLA + "/Montalto_rocca_interni.json") else None
    montaggio_interni = []
    if interni is not None:
        montaggio_interni = list(interni.get("montaggio_cm", []))
        if any(not os.path.exists("%s/glb/%s.glb" % (CARTELLA, v["pezzo"])) for v in montaggio_interni):
            avviso("interni della rocca: manca qualche modello in glb/, li salto")
            montaggio_interni = []
    luci_dei_pezzi = dict((n, (d_ or {}).get("luci_cm", [])) for n, d_ in ((interni or {}).get("pezzi", {})).items())
    dati["montaggio_cm"] = list(dati["montaggio_cm"]) + montaggio_interni
    nostri = sorted(set(v["pezzo"] for v in dati["montaggio_cm"] if v["pezzo"].startswith("SM_Montalto_")))
    compiti = []
    for n in nostri:
        f = "%s/glb/%s.glb" % (CARTELLA, n)
        if not os.path.exists(f):
            avviso("manca il file " + f)
            continue
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", DEST_ROCCA)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", True)
        compiti.append(c)
    scrivi("importo %d pezzi della rocca..." % len(compiti))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(compiti)
    pezzi = {}
    for n in nostri:
        m = modello_importato(n, DEST_ROCCA)
        if m is None:
            avviso("pezzo non importato: " + n)
            continue
        vesti_modello(m)
        pezzi[n] = m
    scrivi("pezzi pronti: %d su %d" % (len(pezzi), len(nostri)))
    if len(pezzi) < len(nostri):
        avviso("mancano dei pezzi: non monto la rocca a meta'")
        return
    # i pezzi che sono gia' del progetto (la grata del passo)
    for v in dati["montaggio_cm"]:
        if v["pezzo"] not in pezzi:
            m = trova_modello_per_nome(v["pezzo"])
            if m is None:
                avviso("non trovo nel progetto %s (%s): quel pezzo non lo metto" % (v["pezzo"], v["cosa"]))
            else:
                pezzi[v["pezzo"]] = m

    # la Y specchiata: la cappella va da y -720 a +971 (l'abside e' a nord)
    b = pezzi["SM_Montalto_Rocca_Cappella"].get_bounding_box()
    scrivi("prova assi sulla cappella: y da %.0f a %.0f (attesi -720 e 971), z fino a %.0f (atteso 1278)"
           % (b.min.y, b.max.y, b.max.z))
    if b.max.y > 900 and b.min.y > -800 and b.max.z > 1100:
        sy = 1.0
    elif b.min.y < -900 and b.max.y < 800 and b.max.z > 1100:
        sy = -1.0
        scrivi("Unreal ha specchiato la Y: la correggo con la scala Y = -1, come per le mura")
    else:
        sy = 1.0
        avviso("orientamento dei modelli strano: guardare la rocca a occhio e dirlo a Claude")

    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()
    lr = luoghi["rocca"]
    cx, cy = angolo.x + lr["centro_unreal_cm"][0], angolo.y + lr["centro_unreal_cm"][1]
    z0 = angolo.z + (lr["cortile_m"] * 100.0 / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
    giro = float(lr["giro_gradi"])
    ca, sa = math.cos(math.radians(giro)), math.sin(math.radians(giro))

    def mondo(p):
        return unreal.Vector(cx + p[0] * ca - p[1] * sa, cy + p[0] * sa + p[1] * ca, z0 + p[2])

    for a in attori.get_all_level_actors():
        if ETICHETTA_ROCCA in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    messi = 0
    luci_messe = 0
    for i, v in enumerate(dati["montaggio_cm"]):
        if v["pezzo"] not in pezzi:
            continue
        try:
            a = attori.spawn_actor_from_object(pezzi[v["pezzo"]], mondo(v["pos"]),
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=giro + v["yaw"]))
            if a is None:
                avviso("non riesco a mettere " + v["cosa"])
                continue
            if sy < 0:
                a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
            a.set_actor_label(("Rocca %02d - %s" % (i + 1, v["cosa"]))[:90])
            a.set_folder_path("Montalto/Rocca")
            a.tags = [unreal.Name(ETICHETTA_ROCCA)]
            messi += 1
            g2 = math.radians(giro + v["yaw"])
            base = mondo(v["pos"])
            for j, lu in enumerate(luci_dei_pezzi.get(v["pezzo"], [])):
                q = lu["pos"]
                fuoco = any(w in str(lu.get("tipo", "")) for w in ("camino", "fuoco"))
                if luce_calda(unreal.Vector(base.x + q[0] * math.cos(g2) - q[1] * math.sin(g2),
                                            base.y + q[0] * math.sin(g2) + q[1] * math.cos(g2), base.z + q[2]),
                              "Rocca %02d luce %d" % (i + 1, j + 1), "Montalto/Rocca/Luci", ETICHETTA_ROCCA,
                              5.0 if fuoco else 3.0, 600.0) is not None:
                    luci_messe += 1
        except Exception as e:  # noqa: BLE001
            avviso("%s: errore nel metterlo (%s)" % (v["cosa"], e))
    if messi < len(dati["montaggio_cm"]):
        avviso("la rocca NON e' completa: %d pezzi su %d" % (messi, len(dati["montaggio_cm"])))
    scrivi("rocca: %d pezzi su %d, centro a %.0f %.0f, cortile a quota %.0f cm, girata di %.0f gradi (la porta guarda a sud)"
           % (messi, len(dati["montaggio_cm"]), cx, cy, z0, giro))
    if montaggio_interni:
        scrivi("interni: gli alloggi delle guardie contro le mura a ovest del portone (armeria, cucina, refettorio, sopra "
               "la camerata) e l'appartamento del duca nel mastio (ospiti, sala grande, camera e cappella, famiglia); "
               "%d luci calde" % luci_messe)
    scrivi("Da guardare: le mura appoggiate sul piano tutto intorno (nessun angolo per aria o sepolto); la porta con la "
           "grata e la strada che ci arriva davanti; il mastio, il palazzo, la cappella, le scuderie e il pozzo nel "
           "cortile. La grata e' ferma (chiusa): la leva arriva con il C++.")
    try:
        livelli.save_current_level()
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a salvare il livello (%s)" % e)


# ====================================================================================== 5. il porto
FILE_PORTO = CARTELLA + "/Montalto_porto.json"
DEST_PORTO = "/Game/Montalto/Porto"
ETICHETTA_PORTO = "MontaltoPorto"


def porto():
    """La «riva tipo» della chat 3D sulla riva della citta', a valle del ponte: banchina, pontile, gru, tre magazzini
    della lana, muro di sponda, scala, rampa dei carri, la chiatta e la nave ormeggiate."""
    import math
    aperto = nome_del_livello_aperto()
    if aperto != NOME_LIVELLO:
        avviso("il livello aperto e' «%s», non L_Montalto: non tocco niente." % aperto)
        return
    if not paesaggi_del_livello():
        avviso("in L_Montalto non c'e' il Paesaggio: prima i passi 1 e 2")
        return
    if not terreno_aggiornato("il porto"):
        return
    dati = leggi_json(FILE_PORTO)
    luoghi = leggi_json(FILE_LUOGHI)
    if dati is None or luoghi is None:
        return
    if "porto" not in luoghi or any(k not in luoghi["porto"] for k in ("origine_unreal_cm", "giro_gradi", "banchina_m")):
        avviso("Luoghi_%d.json e' quello vecchio (senza il posto del porto): serve il terreno nuovo, reimportato" % LATO)
        return
    montaggio = list(dati["montaggio_riva_tipo_cm"])
    # (25/11) la chiusura in cima alla rampa: un muro davanti e tre sul fianco verso monte
    chiusura = dati.get("chiusura_in_cima_alla_rampa_cm", {}).get("pezzi", [])
    montaggio += chiusura
    scrivi("riva tipo: %d pezzi; chiusura in cima alla rampa: %d pezzi" % (len(montaggio) - len(chiusura), len(chiusura)))
    if not chiusura:
        avviso("Montalto_porto.json e' quello vecchio (senza la chiusura in cima alla rampa): li' restera' una scarpata")
    nomi = sorted(set(v["pezzo"] for v in montaggio))
    compiti = []
    for n in nomi:
        f = "%s/glb/%s.glb" % (CARTELLA, n)
        if not os.path.exists(f):
            avviso("manca il file " + f)
            continue
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", DEST_PORTO)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", True)
        compiti.append(c)
    scrivi("importo %d pezzi del porto..." % len(compiti))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(compiti)
    pezzi = {}
    for n in nomi:
        m = modello_importato(n, DEST_PORTO)
        if m is None:
            avviso("pezzo non importato: " + n)
            continue
        vesti_modello(m)
        pezzi[n] = m
    scrivi("pezzi pronti: %d su %d" % (len(pezzi), len(nomi)))
    if len(pezzi) < len(nomi):
        avviso("mancano dei pezzi: non monto il porto a meta'")
        return

    # la Y specchiata: la gru va da y -832 (il braccio sul fiume) a +600
    b = pezzi["SM_Montalto_Porto_Gru"].get_bounding_box()
    scrivi("prova assi sulla gru: y da %.0f a %.0f (attesi -832 e 600), z fino a %.0f (atteso 1085)"
           % (b.min.y, b.max.y, b.max.z))
    if b.min.y < -750 and b.max.y < 700 and b.max.z > 900:
        sy = 1.0
    elif b.max.y > 750 and b.min.y > -700 and b.max.z > 900:
        sy = -1.0
        scrivi("Unreal ha specchiato la Y: la correggo con la scala Y = -1, come per le mura")
    else:
        avviso("orientamento dei modelli strano: non monto il porto. Dillo a Claude.")
        return

    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()
    lp = luoghi["porto"]
    cx, cy = angolo.x + lp["origine_unreal_cm"][0], angolo.y + lp["origine_unreal_cm"][1]
    z0 = angolo.z + (lp["banchina_m"] * 100.0 / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
    giro = float(lp["giro_gradi"])
    ca, sa = math.cos(math.radians(giro)), math.sin(math.radians(giro))

    def mondo(p):
        return unreal.Vector(cx + p[0] * ca - p[1] * sa, cy + p[0] * sa + p[1] * ca, z0 + p[2])

    for a in attori.get_all_level_actors():
        if ETICHETTA_PORTO in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    messi = 0
    for i, v in enumerate(montaggio):
        try:
            a = attori.spawn_actor_from_object(pezzi[v["pezzo"]], mondo(v["pos"]),
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=giro + v["yaw"]))
            if a is None:
                avviso("non riesco a mettere " + v["cosa"])
                continue
            if sy < 0:
                a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
            a.set_actor_label("Porto %02d - %s" % (i + 1, v["cosa"][:60]))
            a.set_folder_path("Montalto/Porto")
            a.tags = [unreal.Name(ETICHETTA_PORTO)]
            messi += 1
        except Exception as e:  # noqa: BLE001
            avviso("%s: errore nel metterlo (%s)" % (v["cosa"], e))
    if messi < len(montaggio):
        avviso("il porto NON e' completo: %d pezzi su %d" % (messi, len(montaggio)))
    scrivi("porto: %d pezzi su %d, origine a %.0f %.0f, banchina a quota %.0f cm, girato di %.1f gradi"
           % (messi, len(montaggio), cx, cy, z0, giro))
    scrivi("Da guardare: l'acqua che arriva alla banchina (senza terra in mezzo); il piazzale in piano tra banchina e muro; "
           "il muro di sponda che tiene la citta' (niente terra che esce dal muro); la rampa e la scala che arrivano in "
           "citta'; le due barche che galleggiano; i magazzini con il retro contro il muro.")
    try:
        livelli.save_current_level()
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a salvare il livello (%s)" % e)


# ====================================================================================== 6. il Granaio della Corona
def monta_semplice(titolo, file_dati, blocco, quota, dest, etichetta, cartella, prova):
    """Importa i pezzi di un json della chat 3D (montaggio_cm) e li monta nel posto scritto in Luoghi (blocco: centro,
    giro, quota). prova = (pezzo, funzione(bounding box) -> +1 dritto, -1 specchiato, 0 strano)."""
    import math
    aperto = nome_del_livello_aperto()
    if aperto != NOME_LIVELLO:
        avviso("il livello aperto e' «%s», non L_Montalto: non tocco niente." % aperto)
        return
    if not paesaggi_del_livello():
        avviso("in L_Montalto non c'e' il Paesaggio: prima i passi 1 e 2")
        return
    if not terreno_aggiornato(titolo):
        return
    dati = leggi_json(file_dati)
    luoghi = leggi_json(FILE_LUOGHI)
    if dati is None or luoghi is None:
        return
    lb = luoghi.get(blocco)
    if lb is None or any(k not in lb for k in ("centro_unreal_cm", "giro_gradi", quota)):
        avviso("Luoghi_%d.json e' quello vecchio (senza il posto per %s): serve il terreno nuovo, reimportato" % (LATO, titolo))
        return
    montaggio = dati["montaggio_cm"]
    nomi = sorted(set(v["pezzo"] for v in montaggio))
    compiti = []
    for n in nomi:
        f = "%s/glb/%s.glb" % (CARTELLA, n)
        if not os.path.exists(f):
            avviso("manca il file " + f)
            continue
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", dest)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", True)
        compiti.append(c)
    scrivi("%s: importo %d pezzi..." % (titolo, len(compiti)))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(compiti)
    pezzi = {}
    for n in nomi:
        m = modello_importato(n, dest)
        if m is None:
            avviso("pezzo non importato: " + n)
            continue
        vesti_modello(m)
        pezzi[n] = m
    if len(pezzi) < len(nomi):
        avviso("%s: mancano dei pezzi, non monto a meta'" % titolo)
        return
    b = pezzi[prova[0]].get_bounding_box()
    scrivi("prova assi su %s: y da %.0f a %.0f, z fino a %.0f" % (prova[0], b.min.y, b.max.y, b.max.z))
    verso = prova[1](b)
    if verso == 0:
        avviso("%s: orientamento dei modelli strano, non monto. Dillo a Claude." % titolo)
        return
    sy = float(verso)
    if sy < 0:
        scrivi("Unreal ha specchiato la Y: la correggo con la scala Y = -1, come per le mura")
    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()
    cx, cy = angolo.x + lb["centro_unreal_cm"][0], angolo.y + lb["centro_unreal_cm"][1]
    z0 = angolo.z + (lb[quota] * 100.0 / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
    giro = float(lb["giro_gradi"])
    ca, sa = math.cos(math.radians(giro)), math.sin(math.radians(giro))
    for a in attori.get_all_level_actors():
        if etichetta in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    messi = 0
    luci_messe = 0
    for i, v in enumerate(montaggio):
        try:
            p = v["pos"]
            dove = unreal.Vector(cx + p[0] * ca - p[1] * sa, cy + p[0] * sa + p[1] * ca, z0 + p[2])
            a = attori.spawn_actor_from_object(pezzi[v["pezzo"]], dove,
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=giro + v["yaw"]))
            if a is None:
                avviso("non riesco a mettere " + v["pezzo"])
                continue
            if sy < 0:
                a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
            a.set_actor_label("%s %02d - %s" % (titolo, i + 1, v.get("cosa", v["pezzo"])[:60]))
            a.set_folder_path(cartella)
            a.tags = [unreal.Name(etichetta)]
            messi += 1
            # (26/11) le luci che il json da' per quel pezzo (le candele della cappella), nel sistema del pezzo
            g2 = math.radians(giro + v["yaw"])
            for j, lu in enumerate((dati.get("pezzi", {}).get(v["pezzo"], {}) or {}).get("luci_cm", [])):
                try:
                    q = lu["pos"]
                    debole = "candela" in str(lu.get("tipo", ""))
                    luce_calda(unreal.Vector(dove.x + q[0] * math.cos(g2) - q[1] * math.sin(g2),
                                             dove.y + q[0] * math.sin(g2) + q[1] * math.cos(g2), dove.z + q[2]),
                               "%s luce %02d-%d" % (titolo, i + 1, j + 1), cartella + "/Luci", etichetta,
                               1.2 if debole else 2.0, 250.0 if debole else 500.0)
                    luci_messe += 1
                except Exception as e:  # noqa: BLE001
                    avviso("%s: una luce non messa (%s)" % (v["pezzo"], e))
        except Exception as e:  # noqa: BLE001
            avviso("%s: errore nel metterlo (%s)" % (v["pezzo"], e))
    if messi < len(montaggio):
        avviso("%s NON e' completo: %d pezzi su %d" % (titolo, messi, len(montaggio)))
    if luci_messe:
        scrivi("%s: %d luci calde (candele e lanterne)" % (titolo, luci_messe))
    scrivi("%s: %d pezzi su %d, centro a %.0f %.0f, suolo a quota %.0f cm, girato di %.0f gradi"
           % (titolo, messi, len(montaggio), cx, cy, z0, giro))
    try:
        livelli.save_current_level()
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a salvare il livello (%s)" % e)


def granaio():
    # il granaio va da y -940 (il fronte) a +860: se arriva da -860 a +940 Unreal l'ha specchiato
    def verso(b):
        if b.max.z < 1600:
            return 0
        if b.min.y < -900 and b.max.y < 900:
            return 1
        if b.max.y > 900 and b.min.y > -900:
            return -1
        return 0
    monta_semplice("Granaio", CARTELLA + "/Montalto_granaio.json", "granaio", "suolo_m", "/Game/Montalto/Granaio",
                   "MontaltoGranaio", "Montalto/Granaio", ("SM_Montalto_Granaio", verso))
    scrivi("Da guardare: il granaio in piano nella citta' bassa, il piano di carico e la rampa dei carri sul fronte (a sud), "
           "lo stemma della Corona; dentro, i due piani di legno con i sacchi e i mucchi di grano.")


# ====================================================================================== 9. il cimitero vecchio
def cimitero():
    # la fossa rialzata (Tomba_11_Rialzata) va da y -95 a +280: se arriva da -280 a +95 Unreal l'ha specchiata
    def verso(b):
        if b.max.z < 120:
            return 0
        if b.max.y > 230 and b.min.y > -150:
            return 1
        if b.min.y < -230 and b.max.y < 150:
            return -1
        return 0
    monta_semplice("Cimitero", CARTELLA + "/Montalto_cimitero.json", "cimitero", "suolo_m", "/Game/Montalto/Cimitero",
                   "MontaltoCimitero", "Montalto/Cimitero", ("SM_Montalto_Cimitero_Tomba_11_Rialzata", verso))
    scrivi("Da guardare: il recinto in piano di fianco alla Strada del nord, il cancello verso la strada, la cappella in "
           "fondo al viale, le tombe in fila con i tumuli di terra smossa, la fossa aperta con il bordo di terra, la pala "
           "e la lapide caduta.")


# ====================================================================================== 7. le case di citta'
FILE_CASE = CARTELLA + "/Case_%d.json" % LATO
DEST_CASE = "/Game/Montalto/Case"
ETICHETTA_CASE = "MontaltoCase"


def specchio_gia_noto():
    """Le case sono simmetriche e da sole non dicono se Unreal ha specchiato la Y: lo si legge da un pezzo gia' importato
    (l'ala del levatoio, y da -13 a 998; oppure il granaio, y da -940 a 860). +1 dritto, -1 specchiato, 0 non si sa."""
    m = modello_importato("SM_Montalto_Ponte_Levatoio", DEST_PONTE)
    if m is not None:
        b = m.get_bounding_box()
        if b.max.y > 900 and b.min.y > -100:
            return 1
        if b.min.y < -900 and b.max.y < 100:
            return -1
    m = modello_importato("SM_Montalto_Granaio", "/Game/Montalto/Granaio")
    if m is not None:
        b = m.get_bounding_box()
        if b.min.y < -900 and b.max.y < 900:
            return 1
        if b.max.y > 900 and b.min.y > -900:
            return -1
    return 0


# (25/11 sera, dalle foto) le prime tinte si distinguevano appena: ora sono piu' decise
TINTE_CASE = ((1.00, 1.00, 1.00), (0.92, 0.78, 0.52), (0.88, 0.62, 0.50), (0.74, 0.75, 0.72), (0.98, 0.90, 0.66),
              (0.68, 0.57, 0.44))        # bianco di calce, ocra, rosa mattone, grigio, paglierino, terra


def intonaci_delle_case():
    """Sei intonaci di colore diverso (figli di MI_Villaggio_Intonaco) per non avere le case tutte uguali."""
    base = unreal.load_asset(MATERIALI_VILLAGGIO + "/MI_Villaggio_Intonaco") \
        if libreria.does_asset_exist(MATERIALI_VILLAGGIO + "/MI_Villaggio_Intonaco") else None
    if base is None:
        avviso("manca MI_Villaggio_Intonaco: le case restano tutte dello stesso colore")
        return []
    mel = unreal.MaterialEditingLibrary
    dest = "/Game/Montalto/Materiali"
    fatti = []
    for i, t in enumerate(TINTE_CASE):
        try:
            percorso = "%s/MI_Montalto_Intonaco_%d" % (dest, i)
            mi = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
            if mi is None:
                mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                    "MI_Montalto_Intonaco_%d" % i, dest, unreal.MaterialInstanceConstant,
                    unreal.MaterialInstanceConstantFactoryNew())
            mel.set_material_instance_parent(mi, base)
            mel.set_material_instance_vector_parameter_value(mi, "Tinta", unreal.LinearColor(t[0], t[1], t[2], 1.0))
            mel.update_material_instance(mi)
            libreria.save_loaded_asset(mi)
            fatti.append(mi)
        except Exception as e:  # noqa: BLE001
            avviso("intonaco %d non fatto (%s)" % (i, e))
            return []
    return fatti


def case():
    aperto = nome_del_livello_aperto()
    if aperto != NOME_LIVELLO:
        avviso("il livello aperto e' «%s», non L_Montalto: non tocco niente." % aperto)
        return
    if not paesaggi_del_livello():
        avviso("in L_Montalto non c'e' il Paesaggio: prima i passi 1 e 2")
        return
    if not terreno_aggiornato("le case"):
        return
    piano = leggi_json(FILE_CASE)
    if piano is None:
        return
    lista = piano.get("case", [])
    if not lista:
        avviso("Case_%d.json non ha case: non tocco niente" % LATO)
        return
    verso = specchio_gia_noto()
    if verso == 0:
        avviso("per le case devo prima sapere come Unreal importa i modelli: lancia prima «Montalto: 3» (il ponte) "
               "o «Montalto: 6» (il granaio), poi di nuovo questa voce")
        return
    # (26/11) le sedi delle gilde nei lotti: si importano con le case, se i modelli ci sono tutti
    sedi = [v for v in piano.get("lotti_gilde", []) if v.get("sede")]
    if any(not os.path.exists("%s/glb/%s.glb" % (CARTELLA, v["sede"])) for v in sedi):
        avviso("sedi delle gilde: manca qualche modello in glb/, i lotti restano vuoti")
        sedi = []
    nomi = sorted(set([v["pezzo"] for v in lista] + [v["sede"] for v in sedi]))
    compiti = []
    for n in nomi:
        f = "%s/glb/%s.glb" % (CARTELLA, n)
        if not os.path.exists(f):
            avviso("manca il file " + f)
            continue
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", DEST_CASE)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", True)
        compiti.append(c)
    scrivi("case: importo %d modelli..." % len(compiti))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(compiti)
    pezzi = {}
    for n in nomi:
        m = modello_importato(n, DEST_CASE)
        if m is None:
            avviso("modello non importato: " + n)
            continue
        vesti_modello(m)
        pezzi[n] = m
    if len(pezzi) < len(nomi):
        avviso("case: mancano dei modelli, non le metto a meta'")
        return
    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()
    intonaci = intonaci_delle_case()
    # in quale slot di ogni modello sta l'intonaco
    slot_intonaco = {}
    for n, m in pezzi.items():
        slot_intonaco[n] = []
        for i, sm in enumerate(m.get_editor_property("static_materials")):
            mat = sm.get_editor_property("material_interface")
            if "intonaco" in str(sm.get_editor_property("material_slot_name")).lower() \
                    or (mat is not None and "intonaco" in mat.get_name().lower()):
                slot_intonaco[n].append(i)
        if intonaci and not slot_intonaco[n]:
            avviso("%s: non trovo lo slot dell'intonaco, queste case restano del colore di prima" % n)
    for a in attori.get_all_level_actors():
        if ETICHETTA_CASE in [str(t) for t in a.tags] or "MontaltoLotto" in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    messe = 0
    with unreal.ScopedSlowTask(len(lista), "Montalto: le case di citta'") as lavoro:
        lavoro.make_dialog(True)
        for i, v in enumerate(lista):
            if lavoro.should_cancel():
                avviso("fermato a mano dopo %d case" % messe)
                break
            lavoro.enter_progress_frame(1)
            try:
                x, y, z = v["unreal_cm"]
                z_mondo = angolo.z + (z / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
                a = attori.spawn_actor_from_object(pezzi[v["pezzo"]], unreal.Vector(angolo.x + x, angolo.y + y, z_mondo),
                                                   unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(v["yaw"])))
                if a is None:
                    avviso("casa %d: non riesco a metterla" % (i + 1))
                    continue
                if verso < 0:
                    a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
                a.set_actor_label("Casa %03d %s" % (i + 1, v["pezzo"].rsplit("_", 1)[-1]))
                a.set_folder_path("Montalto/Case")
                a.tags = [unreal.Name(ETICHETTA_CASE)]
                if intonaci:
                    comp = a.static_mesh_component
                    for sl in slot_intonaco.get(v["pezzo"], []):
                        comp.set_material(sl, intonaci[int(v.get("tinta", 0)) % len(intonaci)])
                messe += 1
            except Exception as e:  # noqa: BLE001
                avviso("casa %d: errore (%s)" % (i + 1, e))
    scrivi("case di citta': %d su %d (%s)" % (messe, len(lista), ", ".join(
        "%s %d" % (k.rsplit("_", 1)[-1], n_) for k, n_ in sorted(piano.get("conti", {}).items()))))
    if messe < len(lista):
        avviso("le case NON sono tutte: %d su %d. Il livello NON e' stato salvato: rilancia questa voce." % (messe, len(lista)))
        return
    # i lotti lasciati liberi per le sedi delle gilde: una nota per ognuno (si vedono solo nell'editor)
    classe_nota = getattr(unreal, "Note", None) or unreal.TargetPoint
    lotti = 0
    sedi_messe = 0
    for v in piano.get("lotti_gilde", []):
        try:
            x, y, z = v["unreal_cm"]
            z_mondo = angolo.z + (z / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
            nota = attori.spawn_actor_from_class(classe_nota, unreal.Vector(angolo.x + x, angolo.y + y, z_mondo + 300.0),
                                                 unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(v["yaw"])))
            if nota is not None:
                nota.set_actor_label("%s (%d x %d m, su %s)" % (v["nome"], v["fronte_m"], v["fondo_m"], v["strada"]))
                nota.set_folder_path("Montalto/Lotti delle gilde")
                nota.tags = [unreal.Name("MontaltoLotto")]
            if v.get("sede") in pezzi and v in sedi:
                sede = attori.spawn_actor_from_object(pezzi[v["sede"]], unreal.Vector(angolo.x + x, angolo.y + y, z_mondo),
                                                      unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(v["yaw"])))
                if sede is not None:
                    if verso < 0:
                        sede.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
                    sede.set_actor_label("Sede di gilda %d - %s (su %s)"
                                         % (sedi_messe + 1, v["sede"].replace("SM_Montalto_Gilda_", ""), v["strada"]))
                    sede.set_folder_path("Montalto/Lotti delle gilde")
                    sede.tags = [unreal.Name("MontaltoLotto")]
                    sedi_messe += 1
            try:
                nota.set_editor_property("text", v["nome"])
            except Exception:  # noqa: BLE001
                pass
            lotti += 1
        except Exception as e:  # noqa: BLE001
            avviso("un lotto delle gilde non segnato (%s)" % e)
    scrivi("lotti per le gilde: %d, con %d sedi montate (nell'Outliner: cartella «Montalto/Lotti delle gilde»). Gli "
           "stendardi non ci sono ancora: cambiano per gilda." % (lotti, sedi_messe))
    scrivi("Da guardare: le file lungo la Strada del Re, la via bassa, la via alta e la salita; le porte alla quota della "
           "strada; nessuna casa in mezzo a una strada, nell'acqua o dentro un'altra; le case d'angolo in testa alle file.")
    try:
        livelli.save_current_level()
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a salvare il livello (%s)" % e)


# ====================================================================================== 8. le mura e la porta nord
FILE_MURA = CARTELLA + "/Mura_%d.json" % LATO
DEST_MURA = "/Game/Montalto/Mura"
ETICHETTA_MURA = "MontaltoMura"


def mura():
    """Il giro delle mura della citta' bassa (Mura_4000.json, fatto da montalto.py): cortine, torri, contrafforti sui
    giunti, i pezzi di riva lungo l'Aure, le torri di testata ai varchi e la Porta nord con la grata del passo."""
    aperto = nome_del_livello_aperto()
    if aperto != NOME_LIVELLO:
        avviso("il livello aperto e' «%s», non L_Montalto: non tocco niente." % aperto)
        return
    if not paesaggi_del_livello():
        avviso("in L_Montalto non c'e' il Paesaggio: prima i passi 1 e 2")
        return
    if not terreno_aggiornato("le mura"):
        return
    piano = leggi_json(FILE_MURA)
    if piano is None:
        return
    lista = piano.get("pezzi", [])
    if not lista:
        avviso("Mura_%d.json non ha pezzi: non tocco niente" % LATO)
        return
    nostri = sorted(set(v["pezzo"] for v in lista if v["pezzo"].startswith("SM_Montalto_")))
    ARREDO = "SM_Montalto_Mura_Torre_Arredo"      # (26/11) l'arredo da corpo di guardia: stesso posto e yaw della torre
    con_arredo = os.path.exists("%s/glb/%s.glb" % (CARTELLA, ARREDO))
    if con_arredo:
        nostri.append(ARREDO)
    luci_arredo = ((leggi_json(CARTELLA + "/Montalto_mura.json") or {}).get("pezzi", {}).get(ARREDO, {}) or {}).get("luci_cm", []) \
        if con_arredo else []
    compiti = []
    for n in nostri:
        f = "%s/glb/%s.glb" % (CARTELLA, n)
        if not os.path.exists(f):
            avviso("manca il file " + f)
            continue
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", DEST_MURA)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", True)
        compiti.append(c)
    scrivi("mura: importo %d modelli..." % len(compiti))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(compiti)
    pezzi = {}
    for n in nostri:
        m = modello_importato(n, DEST_MURA)
        if m is None:
            avviso("modello non importato: " + n)
            continue
        vesti_modello(m)
        pezzi[n] = m
    if len(pezzi) < len(nostri):
        avviso("mura: mancano dei modelli, non le monto a meta'")
        return
    # la Y specchiata: la porta nord va da y -860 (fuori) a +730 (dentro)
    b = pezzi["SM_Montalto_Porta_Nord"].get_bounding_box() if "SM_Montalto_Porta_Nord" in pezzi else None
    if b is not None and b.min.y < -800 and b.max.y < 800 and b.max.z > 1400:
        verso = 1
    elif b is not None and b.max.y > 800 and b.min.y > -800 and b.max.z > 1400:
        verso = -1
        scrivi("Unreal ha specchiato la Y: la correggo con la scala Y = -1, come per le mura di Valdorso")
    else:
        avviso("mura: orientamento dei modelli strano, non le monto. Dillo a Claude.")
        return
    mancanti = set()
    for v in lista:
        if v["pezzo"] not in pezzi and v["pezzo"] not in mancanti:
            m = trova_modello_per_nome(v["pezzo"])
            if m is None:
                mancanti.add(v["pezzo"])
                avviso("non trovo nel progetto %s: quel pezzo non lo metto" % v["pezzo"])
            else:
                pezzi[v["pezzo"]] = m
    paesaggio = paesaggi_del_livello()[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()
    for a in attori.get_all_level_actors():
        if ETICHETTA_MURA in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    import math
    messi = 0
    arredi = 0
    attesi = sum(1 for v in lista if v["pezzo"] in pezzi)
    with unreal.ScopedSlowTask(len(lista), "Montalto: le mura della citta'") as lavoro:
        lavoro.make_dialog(True)
        for i, v in enumerate(lista):
            if lavoro.should_cancel():
                avviso("fermato a mano dopo %d pezzi" % messi)
                break
            lavoro.enter_progress_frame(1)
            if v["pezzo"] not in pezzi:
                continue
            try:
                x, y, z = v["unreal_cm"]
                z_mondo = angolo.z + (z / QUOTA_MAX_CM * 65535.0 - 32768.0) * scala.z / 128.0
                a = attori.spawn_actor_from_object(pezzi[v["pezzo"]], unreal.Vector(angolo.x + x, angolo.y + y, z_mondo),
                                                   unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(v["yaw"])))
                if a is None:
                    avviso("pezzo %d (%s): non riesco a metterlo" % (i + 1, v["pezzo"]))
                    continue
                if verso < 0:
                    a.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
                corto = v["pezzo"].replace("SM_Montalto_", "")
                a.set_actor_label("Mura %03d %s" % (i + 1, corto))
                a.set_folder_path("Montalto/Mura/Contrafforti" if "Contrafforte" in corto else "Montalto/Mura")
                a.tags = [unreal.Name(ETICHETTA_MURA)]
                messi += 1
                if con_arredo and "Torre" in corto and ARREDO in pezzi:
                    ar = attori.spawn_actor_from_object(pezzi[ARREDO], unreal.Vector(angolo.x + x, angolo.y + y, z_mondo),
                                                        unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(v["yaw"])))
                    if ar is not None:
                        if verso < 0:
                            ar.set_actor_scale3d(unreal.Vector(1.0, -1.0, 1.0))
                        ar.set_actor_label("Mura %03d arredo delle guardie" % (i + 1))
                        ar.set_folder_path("Montalto/Mura/Arredo")
                        ar.tags = [unreal.Name(ETICHETTA_MURA)]
                        arredi += 1
                        # due luci per torre (le altre due lanterne del json no: 30 torri, meglio non esagerare):
                        # il braciere a terra e la lanterna al piano del camminamento
                        g2 = math.radians(float(v["yaw"]))
                        for j, lu in enumerate(luci_arredo):
                            tipo = str(lu.get("tipo", ""))
                            if "braciere" not in tipo and "appesa" not in tipo:
                                continue
                            q = lu["pos"]
                            luce_calda(unreal.Vector(angolo.x + x + q[0] * math.cos(g2) - q[1] * math.sin(g2),
                                                     angolo.y + y + q[0] * math.sin(g2) + q[1] * math.cos(g2),
                                                     z_mondo + q[2]),
                                       "Mura %03d luce %d" % (i + 1, j + 1), "Montalto/Mura/Luci", ETICHETTA_MURA,
                                       4.0 if "braciere" in tipo else 3.0, 500.0)
            except Exception as e:  # noqa: BLE001
                avviso("pezzo %d: errore (%s)" % (i + 1, e))
    scrivi("mura: %d pezzi su %d (%s)" % (messi, len(lista), ", ".join(
        "%s %d" % (k.replace("SM_Montalto_", ""), n_) for k, n_ in sorted(piano.get("conti", {}).items()))))
    scrivi("varchi: " + "; ".join(piano.get("varchi", [])))
    if con_arredo:
        scrivi("arredo da corpo di guardia: in %d torri (rastrelliera, tavolo, braciere a terra; brande al piano del "
               "camminamento), con due luci calde per torre" % arredi)
    if messi < attesi:
        avviso("le mura NON sono complete: %d pezzi su %d. Il livello NON e' stato salvato: rilancia questa voce."
               % (messi, attesi))
        return
    scrivi("Da guardare: il giro chiuso senza buchi tra i moduli; i contrafforti sui giunti; le torri che sporgono FUORI; "
           "la Porta nord con la grata e la strada che esce; i varchi con le torri di testata (Strada del Re verso il "
           "ponte e il porto, verso la Brina, via delle vigne); lungo l'Aure il muro che scende fino all'acqua.")
    try:
        livelli.save_current_level()
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a salvare il livello (%s)" % e)


scrivi("Terre di Montalto (prova), passo «%s»" % MODO)
try:
    if MODO == "vesti":
        vesti()
    elif MODO == "ponte":
        ponte()
    elif MODO == "rocca":
        rocca()
    elif MODO == "porto":
        porto()
    elif MODO == "granaio":
        granaio()
    elif MODO == "case":
        case()
    elif MODO == "mura":
        mura()
    elif MODO == "cimitero":
        cimitero()
    else:
        crea_livello()
finally:
    salva_resoconto()
    unreal.log("[Valdorso] Montalto: resoconto in Saved/Claude/montalto.txt")
