# Valdorso - Rifiniture della scena del menu (Lvl_Menu), scritto da Claude il 03/10/2026.
# Si lancia in Valdorso DOPO aver compilato il codice nuovo: Strumenti -> Esegui script Python -> questo file.
# Prima: File -> Salva tutto, e il gioco fermo (Stop).
#
# Cosa fa:
#   - importa da V:/Texture/valdorso le tre immagini fatte da Claude (cielo, cerchio di rune, ombra dell'Orso);
#   - crea i materiali M_CieloNotte, M_Rune, M_Fuoco, M_OmbraOrso, M_Monti (in Content/Menu);
#   - nel livello: il cielo nuovo sulla volta, la nebbia che non copre il cielo, l'altare grigio freddo,
#     il terreno piu' grande, la luna bassa dietro l'altare con i raggi nella nebbia, lo sfondo appena sfocato,
#     il fuoco nei bracieri, il cerchio di rune, il pulviscolo e le lucciole, l'ombra dell'Orso, i monti.
# Si puo' rilanciare: non duplica niente. Alla fine il Registro output scrive "[Valdorso] Rifiniture: fatto".

import unreal

SORGENTE = "V:/Texture/valdorso"
CARTELLA = "/Game/Menu"
TEXTURE = CARTELLA + "/Texture/valdorso"
LIVELLO = CARTELLA + "/Lvl_Menu"

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

SUBSTRATE = hasattr(unreal, "MaterialExpressionSubstrateUnlitBSDF") and hasattr(unreal.MaterialProperty, "MP_FRONT_MATERIAL")


def scrivi(testo):
    unreal.log("[Valdorso] Rifiniture: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] Rifiniture: " + testo)


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


def crea_cielo(tex):
    m, nuovo = nuovo_materiale("M_CieloNotte", unreal.BlendMode.BLEND_OPAQUE)
    if not nuovo:
        return m
    imposta(m, "is_sky", True)
    direzione = mel.create_material_expression(m, unreal.MaterialExpressionCameraVectorWS, -1200, 0)
    meno = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -1000, 0)
    collega(direzione, "", meno, "A")
    meno.set_editor_property("const_b", -1.0)
    uv = codice(m,
                "float3 d = normalize(D);\n"
                "float u = atan2(d.y, d.x) / 6.2831853 + 0.5;\n"
                "float v = acos(clamp(d.z, -1.0, 1.0)) / 3.1415927;\n"
                "return float2(u, v);",
                [("D", meno)], FLOAT2, -800, 0, "Cielo: da direzione a foto")
    campione = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
    campione.set_editor_property("parameter_name", "Cielo")
    campione.set_editor_property("texture", tex)
    collega(uv, "", campione, "UVs")
    luce = scalare(m, "Luminosita", 1.0, -500, 300)
    acceso = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -200, 100)
    collega(campione, "RGB", acceso, "A")
    collega(luce, "", acceso, "B")
    uscita_accesa(m, acceso)
    chiudi(m, "M_CieloNotte")
    return m


def crea_rune(tex):
    m, nuovo = nuovo_materiale("M_Rune", unreal.BlendMode.BLEND_ADDITIVE, False)
    if not nuovo:
        return m
    p = posizione_locale(m, -1300, 0)
    uv = codice(m, "return float2(0.5 + P.y / 100.0, 0.5 - P.x / 100.0);",
                [("P", p)], FLOAT2, -900, 0, "Rune: coordinate dal piano")
    campione = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    campione.set_editor_property("parameter_name", "Rune")
    campione.set_editor_property("texture", tex)
    collega(uv, "", campione, "UVs")
    colore = vettore(m, "Colore", unreal.LinearColor(1.0, 0.55, 0.2, 1.0), -600, 300)
    forza = scalare(m, "Intensita", 1.5, -600, 500)
    a = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 100)
    collega(campione, "R", a, "A")
    collega(colore, "", a, "B")
    b = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -150, 200)
    collega(a, "", b, "A")
    collega(forza, "", b, "B")
    uscita_accesa(m, b)
    chiudi(m, "M_Rune")
    return m


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


def crea_fuoco():
    m, nuovo = nuovo_materiale("M_Fuoco", unreal.BlendMode.BLEND_ADDITIVE)
    if not nuovo:
        return m
    p = posizione_locale(m, -1300, 0)
    tempo = mel.create_material_expression(m, unreal.MaterialExpressionTime, -1300, 200)
    seme = scalare(m, "Seme", 0.0, -1300, 350)
    forza = scalare(m, "Intensita", 2.5, -1300, 500)
    fuoco = codice(m, FUOCO_HLSL, [("P", p), ("Tempo", tempo), ("Seme", seme), ("Intensita", forza)],
                   FLOAT3, -800, 100, "Fuoco")
    uscita_accesa(m, fuoco)
    chiudi(m, "M_Fuoco")
    return m


def crea_orso(tex):
    m, nuovo = nuovo_materiale("M_OmbraOrso", unreal.BlendMode.BLEND_MASKED)
    if not nuovo:
        return m
    p = posizione_locale(m, -1400, 0)
    verso = scalare(m, "Verso", 1.0, -1400, 200)
    uv = codice(m, "return float2(0.5 + Verso * P.y / 100.0, 0.5 - P.x / 100.0);",
                [("P", p), ("Verso", verso)], FLOAT2, -1000, 0, "Orso: coordinate dal piano")
    campione = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
    campione.set_editor_property("parameter_name", "Sagoma")
    campione.set_editor_property("texture", tex)
    collega(uv, "", campione, "UVs")
    opacita = scalare(m, "Opacita", 1.0, -700, 300)
    maschera = codice(m,
                      "float n = frac(sin(dot(floor(Parameters.SvPosition.xy), float2(12.9898, 78.233))) * 43758.5453);\n"
                      "return (A * Opacita > n * 0.96 + 0.02) ? 1.0 : 0.0;",
                      [("A", campione, "A"), ("Opacita", opacita)], FLOAT1, -400, 200, "Orso: compare e sparisce")
    colore = vettore(m, "Colore", unreal.LinearColor(0.002, 0.0025, 0.004, 1.0), -400, 0)
    uscita_accesa(m, colore, maschera)
    chiudi(m, "M_OmbraOrso")
    return m


MONTI_HLSL = """
float u = (Indice + 0.5 + P.y / 100.0) / max(Pezzi, 1.0);
float v = P.x / 100.0 + 0.5;
float t = 6.2831853 * u;
float s = Seme;
float h = 0.40
    + 0.20 * sin(2.0 * t + s)
    + 0.12 * sin(3.0 * t + 1.7 * s)
    + 0.09 * (1.0 - abs(sin(5.0 * t + 2.3 * s)))
    + 0.06 * sin(9.0 * t + 3.1 * s)
    + 0.035 * (1.0 - abs(sin(17.0 * t + 4.7 * s)))
    + 0.02 * sin(31.0 * t + 6.1 * s)
    + 0.01 * sin(57.0 * t + 7.3 * s);
h = clamp(h, 0.15, 1.0);
return v < h ? 1.0 : 0.0;
"""


def crea_monti():
    m, nuovo = nuovo_materiale("M_Monti", unreal.BlendMode.BLEND_MASKED)
    if not nuovo:
        return m
    p = posizione_locale(m, -1300, 0)
    indice = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceCustomData, -1300, 200)
    indice.set_editor_property("data_index", 0)
    pezzi = scalare(m, "Pezzi", 24.0, -1300, 350)
    seme = scalare(m, "Seme", 1.0, -1300, 500)
    maschera = codice(m, MONTI_HLSL, [("P", p), ("Indice", indice), ("Pezzi", pezzi), ("Seme", seme)],
                      FLOAT1, -800, 200, "Monti: le creste")
    colore = vettore(m, "Colore", unreal.LinearColor(0.004, 0.006, 0.012, 1.0), -500, 0)
    uscita_accesa(m, colore, maschera)
    chiudi(m, "M_Monti")
    return m


# ------------------------------------------------------------------------------------------
# Il livello
# ------------------------------------------------------------------------------------------

def trova(etichetta):
    for a in attori.get_all_level_actors():
        if a.get_actor_label() == etichetta:
            return a
    return None


def metti(classe, etichetta, posizione, rotazione=None):
    a = trova(etichetta)
    if a is None:
        a = attori.spawn_actor_from_class(classe, posizione, rotazione or unreal.Rotator())
        a.set_actor_label(etichetta)
        a.set_folder_path("Scena")
        scrivi("messo " + etichetta)
    return a


def ricostruisci(a):
    # Fa ripartire OnConstruction (materiali e dimensioni) dopo aver cambiato le proprieta'.
    try:
        a.rerun_construction_scripts()
    except Exception:
        a.set_actor_location(a.get_actor_location(), False, False)


def esegui():
    try:
        if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
            avviso("il gioco e' acceso: premi Stop e rilancia lo script")
            return
    except Exception:
        pass

    for classe in ("ValdorsoCerchioRune", "ValdorsoPulviscolo", "ValdorsoOmbraOrso", "ValdorsoMonti"):
        if not hasattr(unreal, classe):
            avviso("manca la classe " + classe + ": il codice nuovo non e' compilato. Compila e riapri Valdorso.")
            return

    # 1. Le texture.
    t_cielo = importa("T_CieloNotte.png")
    t_rune = importa("T_CerchioRune.png")
    t_orso = importa("T_OmbraOrso.png")
    if None in (t_cielo, t_rune, t_orso):
        return
    prepara_cielo(t_cielo)
    scrivi("texture pronte")

    # 2. I materiali.
    m_cielo = crea_cielo(t_cielo)
    m_rune = crea_rune(t_rune)
    m_fuoco = crea_fuoco()
    m_orso = crea_orso(t_orso)
    m_monti = crea_monti()
    m_luce = unreal.load_asset(CARTELLA + "/M_Luminoso")

    # L'altare grigio freddo (anche le basi dei bracieri) e il terreno piu' fitto, per il terreno piu' grande.
    mi_altare = unreal.load_asset(CARTELLA + "/MI_Altare")
    if mi_altare:
        mel.set_material_instance_vector_parameter_value(mi_altare, "Tinta", unreal.LinearColor(0.55, 0.58, 0.62, 1.0))
        mel.update_material_instance(mi_altare)
        libreria.save_loaded_asset(mi_altare)
    mi_terreno = unreal.load_asset(CARTELLA + "/MI_Terreno")
    if mi_terreno:
        mel.set_material_instance_scalar_parameter_value(mi_terreno, "RipetizioneU", 250.0)
        mel.set_material_instance_scalar_parameter_value(mi_terreno, "RipetizioneV", 250.0)
        mel.update_material_instance(mi_terreno)
        libreria.save_loaded_asset(mi_terreno)
    scrivi("materiali pronti")

    # 3. Il livello.
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if mondo is None or mondo.get_name() != "Lvl_Menu":
        livelli.load_level(LIVELLO)

    volta = trova("VoltaDelCielo")
    if volta:
        volta.static_mesh_component.set_material(0, m_cielo)

    terreno = trova("Terreno")
    if terreno:
        terreno.set_actor_scale3d(unreal.Vector(500, 500, 1))

    nebbia = trova("Nebbia")
    if nebbia:
        # Oltre un chilometro la nebbia si ferma: il cielo resta pulito, i monti restano nella foschia.
        imposta(nebbia.component, "fog_cutoff_distance", 100000.0)

    luna = trova("Luna")
    if luna:
        # Bassa, dietro l'altare, dove il cielo e' chiaro: controluce e raggi nella nebbia.
        luna.set_actor_rotation(unreal.Rotator(roll=0, pitch=-24, yaw=185), False)
        lc = luna.light_component
        imposta(lc, "volumetric_scattering_intensity", 2.5)
        imposta(lc, "enable_light_shaft_occlusion", True)
        imposta(lc, "enable_light_shaft_bloom", True)
        imposta(lc, "bloom_scale", 0.15)

    atmosfera = trova("Atmosfera")
    if atmosfera:
        s = atmosfera.get_editor_property("settings")
        for nome, valore in (("override_depth_of_field_focal_distance", True), ("depth_of_field_focal_distance", 1000.0),
                             ("override_depth_of_field_fstop", True), ("depth_of_field_fstop", 2.8)):
            imposta(s, nome, valore)
        atmosfera.set_editor_property("settings", s)

    for a in attori.get_all_level_actors():
        if isinstance(a, unreal.ValdorsoBraciere):
            a.set_editor_property("materiale_fuoco", m_fuoco)
            ricostruisci(a)

    rune = metti(unreal.ValdorsoCerchioRune, "CerchioRune", unreal.Vector(0, 0, 4))
    rune.set_editor_property("materiale", m_rune)
    ricostruisci(rune)

    polvere = metti(unreal.ValdorsoPulviscolo, "Pulviscolo", unreal.Vector(0, 0, 0))
    polvere.set_editor_property("materiale", m_luce)

    orso = metti(unreal.ValdorsoOmbraOrso, "OmbraOrso", unreal.Vector(3800, -1100, 0))
    orso.set_editor_property("materiale", m_orso)
    ricostruisci(orso)

    for etichetta, raggio, altezza, pezzi, seme, colore in (
            ("MontiVicini", 14000.0, 3200.0, 24, 1.3, unreal.LinearColor(0.003, 0.004, 0.008, 1.0)),
            ("MontiLontani", 24000.0, 5200.0, 32, 4.1, unreal.LinearColor(0.006, 0.008, 0.016, 1.0))):
        monti = metti(unreal.ValdorsoMonti, etichetta, unreal.Vector(0, 0, 0))
        monti.set_editor_property("materiale", m_monti)
        monti.set_editor_property("raggio", raggio)
        monti.set_editor_property("altezza", altezza)
        monti.set_editor_property("pezzi", pezzi)
        monti.set_editor_property("seme", seme)
        monti.set_editor_property("colore", colore)
        ricostruisci(monti)

    livelli.save_current_level()
    scrivi("fatto")


esegui()
