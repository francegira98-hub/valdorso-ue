# Valdorso - Il materiale della valle (v0.1.3). Scritto da Claude il 06/10/2026.
# Si lancia nell'editor, con il livello della valle aperto: Strumenti -> Esegui script Python -> questo file.
# Si può rilanciare: rimpiazza quello che c'è.
#
# Cosa fa:
#   1. importa da V:/Texture/valdorso/terreno le texture fatte da Tools/Generatori/terreno.py (erba, roccia, terra,
#      ghiaia, neve: colore e normal map; più le macchie di variazione) e da V:/Texture/valdorso/valle le maschere
#      della valle (T_ValleMaschere_3000.png: R roccia, G neve, B terra, A ghiaia), in Content/Valle/Terreno;
#   2. crea il materiale M_Valle: erba dappertutto, poi terra, ghiaia, roccia e neve dove dicono le maschere;
#      le texture si ripetono in metri veri (l'erba ogni 4 m, la roccia ogni 8 m...), con macchie grandi per non
#      vedere la ripetizione da lontano;
#   3. lo mette sul Landscape del livello aperto.
# Non usa i livelli di pittura del paesaggio: le maschere vengono dal generatore della valle. Quando servirà dipingere
# a mano (sentieri nuovi, campi), si aggiungeranno i livelli sopra a questo.

import unreal

CARTELLA_TEXTURE = "V:/Texture/valdorso/terreno"
MASCHERE = "V:/Texture/valdorso/valle/T_ValleMaschere_3000.png"
DESTINAZIONE = "/Game/Valle/Terreno"
LATO_VALLE_CM = 300000.0       # 3 km (decisa da Fra il 06/10)

# Superficie, quanti centimetri prima che la texture si ripeta, quanto rilievo (forza della normal map).
SUPERFICI = [
    ("Erba", 400.0, 1.0),
    ("Terra", 300.0, 1.0),
    ("Ghiaia", 250.0, 1.0),
    ("Roccia", 800.0, 1.0),
    ("Neve", 600.0, 1.0),
]

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def importa(file, colore=True, normale=False, maschera=False):
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", file)
    compito.set_editor_property("destination_path", DESTINAZIONE)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])
    nome = file.replace("\\", "/").split("/")[-1].rsplit(".", 1)[0]
    tex = unreal.load_asset(DESTINAZIONE + "/" + nome + "." + nome)
    if tex is None:
        unreal.log_warning("[Valdorso] Valle: non riesco a importare " + file)
        return None
    if normale:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("flip_green_channel", False)
    elif maschera:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
        # (06/10) Con le mipmap: senza, da lontano la maschera "sfarfalla" e disegna strisce sul terreno.
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
        tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    else:
        tex.set_editor_property("srgb", colore)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    libreria.save_loaded_asset(tex)
    unreal.log("[Valdorso] Valle: importata " + nome)
    return tex


def nodo(materiale, classe, x, y):
    return mel.create_material_expression(materiale, classe, x, y)


def collega(da, uscita, a, ingresso):
    mel.connect_material_expressions(da, uscita, a, ingresso)


def uv_mondo(materiale, posizione_xy, scala_cm, x, y):
    """UV = posizione del mondo (X, Y) / scala_cm."""
    divide = nodo(materiale, unreal.MaterialExpressionDivide, x, y)
    divide.set_editor_property("const_b", scala_cm)
    collega(posizione_xy, "", divide, "A")
    return divide


def campiona(materiale, texture, uv, x, y, tipo):
    t = nodo(materiale, unreal.MaterialExpressionTextureSample, x, y)
    t.set_editor_property("texture", texture)
    t.set_editor_property("sampler_type", tipo)
    collega(uv, "", t, "UVs")
    return t


def lerp(materiale, a, a_uscita, b, b_uscita, alfa, alfa_uscita, x, y):
    l = nodo(materiale, unreal.MaterialExpressionLinearInterpolate, x, y)
    collega(a, a_uscita, l, "A")
    collega(b, b_uscita, l, "B")
    collega(alfa, alfa_uscita, l, "Alpha")
    return l


def esegui():
    # 1. Le texture.
    colori = {}
    normali = {}
    for nome, _, _ in SUPERFICI:
        colori[nome] = importa(CARTELLA_TEXTURE + "/T_" + nome + "_C.png", colore=True)
        normali[nome] = importa(CARTELLA_TEXTURE + "/T_" + nome + "_N.png", normale=True)
    variazione = importa(CARTELLA_TEXTURE + "/T_Variazione.png", colore=False)
    maschere = importa(MASCHERE, maschera=True)
    if maschere is None or None in colori.values() or None in normali.values() or variazione is None:
        unreal.log_error("[Valdorso] Valle: mancano delle texture, il materiale non si crea (vedi gli avvisi sopra).")
        return

    # Il paesaggio del livello aperto (serve la sua posizione per le maschere).
    paesaggi = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
                if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Valle: nel livello aperto non c'è un Landscape. Apri L_Valle e rilancia lo script.")
        return
    origine = paesaggi[0].get_actor_location()
    unreal.log("[Valdorso] Valle: angolo del paesaggio in X {:.0f}, Y {:.0f}".format(origine.x, origine.y))

    # 2. Il materiale (rifatto da capo a ogni lancio).
    percorso = DESTINAZIONE + "/M_Valle"
    if libreria.does_asset_exist(percorso):
        libreria.delete_asset(percorso)
    materiale = strumenti.create_asset("M_Valle", DESTINAZIONE, unreal.Material, unreal.MaterialFactoryNew())
    materiale.set_editor_property("use_material_attributes", False)

    posizione = nodo(materiale, unreal.MaterialExpressionWorldPosition, -2400, 0)
    xy = nodo(materiale, unreal.MaterialExpressionComponentMask, -2200, 0)
    xy.set_editor_property("r", True)
    xy.set_editor_property("g", True)
    xy.set_editor_property("b", False)
    xy.set_editor_property("a", False)
    collega(posizione, "", xy, "")

    # Le maschere coprono tutta la valle, a partire dall'angolo del paesaggio (la posizione dell'attore Landscape:
    # Unreal può averlo messo centrato sull'origine, quindi non si dà per scontato che parta da 0, 0).
    angolo = nodo(materiale, unreal.MaterialExpressionConstant2Vector, -2400, -700)
    angolo.set_editor_property("r", origine.x)
    angolo.set_editor_property("g", origine.y)
    dall_angolo = nodo(materiale, unreal.MaterialExpressionSubtract, -2200, -650)
    collega(xy, "", dall_angolo, "A")
    collega(angolo, "", dall_angolo, "B")
    uv_valle = uv_mondo(materiale, dall_angolo, LATO_VALLE_CM, -2000, -600)
    # (06/10) Compressione "Masks" vuole il campionatore "Masks", altrimenti il materiale non si compila.
    m = campiona(materiale, maschere, uv_valle, -1800, -600, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)

    # Ogni superficie: colore e normale, ripetuti alla sua scala.
    campioni = {}
    riga = 0
    for nome, scala, _ in SUPERFICI:
        uv = uv_mondo(materiale, xy, scala, -2000, 200 + riga * 500)
        c = campiona(materiale, colori[nome], uv, -1700, 200 + riga * 500, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        n = campiona(materiale, normali[nome], uv, -1700, 450 + riga * 500, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        campioni[nome] = (c, n)
        riga += 1

    # L'erba anche a una scala più grande (17 m), mescolata a metà: la ripetizione si nota meno.
    uv_erba2 = uv_mondo(materiale, xy, 1700.0, -2000, -1100)
    erba2 = campiona(materiale, colori["Erba"], uv_erba2, -1700, -1100, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    mezzo = nodo(materiale, unreal.MaterialExpressionConstant, -1500, -1200)
    mezzo.set_editor_property("r", 0.5)
    erba = lerp(materiale, campioni["Erba"][0], "RGB", erba2, "RGB", mezzo, "", -1300, -1100)

    # Colore: erba -> terra (B) -> ghiaia (A) -> roccia (R) -> neve (G).
    c1 = lerp(materiale, erba, "", campioni["Terra"][0], "RGB", m, "B", -1000, -400)
    c2 = lerp(materiale, c1, "", campioni["Ghiaia"][0], "RGB", m, "A", -800, -400)
    c3 = lerp(materiale, c2, "", campioni["Roccia"][0], "RGB", m, "R", -600, -400)
    c4 = lerp(materiale, c3, "", campioni["Neve"][0], "RGB", m, "G", -400, -400)

    # Macchie grandi (50 m): un po' più chiaro o più scuro qua e là.
    uv_var = uv_mondo(materiale, xy, 5000.0, -2000, -1500)
    var = campiona(materiale, variazione, uv_var, -1700, -1500, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    scuro = nodo(materiale, unreal.MaterialExpressionConstant, -1300, -1650)
    scuro.set_editor_property("r", 0.8)
    chiaro = nodo(materiale, unreal.MaterialExpressionConstant, -1300, -1550)
    chiaro.set_editor_property("r", 1.12)
    fattore = lerp(materiale, scuro, "", chiaro, "", var, "R", -1100, -1550)
    finale = nodo(materiale, unreal.MaterialExpressionMultiply, -200, -400)
    collega(c4, "", finale, "A")
    collega(fattore, "", finale, "B")
    mel.connect_material_property(finale, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Normali, con le stesse maschere.
    n1 = lerp(materiale, campioni["Erba"][1], "RGB", campioni["Terra"][1], "RGB", m, "B", -1000, 400)
    n2 = lerp(materiale, n1, "", campioni["Ghiaia"][1], "RGB", m, "A", -800, 400)
    n3 = lerp(materiale, n2, "", campioni["Roccia"][1], "RGB", m, "R", -600, 400)
    n4 = lerp(materiale, n3, "", campioni["Neve"][1], "RGB", m, "G", -400, 400)
    mel.connect_material_property(n4, "", unreal.MaterialProperty.MP_NORMAL)

    # Ruvidità: l'erba e la terra opache, la neve un poco lucida.
    ruvido = nodo(materiale, unreal.MaterialExpressionConstant, -600, 900)
    ruvido.set_editor_property("r", 0.9)
    lucido = nodo(materiale, unreal.MaterialExpressionConstant, -600, 1000)
    lucido.set_editor_property("r", 0.55)
    ruvidita = lerp(materiale, ruvido, "", lucido, "", m, "G", -400, 950)
    mel.connect_material_property(ruvidita, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.layout_material_expressions(materiale)
    mel.recompile_material(materiale)
    libreria.save_loaded_asset(materiale)
    unreal.log("[Valdorso] Valle: creato " + percorso)

    # 3. Sul paesaggio del livello aperto.
    for attore in paesaggi:
        attore.set_editor_property("landscape_material", materiale)
    unreal.log("[Valdorso] Valle: materiale messo sul paesaggio. Fatto.")


esegui()
