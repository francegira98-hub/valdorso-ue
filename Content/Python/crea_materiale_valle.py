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
MASCHERE2 = "V:/Texture/valdorso/valle/T_ValleMaschere2_3000.png"      # (07/10) R = bagnato
# (07/10) Le funzioni di Unreal che proiettano una texture da tre lati (triplanare): sulle pareti dritte la roccia
# non si stira più in righe verticali.
FUNZIONE_TRIPLANARE = "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture"
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


# (08/10) Forza degli strati della roccia: 1 = normale (0 = senza strati: usato per la prova R3-b, che ha mostrato che
# le "increspature" sui pendii grigi non venivano dagli strati ma dal rilievo della roccia, vedi ROCCIA_PIATTA).
FORZA_STRATI = 1.0
# (08/10) R3-b: il rilievo (normale) della roccia è proiettato dall'alto e sui pendii ripidi si stira in piccole onde
# regolari, come sabbia mossa dal vento. Lo si appiattisce: 0 = rilievo pieno, 1 = piatto. Il colore resta triplanare.
ROCCIA_PIATTA = 0.75
# (08/10) R3-d: con "Solo illuminazione" le chiazze sparivano: venivano dal COLORE. La texture della roccia ha macchie
# larghe che, ripetute ogni 8 m su un pendio grande, fanno un disegno regolare. Si avvicina il colore alla sua media
# (misurata: 0,165 / 0,145 / 0,117 lineare): 0 = texture com'è, 1 = colore piatto. La varietà grande resta (macchie a 50 m).
ROCCIA_ATTENUATA = 0.6
ROCCIA_MEDIA = (0.165, 0.145, 0.117)
# (08/10) R4: quanto scurire il prato dipinto sotto l'erba vera (1 = com'era; meno di 1 = più scuro), per R, G, B.
ERBA_TINTA = (0.55, 0.62, 0.42)


def roccia_vera(materiale, texture, x, y):
    """(07/10) La roccia: proiettata da tre lati (WorldAlignedTexture, 8 m), più scura e con gli strati.
    - scura: x 0,62 (la texture da sola sembrava gesso);
    - strati: bande orizzontali morbide ogni 3,5 m di quota, appena più chiare e più scure (dall'08/10 x 0,92 .. 1,08,
      con un secondo seno a 9,7 m che le rende meno regolari);
    - macchie: la stessa roccia a 31 m, mescolata un poco, per non vedere la ripetizione sulle pareti grandi."""
    funzione = unreal.load_asset(FUNZIONE_TRIPLANARE)
    if funzione is None:
        unreal.log_warning("[Valdorso] Valle: non trovo WorldAlignedTexture, la roccia resta proiettata dall'alto.")
        uv = uv_mondo(materiale, xy_globale[0], 800.0, x - 300, y)
        return campiona(materiale, texture, uv, x, y, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)

    def triplanare(misura, dx, dy):
        oggetto = nodo(materiale, unreal.MaterialExpressionTextureObject, x - 500 + dx, y + dy)
        oggetto.set_editor_property("texture", texture)
        grandezza = nodo(materiale, unreal.MaterialExpressionConstant3Vector, x - 500 + dx, y + 120 + dy)
        grandezza.set_editor_property("constant", unreal.LinearColor(misura, misura, misura, 1.0))
        chiamata = nodo(materiale, unreal.MaterialExpressionMaterialFunctionCall, x - 250 + dx, y + dy)
        chiamata.set_editor_property("material_function", funzione)
        collega(oggetto, "", chiamata, "TextureObject")
        collega(grandezza, "", chiamata, "TextureSize")
        return chiamata

    vicina = triplanare(800.0, 0, 0)
    lontana = triplanare(3100.0, 0, 300)
    mescola = nodo(materiale, unreal.MaterialExpressionConstant, x - 100, y + 450)
    mescola.set_editor_property("r", 0.35)
    insieme = lerp(materiale, vicina, "XYZ Texture", lontana, "XYZ Texture", mescola, "", x, y + 150)
    media = nodo(materiale, unreal.MaterialExpressionConstant3Vector, x - 100, y + 520)
    media.set_editor_property("constant", unreal.LinearColor(ROCCIA_MEDIA[0], ROCCIA_MEDIA[1], ROCCIA_MEDIA[2], 1.0))
    attenua = nodo(materiale, unreal.MaterialExpressionConstant, x - 100, y + 560)
    attenua.set_editor_property("r", ROCCIA_ATTENUATA)
    insieme = lerp(materiale, insieme, "", media, "", attenua, "", x + 50, y + 150)
    # strati: 1 + 0,12 * seno(quota / 3,5 m)
    quota = nodo(materiale, unreal.MaterialExpressionWorldPosition, x - 500, y + 600)
    z = nodo(materiale, unreal.MaterialExpressionComponentMask, x - 350, y + 600)
    for canale, acceso in (("r", False), ("g", False), ("b", True), ("a", False)):
        z.set_editor_property(canale, acceso)
    collega(quota, "", z, "")
    seno = nodo(materiale, unreal.MaterialExpressionSine, x - 200, y + 600)
    seno.set_editor_property("period", 350.0)
    collega(z, "", seno, "")
    ampiezza = nodo(materiale, unreal.MaterialExpressionMultiply, x - 50, y + 600)
    # (08/10) R3: strati più leggeri (prima 0,12: sui pendii chiari si vedevano "onde" regolari) e meno regolari:
    # un secondo seno a 9,7 m, più debole, rompe il passo fisso di 3,5 m
    ampiezza.set_editor_property("const_b", 0.05 * FORZA_STRATI)
    collega(seno, "", ampiezza, "A")
    seno2 = nodo(materiale, unreal.MaterialExpressionSine, x - 200, y + 750)
    seno2.set_editor_property("period", 970.0)
    collega(z, "", seno2, "")
    ampiezza2 = nodo(materiale, unreal.MaterialExpressionMultiply, x - 50, y + 750)
    ampiezza2.set_editor_property("const_b", 0.03 * FORZA_STRATI)
    collega(seno2, "", ampiezza2, "A")
    somma = nodo(materiale, unreal.MaterialExpressionAdd, x + 30, y + 680)
    collega(ampiezza, "", somma, "A")
    collega(ampiezza2, "", somma, "B")
    # (08/10) R3-f: gli strati solo sulle pareti quasi verticali (come nella realtà). Sui pendii grandi e lisci
    # diventavano righe ondulate che seguono le curve di livello. ripido = saturo((0,5 - normale.z) x 4):
    # 0 sui pendii sotto i 60 gradi, pieno sulle pareti oltre i 75.
    normale = nodo(materiale, unreal.MaterialExpressionVertexNormalWS, x - 500, y + 900)
    nz = nodo(materiale, unreal.MaterialExpressionComponentMask, x - 350, y + 900)
    for canale, acceso in (("r", False), ("g", False), ("b", True), ("a", False)):
        nz.set_editor_property(canale, acceso)
    collega(normale, "", nz, "")
    meno = nodo(materiale, unreal.MaterialExpressionOneMinus, x - 250, y + 900)
    collega(nz, "", meno, "")
    sposta = nodo(materiale, unreal.MaterialExpressionSubtract, x - 150, y + 900)
    sposta.set_editor_property("const_b", 0.5)
    collega(meno, "", sposta, "A")
    per4 = nodo(materiale, unreal.MaterialExpressionMultiply, x - 50, y + 900)
    per4.set_editor_property("const_b", 4.0)
    collega(sposta, "", per4, "A")
    ripido = nodo(materiale, unreal.MaterialExpressionSaturate, x + 50, y + 900)
    collega(per4, "", ripido, "")
    solo_pareti = nodo(materiale, unreal.MaterialExpressionMultiply, x + 60, y + 760)
    collega(somma, "", solo_pareti, "A")
    collega(ripido, "", solo_pareti, "B")
    strati = nodo(materiale, unreal.MaterialExpressionAdd, x + 100, y + 600)
    strati.set_editor_property("const_b", 1.0)
    collega(solo_pareti, "", strati, "A")
    scura = nodo(materiale, unreal.MaterialExpressionMultiply, x + 150, y + 200)
    scura.set_editor_property("const_b", 0.62)
    collega(insieme, "", scura, "A")
    finale = nodo(materiale, unreal.MaterialExpressionMultiply, x + 300, y + 300)
    collega(scura, "", finale, "A")
    collega(strati, "", finale, "B")
    return finale


xy_globale = [None]


def esegui():
    # 1. Le texture.
    colori = {}
    normali = {}
    for nome, _, _ in SUPERFICI:
        colori[nome] = importa(CARTELLA_TEXTURE + "/T_" + nome + "_C.png", colore=True)
        normali[nome] = importa(CARTELLA_TEXTURE + "/T_" + nome + "_N.png", normale=True)
    variazione = importa(CARTELLA_TEXTURE + "/T_Variazione.png", colore=False)
    maschere = importa(MASCHERE, maschera=True)
    maschere2 = importa(MASCHERE2, maschera=True)
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
    # (08/10) Non si cancella più M_Valle per rifarlo: il paesaggio lo sta usando e Unreal a volte non riesce a
    # scaricarlo ("Failed to unload all packages during ForceDeleteObjects"). Si svuota e si riempie lo stesso materiale.
    if libreria.does_asset_exist(percorso):
        materiale = libreria.load_asset(percorso)
        mel.delete_all_material_expressions(materiale)
    else:
        materiale = strumenti.create_asset("M_Valle", DESTINAZIONE, unreal.Material, unreal.MaterialFactoryNew())
    materiale.set_editor_property("use_material_attributes", False)

    posizione = nodo(materiale, unreal.MaterialExpressionWorldPosition, -2400, 0)
    xy = nodo(materiale, unreal.MaterialExpressionComponentMask, -2200, 0)
    xy.set_editor_property("r", True)
    xy.set_editor_property("g", True)
    xy.set_editor_property("b", False)
    xy.set_editor_property("a", False)
    collega(posizione, "", xy, "")
    xy_globale[0] = xy

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
    # (08/10) R4: con l'erba vera sopra, il prato dipinto sotto era troppo chiaro e giallo: tra un ciuffo e l'altro e
    # oltre gli 80 m (dove l'erba vera finisce) si vedeva un tappeto pallido. Lo si scurisce verso il verde oliva dei fili.
    tinta = nodo(materiale, unreal.MaterialExpressionConstant3Vector, -1300, -950)
    tinta.set_editor_property("constant", unreal.LinearColor(ERBA_TINTA[0], ERBA_TINTA[1], ERBA_TINTA[2], 1.0))
    erba_scura = nodo(materiale, unreal.MaterialExpressionMultiply, -1150, -1050)
    collega(erba, "", erba_scura, "A")
    collega(tinta, "", erba_scura, "B")
    erba = erba_scura

    # Colore: erba -> terra (B) -> ghiaia (A) -> roccia (R) -> neve (G).
    c1 = lerp(materiale, erba, "", campioni["Terra"][0], "RGB", m, "B", -1000, -400)
    # (08/10) R4: la ghiaia ripetuta ogni 2,5 m faceva un "selciato" di celle tutte uguali: la si mescola a metà con
    # se stessa a 13 m (come l'erba), così le celle non si mettono più in fila.
    uv_ghiaia2 = uv_mondo(materiale, xy, 1300.0, -2000, -1300)
    ghiaia2 = campiona(materiale, colori["Ghiaia"], uv_ghiaia2, -1700, -1300, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    mezzo_g = nodo(materiale, unreal.MaterialExpressionConstant, -1500, -1350)
    mezzo_g.set_editor_property("r", 0.5)
    ghiaia = lerp(materiale, campioni["Ghiaia"][0], "RGB", ghiaia2, "RGB", mezzo_g, "", -1300, -1300)
    c2 = lerp(materiale, c1, "", ghiaia, "", m, "A", -800, -400)
    roccia = roccia_vera(materiale, colori["Roccia"], -1000, -2400)
    c3 = lerp(materiale, c2, "", roccia, "", m, "R", -600, -400)
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
    # (07/10) Bagnato (maschere2.R): la roccia e la ghiaia sotto gli spruzzi diventano più scure e lucide.
    m2 = None
    if maschere2 is not None:
        m2 = campiona(materiale, maschere2, uv_valle, -1800, -2900, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        scurito = nodo(materiale, unreal.MaterialExpressionMultiply, -150, -600)
        scurito.set_editor_property("const_b", 0.55)
        collega(finale, "", scurito, "A")
        finale = lerp(materiale, finale, "", scurito, "", m2, "R", -50, -450)
    mel.connect_material_property(finale, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Normali, con le stesse maschere.
    n1 = lerp(materiale, campioni["Erba"][1], "RGB", campioni["Terra"][1], "RGB", m, "B", -1000, 400)
    n2 = lerp(materiale, n1, "", campioni["Ghiaia"][1], "RGB", m, "A", -800, 400)
    piatta = nodo(materiale, unreal.MaterialExpressionConstant3Vector, -1000, 750)
    piatta.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 0.0))
    quanto = nodo(materiale, unreal.MaterialExpressionConstant, -1000, 850)
    quanto.set_editor_property("r", ROCCIA_PIATTA)
    roccia_n = lerp(materiale, campioni["Roccia"][1], "RGB", piatta, "", quanto, "", -800, 750)
    n3 = lerp(materiale, n2, "", roccia_n, "", m, "R", -600, 400)
    n4 = lerp(materiale, n3, "", campioni["Neve"][1], "RGB", m, "G", -400, 400)
    mel.connect_material_property(n4, "", unreal.MaterialProperty.MP_NORMAL)

    # Ruvidità: l'erba e la terra opache, la neve un poco lucida.
    ruvido = nodo(materiale, unreal.MaterialExpressionConstant, -600, 900)
    ruvido.set_editor_property("r", 0.9)
    lucido = nodo(materiale, unreal.MaterialExpressionConstant, -600, 1000)
    lucido.set_editor_property("r", 0.55)
    ruvidita = lerp(materiale, ruvido, "", lucido, "", m, "G", -400, 950)
    if m2 is not None:
        bagnata = nodo(materiale, unreal.MaterialExpressionConstant, -400, 1100)
        bagnata.set_editor_property("r", 0.25)
        ruvidita = lerp(materiale, ruvidita, "", bagnata, "", m2, "R", -250, 1000)
    mel.connect_material_property(ruvidita, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # (08/10) R4: dove seminare l'erba vera (LandscapeGrassType di crea_erba_valle.py). "Prato" = dove il terreno è
    # erba (né terra, né ghiaia, né roccia, né neve); "Felci" = prato bagnato (maschere2.R: lungo fiumi e cascata).
    erba_tipi = {}
    for nome in ("Prato", "Felci"):
        percorso_lgt = "/Game/Valle/Erba/LGT_" + nome
        if libreria.does_asset_exist(percorso_lgt):
            erba_tipi[nome] = libreria.load_asset(percorso_lgt)
    if erba_tipi:
        resto = None
        for canale in ("B", "A", "R", "G"):
            meno = nodo(materiale, unreal.MaterialExpressionOneMinus, -300, 1400 + len(canale) * 0)
            collega(m, canale, meno, "")
            if resto is None:
                resto = meno
            else:
                per = nodo(materiale, unreal.MaterialExpressionMultiply, -200, 1400)
                collega(resto, "", per, "A")
                collega(meno, "", per, "B")
                resto = per
        uscite = {"Prato": resto}
        if m2 is not None:
            felci = nodo(materiale, unreal.MaterialExpressionMultiply, -100, 1550)
            collega(resto, "", felci, "A")
            collega(m2, "R", felci, "B")
            uscite["Felci"] = felci
        erba = nodo(materiale, unreal.MaterialExpressionLandscapeGrassOutput, 100, 1400)
        voci = []
        for nome, tipo in erba_tipi.items():
            if nome not in uscite:
                continue
            voce = unreal.GrassInput()
            voce.set_editor_property("name", nome)
            voce.set_editor_property("grass_type", tipo)
            voci.append(voce)
        erba.set_editor_property("grass_types", voci, unreal.PropertyAccessChangeNotifyMode.ALWAYS)
        for voce in voci:
            nome = str(voce.get_editor_property("name"))
            if not mel.connect_material_expressions(uscite[nome], "", erba, nome):
                unreal.log_warning("[Valdorso] Valle: non riesco a collegare l'erba '{}'".format(nome))
        unreal.log("[Valdorso] Valle: erba vera su: " + ", ".join(str(v.get_editor_property("name")) for v in voci))
    else:
        unreal.log("[Valdorso] Valle: niente erba vera (manca /Game/Valle/Erba: prima crea_erba_valle.py)")

    mel.layout_material_expressions(materiale)
    mel.recompile_material(materiale)
    libreria.save_loaded_asset(materiale)
    unreal.log("[Valdorso] Valle: creato " + percorso)

    # 3. Sul paesaggio del livello aperto.
    for attore in paesaggi:
        attore.set_editor_property("landscape_material", materiale)
    unreal.log("[Valdorso] Valle: materiale messo sul paesaggio. Fatto.")


esegui()
