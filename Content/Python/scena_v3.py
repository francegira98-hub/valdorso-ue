# Valdorso - Scena del menu, terzo giro (Lvl_Menu), scritto da Claude il 03/10/2026.
# Strumenti -> Esegui script Python -> questo file (a gioco fermo).
#
# Cosa fa:
#   - montagne nuove (SM_MontiValdorso2): piu' basse, boschi scuri sui pendii, roccia che cambia, neve solo in cima;
#   - fuoco e rune rifatti come materiali "mascherati" (quelli "additivi" con Substrate non si vedevano);
#   - frammento meno bianco;
#   - l'Orso vero: importa il modello animato (V:/Mixamo/Orso), gli da' i suoi colori e la camminata lenta.
# Si puo' rilanciare: non duplica niente. Alla fine: "[Valdorso] Scena v3: fatto".

import unreal

SORGENTE = "V:/Texture/valdorso"
ORSO_FBX = "V:/Mixamo/Orso/source/Bear Animated.fbx"
ORSO_TEX = "V:/Mixamo/Orso/textures"
CARTELLA = "/Game/Menu"
ORSO = CARTELLA + "/Orso"
TEXTURE = CARTELLA + "/Texture/valdorso"
LIVELLO = CARTELLA + "/Lvl_Menu"

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

SUBSTRATE = hasattr(unreal, "MaterialExpressionSubstrateUnlitBSDF") and hasattr(unreal.MaterialProperty, "MP_FRONT_MATERIAL")


def scrivi(testo):
    unreal.log("[Valdorso] Scena v3: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] Scena v3: " + testo)


def imposta(oggetto, nome, valore):
    try:
        oggetto.set_editor_property(nome, valore)
        return True
    except Exception as errore:
        avviso("proprieta' " + nome + " non impostata (" + str(errore) + ")")
        return False


def collega(sorgente, uscita, destinazione, ingresso):
    try:
        mel.connect_material_expressions(sorgente, uscita, destinazione, ingresso)
        return True
    except Exception as errore:
        avviso("collegamento " + ingresso + " non riuscito (" + str(errore) + ")")
        return False


# ------------------------------------------------------------------------------------------
# Texture
# ------------------------------------------------------------------------------------------

def importa(nome_file):
    nome = nome_file.rsplit(".", 1)[0]
    percorso = TEXTURE + "/" + nome
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso + "." + nome)
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", SORGENTE + "/" + nome_file)
    compito.set_editor_property("destination_path", TEXTURE)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])
    asset = unreal.load_asset(percorso + "." + nome)
    if asset is None:
        avviso("non riesco a importare " + nome_file)
    return asset


def prepara_cielo(tex):
    # Niente mipmap: la proiezione del cielo li confonderebbe (si vedrebbe una riga).
    imposta(tex, "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    imposta(tex, "compression_settings", unreal.TextureCompressionSettings.TC_BC7)
    imposta(tex, "srgb", True)
    libreria.save_loaded_asset(tex)


# ------------------------------------------------------------------------------------------
# Materiali
# ------------------------------------------------------------------------------------------

def nuovo_materiale(nome, fusione, due_lati=True):
    """Restituisce (materiale, True) se l'ha appena creato, (materiale, False) se c'era gia'."""
    percorso = CARTELLA + "/" + nome
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso), False
    m = strumenti.create_asset(nome, CARTELLA, unreal.Material, unreal.MaterialFactoryNew())
    imposta(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    imposta(m, "blend_mode", fusione)
    imposta(m, "two_sided", due_lati)
    return m, True


def scalare(m, nome, valore, x, y):
    p = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", nome)
    p.set_editor_property("default_value", valore)
    return p


def vettore(m, nome, colore, x, y):
    p = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    p.set_editor_property("parameter_name", nome)
    p.set_editor_property("default_value", colore)
    return p


def posizione_locale(m, x, y):
    """La posizione nel pezzo stesso (il piano va da -50 a +50 cm), qualunque sia la sua rotazione."""
    if hasattr(unreal, "MaterialExpressionLocalPosition"):
        return mel.create_material_expression(m, unreal.MaterialExpressionLocalPosition, x, y)
    avviso("nodo Local Position non trovato: uso le coordinate della texture")
    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, x, y)
    per = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, x + 150, y)
    collega(uv, "", per, "A")
    per.set_editor_property("const_b", 100.0)
    meno = mel.create_material_expression(m, unreal.MaterialExpressionSubtract, x + 300, y)
    collega(per, "", meno, "A")
    meno.set_editor_property("const_b", 50.0)
    return meno


def codice(m, testo, ingressi, uscita, x, y, descrizione):
    """Un nodo Custom: qualche riga di HLSL, con gli ingressi chiamati per nome."""
    c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("code", testo)
    c.set_editor_property("output_type", uscita)
    c.set_editor_property("description", descrizione)
    lista = []
    for voce in ingressi:
        i = unreal.CustomInput()
        i.set_editor_property("input_name", voce[0])
        lista.append(i)
    c.set_editor_property("inputs", lista)
    for voce in ingressi:
        # (nome, nodo) oppure (nome, nodo, uscita del nodo)
        collega(voce[1], voce[2] if len(voce) > 2 else "", c, voce[0])
    return c


def uscita_accesa(m, emissivo, maschera=None):
    """Collega l'emissivo (e la maschera) sia al modo classico sia a Substrate."""
    mel.connect_material_property(emissivo, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if maschera is not None:
        mel.connect_material_property(maschera, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    if SUBSTRATE:
        spento = mel.create_material_expression(m, unreal.MaterialExpressionSubstrateUnlitBSDF, 400, 0)
        collega(emissivo, "", spento, "Emissive Color")
        mel.connect_material_property(spento, "", unreal.MaterialProperty.MP_FRONT_MATERIAL)


def chiudi(m, nome):
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("creato " + nome)


FLOAT1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
FLOAT2 = unreal.CustomMaterialOutputType.CMOT_FLOAT2
FLOAT3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3


FUOCO_HLSL = """
float h = P.x / 100.0 + 0.5;
float r = abs(P.y) / 50.0;
float t = Tempo * 1.7 + Seme;
float2 x = float2(P.y / 50.0 * 2.4 + Seme * 3.1, h * 4.0 - t * 2.2);
float n = 0.0;
float a = 0.5;
for (int i = 0; i < 4; i++)
{
    float2 c = floor(x);
    float2 f = frac(x);
    f = f * f * (3.0 - 2.0 * f);
    float h00 = frac(sin(dot(c, float2(127.1, 311.7))) * 43758.5453);
    float h10 = frac(sin(dot(c + float2(1.0, 0.0), float2(127.1, 311.7))) * 43758.5453);
    float h01 = frac(sin(dot(c + float2(0.0, 1.0), float2(127.1, 311.7))) * 43758.5453);
    float h11 = frac(sin(dot(c + float2(1.0, 1.0), float2(127.1, 311.7))) * 43758.5453);
    n += a * lerp(lerp(h00, h10, f.x), lerp(h01, h11, f.x), f.y);
    x = x * 2.03 + float2(1.7, 9.2);
    a *= 0.5;
}
float ondeggio = (n - 0.5) * 0.7 * h;
float rr = abs(P.y / 50.0 + ondeggio);
float larghezza = lerp(0.85, 0.05, pow(saturate(h), 0.7));
float forma = saturate(1.0 - rr / larghezza);
float v = saturate(forma * 1.15 + (n - 0.5) * 2.4 - h * 0.95);
v *= saturate(h * 6.0) * saturate((1.0 - h) * 3.0);
v = v * v;
float3 colore = lerp(float3(0.85, 0.10, 0.015), float3(1.0, 0.48, 0.09), saturate(v * 1.6));
colore = lerp(colore, float3(1.0, 0.86, 0.55), saturate(v * 2.4 - 1.4));
return colore * v * Intensita;
"""



FUOCO_COLORE = FUOCO_HLSL
FUOCO_MASCHERA = FUOCO_HLSL.replace(
    "return colore * v * Intensita;",
    "float d = frac(sin(dot(floor(Parameters.SvPosition.xy), float2(12.9898, 78.233))) * 43758.5453);\n"
    "return (v * 2.5 > d * 0.9 + 0.05) ? 1.0 : 0.0;")


def nuovo_mascherato(nome):
    return nuovo_materiale(nome, unreal.BlendMode.BLEND_MASKED)


def crea_fuoco2():
    m, nuovo = nuovo_mascherato("M_Fuoco2")
    if not nuovo:
        return m
    p = posizione_locale(m, -1300, 0)
    tempo = mel.create_material_expression(m, unreal.MaterialExpressionTime, -1300, 200)
    seme = scalare(m, "Seme", 0.0, -1300, 350)
    forza = scalare(m, "Intensita", 2.5, -1300, 500)
    ingressi = [("P", p), ("Tempo", tempo), ("Seme", seme), ("Intensita", forza)]
    colore = codice(m, FUOCO_COLORE, ingressi, FLOAT3, -800, 0, "Fuoco: colore")
    maschera = codice(m, FUOCO_MASCHERA, ingressi, FLOAT1, -800, 300, "Fuoco: forma")
    uscita_accesa(m, colore, maschera)
    chiudi(m, "M_Fuoco2")
    return m


def crea_rune2():
    m, nuovo = nuovo_materiale("M_Rune2", unreal.BlendMode.BLEND_MASKED, False)
    if not nuovo:
        return m
    t_rune = unreal.load_asset(TEXTURE + "/T_CerchioRune.T_CerchioRune")
    p = posizione_locale(m, -1300, 0)
    uv = codice(m, "return float2(0.5 + P.y / 100.0, 0.5 - P.x / 100.0);",
                [("P", p)], FLOAT2, -900, 0, "Rune: coordinate dal piano")
    campione = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    campione.set_editor_property("parameter_name", "Rune")
    campione.set_editor_property("texture", t_rune)
    collega(uv, "", campione, "UVs")
    colore = vettore(m, "Colore", unreal.LinearColor(1.0, 0.55, 0.2, 1.0), -600, 300)
    forza = scalare(m, "Intensita", 1.5, -600, 500)
    a = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 100)
    collega(campione, "R", a, "A")
    collega(colore, "", a, "B")
    b = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -150, 200)
    collega(a, "", b, "A")
    collega(forza, "", b, "B")
    maschera = codice(m,
                      "float d = frac(sin(dot(floor(Parameters.SvPosition.xy), float2(12.9898, 78.233))) * 43758.5453);\n"
                      "return (A > d * 0.7 + 0.15) ? 1.0 : 0.0;",
                      [("A", campione, "R")], FLOAT1, -300, 400, "Rune: dove c'e' il segno")
    uscita_accesa(m, b, maschera)
    chiudi(m, "M_Rune2")
    return m


# ------------------------------------------------------------------------------------------
# Montagne
# ------------------------------------------------------------------------------------------

def primo_asset(cartella, classe, filtro=None):
    if not libreria.does_directory_exist(cartella):
        return None
    for percorso in libreria.list_assets(cartella, recursive=True):
        asset = unreal.load_asset(percorso.split(".")[0])
        if isinstance(asset, classe) and (filtro is None or filtro(asset)):
            return asset
    return None


def importa_file(file, cartella):
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", file)
    compito.set_editor_property("destination_path", cartella)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])


def montagne2():
    cartella = CARTELLA + "/Montagne2"
    mesh = primo_asset(cartella, unreal.StaticMesh)
    if mesh is None:
        importa_file(SORGENTE + "/SM_MontiValdorso2.glb", cartella)
        mesh = primo_asset(cartella, unreal.StaticMesh)
        if mesh is None:
            avviso("non riesco a importare SM_MontiValdorso2.glb")
            return None, None
        try:
            nanite = mesh.get_editor_property("nanite_settings")
            nanite.set_editor_property("enabled", True)
            mesh.set_editor_property("nanite_settings", nanite)
        except Exception as errore:
            avviso("Nanite non acceso (" + str(errore) + ")")
        libreria.save_loaded_asset(mesh)
        scrivi("importate le montagne nuove")

    percorso = CARTELLA + "/M_Montagna2"
    if libreria.does_asset_exist(percorso):
        return mesh, unreal.load_asset(percorso)

    roccia = CARTELLA + "/Texture/mossy_rock/"
    colore_t = unreal.load_asset(roccia + "mossy_rock_diff_4k")
    normale_t = unreal.load_asset(roccia + "mossy_rock_nor_dx_4k")
    arm_t = unreal.load_asset(roccia + "mossy_rock_arm_4k")

    m = strumenti.create_asset("M_Montagna2", CARTELLA, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("two_sided", True)
    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1600, 0)

    def campione(nome, tex, tipo, y):
        s = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -1300, y)
        s.set_editor_property("parameter_name", nome)
        s.set_editor_property("texture", tex)
        if tipo == "normale":
            s.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif tipo == "arm":
            s.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        collega(uv, "", s, "UVs")
        return s

    tex = campione("Colore", colore_t, "colore", -400)
    nor = campione("Normale", normale_t, "normale", -100)
    arm = campione("ARM", arm_t, "arm", 200)
    vc = mel.create_material_expression(m, unreal.MaterialExpressionVertexColor, -1300, 500)

    # La roccia: due toni scuri, mescolati dalla variazione (blu dei vertici).
    r1 = vettore(m, "RocciaScura", unreal.LinearColor(0.10, 0.095, 0.09, 1.0), -1000, -700)
    r2 = vettore(m, "RocciaChiara", unreal.LinearColor(0.22, 0.20, 0.18, 1.0), -1000, -550)
    tono = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -800, -600)
    collega(r1, "", tono, "A"); collega(r2, "", tono, "B"); collega(vc, "B", tono, "Alpha")
    roccia = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -600, -500)
    collega(tex, "RGB", roccia, "A"); collega(tono, "", roccia, "B")
    # Il bosco: verde cupo (verde dei vertici).
    verde = vettore(m, "Bosco", unreal.LinearColor(0.10, 0.17, 0.07, 1.0), -1000, -350)
    bosco = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -600, -300)
    collega(tex, "RGB", bosco, "A"); collega(verde, "", bosco, "B")
    misto = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, -400)
    collega(roccia, "", misto, "A"); collega(bosco, "", misto, "B"); collega(vc, "G", misto, "Alpha")
    # La neve (rosso dei vertici), un po' spenta: e' notte.
    neve = vettore(m, "Neve", unreal.LinearColor(0.42, 0.45, 0.52, 1.0), -1000, -150)
    base = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -200, -300)
    collega(misto, "", base, "A"); collega(neve, "", base, "B"); collega(vc, "R", base, "Alpha")

    piatta = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -800, 0)
    piatta.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    normale = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, 0)
    collega(nor, "RGB", normale, "A"); collega(piatta, "", normale, "B"); collega(vc, "R", normale, "Alpha")

    ruvido = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -800, 300)
    ruvido.set_editor_property("r", 0.85)

    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(normale, "", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(ruvido, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(arm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    if SUBSTRATE:
        lastra = mel.create_material_expression(m, unreal.MaterialExpressionSubstrateSlabBSDF, 100, 0)
        collega(base, "", lastra, "Diffuse Albedo")
        collega(ruvido, "", lastra, "Roughness")
        collega(normale, "", lastra, "Normal")
        mel.connect_material_property(lastra, "", unreal.MaterialProperty.MP_FRONT_MATERIAL)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    scrivi("creato M_Montagna2")
    return mesh, m


# ------------------------------------------------------------------------------------------
# L'Orso
# ------------------------------------------------------------------------------------------

def texture_orso(nome):
    percorso = ORSO + "/" + nome
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso)
    importa_file(ORSO_TEX + "/" + nome + ".png", ORSO)
    return unreal.load_asset(percorso)


def materiale_orso(nome, tex, pelo):
    percorso = ORSO + "/" + nome
    if libreria.does_asset_exist(percorso):
        return unreal.load_asset(percorso)
    m = strumenti.create_asset(nome, ORSO, unreal.Material, unreal.MaterialFactoryNew())
    if pelo:
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        m.set_editor_property("two_sided", True)
    s = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
    s.set_editor_property("parameter_name", "Colore")
    s.set_editor_property("texture", tex)
    ruvido = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -700, 300)
    ruvido.set_editor_property("r", 0.9)
    mel.connect_material_property(s, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(ruvido, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if pelo:
        mel.connect_material_property(s, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    if SUBSTRATE:
        lastra = mel.create_material_expression(m, unreal.MaterialExpressionSubstrateSlabBSDF, -300, 0)
        collega(s, "RGB", lastra, "Diffuse Albedo")
        collega(ruvido, "", lastra, "Roughness")
        mel.connect_material_property(lastra, "", unreal.MaterialProperty.MP_FRONT_MATERIAL)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    return m


def orso():
    corpo = primo_asset(ORSO, unreal.SkeletalMesh)
    if corpo is None:
        scrivi("importo l'orso (81 animazioni: ci vuole un po')")
        importa_file(ORSO_FBX, ORSO)
        corpo = primo_asset(ORSO, unreal.SkeletalMesh)
        if corpo is None:
            avviso("non riesco a importare l'orso")
            return None, None, 1.0

    # I colori: il file dell'orso cerca le texture in una cartella che non c'e', li rimettiamo noi.
    m_corpo = materiale_orso("M_Orso", texture_orso("Bear_color"), False)
    m_pelo = materiale_orso("M_OrsoPelo", texture_orso("Bear_fur_color"), True)
    try:
        slot = corpo.get_editor_property("materials")
        for s in slot:
            nome = str(s.get_editor_property("material_slot_name")).lower()
            s.set_editor_property("material_interface", m_pelo if "fur" in nome else m_corpo)
            scrivi("slot " + nome + (" -> pelo" if "fur" in nome else " -> corpo"))
        corpo.set_editor_property("materials", slot)
        libreria.save_loaded_asset(corpo)
    except Exception as errore:
        avviso("materiali dell'orso non assegnati (" + str(errore) + ")")

    # La camminata lenta, ferma sul posto (a farlo avanzare ci pensa il codice).
    def e_camminata(nome_cercato):
        return lambda a: a.get_name().lower().endswith(nome_cercato)
    passo = primo_asset(ORSO, unreal.AnimSequence, e_camminata("walkslow")) or \
        primo_asset(ORSO, unreal.AnimSequence, e_camminata("walk"))
    if passo is None:
        avviso("camminata non trovata")
    else:
        try:
            passo.set_editor_property("force_root_lock", True)
            libreria.save_loaded_asset(passo)
        except Exception as errore:
            avviso("force_root_lock non impostato (" + str(errore) + ")")
        scrivi("camminata: " + passo.get_name())

    # Alto circa 6 metri: e' l'Orso della leggenda.
    scala = 3.0
    try:
        limiti = corpo.get_bounds()
        altezza = limiti.box_extent.z * 2.0
        if altezza > 1.0:
            scala = max(0.5, min(30.0, 600.0 / altezza))
        scrivi("altezza del modello " + str(round(altezza)) + " cm, scala " + str(round(scala, 2)))
    except Exception as errore:
        avviso("misura dell'orso non riuscita (" + str(errore) + ")")
    return corpo, passo, scala


# ------------------------------------------------------------------------------------------

def trova(etichetta):
    for a in attori.get_all_level_actors():
        if a.get_actor_label() == etichetta:
            return a
    return None


def ricostruisci(a):
    try:
        a.rerun_construction_scripts()
    except Exception:
        pass


def esegui():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
        avviso("il gioco e' acceso: premi Stop e rilancia lo script")
        return

    mesh, m_monti = montagne2()
    m_fuoco = crea_fuoco2()
    m_rune = crea_rune2()
    corpo, passo, scala = orso()

    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if mondo is None or mondo.get_name() != "Lvl_Menu":
        livelli.load_level(CARTELLA + "/Lvl_Menu")

    montagne = trova("Montagne")
    if montagne is not None and mesh is not None:
        montagne.static_mesh_component.set_static_mesh(mesh)
        montagne.static_mesh_component.set_material(0, m_monti)

    for a in attori.get_all_level_actors():
        if isinstance(a, unreal.ValdorsoBraciere):
            a.set_editor_property("materiale_fuoco", m_fuoco)
            ricostruisci(a)
        elif isinstance(a, unreal.ValdorsoCerchioRune):
            a.set_editor_property("materiale", m_rune)
            ricostruisci(a)
        elif isinstance(a, unreal.ValdorsoFrammentoCuore):
            a.set_editor_property("baglior_quiete", 1.0)
            a.set_editor_property("baglior_colpo", 5.0)
        elif isinstance(a, unreal.ValdorsoOmbraOrso) and corpo is not None:
            try:
                a.set_editor_property("modello", corpo)
                if passo is not None:
                    a.set_editor_property("camminata", passo)
                a.set_editor_property("scala_modello", scala)
                ricostruisci(a)
                scrivi("l'Orso vero e' al suo posto")
            except Exception as errore:
                avviso("l'Orso vero non c'e' ancora nel codice: compila e rilancia (" + str(errore) + ")")

    livelli.save_current_level()
    scrivi("fatto")


esegui()
