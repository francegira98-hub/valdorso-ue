# Valdorso - Pianta il bosco nella valle (v0.1.4, 09/11/2026, chiesto da Fra). Scritto da Claude.
# Si lancia in L_Valle dal menu Valdorso -> Villaggio -> "Bosco: pianta gli alberi" (o Strumenti -> Esegui script).
# Si puo' rilanciare: toglie i boschi messi prima (etichetta "ValdorsoBosco") e li rimette.
#
# Legge V:/Texture/valdorso/bosco (o bosco.zip accanto), fatta da Tools/Generatori/bosco.py:
#   T_*.png           -> /Game/Valle/Bosco/Texture (cortecce e fronde, fatte da zero)
#   SM_Bosco_*.glb    -> /Game/Valle/Bosco/Modelli (gli alberi, con Nanite)
#   Bosco_3000.json   -> dove va ogni albero
# Fa i materiali M_BoscoCorteccia (opaco) e M_BoscoFronde (mascherato, a due facce, "fogliame a due lati": la luce
# passa attraverso le foglie), poi divide la valle in 16 pezzi (750 x 750 m) e per ognuno mette un AValdorsoBosco
# (C++, ValdorsoBosco.h) con le sue specie. Ogni albero si appoggia al paesaggio vero (una linea dall'alto).
# Cartella nel Contorno: Valle/Bosco. Resoconto in Saved/Claude/bosco.txt. Alla fine salva.
#
# DENSITA: 1 = tutti gli alberi; 0.6 = sei su dieci (se gli fps calano, si abbassa e si rilancia).
# REIMPORTA: False = usa i modelli gia' importati (piu' veloce, se il generatore non e' cambiato).
#
# (09/11) Gli alberi di Fab al posto dei nostri: in Content/Python/alberi_fab.json, per ogni nostro modello quello di Fab
# e la sua scala, per esempio
#   {"SM_Bosco_Abete_1": {"modello": "/Game/Fab/Abeti/SM_Spruce_A", "scala": 1.0}}
# Le posizioni restano le stesse; i modelli che non ci sono restano i nostri. Poi si rilancia "Bosco: pianta gli alberi".

import json
import math
import os
import zipfile

import unreal

CARTELLA = "V:/Texture/valdorso/bosco"
DEST = "/Game/Valle/Bosco"
ETICHETTA = "ValdorsoBosco"
QUOTA_MAX_CM = 80000.0
DENSITA = 1.0
REIMPORTA = True
PEZZI = 4                      # la valle divisa in 4 x 4 pezzi

DENSITA = float(globals().get("DENSITA_BOSCO", DENSITA))

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []

# Da quanto lontano spariscono le cose piccole (cm); 0 = sempre visibili. Ombra: i cespugli non la fanno lontano.
SPARISCE = {"Nocciolo": 9000.0, "AbeteGiovane": 15000.0, "Ceppo": 6000.0, "Tronco": 9000.0}
SENZA_OMBRA = ("Ceppo",)


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Bosco: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Bosco: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "bosco.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))


def scompatta():
    zf = CARTELLA + ".zip"
    segno = CARTELLA + "/Bosco_3000.json"
    if os.path.exists(zf) and (not os.path.exists(segno) or os.path.getmtime(zf) > os.path.getmtime(segno)):
        os.makedirs(CARTELLA, exist_ok=True)
        with zipfile.ZipFile(zf) as z:
            z.extractall(CARTELLA)
        os.utime(segno, None)
        scrivi("scompattato %s" % zf)


def compito(file, destinazione):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", file)
    c.set_editor_property("destination_path", destinazione)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    return c


def cerca(cartella, nome, classe):
    a = unreal.load_asset("%s/%s.%s" % (cartella, nome, nome))
    if isinstance(a, classe):
        return a
    for percorso in libreria.list_assets(cartella, recursive=True, include_folder=False):
        if percorso.rsplit("/", 1)[-1].split(".")[0] == nome:
            a = unreal.load_asset(percorso)
            if isinstance(a, classe):
                return a
    return None


# ------------------------------------------------------------------------------------------------ texture

CORTECCE = ["Abete", "Faggio", "Betulla", "Melo", "Anelli"]
FRONDE = ["RamoAbete", "RamoLarice", "FoglieFaggio", "FoglieBetulla", "FoglieMelo", "FoglieNocciolo"]


def importa_texture():
    cartella = DEST + "/Texture"
    compiti = []
    for n in CORTECCE:
        for suff in ("", "_N"):
            f = "%s/T_Corteccia%s%s.png" % (CARTELLA, n, suff)
            if os.path.exists(f):
                compiti.append(compito(f, cartella))
    for n in FRONDE:
        f = "%s/T_%s.png" % (CARTELLA, n)
        if os.path.exists(f):
            compiti.append(compito(f, cartella))
    if REIMPORTA or not libreria.does_asset_exist(cartella + "/T_RamoAbete"):
        strumenti.import_asset_tasks(compiti)
    tex = {}
    for n in CORTECCE:
        c = cerca(cartella, "T_Corteccia" + n, unreal.Texture2D)
        nn = cerca(cartella, "T_Corteccia%s_N" % n, unreal.Texture2D)
        if c is None or nn is None:
            avviso("manca la corteccia %s" % n)
            continue
        nn.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        nn.set_editor_property("srgb", False)
        libreria.save_loaded_asset(nn)
        tex["Corteccia" + n] = (c, nn)
    for n in FRONDE:
        t = cerca(cartella, "T_" + n, unreal.Texture2D)
        if t is None:
            avviso("manca la fronda %s" % n)
            continue
        # le fronde non si ripetono: bordi fermi (niente righe sui lati delle carte)
        t.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        t.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        libreria.save_loaded_asset(t)
        tex[n] = (t, None)
    scrivi("texture: %d" % len(tex))
    return tex


# ------------------------------------------------------------------------------------------------ materiali

def nodo(m, classe, x, y):
    return mel.create_material_expression(m, classe, x, y)


def materiale(nome, fronde, predefinita=None, rilievo=None):
    percorso = "%s/Materiali/%s" % (DEST, nome)
    m = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if m is not None:
        mel.delete_all_material_expressions(m)
    else:
        m = strumenti.create_asset(nome, DEST + "/Materiali", unreal.Material, unreal.MaterialFactoryNew())
    # per il bosco (migliaia di istanze) e Nanite
    m.set_editor_property("used_with_instanced_static_meshes", True)
    tinta = nodo(m, unreal.MaterialExpressionVectorParameter, -900, -300)
    tinta.set_editor_property("parameter_name", "Tinta")
    tinta.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    col = nodo(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, -60)
    col.set_editor_property("parameter_name", "Colore")
    col.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    if predefinita is not None:
        col.set_editor_property("texture", predefinita)
    vc = nodo(m, unreal.MaterialExpressionVertexColor, -900, 200)
    per1 = nodo(m, unreal.MaterialExpressionMultiply, -600, -200)
    mel.connect_material_expressions(col, "RGB", per1, "A")
    mel.connect_material_expressions(tinta, "", per1, "B")
    # l'ombra dentro la chioma: il colore dei vertici (R)
    per2 = nodo(m, unreal.MaterialExpressionMultiply, -400, -100)
    mel.connect_material_expressions(per1, "", per2, "A")
    mel.connect_material_expressions(vc, "R", per2, "B")
    mel.connect_material_property(per2, "", unreal.MaterialProperty.MP_BASE_COLOR)
    ruv = nodo(m, unreal.MaterialExpressionScalarParameter, -400, 120)
    ruv.set_editor_property("parameter_name", "Ruvidezza")
    ruv.set_editor_property("default_value", 0.8 if fronde else 0.9)
    mel.connect_material_property(ruv, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if fronde:
        m.set_editor_property("two_sided", True)
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        m.set_editor_property("opacity_mask_clip_value", 0.4)
        mel.connect_material_property(col, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
        try:
            m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
            # la luce che passa attraverso le foglie: il loro colore, piu' scuro
            mezzo = nodo(m, unreal.MaterialExpressionConstant, -400, 300)
            mezzo.set_editor_property("r", 0.45)
            sss = nodo(m, unreal.MaterialExpressionMultiply, -200, 260)
            mel.connect_material_expressions(per2, "", sss, "A")
            mel.connect_material_expressions(mezzo, "", sss, "B")
            mel.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        except Exception as e:  # noqa: BLE001
            avviso("fogliame a due lati non impostato (%s): resta normale" % e)
    else:
        nor = nodo(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 420)
        nor.set_editor_property("parameter_name", "Rilievo")
        nor.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        if rilievo is not None:
            nor.set_editor_property("texture", rilievo)
        mel.connect_material_property(nor, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    return m


def istanza(nome, genitore, colore, rilievo=None, tinta=None):
    percorso = "%s/Materiali/%s" % (DEST, nome)
    mi = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if mi is None:
        mi = strumenti.create_asset(nome, DEST + "/Materiali", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, genitore)
    mel.set_material_instance_texture_parameter_value(mi, "Colore", colore)
    if rilievo is not None:
        mel.set_material_instance_texture_parameter_value(mi, "Rilievo", rilievo)
    if tinta is not None:
        mel.set_material_instance_vector_parameter_value(mi, "Tinta", unreal.LinearColor(tinta[0], tinta[1], tinta[2], 1.0))
    mel.update_material_instance(mi)
    libreria.save_loaded_asset(mi)
    return mi


def crea_materiali(tex):
    primo_c = tex.get("CortecciaAbete", (None, None))
    corteccia = materiale("M_BoscoCorteccia", False, primo_c[0], primo_c[1])
    fronde = materiale("M_BoscoFronde", True, tex.get("RamoAbete", (None,))[0])
    mat = {}
    for n, (c, nn) in tex.items():
        if n.startswith("Corteccia"):
            mat[n] = istanza("MI_Bosco_" + n, corteccia, c, nn)
        else:
            mat[n] = istanza("MI_Bosco_" + n, fronde, c)
    scrivi("materiali: %s" % ", ".join(sorted(mat)))
    return mat


# ------------------------------------------------------------------------------------------------ modelli

def importa_modelli(nomi, mat):
    cartella = DEST + "/Modelli"
    if REIMPORTA or not libreria.does_asset_exist(cartella + "/" + nomi[0]):
        compiti = [compito("%s/%s.glb" % (CARTELLA, n), cartella) for n in nomi if os.path.exists("%s/%s.glb" % (CARTELLA, n))]
        scrivi("importo %d modelli..." % len(compiti))
        strumenti.import_asset_tasks(compiti)
    modelli = {}
    for n in nomi:
        m = cerca(cartella, n, unreal.StaticMesh)
        if m is None:
            avviso("modello non importato: %s" % n)
            continue
        for i, sm in enumerate(m.get_editor_property("static_materials")):
            nome = str(sm.get_editor_property("material_slot_name"))
            mi = sm.get_editor_property("material_interface")
            if (not nome or nome == "None") and mi is not None:
                nome = mi.get_name()
            chiave = nome.split("MV_", 1)[-1].split("_")[0] if "MV_" in nome else None
            if chiave in mat:
                m.set_material(i, mat[chiave])
            else:
                avviso("%s: slot %s senza materiale" % (n, nome))
        try:
            ns = m.get_editor_property("nanite_settings")
            ns.set_editor_property("enabled", True)
            m.set_editor_property("nanite_settings", ns)
        except Exception as e:  # noqa: BLE001
            avviso("%s: Nanite non acceso (%s)" % (n, e))
        try:
            # niente collisione sul modello: la fanno i cilindri dei tronchi (AValdorsoBosco)
            bs = m.get_editor_property("body_setup")
            if bs is not None:
                bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
                m.set_editor_property("body_setup", bs)
            sotto = getattr(unreal, "StaticMeshEditorSubsystem", None)
            if sotto is not None:
                unreal.get_editor_subsystem(sotto).remove_collisions(m)
            else:
                unreal.EditorStaticMeshLibrary.remove_collisions(m)
        except Exception as e:  # noqa: BLE001
            avviso("%s: collisione non tolta (%s)" % (n, e))
        libreria.save_loaded_asset(m)
        modelli[n] = m
    scrivi("modelli pronti: %d su %d" % (len(modelli), len(nomi)))
    return modelli


# ------------------------------------------------------------------------------------------------ nel livello

def principale():
    scompatta()
    if not os.path.exists(CARTELLA + "/Bosco_3000.json"):
        unreal.log_error("[Valdorso] Bosco: manca %s/Bosco_3000.json (copiare la cartella del bosco)" % CARTELLA)
        return
    with open(CARTELLA + "/Bosco_3000.json", "r", encoding="utf-8") as f:
        dati = json.load(f)
    classe = getattr(unreal, "ValdorsoBosco", None)
    if classe is None:
        unreal.log_error("[Valdorso] Bosco: manca la classe C++ ValdorsoBosco (compilare Valdorso)")
        return
    paesaggi = [a for a in attori.get_all_level_actors() if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Bosco: nel livello aperto non c'e' un Landscape. Apri L_Valle.")
        return
    angolo = paesaggi[0].get_actor_location()
    scala = paesaggi[0].get_actor_scale3d()

    def nel_mondo(x, y, zq):
        valore = zq / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + x, angolo.y + y, z)

    # 1. via quelli di prima
    tolti = 0
    for a in attori.get_all_level_actors():
        if a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            tolti += 1
    scrivi("tolti %d boschi del lancio precedente" % tolti)

    # 2. texture, materiali, modelli
    tex = importa_texture()
    mat = crea_materiali(tex)
    ordine = dati["ordine"]
    modelli = importa_modelli(ordine, mat)
    info = dati["modelli"]
    scale_fab = {}
    file_fab = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), "Python", "alberi_fab.json")
    if os.path.exists(file_fab):
        try:
            with open(file_fab, "r", encoding="utf-8") as f:
                scelte = json.load(f)
            for nostro, scelta in scelte.items():
                vero = unreal.load_asset(scelta.get("modello", "")) if libreria.does_asset_exist(scelta.get("modello", "")) else None
                if isinstance(vero, unreal.StaticMesh) and nostro in modelli:
                    modelli[nostro] = vero
                    scale_fab[nostro] = float(scelta.get("scala", 1.0))
                    scrivi("Fab: %s al posto di %s" % (vero.get_name(), nostro))
                else:
                    avviso("Fab: %s non trovato (resta %s)" % (scelta.get("modello", "?"), nostro))
        except Exception as e:  # noqa: BLE001
            avviso("alberi_fab.json non letto (%s): restano i nostri" % e)

    # 3. ogni albero al suo posto (la quota vera del paesaggio, con una linea dall'alto)
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

    def a_terra(p):
        try:
            r = unreal.SystemLibrary.line_trace_multi(mondo, p + unreal.Vector(0, 0, 3000), p - unreal.Vector(0, 0, 3000),
                                                      unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
            colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
            for hit in colpi or []:
                t = hit.to_tuple()
                if t[9] is not None and "Landscape" in t[9].get_class().get_name():
                    return t[4].z
        except Exception:  # noqa: BLE001
            pass
        return None

    lato_cm = 300000.0
    pezzo_cm = lato_cm / PEZZI
    gruppi = {}           # (pi, pj) -> {nome: [Transform]}
    contati = 0
    saltati = 0
    corretti = 0
    for k, a in enumerate(dati["alberi"]):
        # sfoltire: sempre gli stessi (un numero fisso per albero)
        if DENSITA < 1.0 and ((k * 2654435761) % 1000) / 1000.0 >= DENSITA:
            saltati += 1
            continue
        nome = ordine[a[0]]
        if nome not in modelli:
            continue
        x, y, zq, gradi, s, piega, verso = a[1], a[2], a[3], a[4], a[5], a[6], a[7]
        p = nel_mondo(x, y, zq)
        z = a_terra(p)
        if z is not None:
            if abs(z - p.z) > 15:
                corretti += 1
            p = unreal.Vector(p.x, p.y, z)
        # la piega: una piccola inclinazione verso 'verso' (gli alberi non sono mai dritti come pali)
        pv = math.radians(verso)
        rot = unreal.Rotator(roll=piega * math.sin(pv), pitch=piega * math.cos(pv), yaw=gradi)
        pi, pj = min(PEZZI - 1, max(0, int(x // pezzo_cm))), min(PEZZI - 1, max(0, int(y // pezzo_cm)))
        centro = gruppi.setdefault((pi, pj), {"_centro": nel_mondo((pi + 0.5) * pezzo_cm, (pj + 0.5) * pezzo_cm, 0.0)})
        c = centro["_centro"]
        s = s * scale_fab.get(nome, 1.0)
        t = unreal.Transform(location=unreal.Vector(p.x - c.x, p.y - c.y, p.z - c.z), rotation=rot, scale=unreal.Vector(s, s, s))
        centro.setdefault(nome, []).append(t)
        contati += 1
    scrivi("alberi: %d messi, %d tolti per la densita' %.2f, %d appoggiati al paesaggio vero (scarto > 15 cm)" % (contati, saltati, DENSITA, corretti))

    # 4. un AValdorsoBosco per pezzo
    fatti = 0
    for (pi, pj), specie in sorted(gruppi.items()):
        c = specie.pop("_centro")
        bosco = attori.spawn_actor_from_class(classe, c, unreal.Rotator(0, 0, 0))
        if bosco is None:
            avviso("bosco %d-%d non creato" % (pi + 1, pj + 1))
            continue
        bosco.set_actor_label("Bosco %d-%d" % (pi + 1, pj + 1))
        bosco.set_folder_path("Valle/Bosco")
        bosco.set_editor_property("tags", [unreal.Name(ETICHETTA)])
        try:
            bosco.set_editor_property("is_spatially_loaded", False)
        except Exception:  # noqa: BLE001
            pass
        bosco.svuota()
        for nome, dove in sorted(specie.items()):
            i = info[nome]
            tipo = nome.split("_")[2]
            coll = i.get("collisione", "tronco")
            raggio = float(i.get("raggio_tronco", 0.0)) if coll in ("tronco", "ceppo", "tronco_a_terra") else 0.0
            altezza = float(i.get("lungo", 0.0)) if coll == "tronco_a_terra" else min(450.0, float(i.get("altezza", 400.0)))
            bosco.aggiungi_specie(modelli[nome], dove, raggio * 0.9, altezza, coll == "tronco_a_terra",
                                  SPARISCE.get(tipo, 0.0), tipo not in SENZA_OMBRA)
        bosco.ricostruisci()
        fatti += 1
        scrivi("Bosco %d-%d: %d alberi" % (pi + 1, pj + 1, bosco.quanti_alberi()))
    scrivi("boschi messi: %d" % fatti)

    # 5. si salva
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("salvato (livello e modelli)")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): salvare a mano con Ctrl+Maiusc+S" % e)


try:
    principale()
finally:
    salva_resoconto()
