# Valdorso - Dimmi cosa ho selezionato (25/11/2026, scritto da Claude).
# NON cambia niente nel livello: guarda e scrive.
#
# Serve quando Fra vede un difetto su un pezzo e la chat 3D (o Claude) chiede "quale pezzo e', e dove sta?".
# Fra clicca sul pezzo (o su piu' pezzi, con Ctrl), inquadra il difetto e lancia questa voce:
#   Menu Valdorso -> Villaggio -> "Dimmi cosa ho selezionato (per Claude e la chat 3D)"
# oppure, senza riavviare Unreal, nella riga "Cmd" in basso:   py "V:/Valdorso/Content/Python/nuovi/dimmi_selezione.py"
# Risultato: Saved/Claude/selezione.txt (nome nell'Outliner, modello, posizione, giro, scala, etichette, cartella,
# materiali per slot, ingombro) piu' dove sta la telecamera, e una foto di quello che si vede: Saved/Claude/selezione.png.
# Claude li legge da solo dal PC: non serve mandare niente.

import os

import unreal

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(t):
    righe.append(t)
    unreal.log("[Valdorso] Selezione: " + t)


def uno(a):
    scrivi("- nome nell'Outliner: " + a.get_actor_label())
    scrivi("  tipo: " + a.get_class().get_name())
    try:
        scrivi("  cartella: " + str(a.get_folder_path()))
    except Exception:  # noqa: BLE001
        pass
    p, r, s = a.get_actor_location(), a.get_actor_rotation(), a.get_actor_scale3d()
    scrivi("  posizione (cm): X %.1f  Y %.1f  Z %.1f" % (p.x, p.y, p.z))
    scrivi("  giro (gradi): yaw %.2f  pitch %.2f  roll %.2f" % (r.yaw, r.pitch, r.roll))
    scrivi("  scala: %.3f  %.3f  %.3f%s" % (s.x, s.y, s.z, "   (Y a specchio)" if s.y < 0 else ""))
    etichette = [str(t) for t in a.tags]
    if etichette:
        scrivi("  etichette: " + ", ".join(etichette))
    for comp in a.get_components_by_class(unreal.StaticMeshComponent):
        mesh = comp.get_editor_property("static_mesh")
        if mesh is None:
            continue
        scrivi("  modello: %s   (%s)" % (mesh.get_name(), mesh.get_path_name().split(".")[0]))
        try:
            b = mesh.get_bounding_box()
            scrivi("    ingombro del modello (cm): x da %.0f a %.0f, y da %.0f a %.0f, z da %.0f a %.0f"
                   % (b.min.x, b.max.x, b.min.y, b.max.y, b.min.z, b.max.z))
        except Exception:  # noqa: BLE001
            pass
        try:
            for i, sm in enumerate(mesh.get_editor_property("static_materials")):
                usato = comp.get_material(i)
                scrivi("    slot %d «%s»: %s" % (i, str(sm.get_editor_property("material_slot_name")),
                                               usato.get_name() if usato is not None else "nessun materiale"))
        except Exception:  # noqa: BLE001
            pass


def main():
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    scrivi("Livello: " + (mondo.get_name() if mondo is not None else "?"))
    scelti = list(attori.get_selected_level_actors())
    if not scelti:
        scrivi("ATTENZIONE: non c'e' niente di selezionato. Clicca sul pezzo con il difetto (lo vedi evidenziato "
               "nell'Outliner) e rilancia.")
    else:
        scrivi("Pezzi selezionati: %d" % len(scelti))
        for a in scelti[:40]:
            try:
                uno(a)
            except Exception as e:  # noqa: BLE001
                scrivi("  (errore nel leggere questo pezzo: %s)" % e)
    # dove sta la telecamera dell'editor (cosi' si puo' ritrovare la stessa inquadratura)
    try:
        r = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_level_viewport_camera_info()
        if r:
            pos, rot = (r[0], r[1]) if len(r) == 2 else (r[1], r[2])
            scrivi("Telecamera: X %.0f  Y %.0f  Z %.0f, yaw %.1f  pitch %.1f" % (pos.x, pos.y, pos.z, rot.yaw, rot.pitch))
    except Exception as e:  # noqa: BLE001
        scrivi("(telecamera non letta: %s)" % e)
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    # la foto di quello che si vede adesso (arriva qualche secondo dopo)
    foto = os.path.join(cartella, "selezione.png").replace("\\", "/")
    try:
        if os.path.exists(foto):
            os.remove(foto)
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, foto)
        scrivi("Foto chiesta: " + foto + " (se non compare, cercala in Saved/Screenshots/WindowsEditor)")
    except Exception as e:  # noqa: BLE001
        scrivi("(foto non fatta: %s. Va bene lo stesso: bastano i dati qui sopra)" % e)
    with open(os.path.join(cartella, "selezione.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
    unreal.log("[Valdorso] Selezione: scritto Saved/Claude/selezione.txt")
    try:
        unreal.EditorDialog.show_message("Valdorso", "Fatto: ho scritto %d pezzi in Saved/Claude/selezione.txt.\n"
                                         "Dillo a Claude: li legge da solo." % len(scelti),
                                         unreal.AppMsgType.OK)
    except Exception:  # noqa: BLE001
        pass


main()
