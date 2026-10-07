# Valdorso - La cascata della valle (v0.1.3). Scritto da Claude il 07/10/2026.
# Si lancia nell'editor, con L_Valle aperto, DOPO crea_acqua_valle.py: Strumenti -> Esegui script Python -> questo file.
# Si può rilanciare: rimpiazza la cascata fatta prima (etichetta "ValdorsoCascata").
#
# Cosa fa:
#   1. importa da V:/Texture/valdorso/valle/cascata i pezzi fatti da Tools/Generatori/cascata.py (i veli d'acqua,
#      il disco di schiuma, due texture e il suono) in Content/Valle/Cascata;
#   2. crea i materiali M_Cascata (veli trasparenti con le strisce che scendono, più schiuma in basso) e
#      M_CascataSchiuma (schiuma che si muove piano sulla pozza);
#   3. mette la cascata sull'orlo letto da V:/Texture/valdorso/valle/Acqua_3000.json, girata come il fiume,
#      la schiuma sul pelo della pozza e il suono della cascata (si sente da circa 90 m).
# Il plugin Water non fa cascate: per questo è un pezzo a parte. Gli spruzzi e la nebbia (Niagara) arrivano dopo.

import json
import math

import unreal

CARTELLA_PEZZI = "V:/Texture/valdorso/valle/cascata"
FILE_ACQUA = "V:/Texture/valdorso/valle/Acqua_3000.json"
DESTINAZIONE = "/Game/Valle/Cascata"
ETICHETTA = "ValdorsoCascata"
QUOTA_MAX_CM = 80000.0         # i 16 bit delle altezze coprono 0-800 m (valle.py)

strumenti = unreal.AssetToolsHelpers.get_asset_tools()
libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def log(testo):
    unreal.log("[Valdorso] Cascata: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] Cascata: " + testo)


def importa(nome_file):
    compito = unreal.AssetImportTask()
    compito.set_editor_property("filename", CARTELLA_PEZZI + "/" + nome_file)
    compito.set_editor_property("destination_path", DESTINAZIONE)
    compito.set_editor_property("automated", True)
    compito.set_editor_property("replace_existing", True)
    compito.set_editor_property("save", True)
    strumenti.import_asset_tasks([compito])
    nome = nome_file.rsplit(".", 1)[0]
    asset = unreal.load_asset(DESTINAZIONE + "/" + nome + "." + nome)
    if asset is None:
        # l'import dei .glb a volte mette la mesh in una sottocartella o con un altro nome: la si cerca
        for percorso in libreria.list_assets(DESTINAZIONE, recursive=True):
            a = unreal.load_asset(percorso)
            if a is not None and a.get_name() == nome:
                return a
        avviso("non riesco a importare " + nome_file)
    return asset


def mesh_da(nome_file):
    asset = importa(nome_file)
    if isinstance(asset, unreal.StaticMesh):
        return asset
    nome = nome_file.rsplit(".", 1)[0]
    for percorso in libreria.list_assets(DESTINAZIONE, recursive=True):
        a = unreal.load_asset(percorso)
        # nome esatto: "SM_Cascata" è anche l'inizio di "SM_CascataSchiuma"
        if isinstance(a, unreal.StaticMesh) and a.get_name() in (nome, "SM_" + nome, nome + "_0"):
            return a
    avviso("non trovo la mesh di " + nome_file)
    return None


def nodo(materiale, classe, x, y):
    return mel.create_material_expression(materiale, classe, x, y)


def collega(da, uscita, a, ingresso):
    mel.connect_material_expressions(da, uscita, a, ingresso)


def costante(materiale, valore, x, y):
    c = nodo(materiale, unreal.MaterialExpressionConstant, x, y)
    c.set_editor_property("r", valore)
    return c


def colore(materiale, r, g, b, x, y):
    c = nodo(materiale, unreal.MaterialExpressionConstant3Vector, x, y)
    c.set_editor_property("constant", unreal.LinearColor(r, g, b, 1.0))
    return c


def per(materiale, a, a_uscita, b, b_uscita, x, y):
    m = nodo(materiale, unreal.MaterialExpressionMultiply, x, y)
    collega(a, a_uscita, m, "A")
    collega(b, b_uscita, m, "B")
    return m


def piu(materiale, a, a_uscita, b, b_uscita, x, y):
    m = nodo(materiale, unreal.MaterialExpressionAdd, x, y)
    collega(a, a_uscita, m, "A")
    collega(b, b_uscita, m, "B")
    return m


def scorre(materiale, texture, scala_u, scala_v, velocita_u, velocita_v, x, y):
    """La texture che scorre: UV x scala, poi Panner con la sua velocità."""
    uv = nodo(materiale, unreal.MaterialExpressionTextureCoordinate, x - 400, y)
    uv.set_editor_property("u_tiling", scala_u)
    uv.set_editor_property("v_tiling", scala_v)
    pan = nodo(materiale, unreal.MaterialExpressionPanner, x - 200, y)
    pan.set_editor_property("speed_x", velocita_u)
    pan.set_editor_property("speed_y", velocita_v)
    collega(uv, "", pan, "Coordinate")
    t = nodo(materiale, unreal.MaterialExpressionTextureSample, x, y)
    t.set_editor_property("texture", texture)
    t.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    collega(pan, "", t, "UVs")
    return t


def nuovo_materiale(nome):
    percorso = DESTINAZIONE + "/" + nome
    if libreria.does_asset_exist(percorso):
        libreria.delete_asset(percorso)
    m = strumenti.create_asset(nome, DESTINAZIONE, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    try:
        m.set_editor_property("translucency_lighting_mode",
                              unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    except Exception as e:
        avviso("luce della trasparenza lasciata com'era ({})".format(e))
    return m


def sfuma_nel_terreno(materiale, opacita, distanza, x, y):
    """Depth Fade: dove il velo tocca l'acqua o la roccia sparisce piano, senza la riga dura."""
    df = nodo(materiale, unreal.MaterialExpressionDepthFade, x, y)
    df.set_editor_property("fade_distance_default", distanza)
    collega(opacita, "", df, "Opacity")
    return df


def materiale_veli(tex_acqua):
    m = nuovo_materiale("M_Cascata")
    # Due strati di strisce che scendono a velocità diverse: l'acqua non sembra mai la stessa.
    a = scorre(m, tex_acqua, 1.0, 1.0, 0.0, -2.2, -1400, -300)
    b = scorre(m, tex_acqua, 1.7, 0.6, 0.0, -1.5, -1400, 100)
    vc = nodo(m, unreal.MaterialExpressionVertexColor, -1400, 500)
    # strisce = A.R * 0.7 + B.R * 0.4
    s1 = per(m, a, "R", costante(m, 0.7, -1100, -350), "", -1000, -300)
    s2 = per(m, b, "R", costante(m, 0.4, -1100, 50), "", -1000, 100)
    strisce = piu(m, s1, "", s2, "", -850, -100)
    # schiuma = (B.G + A.B * 0.5) * (0.3 + quanto è sceso): più bianca in basso
    gocce = per(m, a, "B", costante(m, 0.5, -1100, -150), "", -1000, -150)
    fiocchi = piu(m, b, "G", gocce, "", -850, 150)
    in_basso = piu(m, vc, "B", costante(m, 0.3, -1100, 600), "", -1000, 550)
    schiuma = per(m, fiocchi, "", in_basso, "", -700, 250)
    bianco = nodo(m, unreal.MaterialExpressionClamp, -550, 0)
    collega(piu(m, per(m, strisce, "", costante(m, 0.6, -800, 0), "", -700, 0), "", schiuma, "", -620, 50), "", bianco, "")
    acqua = colore(m, 0.10, 0.17, 0.18, -550, -400)     # (07/10) più scura: l'acqua vera, non un lenzuolo
    neve = colore(m, 0.92, 0.95, 0.96, -550, -300)
    base = nodo(m, unreal.MaterialExpressionLinearInterpolate, -350, -300)
    collega(acqua, "", base, "A")
    collega(neve, "", base, "B")
    collega(bianco, "", base, "Alpha")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Opacità = bordo (A) x velo (R) x (0.25 + bianco * 0.75), poi sfumata dove tocca.
    # (07/10) Più trasparente: dove non c'è schiuma il velo quasi non si vede (prima 0,25 + 0,75 x bianco: un lenzuolo);
    # bianco al quadrato lascia filamenti e gocce staccati.
    bianco2 = per(m, bianco, "", bianco, "", -450, 450)
    densita = piu(m, costante(m, 0.06, -500, 300), "", per(m, bianco2, "", costante(m, 0.8, -500, 400), "", -400, 400), "", -300, 350)
    velo = per(m, vc, "A", vc, "R", -300, 550)
    opacita = per(m, densita, "", velo, "", -150, 450)
    mel.connect_material_property(sfuma_nel_terreno(m, opacita, 120.0, 0, 450), "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(costante(m, 0.2, -150, 700), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.layout_material_expressions(m)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    log("creato M_Cascata")
    return m


def materiale_schiuma(tex_schiuma):
    m = nuovo_materiale("M_CascataSchiuma")
    a = scorre(m, tex_schiuma, 1.0, 1.0, 0.03, 0.05, -1200, -200)
    b = scorre(m, tex_schiuma, 1.6, 1.6, -0.04, 0.02, -1200, 200)
    vc = nodo(m, unreal.MaterialExpressionVertexColor, -1200, 600)
    insieme = per(m, a, "R", b, "R", -900, 0)
    forte = per(m, insieme, "", costante(m, 2.2, -1000, 150), "", -800, 50)
    vicino = piu(m, per(m, vc, "R", costante(m, 1.2, -1000, 600), "", -900, 600), "", costante(m, 0.35, -900, 700), "", -750, 600)
    bianco = nodo(m, unreal.MaterialExpressionClamp, -600, 200)
    collega(per(m, forte, "", vicino, "", -700, 300), "", bianco, "")
    mel.connect_material_property(colore(m, 0.9, 0.93, 0.94, -400, -200), "", unreal.MaterialProperty.MP_BASE_COLOR)
    opacita = per(m, bianco, "", vc, "A", -400, 300)
    mel.connect_material_property(sfuma_nel_terreno(m, opacita, 40.0, -200, 300), "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(costante(m, 0.6, -200, 600), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.layout_material_expressions(m)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    log("creato M_CascataSchiuma")
    return m


CODICE_ARCOBALENO = """
float a = degrees(acos(clamp(C, -1.0, 1.0)));
float t = (a - 40.4) / 2.2;
float3 col = saturate(1.0 - abs(t - float3(0.9, 0.5, 0.1)) / 0.38);
col *= saturate(1.0 - abs(t - 0.5) / 0.8);
return col;
"""


def materiale_arcobaleno():
    """(07/10) L'arcobaleno vero: compare a 40-42 gradi dal punto opposto al sole, cioè solo quando il sole è alle spalle
    di chi guarda. C = coseno dell'angolo tra la direzione della vista e quella del sole (luce del cielo n. 0)."""
    m = nuovo_materiale("M_CascataArcobaleno")
    try:
        m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception as e:
        avviso("arcobaleno non Unlit ({})".format(e))
    # Alpha Composite: il colore si aggiunge alla scena e l'opacità quasi zero non la scurisce
    # (Additive, con Substrate, nel menu non si vedeva: per questo non lo uso).
    try:
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
    except Exception as e:
        avviso("arcobaleno lasciato Translucent ({})".format(e))
    vista = nodo(m, unreal.MaterialExpressionCameraVectorWS, -900, 0)
    sole = nodo(m, unreal.MaterialExpressionSkyAtmosphereLightDirection, -900, 150)
    coseno = nodo(m, unreal.MaterialExpressionDotProduct, -700, 50)
    collega(vista, "", coseno, "A")
    collega(sole, "", coseno, "B")
    codice = nodo(m, unreal.MaterialExpressionCustom, -500, 50)
    codice.set_editor_property("code", CODICE_ARCOBALENO)
    codice.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ingresso = unreal.CustomInput()
    ingresso.set_editor_property("input_name", "C")
    codice.set_editor_property("inputs", [ingresso])
    collega(coseno, "", codice, "C")
    vc = nodo(m, unreal.MaterialExpressionVertexColor, -500, 300)
    luce = per(m, codice, "", vc, "A", -300, 100)
    # forza 2,5: abbastanza luminoso da vedersi su una parete al sole (si può cambiare qui e rilanciare)
    mel.connect_material_property(per(m, luce, "", costante(m, 2.5, -300, 250), "", -150, 100), "",
                                  unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    grigio = colore(m, 0.33, 0.33, 0.33, -300, 400)
    quanto = nodo(m, unreal.MaterialExpressionDotProduct, -150, 350)
    collega(luce, "", quanto, "A")
    collega(grigio, "", quanto, "B")
    mel.connect_material_property(per(m, quanto, "", costante(m, 0.05, -150, 500), "", 0, 400), "",
                                  unreal.MaterialProperty.MP_OPACITY)
    mel.layout_material_expressions(m)
    mel.recompile_material(m)
    libreria.save_loaded_asset(m)
    log("creato M_CascataArcobaleno")
    return m


def giro_della_mesh(mesh):
    """Di quanto girare l'attore perché l'avanti della mesh (dove salta l'acqua) sia il suo +X.
    Lo si legge dalla forma: la mesh va 7-8 m avanti e solo 2-3 m indietro, ed è simmetrica di lato."""
    scatola = mesh.get_bounding_box()
    lo, hi = scatola.min, scatola.max
    if -lo.z < 1000:
        avviso("la mesh non scende lungo Z come atteso (Z da {:.0f} a {:.0f}): controllare il verso".format(lo.z, hi.z))
    if abs(hi.x + lo.x) >= abs(hi.y + lo.y):
        return 0.0 if hi.x > -lo.x else 180.0
    return -90.0 if hi.y > -lo.y else 90.0


def esegui():
    with open(FILE_ACQUA, "r", encoding="utf-8") as f:
        dati = json.load(f)
    if "cascata" not in dati:
        unreal.log_error("[Valdorso] Cascata: Acqua_3000.json non ha la cascata. Serve quello fatto con la valle nuova.")
        return
    with open(CARTELLA_PEZZI + "/Cascata_3000.json", "r", encoding="utf-8") as f:
        info = json.load(f)
    c = dati["cascata"]

    paesaggi = [a for a in attori.get_all_level_actors() if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Cascata: nel livello aperto non c'è un Landscape. Apri L_Valle e rilancia lo script.")
        return
    angolo = paesaggi[0].get_actor_location()
    scala = paesaggi[0].get_actor_scale3d()

    def nel_mondo(p):
        valore = p[2] / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + p[0], angolo.y + p[1], z)

    # 1. I pezzi.
    veli = mesh_da("SM_Cascata.glb")
    disco = mesh_da("SM_CascataSchiuma.glb")
    arco = mesh_da("SM_CascataArcobaleno.glb")
    tex_acqua = importa("T_CascataAcqua.png")
    tex_schiuma = importa("T_CascataSchiuma.png")
    suono = importa("S_Cascata.wav")
    if None in (veli, disco, tex_acqua, tex_schiuma):
        unreal.log_error("[Valdorso] Cascata: mancano dei pezzi (vedi gli avvisi sopra).")
        return
    for t in (tex_acqua, tex_schiuma):
        t.set_editor_property("srgb", False)
        libreria.save_loaded_asset(t)
    if suono is not None:
        suono.set_editor_property("looping", True)
        libreria.save_loaded_asset(suono)

    # 2. I materiali.
    m_veli = materiale_veli(tex_acqua)
    m_schiuma = materiale_schiuma(tex_schiuma)
    coppie = [(veli, m_veli), (disco, m_schiuma)]
    if arco is not None:
        coppie.append((arco, materiale_arcobaleno()))
    for mesh, mat in coppie:
        mesh.set_material(0, mat)
        libreria.save_loaded_asset(mesh)

    # 3. Nel livello.
    for a in attori.get_all_level_actors():
        if ETICHETTA in [str(t) for t in a.tags]:
            attori.destroy_actor(a)
    direzione = c["direzione"]
    giro = math.degrees(math.atan2(direzione[1], direzione[0])) + giro_della_mesh(veli)
    orlo = nel_mondo(c["orlo_cm"])          # al pelo dell'acqua sull'orlo: lì è lo zero del modello
    pelo = nel_mondo(c["pozza_cm"]).z

    def metti(mesh, posto, nome):
        a = attori.spawn_actor_from_object(mesh, posto, unreal.Rotator(0.0, 0.0, giro))
        a.set_actor_label(nome)
        a.tags = [unreal.Name(ETICHETTA)]
        comp = a.get_component_by_class(unreal.StaticMeshComponent)
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        comp.set_editor_property("cast_shadow", False)
        return a

    metti(veli, orlo, "Cascata")
    metti(disco, unreal.Vector(orlo.x, orlo.y, pelo), "Cascata schiuma")
    if arco is not None:
        metti(arco, orlo, "Cascata arcobaleno")
    log("cascata sull'orlo a X {:.0f}, Y {:.0f}, Z {:.0f}; salto {:.1f} m; girata di {:.0f} gradi".format(
        orlo.x, orlo.y, orlo.z, c["salto_cm"] / 100, giro))

    # (07/10) Gli scalini del torrente: la stessa tenda, rimpicciolita all'altezza e alla larghezza di ogni salto.
    correzione = giro_della_mesh(veli)
    for i, sc in enumerate(dati.get("scalini", [])):
        posto = nel_mondo(sc["orlo_cm"])
        imbardata = math.degrees(math.atan2(sc["direzione"][1], sc["direzione"][0])) + correzione
        alto = sc["salto_cm"] / c["salto_cm"]
        largo = sc["larghezza_cm"] / c["larghezza_cm"]
        a = attori.spawn_actor_from_object(veli, posto, unreal.Rotator(0.0, 0.0, imbardata))
        a.set_actor_label("Cascatella {:02d}".format(i + 1))
        a.tags = [unreal.Name(ETICHETTA)]
        a.set_actor_scale3d(unreal.Vector(max(alto ** 0.5, 0.25), largo, alto))   # in avanti salta meno: radice
        comp = a.get_component_by_class(unreal.StaticMeshComponent)
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        comp.set_editor_property("cast_shadow", False)
    if dati.get("scalini"):
        log("{} cascatelle negli scalini del torrente".format(len(dati["scalini"])))

    if suono is not None:
        avanti = info["batte_avanti_cm"]
        dove = unreal.Vector(orlo.x + direzione[0] * avanti, orlo.y + direzione[1] * avanti, pelo + 300)
        s = attori.spawn_actor_from_class(unreal.AmbientSound, dove)
        s.set_actor_label("Cascata suono")
        s.tags = [unreal.Name(ETICHETTA)]
        comp = s.get_component_by_class(unreal.AudioComponent)
        comp.set_editor_property("sound", suono)
        try:
            comp.set_editor_property("override_attenuation", True)
            att = comp.get_editor_property("attenuation_overrides")
            att.set_editor_property("attenuation_shape_extents", unreal.Vector(2000.0, 0.0, 0.0))   # piena fino a 20 m
            att.set_editor_property("falloff_distance", 7000.0)                                     # poi cala fino a 90 m
            # (07/10) da lontano più cupo e ovattato: le note alte si perdono (filtro passa-basso con la distanza)
            att.set_editor_property("attenuate_with_lpf", True)
            att.set_editor_property("lpf_radius_min", 2000.0)
            att.set_editor_property("lpf_radius_max", 9000.0)
            att.set_editor_property("lpf_frequency_at_min", 20000.0)
            att.set_editor_property("lpf_frequency_at_max", 1200.0)
            comp.set_editor_property("attenuation_overrides", att)
        except Exception as e:
            avviso("distanza del suono lasciata quella di Unreal ({})".format(e))
        log("suono della cascata messo nella pozza")
    log("fatto. Per vederla: Finestra -> Contorno (Outliner), doppio clic su \"Cascata\".")


esegui()
