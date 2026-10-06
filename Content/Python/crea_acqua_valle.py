# Valdorso - Il lago, il fiume e l'emissario della valle (v0.1.3), con il plugin Water di Unreal. Scritto da Claude il 06/10/2026.
# Si lancia nell'editor, con il livello della valle aperto e il plugin Water acceso:
#   Strumenti -> Esegui script Python -> questo file. Si può rilanciare: rimpiazza lago e fiume fatti prima.
#
# Cosa fa:
#   - legge V:/Texture/valdorso/valle/Acqua_3000.json (fatto da Tools/Generatori/acqua.py dalle altezze della valle);
#   - mette una zona d'acqua (WaterZone) grande come la valle, se non c'è;
#   - crea il lago (WaterBodyLake) con la riva vera, a 30 m, il fiume (WaterBodyRiver) lungo l'alveo, dalla conca
#     al lago, e l'emissario (un altro WaterBodyRiver) dal lago verso ovest, con larghezza e profondità punto per punto;
#   - alla fine "scuote" lago e fiumi perché Unreal li ridisegni (06/10: senza, l'acqua restava solo nella parte
#     profonda finché Fra non spostava a mano l'attore).
#   - non tocca il terreno ("Affects Landscape" spento): l'alveo e il fondale sono già scavati dal generatore.
# Lago e fiumi hanno l'etichetta "ValdorsoAcqua": rilanciando lo script si tolgono e si rifanno.

import json

import unreal

FILE_ACQUA = "V:/Texture/valdorso/valle/Acqua_3000.json"
ETICHETTA = "ValdorsoAcqua"
QUOTA_MAX_CM = 80000.0         # i 16 bit delle altezze coprono 0-800 m (valle.py)

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def classe(nome):
    c = getattr(unreal, nome, None)
    if c is None:
        raise RuntimeError("manca la classe " + nome + ": il plugin Water è acceso? (Modifica -> Plugin -> Water, poi riavvia)")
    return c


def esegui():
    try:
        Lago = classe("WaterBodyLake")
        Fiume = classe("WaterBodyRiver")
        Zona = classe("WaterZone")
    except RuntimeError as e:
        unreal.log_error("[Valdorso] Acqua: " + str(e))
        return

    with open(FILE_ACQUA, "r", encoding="utf-8") as f:
        dati = json.load(f)

    tutti = attori.get_all_level_actors()
    paesaggi = [a for a in tutti if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] Acqua: nel livello aperto non c'è un Landscape. Apri L_Valle e rilancia lo script.")
        return
    paesaggio = paesaggi[0]
    angolo = paesaggio.get_actor_location()
    scala = paesaggio.get_actor_scale3d()

    def nel_mondo(p):
        """Dal file (cm dall'angolo, quota vera in cm) alla posizione del mondo, con la scala Z del paesaggio."""
        valore = p[2] / QUOTA_MAX_CM * 65535.0
        z = angolo.z + (valore - 32768.0) * scala.z / 128.0
        return unreal.Vector(angolo.x + p[0], angolo.y + p[1], z)

    # Via lago e fiume di un lancio precedente.
    for a in tutti:
        if ETICHETTA in [str(t) for t in a.tags]:
            attori.destroy_actor(a)

    # La zona d'acqua: una sola per tutta la valle.
    if not [a for a in tutti if isinstance(a, Zona)]:
        centro = unreal.Vector(angolo.x + dati["lato_cm"] / 2, angolo.y + dati["lato_cm"] / 2, 0)
        zona = attori.spawn_actor_from_class(Zona, centro)
        try:
            zona.set_editor_property("zone_extent", unreal.Vector2D(dati["lato_cm"] + 20000, dati["lato_cm"] + 20000))
        except Exception as e:
            unreal.log_warning("[Valdorso] Acqua: non riesco a dare la misura alla zona d'acqua ({}).".format(e))
        unreal.log("[Valdorso] Acqua: creata la zona d'acqua")

    def metadati_di(corpo):
        """Le curve punto per punto del fiume (WaterSplineMetadata): in 5.8 non è più una proprietà dell'attore,
        quindi la si cerca anche tra gli oggetti che gli appartengono."""
        for dove in (corpo, corpo.get_water_body_component(), corpo.get_water_spline()):
            try:
                m = dove.get_editor_property("water_spline_metadata")
                if m:
                    return m
            except Exception:
                pass
        classe_m = getattr(unreal, "WaterSplineMetadata", None)
        if classe_m is not None:
            for m in unreal.ObjectIterator(classe_m):
                o = m.get_outer()
                while o is not None:
                    if o == corpo:
                        return m
                    o = o.get_outer()
        return None

    def ridisegna(corpo, spline, chiusa):
        """Unreal ricostruisce l'acqua quando la spline o l'attore 'cambiano' come nell'editor: lo si simula."""
        try:
            spline.set_editor_property("closed_loop", chiusa)
        except Exception:
            pass
        try:
            corpo.get_water_body_component().on_water_body_changed(True, True)
        except Exception:
            pass
        posto = corpo.get_actor_location()
        corpo.set_actor_location(posto + unreal.Vector(0, 0, 1), False, False)
        corpo.set_actor_location(posto, False, False)

    def prepara(corpo, nome):
        corpo.set_actor_label(nome)
        corpo.tags = [unreal.Name(ETICHETTA)]
        componente = corpo.get_water_body_component()
        try:
            componente.set_editor_property("affects_landscape", False)
        except Exception as e:
            unreal.log_warning("[Valdorso] Acqua: non riesco a spegnere 'Affects Landscape' su {} ({}).".format(nome, e))
        return corpo.get_water_spline()

    # ------------------------------------------------------------------ il lago
    riva = [nel_mondo(p) for p in dati["lago"]["riva"]]
    centro_lago = unreal.Vector(sum(p.x for p in riva) / len(riva), sum(p.y for p in riva) / len(riva), riva[0].z)
    lago = attori.spawn_actor_from_class(Lago, centro_lago)
    spline = prepara(lago, "Lago")
    spline.set_spline_points(riva, unreal.SplineCoordinateSpace.WORLD, True)
    spline.set_closed_loop(True, True)
    unreal.log("[Valdorso] Acqua: lago con {} punti di riva, pelo a {:.0f} cm".format(len(riva), riva[0].z))

    # ------------------------------------------------------------------ i fiumi
    def fiume_da(chiave, nome):
        punti = dati[chiave]["punti"]
        posizioni = [nel_mondo(p["posizione"]) for p in punti]
        fiume = attori.spawn_actor_from_class(Fiume, posizioni[0])
        spline = prepara(fiume, nome)
        spline.set_spline_points(posizioni, unreal.SplineCoordinateSpace.WORLD, True)

        # Punto per punto: larghezza e profondità (curve dei metadati della spline d'acqua).
        try:
            metadati = metadati_di(fiume)
            if metadati is None:
                raise RuntimeError("non trovo i metadati della spline")

            def curva(valori):
                c = unreal.InterpCurveFloat()
                punti_curva = []
                for i, v in enumerate(valori):
                    pc = unreal.InterpCurvePointFloat()
                    pc.set_editor_property("in_val", float(i))
                    pc.set_editor_property("out_val", float(v))
                    pc.set_editor_property("interp_mode", unreal.InterpCurveMode.CIM_CURVE_AUTO)
                    punti_curva.append(pc)
                c.set_editor_property("points", punti_curva)
                return c

            metadati.set_editor_property("river_width", curva([p["larghezza_cm"] for p in punti]))
            metadati.set_editor_property("depth", curva([p["profondita_cm"] for p in punti]))
            metadati.set_editor_property("water_velocity_scalar", curva([1.0] * len(punti)))
            metadati.set_editor_property("audio_intensity", curva([1.0] * len(punti)))
            unreal.log("[Valdorso] Acqua: {}: larghezza e profondità punto per punto".format(nome))
        except Exception as e:
            unreal.log_warning("[Valdorso] Acqua: {}: larghezza punto per punto non riuscita, resta quella di Unreal "
                               "(circa 20 m) ({}).".format(nome, e))
        unreal.log("[Valdorso] Acqua: {} con {} punti, da {:.0f} a {:.0f} cm".format(
            nome, len(posizioni), posizioni[0].z, posizioni[-1].z))
        return fiume, spline

    fiume, spline_fiume = fiume_da("fiume", "Fiume")
    corpi = [(lago, lago.get_water_spline(), True), (fiume, spline_fiume, False)]
    if "emissario" in dati:
        emissario, spline_em = fiume_da("emissario", "Emissario")
        corpi.append((emissario, spline_em, False))

    for corpo, spline, chiusa in corpi:
        ridisegna(corpo, spline, chiusa)
    unreal.log("[Valdorso] Acqua: fatto. Se un pezzo d'acqua non si vede tutto, sposta un poco quell'attore con la freccia "
               "e premi Ctrl+Z: Unreal lo ridisegna.")


esegui()
