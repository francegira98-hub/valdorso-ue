# Valdorso - Sistema la valle (v0.1.3). Scritto da Claude il 07/11/2026, dopo la prova di Fra e il suo registro.
# Si lancia in L_Valle: Strumenti -> Esegui script Python -> questo file, oppure dal menu Valdorso ->
# "Valle: sistema (piedi, materiali, Nanite)". Si può rilanciare quante volte si vuole. Alla fine salva.
#
# Cosa sistema:
#   1. I PIEDI CHE AFFONDANO in piazza e sulle strade del villaggio. Il lastricato della piazza, le strade di terra
#      battuta e il piazzale delle carovane sono mesh che seguono il terreno 5-8 cm più in alto, ma erano senza
#      collisione: il colono camminava sul paesaggio, sotto, e i piedi sparivano nei ciottoli. Ora quelle mesh hanno la
#      collisione vera ("complesso come semplice") e si cammina sopra. (In chiesa andava già bene: il pavimento ce l'ha.)
#   2. M_RocciaScansione (le rocce di Poly Haven) non si compilava: le texture di prova dei parametri Normale e Ruvidita
#      erano del tipo sbagliato, e le rocce avevano il materiale di serie (grigio a scacchi). Ora le texture di prova
#      sono quelle di una roccia vera, del tipo giusto. Le istanze (MI_<roccia>) restano com'erano.
#   (secondo giro, 07/11 sera: i piedi affondavano ancora. I triangoli della piazza guardano in giù, il materiale è a
#   due facce e si vedono, ma la collisione ne guardava una sola: ora vale da tutte e due le facce.)
#   3. Nanite con i materiali trasparenti (la cascata, la schiuma, l'arcobaleno, il sigillo della guardia): Nanite non
#      li disegna bene e il registro se ne lamentava. Su quelle mesh Nanite si spegne.
#   4. Nanite acceso sui pezzi del villaggio non trasparenti (il messaggio giallo [VSM] delle ombre).
#   5. La luce dell'altare del tempio più bassa (la pietra ha la sua).
#   6. Le tende del mercato: si entra (collisione con la forma vera) e toccano terra.
#   (terzo giro: la collisione della piazza ora si salva davvero, e una linea dall'alto dice cosa tocca per prima.)
# Resoconto in Saved/Claude/sistema_valle.txt.

import os

import unreal

libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []

SUOLO = ("SM_Villaggio_Piazza", "SM_Villaggio_Strade", "SM_Villaggio_Carovane")
ROCCE = "/Game/Fab/PolyHaven"
# (07/11 sera) "trasparente" = tutto ciò che non è opaco o mascherato (l'arcobaleno era AlphaComposite e sfuggiva)
TRASPARENTI = tuple(getattr(unreal.BlendMode, n) for n in (
    "BLEND_TRANSLUCENT", "BLEND_ADDITIVE", "BLEND_MODULATE", "BLEND_ALPHA_COMPOSITE", "BLEND_ALPHA_HOLDOUT",
    "BLEND_TRANSLUCENT_COLORED_TRANSMITTANCE") if hasattr(unreal.BlendMode, n))


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Sistema la valle: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE " + testo)
    unreal.log_warning("[Valdorso] Sistema la valle: " + testo)


def mesh_del_livello():
    """(attore, componente, mesh) per ogni mesh nel livello."""
    for a in attori.get_all_level_actors():
        for comp in a.get_components_by_class(unreal.StaticMeshComponent):
            sm = comp.get_editor_property("static_mesh")
            if sm is not None:
                yield a, comp, sm


# ---------------------------------------------------------------------------------------------- 1. i piedi
def suolo_calpestabile():
    mesh = {}
    componenti = 0
    for a, comp, sm in mesh_del_livello():
        if not sm.get_name().startswith(SUOLO):
            continue
        mesh[sm.get_path_name()] = sm
        # (terzo giro) modify(): senza, l'attore non risulta cambiato e il livello non lo salva (con World Partition
        # ogni attore è un file suo): al riavvio la piazza tornava senza collisione
        for o in (a, comp):
            try:
                o.modify()
            except Exception:  # noqa: BLE001
                pass
        prima = str(comp.get_collision_profile_name())
        comp.set_collision_profile_name("BlockAll")
        scrivi("%s: collisione %s -> BlockAll" % (a.get_actor_label(), prima))
        try:
            comp.set_editor_property("can_character_step_up_on", unreal.CanBeCharacterBase.ECB_YES)
        except Exception:  # noqa: BLE001
            pass
        componenti += 1
    for percorso, sm in sorted(mesh.items()):
        try:
            bs = sm.get_editor_property("body_setup")
            if bs is None:
                avviso("senza body setup: %s" % percorso)
                continue
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            # (07/11 sera, secondo giro) i triangoli della piazza guardano in giù (il materiale è a due facce e si
            # vedono lo stesso): la collisione a una faccia sola lasciava passare i piedi. Ora vale da tutte e due.
            bs.set_editor_property("double_sided_geometry", True)
            sm.set_editor_property("body_setup", bs)
            # (quarto giro) senza Nanite le coordinate della texture si tengono "a mezza precisione": la piazza le ha
            # grandi (seguono il mondo) e la texture veniva a strisce. Si chiede la precisione piena.
            try:
                sottosistema = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
                costruzione = sottosistema.get_lod_build_settings(sm, 0)
                nome_prop = "use_full_precision_u_vs"
                try:
                    gia = costruzione.get_editor_property(nome_prop)
                except Exception:  # noqa: BLE001
                    nome_prop = "use_full_precision_uvs"
                    gia = costruzione.get_editor_property(nome_prop)
                if not gia:
                    costruzione.set_editor_property(nome_prop, True)
                    sottosistema.set_lod_build_settings(sm, 0, costruzione)
                    scrivi("coordinate della texture a precisione piena (niente strisce): %s" % sm.get_name())
            except Exception as e:  # noqa: BLE001
                avviso("%s: precisione delle coordinate non cambiata (%s)" % (sm.get_name(), e))
            # (terzo giro) con Nanite la collisione si fa sulla copia semplificata della mesh, che può stare qualche
            # centimetro sotto quella che si vede: sulla piazza Nanite si spegne (è una superficie sola, pesa poco)
            try:
                ns = sm.get_editor_property("nanite_settings")
                if ns.get_editor_property("enabled"):
                    ns.set_editor_property("enabled", False)
                    sm.set_editor_property("nanite_settings", ns)
                    scrivi("Nanite spento (collisione uguale a quello che si vede): %s" % sm.get_name())
            except Exception as e:  # noqa: BLE001
                avviso("%s: Nanite non controllato (%s)" % (sm.get_name(), e))
            libreria.save_loaded_asset(sm)
            scrivi("si cammina sopra (due facce): %s" % sm.get_name())
        except Exception as e:  # noqa: BLE001
            avviso("%s: %s" % (percorso, e))
    if not mesh:
        avviso("non ho trovato la piazza e le strade del villaggio: sei in L_Valle? Il villaggio c'è?")
    scrivi("1. suolo: %d mesh, %d nel livello" % (len(mesh), componenti))


# ---------------------------------------------------------------------------------------------- 2. le rocce
def texture_di_prova():
    """Una texture per tipo (colore, normale, ruvidità) presa da una roccia di Poly Haven già importata."""
    trovate = {}
    for percorso in sorted(libreria.list_assets(ROCCE, recursive=True, include_folder=False)):
        if len(trovate) == 3:
            break
        dati = libreria.find_asset_data(percorso)
        if str(dati.asset_class_path.asset_name) != "Texture2D":
            continue
        t = unreal.load_asset(percorso)
        if t is None:
            continue
        nome = t.get_name().lower()
        compressione = t.get_editor_property("compression_settings")
        srgb = t.get_editor_property("srgb")
        # per nome (le texture di Poly Haven: _diff_, _nor_gl_, _rough_) e per tipo, così il campionatore è giusto
        if "_nor_" in nome and compressione == unreal.TextureCompressionSettings.TC_NORMALMAP:
            trovate.setdefault("Normale", t)
        elif "_diff_" in nome and srgb:
            trovate.setdefault("Colore", t)
        elif "_rough_" in nome and not srgb and compressione == unreal.TextureCompressionSettings.TC_DEFAULT:
            trovate.setdefault("Ruvidita", t)
    return trovate


def rocce_materiale():
    percorso = ROCCE + "/M_RocciaScansione"
    if not libreria.does_asset_exist(percorso):
        scrivi("2. rocce: M_RocciaScansione non c'è, salto")
        return
    m = unreal.load_asset(percorso)
    prova = texture_di_prova()
    if len(prova) < 3:
        avviso("2. rocce: non trovo le texture delle rocce in %s (trovate: %s)" % (ROCCE, ", ".join(prova) or "nessuna"))
        return
    # si rifanno i nodi nello stesso materiale (non lo si cancella: le istanze MI_<roccia> restano attaccate)
    mel.delete_all_material_expressions(m)
    tipi = {"Colore": unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
            "Normale": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
            "Ruvidita": unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR}
    nodi = {}
    for k, (nome, tipo) in enumerate(tipi.items()):
        p = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, -200 + 300 * k)
        p.set_editor_property("parameter_name", nome)
        p.set_editor_property("texture", prova[nome])
        p.set_editor_property("sampler_type", tipo)
        nodi[nome] = p
    tinta = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, -350)
    tinta.set_editor_property("parameter_name", "Luminosita")
    tinta.set_editor_property("default_value", 0.8)
    per = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, -250)
    mel.connect_material_expressions(nodi["Colore"], "RGB", per, "A")
    mel.connect_material_expressions(tinta, "", per, "B")
    mel.connect_material_property(per, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nodi["Normale"], "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(nodi["Ruvidita"], "R", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.layout_material_expressions(m)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("2. rocce: M_RocciaScansione rifatto (prova: %s)" % ", ".join(t.get_name() for t in prova.values()))


# ---------------------------------------------------------------------------------------------- 3. Nanite
def trasparente(mi):
    if isinstance(mi, unreal.MaterialInstance):
        try:
            sovr = mi.get_editor_property("base_property_overrides")
            if sovr is not None and sovr.get_editor_property("override_blend_mode"):
                return sovr.get_editor_property("blend_mode") in TRASPARENTI
        except Exception:  # noqa: BLE001
            pass  # (non tutte le versioni lo fanno leggere da Python: si guarda il materiale padre)
    try:
        base = mi.get_base_material()
        return base is not None and base.get_editor_property("blend_mode") in TRASPARENTI
    except Exception:  # noqa: BLE001
        return False


def nanite_trasparenti():
    viste = {}
    for a, comp, sm in mesh_del_livello():
        if sm.get_path_name() in viste:
            continue
        viste[sm.get_path_name()] = sm
    spente = 0
    for percorso, sm in sorted(viste.items()):
        try:
            ns = sm.get_editor_property("nanite_settings")
            if not ns.get_editor_property("enabled"):
                continue
            materiali = [s.get_editor_property("material_interface") for s in sm.get_editor_property("static_materials")]
            if not any(mi is not None and trasparente(mi) for mi in materiali):
                continue
            ns.set_editor_property("enabled", False)
            sm.set_editor_property("nanite_settings", ns)
            libreria.save_loaded_asset(sm)
            spente += 1
            scrivi("Nanite spento: %s" % sm.get_name())
        except Exception as e:  # noqa: BLE001
            avviso("%s: %s" % (percorso, e))
    scrivi("3. Nanite: spento su %d mesh trasparenti" % spente)


# ---------------------------------------------------------------------------------------------- 4. Nanite sul villaggio
VILLAGGIO = "/Game/Valle/Villaggio/"


def nanite_villaggio():
    """I pezzi del villaggio erano senza Nanite: tanti pezzi senza Nanite intasano le ombre virtuali (il messaggio
    giallo "[VSM] Eccedenza coda processi contrassegno non Nanite"). Si accende su quelli non trasparenti."""
    viste = {}
    for a, comp, sm in mesh_del_livello():
        # la piazza e le strade no: con Nanite la collisione si farebbe sulla copia semplificata
        if sm.get_path_name().startswith(VILLAGGIO) and not sm.get_name().startswith(SUOLO):
            viste[sm.get_path_name()] = sm
    accese = 0
    for percorso, sm in sorted(viste.items()):
        try:
            ns = sm.get_editor_property("nanite_settings")
            if ns.get_editor_property("enabled"):
                continue
            materiali = [s.get_editor_property("material_interface") for s in sm.get_editor_property("static_materials")]
            if any(mi is not None and trasparente(mi) for mi in materiali):
                continue
            ns.set_editor_property("enabled", True)
            sm.set_editor_property("nanite_settings", ns)
            libreria.save_loaded_asset(sm)
            accese += 1
        except Exception as e:  # noqa: BLE001
            avviso("%s: %s" % (percorso, e))
    scrivi("4. Nanite acceso su %d pezzi del villaggio (su %d)" % (accese, len(viste)))


# ---------------------------------------------------------------------------------------------- 5. il tempio
def luce_altare():
    """La pietra ha la sua luce: la luce dell'altare si abbassa (il tempio era bianco e nebbioso)."""
    for a in attori.get_all_level_actors():
        if a.get_actor_label() == "Altare del Cuore (luce)":
            lc = a.get_component_by_class(unreal.PointLightComponent)
            if lc:
                lc.set_intensity(2.5)
                lc.set_editor_property("volumetric_scattering_intensity", 0.3)
                scrivi("5. luce dell'altare abbassata a 2,5")
            return
    scrivi("5. luce dell'altare non trovata (niente da fare)")


# ---------------------------------------------------------------------------------------------- linee verso il basso
def mondo_editor():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def colpi_in_giu(x, y, z_alto, z_basso, ignora):
    """Gli urti di una linea verticale (dall'alto in basso), come (z, attore), in ordine."""
    # per tipo di oggetto (statici e dinamici): dà tutti gli urti, non si ferma al primo (casse, bancarelle...)
    r = unreal.SystemLibrary.line_trace_multi_for_objects(
        mondo_editor(), unreal.Vector(x, y, z_alto), unreal.Vector(x, y, z_basso),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2], True, ignora,
        unreal.DrawDebugTrace.NONE, True)
    colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
    fuori = []
    for hit in colpi or []:
        t = hit.to_tuple()
        fuori.append((t[5].z if t[5] is not None else t[4].z, t[9]))
    return fuori


def e_suolo(attore):
    """Vero per il paesaggio e per la piazza, le strade e il piazzale (quello su cui si cammina)."""
    if attore is None:
        return False
    if "Landscape" in attore.get_class().get_name():
        return True
    for comp in attore.get_components_by_class(unreal.StaticMeshComponent):
        sm = comp.get_editor_property("static_mesh")
        if sm is not None and sm.get_name().startswith(SUOLO):
            return True
    return False


def prova_piazza():
    """Una linea dall'alto sul centro della piazza: cosa tocca per primo? Se la piazza, i piedi stanno sopra."""
    for a, comp, sm in mesh_del_livello():
        if not sm.get_name().startswith("SM_Villaggio_Piazza"):
            continue
        origine, estensione = a.get_actor_bounds(False)
        colpi = colpi_in_giu(origine.x, origine.y, origine.z + estensione.z + 500, origine.z - estensione.z - 500, [])
        if not colpi:
            avviso("prova della piazza: la linea non tocca niente")
            return
        z0, primo = colpi[0]
        nome = primo.get_actor_label() if primo is not None else "?"
        terra = [z for z, att in colpi if att is not None and "Landscape" in att.get_class().get_name()]
        scrivi("prova della piazza: la linea tocca prima \"%s\" a z=%.1f%s" % (
            nome, z0, (" (il paesaggio è %.1f cm sotto)" % (z0 - terra[0])) if terra and terra[0] != z0 else ""))
        return


# ---------------------------------------------------------------------------------------------- 6. le tende del mercato
TENDE = ("Mercato (tenda grande)", "Mercato (tenda piccola)")
AFFONDA = 6.0  # (Fra, 07/11: prima 5, poi "un altro cm")
SEGNO_A_TERRA = "ValdorsoATerra"      # messa a terra con 5 cm di pali piantati
SEGNO_RITOCCO = "ValdorsoATerra6"     # e poi un altro centimetro (6 in tutto)


def altezza_da_terra(tenda):
    """Quanto sta sollevato da terra il pezzo più basso della tenda (cm), o None se non si riesce a misurare."""
    origine, estensione = tenda.get_actor_bounds(False)
    ignora = [x for x in attori.get_all_level_actors() if x.get_actor_label() in TENDE]
    distacchi = []
    n = 21
    for i in range(n):
        for j in range(n):
            # l'80% centrale: fuori ci sono i picchetti delle corde, che entrano nel terreno apposta
            x = origine.x + (i / (n - 1.0) - 0.5) * 2.0 * estensione.x * 0.8
            y = origine.y + (j / (n - 1.0) - 0.5) * 2.0 * estensione.y * 0.8
            suolo = None
            for z, att in colpi_in_giu(x, y, origine.z + estensione.z + 1000, origine.z - estensione.z - 2000, ignora):
                if e_suolo(att):
                    suolo = z
                    break
            if suolo is None:
                continue
            # dal basso verso l'alto, solo i pezzi di questa tenda: il primo sopra il suolo
            r = unreal.SystemLibrary.line_trace_multi_for_objects(
                mondo_editor(), unreal.Vector(x, y, suolo - 30.0), unreal.Vector(x, y, suolo + 400.0),
                [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2], True, [],
                unreal.DrawDebugTrace.NONE, True)
            colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
            for hit in colpi or []:
                t = hit.to_tuple()
                if t[9] != tenda:
                    continue
                z = t[5].z if t[5] is not None else t[4].z
                if z > suolo - 2.0:  # i pezzi già sotto terra (pali dove il terreno sale) non contano
                    distacchi.append(z - suolo)
                    break
    if not distacchi:
        return None
    distacchi.sort()
    scrivi("   %s: %d misure, la più bassa %.1f cm, a metà %.1f cm" % (
        tenda.get_actor_label(), len(distacchi), distacchi[0], distacchi[len(distacchi) // 2]))
    return distacchi[0]


def tende_mercato():
    """(07/11 sera, Fra) Le due tende del mercato: non si entrava (la collisione era un guscio intorno a tutta la tenda)
    e stavano sollevate da terra (messe all'altezza del punto più alto del terreno sotto di loro). Ora: collisione con
    la forma vera (si entra dal lato aperto) e la tenda scende finché tocca il suolo."""
    trovate = 0
    for a in attori.get_all_level_actors():
        if a.get_actor_label() not in TENDE:
            continue
        trovate += 1
        # (a) la collisione vera delle mesh della tenda
        for comp in a.get_components_by_class(unreal.StaticMeshComponent):
            sm = comp.get_editor_property("static_mesh")
            if sm is None or sm.get_name().startswith("rope"):
                continue  # le corde di Fab sono rotte (Bad MeshDescription): niente collisione da loro
            try:
                bs = sm.get_editor_property("body_setup")
                if bs is not None and bs.get_editor_property("collision_trace_flag") != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
                    bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
                    sm.set_editor_property("body_setup", bs)
                    libreria.save_loaded_asset(sm)
                    scrivi("tenda: collisione vera su %s" % sm.get_name())
            except Exception as e:  # noqa: BLE001
                avviso("tenda %s: %s" % (sm.get_name(), e))
        # (b) a terra (quarto giro: misurare la scatola non bastava, i pezzi della tenda non partono dal suo fondo).
        #     In una griglia di punti dentro la tenda: dov'è il suolo, e qual è il pezzo della tenda più basso lì sopra
        #     (palo, tappeto, cassa)? La tenda scende finché il pezzo più basso di tutti tocca il suolo.
        # (quinto giro, Fra: "abbassale di tipo 5 centimetri") la tenda scende fino a terra e poi ancora AFFONDA cm,
        # come se i pali fossero piantati. Fatto una volta, la tenda prende l'etichetta ValdorsoATerra e i lanci dopo
        # non la toccano più (se no scenderebbe ogni volta).
        etichette = [str(e) for e in a.get_editor_property("tags")]
        if SEGNO_RITOCCO in etichette:
            scrivi("%s: già sistemata (etichetta %s), non la tocco" % (a.get_actor_label(), SEGNO_RITOCCO))
            continue
        if SEGNO_A_TERRA in etichette:
            # già a terra con 5 cm: manca solo il centimetro in più
            try:
                a.modify()
            except Exception:  # noqa: BLE001
                pass
            p = a.get_actor_location()
            a.set_actor_location(unreal.Vector(p.x, p.y, p.z - (AFFONDA - 5.0)), False, False)
            a.set_editor_property("tags", list(a.get_editor_property("tags")) + [unreal.Name(SEGNO_RITOCCO)])
            scrivi("%s: abbassata di un altro %.0f cm" % (a.get_actor_label(), AFFONDA - 5.0))
            continue
        abbassa = altezza_da_terra(a)
        if abbassa is None:
            avviso("%s: non riesco a misurare quanto è alta da terra" % a.get_actor_label())
            continue
        scrivi("%s: il pezzo più basso sta %.1f cm sopra il suolo" % (a.get_actor_label(), abbassa))
        if abbassa > 80.0:
            avviso("   %.0f cm sono troppi: la lascio dov'è (guardare a mano)" % abbassa)
            continue
        scendi = max(abbassa, 0.0) + AFFONDA
        try:
            a.modify()
        except Exception:  # noqa: BLE001
            pass
        p = a.get_actor_location()
        a.set_actor_location(unreal.Vector(p.x, p.y, p.z - scendi), False, False)
        a.set_editor_property("tags", list(a.get_editor_property("tags")) + [unreal.Name(SEGNO_A_TERRA), unreal.Name(SEGNO_RITOCCO)])
        scrivi("   abbassata di %.0f cm (a terra, più %.0f cm di pali piantati)" % (scendi, AFFONDA))
    scrivi("6. tende del mercato: %d trovate" % trovate)


# ---------------------------------------------------------------------------------------------- 7. l'erba vera
def erba_da_rifare():
    """(07/11 sera) Dopo che il materiale della valle si è rifatto (crea_villaggio lo rifà ogni volta), l'erba vera va
    ricalcolata con la maschera T_SenzaErba già caricata: se no ricresce in piazza e sulle strade."""
    try:
        unreal.SystemLibrary.execute_console_command(mondo_editor(), "grass.FlushCache")
        scrivi("7. erba vera ricalcolata (grass.FlushCache)")
    except Exception as e:  # noqa: BLE001
        avviso("7. erba non ricalcolata (%s): scrivere grass.FlushCache nella console" % e)


def esegui():
    for passo in (suolo_calpestabile, prova_piazza, rocce_materiale, nanite_trasparenti, nanite_villaggio, luce_altare,
                  tende_mercato, erba_da_rifare):
        try:
            passo()
        except Exception as e:  # noqa: BLE001
            avviso("%s non riuscito: %s" % (passo.__name__, e))
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("salvato")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): Ctrl+Maiusc+S" % e)
    scrivi("FATTO. In Play: cammina in piazza e sulle strade, i piedi devono stare sui ciottoli.")


try:
    esegui()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "sistema_valle.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe))
