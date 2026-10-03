# Valdorso - Montagne vere per la scena del menu (Lvl_Menu), scritto da Claude il 03/10/2026.
# Strumenti -> Esegui script Python -> questo file (a gioco fermo, dopo aver compilato).
#
# Cosa fa:
#   - importa SM_MontiValdorso (V:/Texture/valdorso/SM_MontiValdorso.glb): un anello di montagne in 3D,
#     fatto da Claude con il rumore frattale (da 150 m a 1,5 km, cime fino a 240 m, neve sulle cime);
#   - accende Nanite sulla mesh (tanti triangoli senza pesare);
#   - crea M_Montagna: roccia (texture mossy_rock gia' importate) e neve dove la mesh lo dice;
#   - nel livello mette "Montagne" e toglie le sagome piatte di prima (MontiVicini e MontiLontani);
#   - allunga la distanza della nebbia, cosi' le montagne lontane sfumano nella foschia (il cielo resta pulito).
# Si puo' rilanciare: non duplica niente. Alla fine: "[Valdorso] Montagne: fatto".

import unreal

SORGENTE = "V:/Texture/valdorso/SM_MontiValdorso.glb"
CARTELLA = "/Game/Menu"
MONTAGNE = CARTELLA + "/Montagne"
ROCCIA = CARTELLA + "/Texture/mossy_rock"

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def scrivi(testo):
    unreal.log("[Valdorso] Montagne: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] Montagne: " + testo)


def collega(sorgente, uscita, destinazione, ingresso):
    try:
        mel.connect_material_expressions(sorgente, uscita, destinazione, ingresso)
    except Exception as errore:
        avviso("collegamento " + ingresso + " non riuscito (" + str(errore) + ")")


def trova(etichetta):
    for a in attori.get_all_level_actors():
        if a.get_actor_label() == etichetta:
            return a
    return None


def cerca_mesh():
    if not libreria.does_directory_exist(MONTAGNE):
        return None
    for percorso in libreria.list_assets(MONTAGNE, recursive=True):
        asset = unreal.load_asset(percorso.split(".")[0])
        if isinstance(asset, unreal.StaticMesh):
            return asset
    return None


def importa_mesh():
    mesh = cerca_mesh()
    if mesh is not None:
        return mesh
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", SORGENTE)
    compito.set_editor_property("destination_path", MONTAGNE)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])
    mesh = cerca_mesh()
    if mesh is None:
        avviso("non riesco a importare " + SORGENTE)
        return None
    try:
        nanite = mesh.get_editor_property("nanite_settings")
        nanite.set_editor_property("enabled", True)
        mesh.set_editor_property("nanite_settings", nanite)
        scrivi("Nanite acceso")
    except Exception as errore:
        avviso("Nanite non acceso (" + str(errore) + "), va bene lo stesso")
    libreria.save_loaded_asset(mesh)
    scrivi("importata la mesh " + mesh.get_name())
    return mesh


def carica_texture(nome):
    tex = unreal.load_asset(ROCCIA + "/" + nome + "." + nome)
    if tex is None:
        avviso("manca la texture " + nome)
    return tex


def crea_materiale():
    percorso = CARTELLA + "/M_Montagna"
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso)
    colore_t = carica_texture("mossy_rock_diff_4k")
    normale_t = carica_texture("mossy_rock_nor_dx_4k")
    arm_t = carica_texture("mossy_rock_arm_4k")
    if None in (colore_t, normale_t, arm_t):
        return None

    m = strumenti.create_asset("M_Montagna", CARTELLA, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("two_sided", True)

    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1500, 0)
    rip = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1500, 150)
    rip.set_editor_property("parameter_name", "Ripetizione")
    rip.set_editor_property("default_value", 1.0)
    scala = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -1300, 50)
    collega(uv, "", scala, "A")
    collega(rip, "", scala, "B")

    def campione(nome, tex, tipo, y):
        s = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -1100, y)
        s.set_editor_property("parameter_name", nome)
        s.set_editor_property("texture", tex)
        if tipo == "normale":
            s.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif tipo == "arm":
            s.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        collega(scala, "", s, "UVs")
        return s

    colore = campione("Colore", colore_t, "colore", -300)
    normale = campione("Normale", normale_t, "normale", 0)
    arm = campione("ARM", arm_t, "arm", 300)

    # La neve: la mesh porta nel colore dei vertici (rosso) dove c'e' neve.
    neve = mel.create_material_expression(m, unreal.MaterialExpressionVertexColor, -1100, 600)

    tinta = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, -500)
    tinta.set_editor_property("parameter_name", "Tinta")
    tinta.set_editor_property("default_value", unreal.LinearColor(0.45, 0.47, 0.5, 1.0))
    roccia = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -650, -350)
    collega(colore, "RGB", roccia, "A")
    collega(tinta, "", roccia, "B")

    bianco = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, -150)
    bianco.set_editor_property("parameter_name", "ColoreNeve")
    bianco.set_editor_property("default_value", unreal.LinearColor(0.75, 0.78, 0.85, 1.0))
    base = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, -300)
    collega(roccia, "", base, "A")
    collega(bianco, "", base, "B")
    collega(neve, "R", base, "Alpha")

    piatta = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -800, 150)
    piatta.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    norm = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, 0)
    collega(normale, "RGB", norm, "A")
    collega(piatta, "", norm, "B")
    collega(neve, "R", norm, "Alpha")

    liscia = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -800, 450)
    liscia.set_editor_property("r", 0.55)
    ruvido = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, 300)
    collega(arm, "G", ruvido, "A")
    collega(liscia, "", ruvido, "B")
    collega(neve, "R", ruvido, "Alpha")

    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(norm, "", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(ruvido, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(arm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    if hasattr(unreal, "MaterialExpressionSubstrateSlabBSDF") and hasattr(unreal.MaterialProperty, "MP_FRONT_MATERIAL"):
        lastra = mel.create_material_expression(m, unreal.MaterialExpressionSubstrateSlabBSDF, -100, 0)
        collega(base, "", lastra, "Diffuse Albedo")
        collega(ruvido, "", lastra, "Roughness")
        collega(norm, "", lastra, "Normal")
        mel.connect_material_property(lastra, "", unreal.MaterialProperty.MP_FRONT_MATERIAL)

    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("creato M_Montagna")
    return m


def esegui():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
        avviso("il gioco e' acceso: premi Stop e rilancia lo script")
        return

    mesh = importa_mesh()
    materiale = crea_materiale()
    if mesh is None or materiale is None:
        return

    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if mondo is None or mondo.get_name() != "Lvl_Menu":
        livelli.load_level(CARTELLA + "/Lvl_Menu")

    montagne = trova("Montagne")
    if montagne is None:
        montagne = attori.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator())
        montagne.set_actor_label("Montagne")
        montagne.set_folder_path("Scena")
        scrivi("messe le montagne")
    smc = montagne.static_mesh_component
    smc.set_static_mesh(mesh)
    smc.set_material(0, materiale)
    smc.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    for vecchi in ("MontiVicini", "MontiLontani"):
        a = trova(vecchi)
        if a is not None:
            attori.destroy_actor(a)
            scrivi("tolto " + vecchi)

    nebbia = trova("Nebbia")
    if nebbia is not None:
        try:
            nebbia.component.set_editor_property("fog_cutoff_distance", 180000.0)
        except Exception as errore:
            avviso("nebbia non cambiata (" + str(errore) + ")")

    livelli.save_current_level()
    scrivi("fatto")


esegui()
