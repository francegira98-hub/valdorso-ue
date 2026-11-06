# Valdorso - La valle che reagisce (v0.1.3): gli uccellini e i pesci. Scritto da Claude il 06/11/2026.
# Si lancia una volta nell'editor (Strumenti -> Esegui script Python -> questo file), in qualsiasi livello.
#
# Importa da V:/Texture/valdorso/valleviva (fatti da Tools/Generatori/valle_viva.py, da zero, senza licenze) in
# /Game/Valle/Viva: SM_Uccello, SM_Pesce, S_Ali, S_Tuffo; e fa i materiali:
#   M_Uccello  colori dei vertici; con il World Position Offset le ali si chiudono quando l'uccello è posato (dato 0
#              dell'istanza = 0) e battono quando vola (dato 0 = 1), ognuno con il suo tempo (Per Instance Random)
#   M_Pesce    colori dei vertici, un po' lucido
# La logica è in AValdorsoValleViva (C++), che nasce da sola con la modalità di gioco.

import os

import unreal

CARTELLA = "V:/Texture/valdorso/valleviva"
DEST = "/Game/Valle/Viva"
libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def importa(file):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", file)
    c.set_editor_property("destination_path", DEST)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    strumenti.import_asset_tasks([c])


def nodo(m, classe, x, y):
    return mel.create_material_expression(m, classe, x, y)


def costante(m, valore, x, y):
    c = nodo(m, unreal.MaterialExpressionConstant, x, y)
    c.set_editor_property("r", valore)
    return c


def unisci(m, classe, a, b, x, y, pa="", pb=""):
    n = nodo(m, classe, x, y)
    mel.connect_material_expressions(a, pa, n, "A")
    mel.connect_material_expressions(b, pb, n, "B")
    return n


def materiale(nome):
    percorso = "%s/%s" % (DEST, nome)
    m = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if m is not None:
        mel.delete_all_material_expressions(m)
    else:
        m = strumenti.create_asset(nome, DEST, unreal.Material, unreal.MaterialFactoryNew())
    return m


def materiale_uccello():
    m = materiale("M_Uccello")
    m.set_editor_property("two_sided", True)
    colore = nodo(m, unreal.MaterialExpressionVertexColor, -400, -200)
    mel.connect_material_property(colore, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(costante(m, 0.85, -400, -50), "", unreal.MaterialProperty.MP_ROUGHNESS)
    m.set_editor_property("used_with_instanced_static_meshes", True)

    classe_locale = getattr(unreal, "MaterialExpressionLocalPosition", None)
    if classe_locale is None:
        unreal.log_warning("[Valdorso] Valle viva: questa versione non ha il nodo Local Position: le ali non battono")
        return m
    # quanto il vertice sta lontano dal corpo, lungo l'ala: W = max(|Y| - 3.5, 0) (cm)
    locale = nodo(m, classe_locale, -1800, 300)
    y = nodo(m, unreal.MaterialExpressionComponentMask, -1600, 300)
    y.set_editor_property("g", True)
    for canale in ("r", "b", "a"):
        y.set_editor_property(canale, False)         # solo Y: un numero solo
    mel.connect_material_expressions(locale, "", y, "")
    ay = nodo(m, unreal.MaterialExpressionAbs, -1450, 300)
    mel.connect_material_expressions(y, "", ay, "")
    meno = unisci(m, unreal.MaterialExpressionSubtract, ay, costante(m, 3.5, -1450, 400), -1300, 300)
    W = unisci(m, unreal.MaterialExpressionMax, meno, costante(m, 0.0, -1300, 400), -1150, 300)
    segno = nodo(m, unreal.MaterialExpressionSign, -1450, 150)
    mel.connect_material_expressions(y, "", segno, "")
    # in volo (dato 0 dell'istanza)
    volo = nodo(m, unreal.MaterialExpressionPerInstanceCustomData, -1600, 600)
    volo.set_editor_property("data_index", 0)
    # il battito: sin(tempo * 4.5 + caso)
    tempo = nodo(m, unreal.MaterialExpressionTime, -1600, 800)
    veloce = unisci(m, unreal.MaterialExpressionMultiply, tempo, costante(m, 4.5, -1600, 900), -1450, 800)
    caso = nodo(m, unreal.MaterialExpressionPerInstanceRandom, -1600, 1000)
    fase = unisci(m, unreal.MaterialExpressionAdd, veloce, caso, -1300, 800)
    seno = nodo(m, unreal.MaterialExpressionSine, -1150, 800)
    mel.connect_material_expressions(fase, "", seno, "")
    # Z = seno * W * 0.9 * volo
    z1 = unisci(m, unreal.MaterialExpressionMultiply, seno, W, -1000, 700)
    z2 = unisci(m, unreal.MaterialExpressionMultiply, z1, costante(m, 0.9, -1000, 800), -850, 700)
    z = unisci(m, unreal.MaterialExpressionMultiply, z2, volo, -700, 700)
    # Y = -segno * W * 0.85 * (1 - volo): posato, le ali si ripiegano sul corpo
    uno_meno = nodo(m, unreal.MaterialExpressionOneMinus, -1150, 600)
    mel.connect_material_expressions(volo, "", uno_meno, "")
    y1 = unisci(m, unreal.MaterialExpressionMultiply, segno, W, -1000, 300)
    y2 = unisci(m, unreal.MaterialExpressionMultiply, y1, costante(m, -0.85, -1000, 400), -850, 300)
    yy = unisci(m, unreal.MaterialExpressionMultiply, y2, uno_meno, -700, 300)
    zero = costante(m, 0.0, -700, 500)
    xy = unisci(m, unreal.MaterialExpressionAppendVector, zero, yy, -550, 400)
    xyz = unisci(m, unreal.MaterialExpressionAppendVector, xy, z, -400, 500)
    # dal locale al mondo (l'uccello vola girato)
    mondo = nodo(m, unreal.MaterialExpressionTransform, -250, 500)
    mondo.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    mondo.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    mel.connect_material_expressions(xyz, "", mondo, "")
    mel.connect_material_property(mondo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    return m


def materiale_pesce():
    m = materiale("M_Pesce")
    colore = nodo(m, unreal.MaterialExpressionVertexColor, -400, -200)
    mel.connect_material_property(colore, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(costante(m, 0.35, -400, -50), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(costante(m, 0.3, -400, 50), "", unreal.MaterialProperty.MP_METALLIC)
    return m


def mesh(nome):
    """La mesh appena importata (quella nella cartella del .glb) prima di quella già al suo posto."""
    voluto = "%s/%s" % (DEST, nome)
    trovate = []
    for p in libreria.list_assets(DEST, recursive=True, include_folder=False):
        if p.rsplit(".", 1)[-1] == nome:
            a = unreal.load_asset(p)
            if isinstance(a, unreal.StaticMesh):
                trovate.append(a)
    trovate.sort(key=lambda a: a.get_path_name().split(".")[0] == voluto)
    return trovate[0] if trovate else None


def esegui():
    for n in ("SM_Uccello.glb", "SM_Pesce.glb", "S_Ali.wav", "S_Tuffo.wav"):
        f = os.path.join(CARTELLA, n)
        if not os.path.exists(f):
            unreal.log_error("[Valdorso] Valle viva: manca %s" % f)
            return
        importa(f)
    for suono in ("S_Ali", "S_Tuffo"):
        s = unreal.load_asset("%s/%s" % (DEST, suono))
        if s is not None:
            s.set_editor_property("looping", False)
            libreria.save_loaded_asset(s)
    mu = materiale_uccello()
    mp = materiale_pesce()
    for m in (mu, mp):
        mel.recompile_material(m)
        libreria.save_loaded_asset(m)
    # le mesh: il materiale, niente collisione; il C++ le cerca in /Game/Valle/Viva/SM_Uccello (le sposto lì se il .glb
    # le ha messe in una cartella sua)
    for nome, mat in (("SM_Uccello", mu), ("SM_Pesce", mp)):
        a = mesh(nome)
        if a is None:
            unreal.log_error("[Valdorso] Valle viva: non trovo %s dopo l'importazione" % nome)
            continue
        a.set_material(0, mat)
        percorso = a.get_path_name().split(".")[0]
        voluto = "%s/%s" % (DEST, nome)
        libreria.save_loaded_asset(a)
        if percorso != voluto:
            if libreria.does_asset_exist(voluto):
                libreria.delete_asset(voluto)
            libreria.rename_asset(percorso, voluto)
            libreria.save_asset(voluto)
    unreal.log("[Valdorso] Valle viva: FATTO (uccelli, pesci, M_Uccello, M_Pesce, S_Ali, S_Tuffo in %s)" % DEST)


esegui()
