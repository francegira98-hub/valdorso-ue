# Valdorso - La grata di prova (16/11/2026, scritto da Claude; v0.2.6 "Le mura", approvata da Fra il 15/11).
# Serve il C++ della v0.2.6 compilato (ValdorsoMura.h: AValdorsoGrata e AValdorsoLeva; le chiavi nelle porte).
#
# Mette, davanti alla telecamera dell'editor (6 m avanti, sul terreno):
#   - una grata (AValdorsoGrata) di 4 x 4,8 m, girata in modo che si passi nella direzione in cui guardi;
#   - la sua leva (AValdorsoLeva), 3 m a destra, con il lucchetto: la usa chi ha la Chiave_Porta (o lo staff);
#   - una porta chiusa a chiave (AValdorsoPorta), 3 m a sinistra: la apre chi ha la Chiave_Torri.
# Per provare: Play, poi in console
#   Valdorso.Zaino.Dai Chiave_Porta      (la leva: E "Alza la grata")
#   Valdorso.Zaino.Dai Chiave_Torri      (la porta: E "Apri con la chiave")
# Si rilancia quando si vuole: toglie la prova di prima. Menu Valdorso -> Villaggio:
#   "Mura: metti una grata di prova davanti alla telecamera" / "Mura: togli la grata di prova".
# Quando arrivano i modelli della chat 3D, le grate vere le mette lo script delle mura (con le porte della citta').

import math
import os

import unreal

MODO = str(globals().get("GRATA_MODO", "metti"))
ETICHETTA = "ValdorsoGrataProva"
CARTELLA = "Valle/Mura (prova)"

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(t):
    righe.append(t)
    unreal.log("[Valdorso] Grata di prova: " + t)


def avviso(t):
    righe.append("ATTENZIONE: " + t)
    unreal.log_warning("[Valdorso] Grata di prova: " + t)


def a_terra(mondo, x, y, z):
    r = unreal.SystemLibrary.line_trace_single(mondo, unreal.Vector(x, y, z + 3000.0), unreal.Vector(x, y, z - 20000.0),
                                               unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                               unreal.DrawDebugTrace.NONE, True)
    t = r.to_tuple() if r else None
    return t[4].z if t and t[0] else z


def finisci(a, nome):
    a.set_actor_label(nome)
    a.set_folder_path(CARTELLA)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])


def principale():
    via = 0
    for a in attori.get_all_level_actors():
        if a and a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            via += 1
    if via:
        scrivi("tolti %d pezzi della prova di prima" % via)
    if MODO == "togli":
        scrivi("FATTO: prova tolta")
        return
    classe_grata = getattr(unreal, "ValdorsoGrata", None)
    classe_leva = getattr(unreal, "ValdorsoLeva", None)
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    if classe_grata is None or classe_leva is None:
        avviso("mancano le classi ValdorsoGrata e ValdorsoLeva: serve compilare il C++ della v0.2.6")
        return
    r = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_level_viewport_camera_info()
    # (16/11 sera) in Unreal 5.8 i valori sono due (posizione, rotazione), non tre
    if isinstance(r, tuple) and len(r) == 3:
        ok, dove, gira = r
    elif isinstance(r, tuple) and len(r) == 2:
        dove, gira = r
        ok = dove is not None
    else:
        ok, dove, gira = False, None, None
    if not ok:
        avviso("non trovo la telecamera dell'editor")
        return
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    yaw = gira.yaw
    fx, fy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    dx, dy = -fy, fx                      # a destra di chi guarda
    gx, gy = dove.x + fx * 600.0, dove.y + fy * 600.0
    gz = a_terra(mondo, gx, gy, dove.z)
    grata = attori.spawn_actor_from_class(classe_grata, unreal.Vector(gx, gy, gz), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    finisci(grata, "Grata di prova")
    lx, ly = gx + dx * 300.0 - fx * 150.0, gy + dy * 300.0 - fy * 150.0
    leva = attori.spawn_actor_from_class(classe_leva, unreal.Vector(lx, ly, a_terra(mondo, lx, ly, gz)),
                                         unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    leva.set_editor_property("grata", grata)
    finisci(leva, "Leva della grata di prova")
    scrivi("grata a (%.0f, %.0f, %.0f), leva 3 m a destra (chiave: Chiave_Porta)" % (gx, gy, gz))
    if classe_porta is not None:
        px, py = gx - dx * 300.0 - fx * 150.0, gy - dy * 300.0 - fy * 150.0
        porta = attori.spawn_actor_from_class(classe_porta, unreal.Vector(px, py, a_terra(mondo, px, py, gz)),
                                              unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
        porta.set_editor_property("chiusa_a_chiave", True)
        porta.set_editor_property("chiave", unreal.Name("Chiave_Torri"))
        finisci(porta, "Porta chiusa di prova")
        scrivi("porta chiusa a chiave 3 m a sinistra (chiave: Chiave_Torri)")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
    scrivi("FATTO. Play, poi in console: Valdorso.Zaino.Dai Chiave_Porta  e  Valdorso.Zaino.Dai Chiave_Torri")


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "grata_prova.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
