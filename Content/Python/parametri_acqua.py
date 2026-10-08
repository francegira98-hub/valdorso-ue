# Valdorso - Elenca i parametri dei materiali dell'acqua (passo R1-a dei ritocchi). Scritto da Claude il 07/10/2026.
# Non cambia niente: scrive nel Registro output, per ogni corpo d'acqua della valle (Lago, Fiume, Emissario,
# Torrente, Pozza della cascata), quale materiale usa e i nomi esatti dei suoi parametri con il valore di adesso.
# Serve a Claude per scrivere lo script dell'acqua meno turchese con i nomi veri di UE 5.8.3.
# Si lancia con L_Valle aperto: Strumenti -> Esegui script Python -> questo file.
# Poi nel Registro output, nella casella di ricerca, scrivere [Valdorso] e mandare a Claude lo screenshot (o copiare le righe).

import unreal

ETICHETTA = "ValdorsoAcqua"
mel = unreal.MaterialEditingLibrary
attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def scrivi(testo):
    unreal.log("[Valdorso] Acqua R1: " + testo)


def valore(materiale, tipo, nome):
    try:
        if tipo == "scalar":
            return "{:.3f}".format(mel.get_material_instance_scalar_parameter_value(materiale, nome))
        if tipo == "vector":
            c = mel.get_material_instance_vector_parameter_value(materiale, nome)
            return "R {:.3f} G {:.3f} B {:.3f} A {:.3f}".format(c.r, c.g, c.b, c.a)
        if tipo == "texture":
            t = mel.get_material_instance_texture_parameter_value(materiale, nome)
            return t.get_path_name() if t else "(nessuna)"
    except Exception:
        pass
    return "?"


def elenca(materiale, dove):
    if materiale is None:
        scrivi("  {}: nessun materiale".format(dove))
        return
    scrivi("  {}: {} ({})".format(dove, materiale.get_path_name(), materiale.get_class().get_name()))
    istanza = isinstance(materiale, unreal.MaterialInstance)
    if istanza:
        try:
            genitore = materiale.get_editor_property("parent")
            scrivi("    genitore: " + (genitore.get_path_name() if genitore else "(nessuno)"))
        except Exception:
            pass
    for tipo, funzione in (("vector", mel.get_vector_parameter_names),
                           ("scalar", mel.get_scalar_parameter_names),
                           ("texture", mel.get_texture_parameter_names)):
        try:
            nomi = funzione(materiale)
        except Exception as e:
            scrivi("    {}: non leggo i nomi ({})".format(tipo, e))
            continue
        scrivi("    {} parametri {}:".format(len(nomi), tipo))
        for n in nomi:
            scrivi("      {} = {}".format(n, valore(materiale, tipo, n) if istanza else "-"))


def esegui():
    Corpo = getattr(unreal, "WaterBody", None)
    if Corpo is None:
        scrivi("manca la classe WaterBody: il plugin Water è acceso?")
        return
    corpi = [a for a in attori.get_all_level_actors() if isinstance(a, Corpo)]
    if not corpi:
        scrivi("nel livello aperto non ci sono corpi d'acqua. Apri L_Valle e rilancia.")
        return
    visti = set()
    for corpo in corpi:
        scrivi("=== {} ({})".format(corpo.get_actor_label(), corpo.get_class().get_name()))
        componente = corpo.get_water_body_component()
        for proprieta in ("water_material", "underwater_post_process_material", "water_info_material",
                          "water_static_mesh_material", "water_hlod_material"):
            try:
                m = componente.get_editor_property(proprieta)
            except Exception:
                continue
            chiave = m.get_path_name() if m else None
            if chiave in visti:
                scrivi("  {}: {} (già elencato sopra)".format(proprieta, chiave))
                continue
            visti.add(chiave)
            elenca(m, proprieta)
    scrivi("fatto: {} corpi d'acqua.".format(len(corpi)))


esegui()
