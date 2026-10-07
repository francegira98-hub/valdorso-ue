# Valdorso - Diagnosi dell'acqua della valle (v0.1.3). Scritto da Claude il 07/10/2026.
# Non cambia niente: scrive nel Registro output come stanno la zona d'acqua e ogni corpo d'acqua (lago, fiumi, pozza),
# così Claude vede i dati veri prima di dire la causa di un problema (regola di lavoro: prima misurare).
# Si lancia in L_Valle: Strumenti -> Esegui script Python -> questo file. Poi copiare a Claude le righe "[Valdorso] Diagnosi".

import unreal

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def scrivi(testo):
    unreal.log("[Valdorso] Diagnosi: " + testo)


def leggi(oggetto, nome):
    try:
        return oggetto.get_editor_property(nome)
    except Exception as e:
        return "(non leggibile: {})".format(type(e).__name__)


def esegui():
    tutti = attori.get_all_level_actors()
    zone = [a for a in tutti if a.get_class().get_name() == "WaterZone"]
    scrivi("zone d'acqua: {}".format(len(zone)))
    for z in zone:
        scrivi("  zona {} in {} misura {} risoluzione {} tessere {}".format(
            z.get_actor_label(), z.get_actor_location(), leggi(z, "zone_extent"), leggi(z, "render_target_resolution"),
            leggi(z, "local_tessellation_extent")))
    maglie = [a for a in tutti if a.get_class().get_name() == "WaterMeshActor"]
    scrivi("WaterMeshActor (superficie dell'acqua, versioni vecchie): {}".format(len(maglie)))
    corpi = [a for a in tutti if a.get_class().get_name() in ("WaterBodyLake", "WaterBodyRiver", "WaterBodyOcean",
                                                            "WaterBodyCustom")]
    scrivi("corpi d'acqua: {}".format(len(corpi)))
    for c in corpi:
        comp = c.get_water_body_component()
        spline = c.get_water_spline()
        origine, estensione = c.get_actor_bounds(False)
        scrivi("  {} ({}) posto {} grande {} punti {} chiusa {} indice {} nascosto {} visibile {}".format(
            c.get_actor_label(), c.get_class().get_name(), c.get_actor_location(), estensione,
            spline.get_number_of_spline_points(), spline.is_closed_loop(), leggi(comp, "water_body_index"),
            c.is_hidden_ed(), leggi(comp, "visible")))
        scrivi("      zona assegnata {} materiale {} mesh statica {}".format(
            leggi(comp, "water_zone_override"), comp.get_water_material(), leggi(comp, "water_body_static_mesh_enabled")))
        if c.get_class().get_name() == "WaterBodyRiver":
            try:
                n = spline.get_number_of_spline_points()
                larghe = [round(comp.get_river_width_at_spline_input_key(float(i)) / 100, 1) for i in (0, n // 2, n - 1)]
                scrivi("      larghezza (m) all'inizio, a metà, alla fine: {}".format(larghe))
            except Exception as e:
                scrivi("      larghezza non leggibile ({})".format(e))
    scrivi("fatto")


esegui()
