# Valdorso - Costruisce il villaggio, la palude di Nonna Edda e il posto di guardia del passo (v0.1.3).
# Scritto da Claude il 05/11/2026. Si lancia in L_Valle: Strumenti -> Esegui script Python -> questo file.
# Si può rilanciare: toglie tutto quello che ha messo prima (etichetta "ValdorsoVillaggio") e lo rimette.
#
# Legge V:/Texture/valdorso/villaggio (fatta da Tools/Generatori/villaggio.py):
#   T_*.png             -> /Game/Valle/Villaggio/Texture (texture fatte da zero, senza licenze)
#   SM_*.glb            -> /Game/Valle/Villaggio/Pezzi   (edifici, palizzate, strade, canneti...)
#   Villaggio_3000.json -> dove va ogni cosa (posizioni in cm dall'angolo del paesaggio, quota vera)
# e mette nel livello: le mesh, i modelli di Fab (case, mercato, botti, felci, rocce), le porte vere
# (ValdorsoPorta: le case private sono chiuse a chiave, locanda, Gilda, tempio e fucina sono aperte), le panche
# (ValdorsoPanca), le bacheche (ValdorsoBacheca), i letti (BP_Letto), le luci delle lanterne e (06/11) le scale a pioli
# delle torri (ValdorsoScala).
# Cartelle nel Contorno: Valle/Villaggio/<Palude, Villaggio, Guardia>. Resoconto in Saved/Claude/villaggio.txt.
# Alla fine salva il livello e i pezzi nuovi.
#
# (06/11) Le fasi del villaggio: il generatore prepara anche la fase 2 (anello di case, palizzata larga) e la fase 3
# (muro di pietra). Le cose che non ci sono sempre prendono l'etichetta "ValdorsoFase da-fino" (fino 0 = per sempre)
# e stanno nelle cartelle "Fase 2", "Fase 3", "Solo fase 1", "Solo fase 2". In gioco le mostra o le nasconde
# AValdorsoFasiVillaggio (comando Valdorso.Villaggio.Fase N). Nell'editor si vedono solo quelle di FASE_EDITOR
# (le altre sono nascoste solo per questa sessione dell'editor: per rivederle, Valdorso -> Mostra fase del villaggio,
# oppure l'occhio nel Contorno).

import json
import math
import os

import unreal

CARTELLA = "V:/Texture/valdorso/villaggio"
DEST = "/Game/Valle/Villaggio"
ETICHETTA = "ValdorsoVillaggio"
QUOTA_MAX_CM = 80000.0          # i 16 bit delle altezze coprono 0-800 m (valle.py)
REIMPORTA = True                # False: usa i pezzi già importati (più veloce, se il generatore non è cambiato)
FASE_EDITOR = 1                 # (06/11) la fase del villaggio che si vede nell'editor dopo il lancio

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Villaggio: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Villaggio: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "villaggio.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))


# ------------------------------------------------------------------------------------------------ import

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
        if percorso.rsplit(".", 1)[-1] == nome or percorso.rsplit("/", 1)[-1].split(".")[0] == nome:
            a = unreal.load_asset(percorso)
            if isinstance(a, classe):
                return a
    return None


def importa_texture():
    nomi = ["Pietra", "Intonaco", "Assi", "Trave", "Coppi", "Paglia", "Ciottoli", "Terra"]
    cartella = DEST + "/Texture"
    compiti = []
    for n in nomi:
        for suff in ("", "_N"):
            f = "%s/T_%s%s.png" % (CARTELLA, n, suff)
            if os.path.exists(f):
                compiti.append(compito(f, cartella))
    if REIMPORTA or not libreria.does_asset_exist(cartella + "/T_Pietra"):
        strumenti.import_asset_tasks(compiti)
    tex = {}
    for n in nomi:
        c = cerca(cartella, "T_" + n, unreal.Texture2D)
        nn = cerca(cartella, "T_%s_N" % n, unreal.Texture2D)
        if c is None or nn is None:
            avviso("manca la texture %s" % n)
            continue
        nn.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        nn.set_editor_property("srgb", False)
        libreria.save_loaded_asset(nn)
        tex[n] = (c, nn)
    scrivi("texture: %d coppie" % len(tex))
    return tex


# ------------------------------------------------------------------------------------------------ materiali

def nodo(m, classe, x, y):
    return mel.create_material_expression(m, classe, x, y)


def materiale_base(nome, mascherato=False, colore_vertici=False, predefinite=None):
    """M_Villaggio (texture + rilievo), M_VillaggioSuolo (mascherato: i bordi sfrangiati con l'alfa dei vertici),
    M_VillaggioColore (tinta x colore dei vertici, per vetro, ferro, acqua, canne, fiamma...). Tutti a due facce."""
    percorso = "%s/Materiali/%s" % (DEST, nome)
    m = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if m is not None:
        # (05/11) si svuota e si rifà: cancellarlo non si può, lo usano già le istanze e i pezzi
        mel.delete_all_material_expressions(m)
    else:
        m = strumenti.create_asset(nome, DEST + "/Materiali", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("two_sided", True)
    tinta = nodo(m, unreal.MaterialExpressionVectorParameter, -800, -300)
    tinta.set_editor_property("parameter_name", "Tinta")
    tinta.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    ruv = nodo(m, unreal.MaterialExpressionScalarParameter, -400, 200)
    ruv.set_editor_property("parameter_name", "Ruvidezza")
    ruv.set_editor_property("default_value", 0.85)
    mel.connect_material_property(ruv, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if colore_vertici:
        vc = nodo(m, unreal.MaterialExpressionVertexColor, -800, -100)
        per = nodo(m, unreal.MaterialExpressionMultiply, -400, -200)
        mel.connect_material_expressions(tinta, "", per, "A")
        mel.connect_material_expressions(vc, "", per, "B")
        mel.connect_material_property(per, "", unreal.MaterialProperty.MP_BASE_COLOR)
        luce = nodo(m, unreal.MaterialExpressionScalarParameter, -800, 100)
        luce.set_editor_property("parameter_name", "Luce")
        luce.set_editor_property("default_value", 0.0)
        em = nodo(m, unreal.MaterialExpressionMultiply, -400, 50)
        mel.connect_material_expressions(tinta, "", em, "A")
        mel.connect_material_expressions(luce, "", em, "B")
        mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        col = nodo(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, -100)
        col.set_editor_property("parameter_name", "Colore")
        col.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        nor = nodo(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 300)
        nor.set_editor_property("parameter_name", "Rilievo")
        nor.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        # (05/11) senza una texture di serie il materiale non compila e tutto esce a scacchi (visto al primo lancio)
        if predefinite:
            col.set_editor_property("texture", predefinite[0])
            nor.set_editor_property("texture", predefinite[1])
        # (06/11) "Scala": quante volte si ripete la texture (1 = le nostre; le texture vere di Fab ne vogliono un'altra)
        scala = nodo(m, unreal.MaterialExpressionScalarParameter, -1200, 100)
        scala.set_editor_property("parameter_name", "Scala")
        scala.set_editor_property("default_value", 1.0)
        uv = nodo(m, unreal.MaterialExpressionTextureCoordinate, -1200, -50)
        uvs = nodo(m, unreal.MaterialExpressionMultiply, -1000, 0)
        mel.connect_material_expressions(uv, "", uvs, "A")
        mel.connect_material_expressions(scala, "", uvs, "B")
        mel.connect_material_expressions(uvs, "", col, "UVs")
        mel.connect_material_expressions(uvs, "", nor, "UVs")
        per = nodo(m, unreal.MaterialExpressionMultiply, -400, -200)
        mel.connect_material_expressions(col, "RGB", per, "A")
        mel.connect_material_expressions(tinta, "", per, "B")
        mel.connect_material_property(per, "", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(nor, "RGB", unreal.MaterialProperty.MP_NORMAL)
        if mascherato:
            m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
            vc = nodo(m, unreal.MaterialExpressionVertexColor, -800, 600)
            due = nodo(m, unreal.MaterialExpressionConstant, -800, 750)
            due.set_editor_property("r", 2.0)
            doppio = nodo(m, unreal.MaterialExpressionMultiply, -600, 600)
            mel.connect_material_expressions(vc, "A", doppio, "A")
            mel.connect_material_expressions(due, "", doppio, "B")
            inverso = nodo(m, unreal.MaterialExpressionOneMinus, -600, 750)
            mel.connect_material_expressions(col, "A", inverso, "")
            meno = nodo(m, unreal.MaterialExpressionSubtract, -400, 650)
            mel.connect_material_expressions(doppio, "", meno, "A")
            mel.connect_material_expressions(inverso, "", meno, "B")
            mel.connect_material_property(meno, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    return m


def istanza(nome, genitore, colore=None, rilievo=None, tinta=None, ruvidezza=None, luce=None):
    percorso = "%s/Materiali/%s" % (DEST, nome)
    mi = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if mi is None:
        mi = strumenti.create_asset(nome, DEST + "/Materiali", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, genitore)
    if colore is not None:
        mel.set_material_instance_texture_parameter_value(mi, "Colore", colore)
    if rilievo is not None:
        mel.set_material_instance_texture_parameter_value(mi, "Rilievo", rilievo)
    if tinta is not None:
        mel.set_material_instance_vector_parameter_value(mi, "Tinta", unreal.LinearColor(tinta[0], tinta[1], tinta[2], 1.0))
    if ruvidezza is not None:
        mel.set_material_instance_scalar_parameter_value(mi, "Ruvidezza", ruvidezza)
    if luce is not None:
        mel.set_material_instance_scalar_parameter_value(mi, "Luce", luce)
    mel.update_material_instance(mi)
    libreria.save_loaded_asset(mi)
    return mi


def crea_materiali(tex):
    base = materiale_base("M_Villaggio", predefinite=tex.get("Pietra"))
    suolo = materiale_base("M_VillaggioSuolo", mascherato=True, predefinite=tex.get("Terra"))
    semplice = materiale_base("M_VillaggioColore", colore_vertici=True)
    mat = {}
    ruvidi = {"Pietra": 0.9, "Intonaco": 0.92, "Assi": 0.8, "Trave": 0.75, "Coppi": 0.7, "Paglia": 0.95}
    for n, r in ruvidi.items():
        if n in tex:
            mat[n] = istanza("MI_Villaggio_" + n, base, tex[n][0], tex[n][1], ruvidezza=r)
    for n in ("Ciottoli", "Terra"):
        if n in tex:
            mat[n] = istanza("MI_Villaggio_" + n, suolo, tex[n][0], tex[n][1], ruvidezza=0.9)
    colori = {
        "Vetro": ((0.02, 0.025, 0.03), 0.12, 0.0), "Ferro": ((0.05, 0.05, 0.05), 0.45, 0.0),
        "Acqua": ((0.03, 0.04, 0.025), 0.18, 0.0), "Fango": ((0.10, 0.08, 0.05), 0.5, 0.0),
        "Canna": ((1.0, 1.0, 1.0), 0.85, 0.0), "Verde": ((0.16, 0.30, 0.09), 0.8, 0.0),
        "Fiamma": ((1.0, 0.45, 0.12), 0.9, 25.0), "Stoffa": ((0.42, 0.10, 0.07), 0.9, 0.0),
    }
    for n, (c, r, l) in colori.items():
        mat[n] = istanza("MI_Villaggio_" + n, semplice, tinta=c, ruvidezza=r, luce=l)
    scrivi("materiali: %s" % ", ".join(sorted(mat)))
    # (06/11) le texture vere (passo C): se Fra ne ha scelte in texture_vere_villaggio.json, vanno al posto delle nostre
    try:
        import importlib
        import texture_vere
        importlib.reload(texture_vere)
        for riga in texture_vere.applica():
            scrivi("texture vere: " + riga)
    except Exception as e:  # noqa: BLE001
        avviso("texture vere non messe (%s)" % e)
    return mat


def materiale_sigillo():
    """(06/11) M_Sigillo: il velo del sigillo della Corona. Senza luce propria del mondo (unlit), additivo (si somma a
    quello che c'è dietro, come una luce), a due facce. Due strati di rune (T_SigilloRune) che scorrono piano in su,
    a velocità e grandezze diverse, più un chiarore di fondo; sfuma con l'alfa dei vertici (in cima e ai lati).
    Parametri per il C++ (AValdorsoSigillo): Forza (quanto brilla: il battito, la notte) e Colore."""
    cartella = DEST + "/Texture"
    f = CARTELLA + "/T_SigilloRune.png"
    if os.path.exists(f):
        strumenti.import_asset_tasks([compito(f, cartella)])
    rune = cerca(cartella, "T_SigilloRune", unreal.Texture2D)
    if rune is None:
        avviso("manca T_SigilloRune: il sigillo resta senza rune")
        return None
    percorso = "%s/Materiali/M_Sigillo" % DEST
    m = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if m is not None:
        mel.delete_all_material_expressions(m)
    else:
        m = strumenti.create_asset("M_Sigillo", DEST + "/Materiali", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("two_sided", True)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    def strato(x, y, scala, vx, vy):
        uv = nodo(m, unreal.MaterialExpressionTextureCoordinate, x - 600, y)
        uv.set_editor_property("u_tiling", scala)
        uv.set_editor_property("v_tiling", scala)
        pan = nodo(m, unreal.MaterialExpressionPanner, x - 400, y)
        pan.set_editor_property("speed_x", vx)
        pan.set_editor_property("speed_y", vy)
        mel.connect_material_expressions(uv, "", pan, "Coordinate")
        tx = nodo(m, unreal.MaterialExpressionTextureSample, x - 200, y)
        tx.set_editor_property("texture", rune)
        mel.connect_material_expressions(pan, "", tx, "UVs")
        return tx

    a = strato(-600, -300, 1.0, 0.0, -0.035)
    b = strato(-600, 0, 1.7, 0.012, -0.022)
    mezzo = nodo(m, unreal.MaterialExpressionConstant, -600, 250)
    mezzo.set_editor_property("r", 0.45)
    b2 = nodo(m, unreal.MaterialExpressionMultiply, -400, 100)
    mel.connect_material_expressions(b, "R", b2, "A")
    mel.connect_material_expressions(mezzo, "", b2, "B")
    somma = nodo(m, unreal.MaterialExpressionAdd, -250, -150)
    mel.connect_material_expressions(a, "R", somma, "A")
    mel.connect_material_expressions(b2, "", somma, "B")
    fondo = nodo(m, unreal.MaterialExpressionConstant, -400, 300)
    fondo.set_editor_property("r", 0.08)
    chiaro = nodo(m, unreal.MaterialExpressionAdd, -100, -100)
    mel.connect_material_expressions(somma, "", chiaro, "A")
    mel.connect_material_expressions(fondo, "", chiaro, "B")
    colore = nodo(m, unreal.MaterialExpressionVectorParameter, -100, 150)
    colore.set_editor_property("parameter_name", "Colore")
    colore.set_editor_property("default_value", unreal.LinearColor(1.0, 0.78, 0.42, 1.0))
    forza = nodo(m, unreal.MaterialExpressionScalarParameter, -100, 300)
    forza.set_editor_property("parameter_name", "Forza")
    forza.set_editor_property("default_value", 1.0)
    vc = nodo(m, unreal.MaterialExpressionVertexColor, -100, 450)
    p1 = nodo(m, unreal.MaterialExpressionMultiply, 100, -50)
    mel.connect_material_expressions(chiaro, "", p1, "A")
    mel.connect_material_expressions(colore, "", p1, "B")
    p2 = nodo(m, unreal.MaterialExpressionMultiply, 250, 50)
    mel.connect_material_expressions(p1, "", p2, "A")
    mel.connect_material_expressions(forza, "", p2, "B")
    p3 = nodo(m, unreal.MaterialExpressionMultiply, 400, 150)
    mel.connect_material_expressions(p2, "", p3, "A")
    mel.connect_material_expressions(vc, "A", p3, "B")
    # un poco più di 1: in additivo, con l'esposizione automatica di notte, si vede bene senza accecare
    luce = nodo(m, unreal.MaterialExpressionConstant, 400, 300)
    luce.set_editor_property("r", 2.5)
    p4 = nodo(m, unreal.MaterialExpressionMultiply, 550, 200)
    mel.connect_material_expressions(p3, "", p4, "A")
    mel.connect_material_expressions(luce, "", p4, "B")
    mel.connect_material_property(p4, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("materiale del sigillo: M_Sigillo")
    return m


def suono_sigillo():
    """(06/11) S_SigilloRonzio: il ronzio del sigillo, che si ripete (looping)."""
    f = CARTELLA + "/S_SigilloRonzio.wav"
    if not os.path.exists(f):
        return None
    strumenti.import_asset_tasks([compito(f, DEST + "/Suoni")])
    s = cerca(DEST + "/Suoni", "S_SigilloRonzio", unreal.SoundWave)
    if s is not None:
        s.set_editor_property("looping", True)
        libreria.save_loaded_asset(s)
    return s


# ------------------------------------------------------------------------------------------------ i pezzi

def importa_pezzi(nomi):
    cartella = DEST + "/Pezzi"
    if REIMPORTA:
        compiti = [compito("%s/%s.glb" % (CARTELLA, n), cartella) for n in nomi if os.path.exists("%s/%s.glb" % (CARTELLA, n))]
        scrivi("importo %d pezzi (qualche minuto)..." % len(compiti))
        strumenti.import_asset_tasks(compiti)
    pezzi = {}
    for n in nomi:
        a = cerca(cartella, n, unreal.StaticMesh)
        if a is None:
            avviso("pezzo non importato: %s" % n)
        else:
            pezzi[n] = a
    scrivi("pezzi pronti: %d su %d" % (len(pezzi), len(nomi)))
    return pezzi


def veste(mesh, mat, collisione):
    """Mette i nostri materiali negli slot (il nome dello slot è MV_<Materiale>) e la collisione."""
    materiali = mesh.get_editor_property("static_materials")
    for i, sm in enumerate(materiali):
        nome = str(sm.get_editor_property("material_slot_name"))
        mi = sm.get_editor_property("material_interface")
        if (not nome or nome == "None") and mi is not None:
            nome = mi.get_name()
        chiave = nome.split("MV_", 1)[-1].split("_")[0] if "MV_" in nome else None
        if chiave in mat:
            mesh.set_material(i, mat[chiave])
        else:
            avviso("%s: slot %s senza materiale" % (mesh.get_name(), nome))
    try:
        bs = mesh.get_editor_property("body_setup")
        if bs is not None:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
                                   if collisione else unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
            mesh.set_editor_property("body_setup", bs)
    except Exception as e:  # noqa: BLE001
        avviso("%s: collisione non impostata (%s)" % (mesh.get_name(), e))
    libreria.save_loaded_asset(mesh)


def controlla_assi(pezzi):
    """SM_Villaggio_ProvaAssi ha un braccio a +X (100), uno a +Y (300), uno a +Z (50): come li ha messi Unreal?"""
    m = pezzi.get("SM_Villaggio_ProvaAssi")
    if m is None:
        avviso("manca la prova degli assi: suppongo che vada tutto bene")
        return 1.0
    s = m.get_bounding_box()
    scrivi("prova assi: min (%.0f, %.0f, %.0f) max (%.0f, %.0f, %.0f)" % (s.min.x, s.min.y, s.min.z, s.max.x, s.max.y, s.max.z))
    if s.max.y > 250 and s.max.x > 80 and s.max.z > 40:
        return 1.0
    if s.min.y < -250 and s.max.x > 80 and s.max.z > 40:
        scrivi("prova assi: Unreal ha specchiato la Y, la rimetto con la scala Y = -1")
        return -1.0
    avviso("prova assi: orientamento strano, controllare a occhio")
    return 1.0


# ------------------------------------------------------------------------------------------------ nel livello

_modelli = {}


def carica_modello(percorso):
    """Un asset; se il percorso è una cartella (le rocce di Poly Haven), il primo modello dentro (non i LOD)."""
    if percorso in _modelli:
        return _modelli[percorso]
    a = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if a is None and libreria.does_directory_exist(percorso):
        for p in libreria.list_assets(percorso, recursive=True, include_folder=False):
            nome = p.rsplit(".", 1)[-1]
            if nome.endswith(("_LOD1", "_LOD2", "_LOD3")):
                continue
            x = unreal.load_asset(p)
            if isinstance(x, unreal.StaticMesh):
                a = x
                break
    _modelli[percorso] = a
    return a


def scompatta():
    """I pezzi arrivano in un solo file (villaggio.zip, accanto alla cartella): se è più nuovo, si scompatta."""
    import zipfile
    zf = CARTELLA + ".zip"
    segno = CARTELLA + "/Villaggio_3000.json"
    if os.path.exists(zf) and (not os.path.exists(segno) or os.path.getmtime(zf) > os.path.getmtime(segno)):
        os.makedirs(CARTELLA, exist_ok=True)
        with zipfile.ZipFile(zf) as z:
            z.extractall(CARTELLA)
        os.utime(segno, None)
        scrivi("scompattato %s" % zf)


def principale():
    scompatta()
    with open(CARTELLA + "/Villaggio_3000.json", "r", encoding="utf-8") as f:
        dati = json.load(f)
    paesaggi = [a for a in attori.get_all_level_actors() if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Villaggio: nel livello aperto non c'è un Landscape. Apri L_Valle.")
        return
    angolo = paesaggi[0].get_actor_location()
    scala = paesaggi[0].get_actor_scale3d()

    def nel_mondo(p):
        valore = p[2] / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + p[0], angolo.y + p[1], z)

    classi = {n: getattr(unreal, c, None) for n, c in (("porta", "ValdorsoPorta"), ("panca", "ValdorsoPanca"), ("bacheca", "ValdorsoBacheca"),
                                                       ("sigillo", "ValdorsoSigillo"), ("scala", "ValdorsoScala"))}
    for n, c in classi.items():
        if c is None:
            avviso("manca la classe C++ per %s (compilare Valdorso): non le metto" % n)
    letto = unreal.load_asset("/Game/Fab/Letto/BP_Letto")
    classe_letto = letto.generated_class() if letto else None

    # 1. via quello messo prima
    tolti = 0
    for a in attori.get_all_level_actors():
        if a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            tolti += 1
    scrivi("tolti %d attori del lancio precedente" % tolti)

    # 2. texture, materiali, pezzi
    tex = importa_texture()
    mat = crea_materiali(tex)
    # (06/11) il sigillo della Corona: materiale e ronzio
    m_sig = materiale_sigillo()
    if m_sig is not None:
        mat["Sigillo"] = m_sig
    ronzio = suono_sigillo()
    pezzi = importa_pezzi(dati["mesh"])
    con_collisione = set(c["asset"] for c in dati["cose"] if c["tipo"] == "mesh" and c.get("collisione", True))
    for n, m in pezzi.items():
        veste(m, mat, n in con_collisione)
    specchio = controlla_assi(pezzi)

    # 3. le quote: il paesaggio vero (una linea dall'alto, solo il Landscape) contro i dati del generatore.
    #    Le cose che stanno da sole (case, porte, oggetti) si spostano se il terreno vero è diverso; quelle che seguono
    #    il terreno punto per punto (strade, palizzate, canneti) restano e si segnalano.
    correzioni = {}
    try:
        mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        scarti = []
        for i, c in enumerate(dati["cose"]):
            terra = nel_mondo([c["pos"][0], c["pos"][1], c.get("terreno", c["pos"][2])])
            r = unreal.SystemLibrary.line_trace_multi(mondo, terra + unreal.Vector(0, 0, 5000), terra - unreal.Vector(0, 0, 5000),
                                                      unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
            colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
            for hit in colpi or []:
                t = hit.to_tuple()
                if t[9] is not None and "Landscape" in t[9].get_class().get_name():
                    d = t[4].z - terra.z
                    scarti.append((abs(d), d, c["nome"], bool(c.get("suolo"))))
                    if not c.get("suolo") and 15 < abs(d) < 400:
                        correzioni[i] = d
                    break
        if scarti:
            scarti.sort(reverse=True)
            scrivi("quote: %d punti controllati, scarto medio %.0f cm; %d cose spostate per stare a terra" % (
                len(scarti), sum(s[1] for s in scarti) / len(scarti), len(correzioni)))
            for _, d, nome, suolo in scarti[:12]:
                scrivi("   scarto %+.0f cm: %s%s" % (d, nome, " (segue il terreno, non spostata)" if suolo else (" (troppo, non spostata)" if abs(d) >= 400 else "")))
        else:
            scrivi("quote: nessun punto controllato (la valle è caricata? uso i dati)")
    except Exception as e:  # noqa: BLE001
        scrivi("quote: controllo saltato (%s)" % e)

    # 4. si mette tutto
    indice_nome = {c["nome"]: k for k, c in enumerate(dati["cose"])}
    conti = {}
    conti_fasi = {}
    mancano = set()
    for i, c in enumerate(dati["cose"]):
        tipo = c["tipo"]
        p = nel_mondo(c["pos"])
        if tipo == "scala":
            # (06/11) la scala si sposta come la sua torre, non per conto suo (i pioli sono nella mesh della torre)
            torre = indice_nome.get(c["nome"].rsplit(" scala", 1)[0])
            if torre in correzioni:
                p = unreal.Vector(p.x, p.y, p.z + correzioni[torre])
        elif i in correzioni:
            p = unreal.Vector(p.x, p.y, p.z + correzioni[i])
        rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=c["gradi"])
        a = None
        try:
            if tipo == "mesh":
                m = pezzi.get(c["asset"])
                if m is None:
                    continue
                a = attori.spawn_actor_from_object(m, p, rot)
                if a and specchio < 0:
                    a.set_actor_scale3d(unreal.Vector(1, -1, 1))
                comp = a.get_component_by_class(unreal.StaticMeshComponent) if a else None
                if comp:
                    if not c.get("collisione", True):
                        comp.set_collision_profile_name("NoCollision")
                    if c["asset"].startswith(("SM_Villaggio_Strade", "SM_Villaggio_Piazza", "SM_Villaggio_Carovane", "SM_Palude_Pozze")):
                        comp.set_editor_property("cast_shadow", False)
            elif tipo == "fab":
                asset = carica_modello(c["asset"])
                if asset is None:
                    mancano.add(c["asset"])
                    continue
                s = float(c.get("scala", 1.0))
                if isinstance(asset, unreal.Blueprint):
                    a = attori.spawn_actor_from_class(asset.generated_class(), p, rot)
                else:
                    if c.get("modo") == "scatola" and isinstance(asset, unreal.StaticMesh):
                        b = asset.get_bounding_box()
                        cx, cy = (b.min.x + b.max.x) / 2 * s, (b.min.y + b.max.y) / 2 * s
                        g = math.radians(c["gradi"])
                        p = unreal.Vector(p.x - (cx * math.cos(g) - cy * math.sin(g)), p.y - (cx * math.sin(g) + cy * math.cos(g)), p.z - b.min.z * s)
                    a = attori.spawn_actor_from_object(asset, p, rot)
                if a and abs(s - 1.0) > 1e-3:
                    a.set_actor_scale3d(unreal.Vector(s, s, s))
                if a and "Foliage" in c["asset"]:
                    comp = a.get_component_by_class(unreal.StaticMeshComponent)
                    if comp:
                        comp.set_collision_profile_name("NoCollision")
            elif tipo in ("porta", "panca", "bacheca"):
                if classi[tipo] is None:
                    continue
                a = attori.spawn_actor_from_class(classi[tipo], p, rot)
                if tipo == "porta":
                    a.set_editor_property("chiusa_a_chiave", bool(c.get("a_chiave", False)))
                    for comp in a.get_components_by_class(unreal.StaticMeshComponent):
                        if "Assi" in mat:
                            comp.set_material(0, mat["Assi"])
                elif tipo == "panca":
                    for comp in a.get_components_by_class(unreal.StaticMeshComponent):
                        if "Trave" in mat:
                            comp.set_material(0, mat["Trave"])
                elif tipo == "bacheca":
                    if c.get("titolo"):
                        a.set_editor_property("titolo", unreal.Text(c["titolo"]))
                    if c.get("avvisi"):
                        a.set_editor_property("avvisi", c["avvisi"])
            elif tipo == "sigillo":
                # (06/11) il sigillo della Corona: l'attore C++ con le misure della gola, il velo e il ronzio
                if classi["sigillo"] is None:
                    continue
                a = attori.spawn_actor_from_class(classi["sigillo"], p, rot)
                # l'etichetta subito: se qualcosa va storto più sotto, il prossimo lancio lo toglie comunque
                a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
                for prop, chiave in (("larghezza", "larghezza"), ("altezza", "altezza"), ("base", "base"),
                                     ("centro_traverso", "centro_traverso")):
                    a.set_editor_property(prop, float(c.get(chiave, 0.0)))
                velo = a.get_editor_property("velo")
                mv = pezzi.get(c.get("asset", ""))
                if velo and mv:
                    velo.set_static_mesh(mv)
                    if specchio < 0:
                        velo.set_relative_scale3d(unreal.Vector(1, -1, 1))
                if ronzio is not None:
                    a.get_editor_property("ronzio").set_sound(ronzio)
                try:
                    a.rerun_construction_scripts()      # di solito basta già set_editor_property
                except Exception:  # noqa: BLE001
                    pass
            elif tipo == "scala":
                # (06/11) la scala a pioli delle torri: i pioli sono nella mesh della torre, qui l'attore che fa salire
                if classi["scala"] is None:
                    continue
                a = attori.spawn_actor_from_class(classi["scala"], p, rot)
                a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
                a.set_editor_property("altezza", float(c.get("altezza", 1200.0)))
                a.set_editor_property("uscita_cima", float(c.get("uscita_cima", 170.0)))
            elif tipo == "letto":
                if classe_letto is None:
                    mancano.add("/Game/Fab/Letto/BP_Letto")
                    continue
                a = attori.spawn_actor_from_class(classe_letto, p, rot)
            elif tipo == "luce":
                a = attori.spawn_actor_from_class(unreal.PointLight, p, rot)
                lc = a.get_component_by_class(unreal.PointLightComponent)
                lc.set_mobility(unreal.ComponentMobility.MOVABLE)
                try:
                    lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
                except Exception:  # noqa: BLE001
                    pass
                lc.set_intensity(float(c.get("intensita", 4.0)))
                lc.set_attenuation_radius(float(c.get("raggio", 1000.0)))
                col = c.get("colore", [1.0, 0.62, 0.32])
                lc.set_light_color(unreal.LinearColor(col[0], col[1], col[2], 1.0))
                lc.set_cast_shadows(False)
        except Exception as e:  # noqa: BLE001
            avviso("%s (%s): %s" % (c["nome"], tipo, e))
            continue
        if a is None:
            avviso("%s (%s): non messo" % (c["nome"], tipo))
            continue
        a.set_actor_label(c["nome"])
        fase = c.get("fase", [1, 0])
        etichette = [e for e in a.get_editor_property("tags") if str(e) != ETICHETTA] + [unreal.Name(ETICHETTA)]
        cartella = "Valle/Villaggio/%s" % c["sezione"]
        if list(fase) != [1, 0]:
            # (06/11) la fase: "ValdorsoFase 2-0" = dalla fase 2 per sempre, "ValdorsoFase 1-1" = solo nella fase 1
            etichette.append(unreal.Name("ValdorsoFase %d-%d" % (fase[0], fase[1])))
            # fuori dagli HLOD (i modelli semplici da lontano di World Partition): da lontano si vedrebbero tutte le fasi
            try:
                a.set_editor_property("enable_auto_lod_generation", False)
            except Exception:  # noqa: BLE001
                pass
            cartella += "/" + (("Fase %d" % fase[0]) if fase[1] == 0 else ("Solo fase %d" % fase[0]) if fase[0] == fase[1]
                               else ("Fasi %d-%d" % (fase[0], fase[1])))
        a.set_folder_path(cartella)
        a.set_editor_property("tags", etichette)
        visibile = fase[0] <= FASE_EDITOR and (fase[1] == 0 or FASE_EDITOR <= fase[1])
        if not visibile:
            a.set_is_temporarily_hidden_in_editor(True)
        chiave = "fase %d%s" % (fase[0], "" if fase[1] == 0 else ("-%d" % fase[1]))
        conti_fasi[chiave] = conti_fasi.get(chiave, 0) + 1
        conti[c["sezione"]] = conti.get(c["sezione"], 0) + 1
    for k, v in sorted(conti.items()):
        scrivi("%s: %d attori" % (k, v))
    scrivi("fasi: " + ", ".join("%s: %d" % (k, v) for k, v in sorted(conti_fasi.items())) +
           " (nell'editor si vede la fase %d)" % FASE_EDITOR)
    for m in sorted(mancano):
        avviso("manca nel progetto: %s (quelle cose non ci sono)" % m)
    for a in dati.get("avvisi", []):
        scrivi("dal generatore: " + a)

    # 5. (05/11) il materiale della valle si rifà con T_SenzaErba: niente erba vera su strade, piazza e sotto le case
    if os.path.exists(CARTELLA + "/T_SenzaErba.png"):
        valle = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()),
                             "Python", "usati", "crea_materiale_valle.py")
        try:
            with open(valle, "r", encoding="utf-8") as f:
                exec(compile(f.read(), valle, "exec"), {"__name__": "__main__"})
            scrivi("materiale della valle rifatto: niente erba vera su strade, piazza e sotto le case")
        except Exception as e:  # noqa: BLE001
            avviso("materiale della valle non rifatto (%s): lanciare a mano usati/crea_materiale_valle.py" % e)

    # 6. si salva
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("salvato (livello e pezzi)")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): salvare a mano con Ctrl+Maiusc+S" % e)
    scrivi("FATTO. Il villaggio è intorno alla piazza; la palude a sud del lago; il posto di guardia al passo.")


try:
    principale()
finally:
    salva_resoconto()
