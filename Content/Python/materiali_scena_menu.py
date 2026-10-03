# Valdorso - Materiali veri e cielo per la scena del menu (Lvl_Menu), scritto da Claude il 02/10/2026.
# Si lancia una volta sola in Valdorso: Strumenti -> Esegui script Python -> questo file.
# Prima: File -> Salva tutto.
#
# Cosa fa:
#   - importa le texture di Poly Haven (CC0) da V:/Texture in Content/Menu/Texture, con le impostazioni giuste
#     (colore in sRGB; normale come normal map; ARM come maschere);
#   - crea il materiale M_PietraPBR (colore, normale, ARM, ripetizione orizzontale e verticale, tinta)
#     e le sue istanze MI_PietreRitte, MI_Altare, MI_Pavimento, MI_Terreno;
#   - importa il cielo notturno (HDR) e crea M_Cielo, un cielo con le stelle che la nebbia non copre;
#   - nel livello Lvl_Menu: mette i materiali su pietre, altare, gradino, terreno e bracieri,
#     aggiunge un pavimento di pietra intorno all'altare, la volta del cielo e la luce del cielo.
# Alla fine il Registro output scrive "[Valdorso] Materiali del menu: fatto".

import unreal

SORGENTE = "V:/Texture"
CARTELLA = "/Game/Menu"
TEXTURE = CARTELLA + "/Texture"
LIVELLO = CARTELLA + "/Lvl_Menu"

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

# nome del materiale -> risoluzione dei file scaricati
MATERIALI = {
    "mossy_rock": "4k",
    "medieval_blocks_02": "4k",
    "monastery_stone_floor": "4k",
    "forest_ground_04": "2k",
}


def scrivi(testo):
    unreal.log("[Valdorso] Materiali del menu: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] Materiali del menu: " + testo)


def importa(file, destinazione):
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", file)
    compito.set_editor_property("destination_path", destinazione)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])
    nome = file.replace("\\", "/").split("/")[-1].rsplit(".", 1)[0]
    asset = unreal.load_asset(destinazione + "/" + nome + "." + nome)
    if asset is None:
        avviso("non riesco a importare " + file)
    return asset


def prepara_texture(tex, tipo):
    if tipo == "normale":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif tipo == "arm":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    libreria.save_loaded_asset(tex)


def collega(sorgente, uscita, destinazione, ingresso):
    try:
        mel.connect_material_expressions(sorgente, uscita, destinazione, ingresso)
        return True
    except Exception as errore:
        avviso("collegamento " + ingresso + " non riuscito (" + str(errore) + ")")
        return False


def crea_materiale_pbr(esempio):
    """M_PietraPBR: un solo materiale per tutte le pietre; ogni istanza sceglie texture e ripetizione."""
    percorso = CARTELLA + "/M_PietraPBR"
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso)
    m = strumenti.create_asset("M_PietraPBR", CARTELLA, unreal.Material, unreal.MaterialFactoryNew())

    # Le coordinate: si ripetono di RipetizioneU in orizzontale e RipetizioneV in verticale.
    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    ru = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1400, 150)
    ru.set_editor_property("parameter_name", "RipetizioneU")
    ru.set_editor_property("default_value", 1.0)
    rv = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1400, 250)
    rv.set_editor_property("parameter_name", "RipetizioneV")
    rv.set_editor_property("default_value", 1.0)
    unisci = mel.create_material_expression(m, unreal.MaterialExpressionAppendVector, -1200, 200)
    collega(ru, "", unisci, "A")
    collega(rv, "", unisci, "B")
    scala = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -1050, 100)
    collega(uv, "", scala, "A")
    collega(unisci, "", scala, "B")

    def campione(nome, tipo, y):
        s = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, y)
        s.set_editor_property("parameter_name", nome)
        s.set_editor_property("texture", esempio[tipo])
        if tipo == "normale":
            s.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif tipo == "arm":
            s.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        collega(scala, "", s, "UVs")
        return s

    colore = campione("Colore", "colore", -200)
    normale = campione("Normale", "normale", 100)
    arm = campione("ARM", "arm", 400)

    tinta = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, -450)
    tinta.set_editor_property("parameter_name", "Tinta")
    tinta.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    tinto = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -500, -300)
    collega(colore, "RGB", tinto, "A")
    collega(tinta, "", tinto, "B")

    # Il modo classico...
    mel.connect_material_property(tinto, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(normale, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(arm, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(arm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    # ...e Substrate, che il progetto usa.
    if hasattr(unreal, "MaterialExpressionSubstrateSlabBSDF") and hasattr(unreal.MaterialProperty, "MP_FRONT_MATERIAL"):
        lastra = mel.create_material_expression(m, unreal.MaterialExpressionSubstrateSlabBSDF, -150, 0)
        collega(tinto, "", lastra, "Diffuse Albedo")
        collega(arm, "G", lastra, "Roughness")
        collega(normale, "RGB", lastra, "Normal")
        mel.connect_material_property(lastra, "", unreal.MaterialProperty.MP_FRONT_MATERIAL)

    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("creato M_PietraPBR")
    return m


def crea_istanza(nome, genitore, tex, ru, rv, tinta):
    percorso = CARTELLA + "/" + nome
    if libreria.does_asset_exist(percorso):
        mi = unreal.load_asset(percorso)
    else:
        mi = strumenti.create_asset(nome, CARTELLA, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, genitore)
    mel.set_material_instance_texture_parameter_value(mi, "Colore", tex["colore"])
    mel.set_material_instance_texture_parameter_value(mi, "Normale", tex["normale"])
    mel.set_material_instance_texture_parameter_value(mi, "ARM", tex["arm"])
    mel.set_material_instance_scalar_parameter_value(mi, "RipetizioneU", ru)
    mel.set_material_instance_scalar_parameter_value(mi, "RipetizioneV", rv)
    mel.set_material_instance_vector_parameter_value(mi, "Tinta", tinta)
    mel.update_material_instance(mi)
    libreria.save_loaded_asset(mi)
    return mi


def crea_cielo(cubo):
    """M_Cielo: la foto del cielo stellato su una sfera enorme, accesa, che la nebbia non copre."""
    percorso = CARTELLA + "/M_Cielo"
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso)
    m = strumenti.create_asset("M_Cielo", CARTELLA, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    try:
        m.set_editor_property("is_sky", True)
    except Exception:
        avviso("is_sky non trovato")

    direzione = mel.create_material_expression(m, unreal.MaterialExpressionCameraVectorWS, -900, 0)
    meno = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -700, 0)
    collega(direzione, "", meno, "A")
    meno.set_editor_property("const_b", -1.0)

    campione = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameterCube, -500, 0)
    campione.set_editor_property("parameter_name", "Cielo")
    campione.set_editor_property("texture", cubo)
    collega(meno, "", campione, "UVs")

    luce = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -500, 250)
    luce.set_editor_property("parameter_name", "Luminosita")
    luce.set_editor_property("default_value", 6.0)
    acceso = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 100)
    collega(campione, "RGB", acceso, "A")
    collega(luce, "", acceso, "B")

    mel.connect_material_property(acceso, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if hasattr(unreal, "MaterialExpressionSubstrateUnlitBSDF") and hasattr(unreal.MaterialProperty, "MP_FRONT_MATERIAL"):
        spento = mel.create_material_expression(m, unreal.MaterialExpressionSubstrateUnlitBSDF, 0, 0)
        collega(acceso, "", spento, "Emissive Color")
        mel.connect_material_property(spento, "", unreal.MaterialProperty.MP_FRONT_MATERIAL)

    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("creato M_Cielo")
    return m


def oggetto(mesh, posizione, scala, materiale, etichetta):
    a = attori.spawn_actor_from_class(unreal.StaticMeshActor, posizione, unreal.Rotator())
    smc = a.static_mesh_component
    smc.set_static_mesh(unreal.load_asset(mesh))
    smc.set_material(0, materiale)
    a.set_actor_scale3d(scala)
    a.set_actor_label(etichetta)
    a.set_folder_path("Scena")
    return a


def esegui():
    # 1. Le texture.
    texture = {}
    for nome, ris in MATERIALI.items():
        dest = TEXTURE + "/" + nome
        base = SORGENTE + "/" + nome + "/" + nome
        tex = {
            "colore": importa(base + "_diff_" + ris + ".jpg", dest),
            "normale": importa(base + "_nor_dx_" + ris + ".jpg", dest),
            "arm": importa(base + "_arm_" + ris + ".jpg", dest),
        }
        if None in tex.values():
            avviso("manca qualcosa di " + nome + ", mi fermo")
            return
        prepara_texture(tex["normale"], "normale")
        prepara_texture(tex["arm"], "arm")
        texture[nome] = tex
        scrivi("importato " + nome)

    cubo = importa(SORGENTE + "/cielo/qwantani_night_puresky_8k.hdr", TEXTURE + "/cielo")
    if cubo is None:
        return
    cubo.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_HDR_COMPRESSED)
    cubo.set_editor_property("max_texture_size", 4096)
    libreria.save_loaded_asset(cubo)
    scrivi("importato il cielo")

    # 2. I materiali.
    pbr = crea_materiale_pbr(texture["mossy_rock"])
    bianco = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    mi_pietre = crea_istanza("MI_PietreRitte", pbr, texture["mossy_rock"], 1.0, 2.5, unreal.LinearColor(0.75, 0.75, 0.75, 1.0))
    mi_altare = crea_istanza("MI_Altare", pbr, texture["medieval_blocks_02"], 1.0, 0.8, bianco)
    mi_pavimento = crea_istanza("MI_Pavimento", pbr, texture["monastery_stone_floor"], 3.0, 3.0, unreal.LinearColor(0.8, 0.8, 0.8, 1.0))
    mi_terreno = crea_istanza("MI_Terreno", pbr, texture["forest_ground_04"], 40.0, 40.0, unreal.LinearColor(0.6, 0.6, 0.6, 1.0))
    cielo = crea_cielo(cubo)
    scrivi("istanze pronte")

    # 3. Il livello.
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if mondo is None or mondo.get_name() != "Lvl_Menu":
        livelli.load_level(LIVELLO)

    for a in attori.get_all_level_actors():
        nome = a.get_actor_label()
        if nome.startswith("PietraRitta"):
            a.static_mesh_component.set_material(0, mi_pietre)
        elif nome == "Altare":
            a.static_mesh_component.set_material(0, mi_altare)
        elif nome == "Gradino":
            a.static_mesh_component.set_material(0, mi_altare)
        elif nome == "Terreno":
            a.static_mesh_component.set_material(0, mi_terreno)
        elif isinstance(a, unreal.ValdorsoBraciere):
            a.set_editor_property("materiale_pietra", mi_altare)

    # Il pavimento di pietra intorno all'altare (un disco basso).
    oggetto("/Engine/BasicShapes/Cylinder.Cylinder", unreal.Vector(0, 0, 1), unreal.Vector(7.0, 7.0, 0.03), mi_pavimento, "Pavimento")

    # La volta del cielo: una sfera enorme, senza urti e senza ombre.
    volta = oggetto("/Engine/BasicShapes/Sphere.Sphere", unreal.Vector(0, 0, 0), unreal.Vector(4000, 4000, 4000), cielo, "VoltaDelCielo")
    volta.static_mesh_component.set_editor_property("cast_shadow", False)
    volta.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    # La luce del cielo: illumina la scena con i colori del cielo notturno.
    cielo_luce = attori.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300), unreal.Rotator())
    cielo_luce.set_actor_label("LuceDelCielo")
    slc = cielo_luce.light_component
    try:
        slc.set_mobility(unreal.ComponentMobility.MOVABLE)
    except Exception:
        pass
    slc.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    slc.set_editor_property("cubemap", cubo)
    slc.set_intensity(1.0)
    slc.recapture_sky()

    livelli.save_current_level()
    scrivi("fatto")


esegui()
