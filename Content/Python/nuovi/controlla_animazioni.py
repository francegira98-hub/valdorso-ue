# Valdorso - Controlla le animazioni per Claude: numeri al posto degli screenshot. Scritto da Claude il 12/11/2026.
# Fra: "non riesci a fare uno script che ti dice tutti i particolari?". Questo legge, per ogni animazione di una
# cartella, dove stanno le ossa del colono in 7 momenti e cerca le cose che si vedono storte dopo un riadattamento:
#   - braccia tirate più lunghe del vero (il gomito non può allungarsi) o schiacciate;
#   - gomiti e ginocchia piegati al contrario o chiusi del tutto;
#   - mani dentro il corpo;
#   - piedi sotto il pavimento o che volano;
#   - schiena girata o piegata troppo.
# Scrive tutto in Saved/Claude/animazioni.txt (Claude lo legge da solo).
# Si lancia: Valdorso -> Strumenti -> Esegui script Python -> questo file. Di serie guarda /Game/Fab/Armi/Spada;
# per un'altra cartella: CARTELLA_ANIMAZIONI = "/Game/..." prima di lanciarlo (dal menu Valdorso, più avanti).

import math
import os

import unreal

CARTELLA = globals().get("CARTELLA_ANIMAZIONI", "/Game/Fab/Armi/Spada")
MOMENTI = 7

# lo scheletro del manichino di Unreal 5 (il nostro colono): figlio -> padre
PADRI = {
    "pelvis": "root", "spine_01": "pelvis", "spine_02": "spine_01", "spine_03": "spine_02", "spine_04": "spine_03",
    "spine_05": "spine_04", "neck_01": "spine_05", "neck_02": "neck_01", "head": "neck_02",
    "clavicle_l": "spine_05", "upperarm_l": "clavicle_l", "lowerarm_l": "upperarm_l", "hand_l": "lowerarm_l",
    "clavicle_r": "spine_05", "upperarm_r": "clavicle_r", "lowerarm_r": "upperarm_r", "hand_r": "lowerarm_r",
    "thigh_l": "pelvis", "calf_l": "thigh_l", "foot_l": "calf_l", "ball_l": "foot_l",
    "thigh_r": "pelvis", "calf_r": "thigh_r", "foot_r": "calf_r", "ball_r": "foot_r",
}

righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Animazioni: " + testo)


def posa(anim, t):
    """Dove sta ogni osso (cm, nello spazio del colono) al tempo t."""
    locali = {}
    for osso in ["root"] + list(PADRI):
        locali[osso] = unreal.AnimationLibrary.get_bone_pose_for_time(anim, osso, t, False)
    mondo = {"root": locali["root"]}

    def di(osso):
        if osso not in mondo:
            mondo[osso] = unreal.MathLibrary.compose_transforms(locali[osso], di(PADRI[osso]))
        return mondo[osso]

    return {o: di(o).translation for o in locali}


def v(a, b):
    return (b.x - a.x, b.y - a.y, b.z - a.z)


def lung(a):
    return math.sqrt(a[0] ** 2 + a[1] ** 2 + a[2] ** 2)


def angolo(a, b, c):
    """L'angolo in b (gradi) tra b->a e b->c: 180 = dritto, 0 = chiuso del tutto."""
    u, w = v(b, a), v(b, c)
    lu, lw = lung(u), lung(w)
    if lu < 1e-3 or lw < 1e-3:
        return 180.0
    cos = max(-1.0, min(1.0, (u[0] * w[0] + u[1] * w[1] + u[2] * w[2]) / (lu * lw)))
    return math.degrees(math.acos(cos))


def controlla(nome, anim):
    durata = anim.get_play_length()
    p0 = posa(anim, 0.0)
    braccio = {s: lung(v(p0["upperarm_" + s], p0["lowerarm_" + s])) + lung(v(p0["lowerarm_" + s], p0["hand_" + s])) for s in "lr"}
    problemi = []
    righe_momenti = []
    for k in range(MOMENTI):
        t = durata * k / (MOMENTI - 1)
        p = posa(anim, t)
        pezzi = []
        for s, lato in (("l", "sin"), ("r", "des")):
            spalla, gomito, mano = p["upperarm_" + s], p["lowerarm_" + s], p["hand_" + s]
            tesa = lung(v(spalla, mano)) / max(braccio[s], 1.0)
            ag = angolo(spalla, gomito, mano)
            ginocchio = angolo(p["thigh_" + s], p["calf_" + s], p["foot_" + s])
            dentro = lung(v(p["spine_03"], mano))
            piede = p["foot_" + s].z - p["root"].z
            pezzi.append("%s: gomito %3.0f, ginocchio %3.0f, mano-petto %3.0f cm, piede %+4.0f cm, mano a %+4.0f %+4.0f %+4.0f" % (
                lato, ag, ginocchio, dentro, piede, mano.x - p["pelvis"].x, mano.y - p["pelvis"].y, mano.z - p["pelvis"].z))
            if ag < 25:
                problemi.append("t=%.2f %s: gomito chiuso del tutto (%.0f gradi)" % (t, lato, ag))
            if tesa > 1.02:
                problemi.append("t=%.2f %s: braccio tirato più lungo del vero (%.0f%%)" % (t, lato, tesa * 100))
            if ginocchio < 30:
                problemi.append("t=%.2f %s: ginocchio chiuso del tutto (%.0f gradi)" % (t, lato, ginocchio))
            if dentro < 12:
                problemi.append("t=%.2f %s: mano dentro il petto (%.0f cm)" % (t, lato, dentro))
            if piede < -8:
                problemi.append("t=%.2f %s: piede sotto il pavimento (%.0f cm)" % (t, lato, piede))
        schiena = angolo(p["pelvis"], p["spine_03"], p["head"])
        if schiena < 110:
            problemi.append("t=%.2f: schiena piegata molto (%.0f gradi)" % (t, schiena))
        righe_momenti.append("   t=%.2f  testa %+4.0f cm, schiena %3.0f | %s" % (t, p["head"].z - p["root"].z, schiena, " | ".join(pezzi)))
    scrivi("%s (%.2f s): %s" % (nome, durata, "OK" if not problemi else "%d cose da guardare" % len(problemi)))
    for r in problemi[:12]:
        scrivi("   ! " + r)
    righe.extend(righe_momenti)


def principale():
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    dati = [d for d in registro.get_assets_by_path(CARTELLA, recursive=True)
            if str(d.asset_class_path.asset_name) == "AnimSequence"]
    scrivi("cartella %s: %d animazioni" % (CARTELLA, len(dati)))
    for d in sorted(dati, key=lambda x: str(x.asset_name)):
        anim = unreal.EditorAssetLibrary.load_asset(str(d.package_name))
        try:
            controlla(str(d.asset_name), anim)
        except Exception as e:  # noqa: BLE001
            scrivi("%s: non riesco a leggerla (%s)" % (d.asset_name, e))
    scrivi("FATTO")


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "animazioni.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
