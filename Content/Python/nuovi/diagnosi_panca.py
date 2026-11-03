# Valdorso - Diagnosi delle animazioni della panca (v0.1.3). Scritto da Claude il 03/11/2026. Sola lettura: non cambia niente.
# Per ognuna delle 14 animazioni in /Game/Fab/GameAnimationSample/Panca (e, per confronto, MM_Idle del manichino) scrive:
# durata, root motion acceso, il movimento della radice (root) dall'inizio alla fine, dove sta il bacino (pelvis) e verso
# dove guarda il corpo (calcolato dalle due cosce) all'inizio e alla fine.
# Risultato in Saved/Claude/panca_diagnosi.txt (Claude lo legge da solo dal PC di Fra).

import math
import os
import unreal

CARTELLA = "/Game/Fab/GameAnimationSample/Panca/"
NOMI = [
    "Siede_F_Lfoot", "Siede_F_Rfoot", "Siede_L90_Lfoot", "Siede_L90_Rfoot", "Siede_R90_Lfoot", "Siede_R90_Rfoot",
    "Siede_BL_Lfoot", "Siede_BL_Rfoot", "Siede_BR_Lfoot", "Siede_BR_Rfoot",
    "Seduto", "Alza_InPiedi", "Alza_Cammina", "Alza_Corre",
]
RIFERIMENTO = "/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"

CATENE = {
    "pelvis": ["root", "pelvis"],
    "thigh_l": ["root", "pelvis", "thigh_l"],
    "thigh_r": ["root", "pelvis", "thigh_r"],
    "ball_l": ["root", "pelvis", "thigh_l", "calf_l", "foot_l", "ball_l"],
    "ball_r": ["root", "pelvis", "thigh_r", "calf_r", "foot_r", "ball_r"],
    "head": ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "neck_01", "neck_02", "head"],
}

lib = unreal.AnimationLibrary
righe = []


def scrivi(testo=""):
    righe.append(testo)


def v(t):
    p = t.translation
    return "(%7.1f %7.1f %7.1f)" % (p.x, p.y, p.z)


def imbardata(t):
    return t.rotation.rotator().yaw


def locale(anim, osso, tempo, radice_estratta=False):
    try:
        return lib.get_bone_pose_for_time(anim, osso, tempo, radice_estratta)
    except Exception as errore:  # noqa: BLE001
        scrivi("   ! %s a %.2f s: %s" % (osso, tempo, errore))
        return unreal.Transform()


def componente(anim, nome, tempo):
    """Dove sta l'osso nello spazio della mesh (component space), salendo la catena dei genitori."""
    totale = unreal.Transform()
    for osso in CATENE[nome]:
        # C++: Componente(figlio) = Locale(figlio) * Componente(genitore)
        totale = unreal.MathLibrary.compose_transforms(locale(anim, osso, tempo), totale)
    return totale


def verso_corpo(anim, tempo):
    """Imbardata del davanti del corpo (dalle cosce: davanti = sinistra girata di 90 gradi). Manichino normale: 90."""
    sx = componente(anim, "thigh_l", tempo).translation
    dx = componente(anim, "thigh_r", tempo).translation
    lx, ly = sx.x - dx.x, sx.y - dx.y
    return math.degrees(math.atan2(lx, -ly))


def descrivi(percorso):
    anim = unreal.load_asset(percorso)
    if anim is None:
        scrivi("%s: NON TROVATA" % percorso)
        return
    durata = lib.get_sequence_length(anim)
    scrivi("== %s" % anim.get_name())
    try:
        scrivi("   durata %.2f s, fotogrammi %d, root motion %s, blocco %s" % (
            durata, lib.get_num_frames(anim), anim.get_editor_property("enable_root_motion"),
            anim.get_editor_property("root_motion_root_lock")))
    except Exception as errore:  # noqa: BLE001
        scrivi("   durata %.2f s (%s)" % (durata, errore))
    r0 = locale(anim, "root", 0.0)
    r1 = locale(anim, "root", durata)
    scrivi("   root   inizio %s yaw %7.1f | fine %s yaw %7.1f" % (v(r0), imbardata(r0), v(r1), imbardata(r1)))
    try:
        mov = lib.extract_root_motion_from_time_range(anim, 0.0, durata)
        scrivi("   movimento della radice (come lo legge il C++): %s yaw %7.1f" % (v(mov), imbardata(mov)))
    except Exception as errore:  # noqa: BLE001
        scrivi("   movimento della radice: non disponibile (%s)" % errore)
    for tempo, etichetta in ((0.0, "inizio"), (durata * 0.5, "metà  "), (durata, "fine  ")):
        bacino = componente(anim, "pelvis", tempo)
        testa = componente(anim, "head", tempo)
        scrivi("   %s: bacino %s  testa %s  corpo guarda %7.1f  (radice yaw %7.1f)" % (
            etichetta, v(bacino), v(testa), verso_corpo(anim, tempo), imbardata(locale(anim, "root", tempo))))
    scrivi()


def principale():
    scrivi("Diagnosi panca - %s" % unreal.SystemLibrary.get_engine_version())
    scrivi("Spazio della mesh: il manichino normale guarda +Y (yaw 90). Le posizioni sono in cm, relative alla mesh.")
    scrivi()
    descrivi(RIFERIMENTO)
    for nome in NOMI:
        descrivi(CARTELLA + nome)
    cartella = os.path.join(unreal.Paths.project_saved_dir(), "Claude")
    os.makedirs(cartella, exist_ok=True)
    file = os.path.join(cartella, "panca_diagnosi.txt")
    with open(file, "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))
    unreal.log("[Valdorso] Diagnosi panca scritta in %s (%d righe)." % (os.path.abspath(file), len(righe)))


principale()
