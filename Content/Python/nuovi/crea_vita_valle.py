# Valdorso - I materiali della vita della valle (v0.1.4, 10/11/2026). Scritto da Claude, chiesto da Fra: "rendere tutto
# piu' bello e vivo".
# Si lancia una volta nell'editor dal menu Valdorso -> Villaggio -> "Valle: fumo dei camini e lucciole (materiali)"
# (o Strumenti -> Esegui script), in qualsiasi livello. Si puo' rilanciare.
#
# Fa in /Game/Valle/Viva:
#   M_Fumo      il fumo dei camini: grigio, trasparente, morbido ai bordi; quanto si vede lo dice il dato 0 dell'istanza
#               (il C++ lo fa salire e svanire)
#   M_Lucciola  la luce delle lucciole: giallo-verde, si accende da sola (non serve il sole); quanto lo dice il dato 0
# Senza questi due materiali il gioco va lo stesso: niente fumo e niente lucciole (le farfalle e i petali si').
# La logica e' in AValdorsoVitaValle (C++).

import unreal

DEST = "/Game/Valle/Viva"
libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def nodo(m, classe, x, y):
    return mel.create_material_expression(m, classe, x, y)


def costante(m, valore, x, y):
    c = nodo(m, unreal.MaterialExpressionConstant, x, y)
    c.set_editor_property("r", valore)
    return c


def unisci(m, classe, a, b, x, y):
    n = nodo(m, classe, x, y)
    mel.connect_material_expressions(a, "", n, "A")
    mel.connect_material_expressions(b, "", n, "B")
    return n


def materiale(nome):
    percorso = "%s/%s" % (DEST, nome)
    m = unreal.load_asset(percorso) if libreria.does_asset_exist(percorso) else None
    if m is not None:
        mel.delete_all_material_expressions(m)
    else:
        m = strumenti.create_asset(nome, DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_instanced_static_meshes", True)
    return m


def dato_istanza(m, x, y):
    d = nodo(m, unreal.MaterialExpressionPerInstanceCustomData, x, y)
    d.set_editor_property("data_index", 0)
    return d


def materiale_fumo():
    m = materiale("M_Fumo")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", False)
    colore = nodo(m, unreal.MaterialExpressionVectorParameter, -600, -300)
    colore.set_editor_property("parameter_name", "Color")
    colore.set_editor_property("default_value", unreal.LinearColor(0.42, 0.41, 0.40, 1.0))
    mel.connect_material_property(colore, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(costante(m, 1.0, -600, -150), "", unreal.MaterialProperty.MP_ROUGHNESS)
    # bordi morbidi: quello che si vede di taglio (Fresnel alto) e' quasi trasparente
    fresnel = nodo(m, unreal.MaterialExpressionFresnel, -900, 100)
    try:
        fresnel.set_editor_property("exponent", 1.6)
    except Exception:  # noqa: BLE001
        pass
    morbido = nodo(m, unreal.MaterialExpressionOneMinus, -700, 100)
    mel.connect_material_expressions(fresnel, "", morbido, "")
    quanto = unisci(m, unreal.MaterialExpressionMultiply, morbido, dato_istanza(m, -900, 300), -500, 150)
    opaco = unisci(m, unreal.MaterialExpressionMultiply, quanto, costante(m, 0.55, -700, 400), -300, 200)
    mel.connect_material_property(opaco, "", unreal.MaterialProperty.MP_OPACITY)
    return m


def materiale_lucciola():
    m = materiale("M_Lucciola")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    colore = nodo(m, unreal.MaterialExpressionVectorParameter, -700, -200)
    colore.set_editor_property("parameter_name", "Color")
    colore.set_editor_property("default_value", unreal.LinearColor(0.75, 1.0, 0.22, 1.0))
    forte = unisci(m, unreal.MaterialExpressionMultiply, colore, costante(m, 14.0, -700, 0), -500, -100)
    luce = unisci(m, unreal.MaterialExpressionMultiply, forte, dato_istanza(m, -700, 150), -300, 0)
    mel.connect_material_property(luce, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return m


def esegui():
    if not libreria.does_directory_exist(DEST):
        libreria.make_directory(DEST)
    fatti = []
    for fai in (materiale_fumo, materiale_lucciola):
        try:
            m = fai()
            mel.recompile_material(m)
            libreria.save_loaded_asset(m)
            fatti.append(m.get_name())
        except Exception as e:  # noqa: BLE001
            unreal.log_error("[Valdorso] Vita della valle: %s non riuscito (%s)" % (fai.__name__, e))
    unreal.log("[Valdorso] Vita della valle: FATTO (%s in %s)" % (", ".join(fatti), DEST))


esegui()
