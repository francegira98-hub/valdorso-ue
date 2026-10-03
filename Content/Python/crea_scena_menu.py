# Valdorso - Crea la scena del menu principale (Content/Menu/Lvl_Menu), scritta da Claude il 02/10/2026.
# Si lancia una volta sola in Valdorso: Strumenti -> Esegui script Python -> questo file.
# Prima: File -> Salva tutto (lo script apre un livello nuovo).
#
# Cosa crea:
#   - due materiali in Content/Menu: M_Luminoso (Colore e Intensita, si accende) e M_Pietra (pietra scura);
#   - il livello Lvl_Menu: terreno di pietra, altare, il frammento del Cuore che batte, sette pietre ritte
#     in cerchio, due bracieri, luna, nebbia volumetrica, esposizione fissa, la telecamera MenuCamera;
#   - nelle impostazioni del livello la modalita' di gioco del menu (ValdorsoMenuGameMode).
# Alla fine il Registro output scrive "[Valdorso] Scena del menu: fatto".

import math
import random
import unreal

CARTELLA = "/Game/Menu"
LIVELLO = CARTELLA + "/Lvl_Menu"

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def scrivi(testo):
    unreal.log("[Valdorso] Scena del menu: " + testo)


def collega_superficie(materiale, colore, emissivo, ruvidita):
    """Collega base, emissivo e ruvidita' sia al modo classico sia a Substrate (il progetto usa Substrate)."""
    mel.connect_material_property(colore, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if emissivo is not None:
        mel.connect_material_property(emissivo, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(ruvidita, "", unreal.MaterialProperty.MP_ROUGHNESS)

    if hasattr(unreal, "MaterialExpressionSubstrateSlabBSDF") and hasattr(unreal.MaterialProperty, "MP_FRONT_MATERIAL"):
        try:
            lastra = mel.create_material_expression(materiale, unreal.MaterialExpressionSubstrateSlabBSDF, 200, 0)
            mel.connect_material_expressions(colore, "", lastra, "Diffuse Albedo")
            mel.connect_material_expressions(ruvidita, "", lastra, "Roughness")
            if emissivo is not None:
                mel.connect_material_expressions(emissivo, "", lastra, "Emissive Color")
            mel.connect_material_property(lastra, "", unreal.MaterialProperty.MP_FRONT_MATERIAL)
            scrivi("materiale " + materiale.get_name() + " collegato anche a Substrate")
        except Exception as errore:
            unreal.log_warning("[Valdorso] Scena del menu: Substrate non collegato (" + str(errore) + ")")


def crea_materiale_luminoso():
    percorso = CARTELLA + "/M_Luminoso"
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso)
    m = strumenti.create_asset("M_Luminoso", CARTELLA, unreal.Material, unreal.MaterialFactoryNew())

    colore = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, 0)
    colore.set_editor_property("parameter_name", "Colore")
    colore.set_editor_property("default_value", unreal.LinearColor(1.0, 0.6, 0.25, 1.0))

    intensita = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 220)
    intensita.set_editor_property("parameter_name", "Intensita")
    intensita.set_editor_property("default_value", 5.0)

    per = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 120)
    mel.connect_material_expressions(colore, "", per, "A")
    mel.connect_material_expressions(intensita, "", per, "B")

    ruvido = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 320)
    ruvido.set_editor_property("r", 0.25)

    collega_superficie(m, colore, per, ruvido)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("creato M_Luminoso")
    return m


def crea_materiale_pietra():
    percorso = CARTELLA + "/M_Pietra"
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso)
    m = strumenti.create_asset("M_Pietra", CARTELLA, unreal.Material, unreal.MaterialFactoryNew())

    colore = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, 0)
    colore.set_editor_property("parameter_name", "Colore")
    colore.set_editor_property("default_value", unreal.LinearColor(0.045, 0.04, 0.036, 1.0))

    ruvido = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -600, 220)
    ruvido.set_editor_property("r", 0.92)

    collega_superficie(m, colore, None, ruvido)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("creato M_Pietra")
    return m


def oggetto(mesh, posizione, scala, materiale, etichetta, rotazione=None):
    a = attori.spawn_actor_from_class(unreal.StaticMeshActor, posizione, rotazione or unreal.Rotator())
    a.static_mesh_component.set_static_mesh(unreal.load_asset(mesh))
    a.static_mesh_component.set_material(0, materiale)
    a.set_actor_scale3d(scala)
    a.set_actor_label(etichetta)
    a.set_folder_path("Scena")
    return a


def mobile(componente):
    try:
        componente.set_mobility(unreal.ComponentMobility.MOVABLE)
    except Exception:
        pass


def imposta(oggetto_unreal, nomi, valore):
    """Prova piu' nomi della stessa proprieta' (cambiano tra le versioni di Unreal)."""
    for nome in nomi:
        try:
            oggetto_unreal.set_editor_property(nome, valore)
            return True
        except Exception:
            continue
    unreal.log_warning("[Valdorso] Scena del menu: proprieta' non trovata " + str(nomi))
    return False


def crea_scena():
    if libreria.does_asset_exist(LIVELLO):
        unreal.log_warning("[Valdorso] Scena del menu: Lvl_Menu esiste gia', non lo rifaccio. "
                           "Per rifarlo: cancellalo dal Cassetto dei contenuti e rilancia lo script.")
        return

    if not libreria.does_directory_exist(CARTELLA):
        libreria.make_directory(CARTELLA)

    luminoso = crea_materiale_luminoso()
    pietra = crea_materiale_pietra()

    if not livelli.new_level(LIVELLO):
        unreal.log_error("[Valdorso] Scena del menu: non riesco a creare Lvl_Menu")
        return
    scrivi("livello creato")

    random.seed(312)

    # Il terreno e l'altare.
    oggetto("/Engine/BasicShapes/Plane.Plane", unreal.Vector(0, 0, 0), unreal.Vector(80, 80, 1), pietra, "Terreno")
    oggetto("/Engine/BasicShapes/Cube.Cube", unreal.Vector(0, 0, 45), unreal.Vector(1.3, 1.3, 0.9), pietra, "Altare")
    oggetto("/Engine/BasicShapes/Cube.Cube", unreal.Vector(0, 0, 8), unreal.Vector(1.8, 1.8, 0.16), pietra, "Gradino")

    # Il frammento del Cuore, sopra l'altare.
    frammento = attori.spawn_actor_from_class(unreal.ValdorsoFrammentoCuore, unreal.Vector(0, 0, 90), unreal.Rotator())
    frammento.set_actor_label("FrammentoDelCuore")
    frammento.set_editor_property("materiale", luminoso)

    # Sette pietre ritte in cerchio, un po' storte, come lasciate dagli Antichi.
    for i in range(7):
        angolo = math.radians(i * (360.0 / 7.0) + 12.0)
        raggio = 560.0 + random.uniform(-40.0, 40.0)
        altezza = random.uniform(2.2, 3.1)
        posizione = unreal.Vector(math.cos(angolo) * raggio, math.sin(angolo) * raggio, altezza * 50.0 - 15.0)
        rotazione = unreal.Rotator(roll=random.uniform(-6, 6), pitch=random.uniform(-5, 5), yaw=math.degrees(angolo) + 90.0)
        oggetto("/Engine/BasicShapes/Cube.Cube", posizione, unreal.Vector(0.75, 0.38, altezza), pietra, "PietraRitta" + str(i + 1), rotazione)

    # Due bracieri ai lati dell'altare, verso chi guarda.
    for nome, y in (("BraciereSinistro", -260.0), ("BraciereDestro", 260.0)):
        b = attori.spawn_actor_from_class(unreal.ValdorsoBraciere, unreal.Vector(-200, y, 0), unreal.Rotator())
        b.set_actor_label(nome)
        b.set_editor_property("materiale_pietra", pietra)
        b.set_editor_property("materiale_braci", luminoso)

    # La luna: fredda, bassa, con le ombre lunghe.
    luna = attori.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 600),
                                         unreal.Rotator(roll=0, pitch=-32, yaw=55))
    luna.set_actor_label("Luna")
    lc = luna.light_component
    mobile(lc)
    lc.set_intensity(0.8)
    lc.set_light_color(unreal.LinearColor(0.55, 0.66, 1.0, 1.0))

    # La nebbia, con la nebbia volumetrica che fa vedere la luce del frammento e dei bracieri.
    nebbia = attori.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator())
    nebbia.set_actor_label("Nebbia")
    nc = nebbia.component
    imposta(nc, ["fog_density"], 0.06)
    imposta(nc, ["fog_height_falloff"], 0.35)
    imposta(nc, ["fog_inscattering_luminance", "fog_inscattering_color"], unreal.LinearColor(0.012, 0.018, 0.035, 1.0))
    imposta(nc, ["volumetric_fog"], True)
    imposta(nc, ["volumetric_fog_extinction_scale"], 1.5)

    # L'esposizione fissa: la notte resta notte, senza che l'occhio della telecamera schiarisca tutto.
    pp = attori.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator())
    pp.set_actor_label("Atmosfera")
    pp.set_editor_property("unbound", True)
    impostazioni = pp.get_editor_property("settings")
    for nome, valore in (("override_auto_exposure_min_brightness", True), ("auto_exposure_min_brightness", -1.0),
                         ("override_auto_exposure_max_brightness", True), ("auto_exposure_max_brightness", -1.0),
                         ("override_bloom_intensity", True), ("bloom_intensity", 1.2),
                         ("override_vignette_intensity", True), ("vignette_intensity", 0.55)):
        imposta(impostazioni, [nome], valore)
    pp.set_editor_property("settings", impostazioni)

    # La telecamera: guarda un po' a sinistra del frammento, cosi' il frammento sta a destra e il menu a sinistra.
    occhio = unreal.Vector(-980, -120, 210)
    bersaglio = unreal.Vector(0, -330, 120)
    sguardo = unreal.MathLibrary.find_look_at_rotation(occhio, bersaglio)
    camera = attori.spawn_actor_from_class(unreal.CameraActor, occhio, sguardo)
    camera.set_actor_label("MenuCamera")
    camera.set_editor_property("tags", [unreal.Name("MenuCamera")])
    imposta(camera.camera_component, ["field_of_view"], 50.0)
    imposta(camera.camera_component, ["constrain_aspect_ratio"], False)

    # Le impostazioni del livello: modalita' di gioco del menu, nessuna luce precalcolata.
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    impostazioni_mondo = mondo.get_world_settings()
    impostazioni_mondo.set_editor_property("default_game_mode", unreal.ValdorsoMenuGameMode)
    imposta(impostazioni_mondo, ["force_no_precomputed_lighting"], True)

    livelli.save_current_level()
    scrivi("fatto")


crea_scena()
