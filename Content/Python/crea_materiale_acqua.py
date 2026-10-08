# Valdorso - L'acqua della valle meno turchese (passo R1 dei ritocchi). Scritto da Claude l'08/10/2026.
# Si lancia nell'editor, con L_Valle aperto: Strumenti -> Esegui script Python -> questo file. Si può rilanciare.
#
# Cosa fa:
#   - crea in Content/Valle/Acqua quattro "istanze" dei materiali del plugin Water (un'istanza è una copia che
#     cambia solo qualche manopola e tiene tutto il resto dell'originale, che non si tocca):
#       MI_AcquaFiume e MI_AcquaFiume_LOD  (Fiume, Emissario, Torrente)
#       MI_AcquaLago  e MI_AcquaLago_LOD   (Lago, Pozza della cascata)
#   - cambia solo tre manopole di colore:
#       Water Albedo  = quanta luce l'acqua rimanda indietro "da dentro": di serie 0,85 (quasi bianca, l'effetto
#                       piscina turchese); qui scura e appena verde, come un torrente di montagna;
#       Scattering    = il colore di quella luce: qui verde-blu spento invece del bianco;
#       Absorption    = quanto lontano arriva ogni colore nell'acqua (in cm): di serie il blu arriva più lontano
#                       (350) e l'acqua profonda diventa turchese; qui arriva più lontano il verde, il rosso si
#                       spegne subito. A riva, dove l'acqua è bassa, si vede ancora il fondo: è già "più trasparente".
#   - dà le istanze ai corpi d'acqua che ci sono nel livello. crea_acqua_valle.py (dall'08/10) le dà da solo
#     quando rifà l'acqua, così rilanciarlo non torna al turchese.
# Per provare altri colori: aprire MI_AcquaFiume o MI_AcquaLago con doppio clic e muovere i tre valori: si vede
# subito nel livello. I numeri buoni poi si scrivono qui sotto, in COLORI.

import unreal

CARTELLA = "/Game/Valle/Acqua"
SEMPRE = unreal.PropertyAccessChangeNotifyMode.ALWAYS

#          nome                  genitore (materiale di serie del plugin Water)
ISTANZE = {
    "MI_AcquaFiume":     "/Water/Materials/WaterSurface/Water_Material_River",
    "MI_AcquaFiume_LOD": "/Water/Materials/WaterSurface/LODs/Water_Material_River_LOD",
    "MI_AcquaLago":      "/Water/Materials/WaterSurface/Water_Material_Lake",
    "MI_AcquaLago_LOD":  "/Water/Materials/WaterSurface/LODs/Water_Material_Lake_LOD",
}

# Di serie: Water Albedo 0,85/0,85/0,85/0,5 · Scattering 1/1/1/0,5 · Absorption 10/150/350/8.
# La quarta cifra (A) si lascia come quella di serie.
# (08/10) Un colore solo per tutta l'acqua della valle: con due colori appena diversi (lago "più blu") si vedeva
# una riga dritta dove fiume e lago si toccano. Secondo giro, più deciso: meno luce "da dentro", verde più scuro.
ACQUA_VALLE = {
    "Water Albedo": (0.05, 0.09, 0.08, 0.5),
    "Scattering":   (0.35, 0.50, 0.42, 0.5),
    "Absorption":   (15.0, 120.0, 70.0, 8.0),
}
COLORI = {"fiume": ACQUA_VALLE, "lago": ACQUA_VALLE}

# Quale istanza va a quale corpo d'acqua: (materiale dell'acqua, materiale da lontano)
PER_CORPO = {
    "Fiume":               ("MI_AcquaFiume", "MI_AcquaFiume_LOD"),
    "Emissario":           ("MI_AcquaFiume", "MI_AcquaFiume_LOD"),
    "Torrente":            ("MI_AcquaFiume", "MI_AcquaFiume_LOD"),
    "Lago":                ("MI_AcquaLago",  "MI_AcquaLago_LOD"),
    "Pozza della cascata": ("MI_AcquaLago",  "MI_AcquaLago_LOD"),
}

mel = unreal.MaterialEditingLibrary
libreria = unreal.EditorAssetLibrary


def scrivi(testo):
    unreal.log("[Valdorso] Acqua R1: " + testo)


def istanza(nome, genitore_percorso, colori):
    percorso = CARTELLA + "/" + nome
    genitore = libreria.load_asset(genitore_percorso)
    if genitore is None:
        unreal.log_error("[Valdorso] Acqua R1: non trovo " + genitore_percorso + ": il plugin Water è acceso?")
        return None
    if libreria.does_asset_exist(percorso):
        mi = libreria.load_asset(percorso)
    else:
        strumenti = unreal.AssetToolsHelpers.get_asset_tools()
        mi = strumenti.create_asset(nome, CARTELLA, unreal.MaterialInstanceConstant,
                                    unreal.MaterialInstanceConstantFactoryNew())
        if mi is None:
            unreal.log_error("[Valdorso] Acqua R1: non riesco a creare " + percorso)
            return None
    mel.set_material_instance_parent(mi, genitore)
    # (08/10) Su un'istanza appena nata Unreal non "vede" ancora i parametri del genitore finché non la aggiorna:
    # la prima volta set_material_instance_vector_parameter_value diceva "non c'è" per tutti e tre.
    mel.update_material_instance(mi)
    for parametro, (r, g, b, a) in colori.items():
        colore = unreal.LinearColor(r, g, b, a)
        if not mel.set_material_instance_vector_parameter_value(mi, parametro, colore):
            scrivi_diretto(mi, parametro, colore)
    mel.update_material_instance(mi)
    libreria.save_loaded_asset(mi)
    scrivi("{} pronto (genitore {})".format(nome, genitore_percorso.split("/")[-1]))
    for parametro in colori:   # controllo: rilegge il valore vero dall'istanza
        c = mel.get_material_instance_vector_parameter_value(mi, parametro)
        scrivi("    {} = R {:.2f} G {:.2f} B {:.2f} A {:.2f}".format(parametro, c.r, c.g, c.b, c.a))
    return mi


def scrivi_diretto(mi, parametro, colore):
    """Se la via normale non va, scrive il valore direttamente nella lista dei parametri cambiati dell'istanza
    (è la stessa lista che riempie l'editor quando si spunta un parametro e gli si dà un colore)."""
    valori = [v for v in mi.get_editor_property("vector_parameter_values")
              if str(v.get_editor_property("parameter_info").get_editor_property("name")) != parametro]
    nuovo = unreal.VectorParameterValue()
    info = unreal.MaterialParameterInfo()
    info.set_editor_property("name", parametro)
    nuovo.set_editor_property("parameter_info", info)
    nuovo.set_editor_property("parameter_value", colore)
    valori.append(nuovo)
    mi.set_editor_property("vector_parameter_values", valori)
    scrivi("    {}: scritto direttamente".format(parametro))


def dai_materiale(componente, proprieta, funzione, materiale):
    try:
        componente.set_editor_property(proprieta, materiale, SEMPRE)
        return True
    except Exception:
        pass
    try:
        getattr(componente, funzione)(materiale)
        return True
    except Exception as e:
        unreal.log_warning("[Valdorso] Acqua R1: non riesco a dare {} ({})".format(proprieta, e))
        return False


def esegui():
    fatte = {}
    for nome, genitore in ISTANZE.items():
        colori = COLORI["fiume"] if nome.startswith("MI_AcquaFiume") else COLORI["lago"]
        mi = istanza(nome, genitore, colori)
        if mi is None:
            return
        fatte[nome] = mi

    Corpo = getattr(unreal, "WaterBody", None)
    if Corpo is None:
        unreal.log_error("[Valdorso] Acqua R1: manca la classe WaterBody: il plugin Water è acceso?")
        return
    attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cambiati = 0
    for corpo in attori.get_all_level_actors():
        if not isinstance(corpo, Corpo):
            continue
        nome = corpo.get_actor_label()
        if nome not in PER_CORPO:
            scrivi("{}: non è dei nostri, lo lascio com'è".format(nome))
            continue
        vicino, lontano = PER_CORPO[nome]
        componente = corpo.get_water_body_component()
        a = dai_materiale(componente, "water_material", "set_water_material", fatte[vicino])
        b = dai_materiale(componente, "water_static_mesh_material", "set_water_static_mesh_material", fatte[lontano])
        try:   # in alcune versioni l'acqua da lontano usa anche questo posto: se c'è, stesso materiale
            componente.set_editor_property("water_lod_material", fatte[lontano], SEMPRE)
        except Exception:
            pass
        # (08/10) Dove un fiume entra in un lago le due acque si sovrappongono e vinceva il fiume (spicchi turchesi a
        # croce nella pozza): lago e pozza hanno la precedenza, così i fiumi si fermano alla loro riva.
        priorita = 10 if nome in ("Lago", "Pozza della cascata") else 0
        try:
            componente.set_editor_property("overlap_material_priority", priorita, SEMPRE)
        except Exception as e:
            unreal.log_warning("[Valdorso] Acqua R1: {}: non riesco a dare la precedenza ({})".format(nome, e))
        if nome not in ("Lago", "Pozza della cascata"):
            passaggio_nostro(componente, nome, fatte)
        if a:
            cambiati += 1
            scrivi("{}: {}{}".format(nome, vicino, "" if b else " (da lontano resta quello di serie)"))
    scrivi("fatto: {} corpi d'acqua con il colore nuovo. Salva tutto (Ctrl+Maiusc+S).".format(cambiati))


# (08/10) Dove un fiume entra in un lago Unreal usa un terzo materiale, "River to Lake Transition", che sfuma
# l'uno nell'altro: era rimasto quello di serie e faceva gli spicchi turchesi nella pozza. Ne facciamo un'istanza
# con i nostri colori (MI_AcquaFiumeLago), copiata dal materiale di passaggio che il fiume ha adesso.
PASSAGGI = ("lake_transition_material", "river_to_lake_transition_material")


def passaggio_nostro(componente, nome, fatte):
    for proprieta in PASSAGGI:
        try:
            attuale = componente.get_editor_property(proprieta)
        except Exception:
            continue
        if "MI_AcquaFiumeLago" not in fatte:
            if attuale is None:
                scrivi("{}: nessun materiale di passaggio da copiare".format(nome))
                return
            genitore = attuale
            if genitore.get_path_name().startswith(CARTELLA):   # è già il nostro: si copia dal suo genitore
                genitore = genitore.get_editor_property("parent")
            mi = istanza("MI_AcquaFiumeLago", genitore.get_path_name(), COLORI["fiume"])
            if mi is None:
                return
            fatte["MI_AcquaFiumeLago"] = mi
        if dai_materiale(componente, proprieta, "set_lake_transition_material", fatte["MI_AcquaFiumeLago"]):
            scrivi("{}: passaggio fiume-lago con MI_AcquaFiumeLago".format(nome))
        return
    unreal.log_warning("[Valdorso] Acqua R1: {}: non trovo il posto del materiale di passaggio fiume-lago".format(nome))


esegui()
