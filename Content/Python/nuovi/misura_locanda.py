# Valdorso - Misura la locanda di oggi (16/11/2026, scritto da Claude; per la v0.2.10 "Casa mia", locande approvate da Fra il 16/11).
# NON cambia niente nel livello: guarda e scrive.
#
# Cerca la locanda di Bruno (l'attore "Locanda_di_Bruno" e quelli che cominciano con "Locanda") e scrive:
#   - dove sta (posizione, yaw, ingombro, etichette delle fasi);
#   - l'impronta della locanda nuova della chat 3D (15 x 10 m, origine in basso al centro, ingresso verso -Y) messa
#     al posto di quella di oggi: la quota del terreno sui bordi (la fondazione e' di 1 m: se il dislivello e' di piu',
#     lo dice) e cosa c'e' troppo vicino (case, alberi, panche...);
#   - cosa c'e' dentro la locanda di oggi (letti, panche, il baule del colono, ...): andra' rimesso nella nuova.
# Risultato: Saved/Claude/locanda_oggi.txt (da leggere) e Saved/Claude/locanda_oggi.json (da mandare alla chat 3D).
# Menu Valdorso -> Villaggio -> "Locande: misura la locanda di oggi".

import json
import math
import os

import unreal

NUOVA_X, NUOVA_Y = 1500.0, 1000.0          # SM_Locanda_Livia della chat 3D (la locanda di Bruno nuova)
FONDAZIONE = 100.0
MARGINE = 200.0                            # chi sta a meno di 2 m dall'impronta nuova e' "troppo vicino"

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
righe = []
dati = {}


def scrivi(t):
    righe.append(t)
    unreal.log("[Valdorso] Misura locanda: " + t)


def avviso(t):
    righe.append("ATTENZIONE: " + t)
    unreal.log_warning("[Valdorso] Misura locanda: " + t)


def al_mondo(ox, oy, yaw, lx, ly):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return ox + lx * c - ly * s, oy + lx * s + ly * c


def al_locale(ox, oy, yaw, x, y):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    dx, dy = x - ox, y - oy
    return dx * c + dy * s, -dx * s + dy * c


def terreno(x, y, z):
    """La quota del paesaggio (solo il Landscape: salta case, alberi e il resto)."""
    try:
        r = unreal.SystemLibrary.line_trace_multi(mondo, unreal.Vector(x, y, z + 200000.0), unreal.Vector(x, y, z - 200000.0),
                                                  unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                  unreal.DrawDebugTrace.NONE, True)
        colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
    except Exception:  # noqa: BLE001
        colpi = []
    for hit in colpi or []:
        t = hit.to_tuple()
        if t[9] is not None and "Landscape" in t[9].get_class().get_name():
            return t[4].z
    return None


def etichette(a):
    return [str(t) for t in a.tags]


def scheda(a):
    q = a.get_actor_location()
    o, e = a.get_actor_bounds(False)
    return {"nome": a.get_actor_label(), "classe": a.get_class().get_name(),
            "posizione_cm": [round(q.x), round(q.y), round(q.z)], "yaw": round(a.get_actor_rotation().yaw, 1),
            "ingombro_cm": [round(e.x * 2), round(e.y * 2), round(e.z * 2)], "etichette": etichette(a)}


def leggero(a):
    n = a.get_class().get_name()
    return any(k in n for k in ("Landscape", "Foliage", "Grass", "Light", "Sky", "Fog", "Volume", "Water", "PostProcess",
                                "Atmosphere", "Cloud", "WorldSettings", "Brush", "Info", "Camera", "Note", "Player"))


def principale():
    tutti = [a for a in attori.get_all_level_actors() if a]
    locanda = None
    famiglia = []
    for a in tutti:
        n = a.get_actor_label()
        if n == "Locanda_di_Bruno":
            locanda = a
        if n.startswith("Locanda"):
            famiglia.append(a)
    if locanda is None:
        nomi = ", ".join(a.get_actor_label() for a in famiglia) or "nessuno"
        avviso("non trovo l'attore 'Locanda_di_Bruno'. Attori che cominciano con 'Locanda': %s" % nomi)
        return
    q = locanda.get_actor_location()
    yaw = locanda.get_actor_rotation().yaw
    dati["locanda_di_oggi"] = scheda(locanda)
    scrivi("LA LOCANDA DI OGGI: %s a (%.0f, %.0f, %.0f), yaw %.1f, ingombro %s cm, etichette %s"
           % (locanda.get_actor_label(), q.x, q.y, q.z, yaw, dati["locanda_di_oggi"]["ingombro_cm"], etichette(locanda)))
    dati["pezzi_della_locanda"] = [scheda(a) for a in famiglia if a != locanda]
    for s in dati["pezzi_della_locanda"]:
        scrivi("  pezzo: %s (%s) a %s, yaw %s" % (s["nome"], s["classe"], s["posizione_cm"], s["yaw"]))

    # L'impronta nuova, con lo stesso centro e lo stesso yaw.
    hx, hy = NUOVA_X / 2.0, NUOVA_Y / 2.0
    punti = {"angolo davanti-sinistra": (-hx, -hy), "angolo davanti-destra": (hx, -hy), "angolo dietro-destra": (hx, hy),
             "angolo dietro-sinistra": (-hx, hy), "meta' davanti (ingresso)": (0.0, -hy), "meta' dietro": (0.0, hy),
             "meta' sinistra": (-hx, 0.0), "meta' destra": (hx, 0.0), "centro": (0.0, 0.0)}
    quote = {}
    for nome, (lx, ly) in punti.items():
        x, y = al_mondo(q.x, q.y, yaw, lx, ly)
        z = terreno(x, y, q.z)
        quote[nome] = None if z is None else round(z - q.z)
    dati["terreno_sotto_la_nuova_cm"] = {"rispetto_al_pavimento_di_oggi": quote, "nota": "negativo = il terreno sta sotto il pavimento"}
    buone = [v for v in quote.values() if v is not None]
    scrivi("TERRENO sotto l'impronta nuova (15 x 10 m), rispetto al pavimento di oggi (cm): %s" % quote)
    if buone:
        giu, su = min(buone), max(buone)
        if giu < -FONDAZIONE:
            avviso("in un punto il terreno sta %d cm sotto il pavimento: la fondazione (100 cm) non basta, si vedrebbe un buco" % -giu)
        if su > 0:
            avviso("in un punto il terreno sta %d cm sopra il pavimento: entrerebbe dentro la sala" % su)
        if giu >= -FONDAZIONE and su <= 0:
            scrivi("la fondazione da 1 m basta dappertutto")

    # Chi sta troppo vicino (fuori dalla famiglia della locanda) e cosa c'e' dentro quella di oggi.
    vicini, dentro = [], []
    for a in tutti:
        if a in famiglia or leggero(a):
            continue
        o, e = a.get_actor_bounds(False)
        if e.x > 5000 or e.y > 5000:
            continue
        lx, ly = al_locale(q.x, q.y, yaw, o.x, o.y)
        r = max(e.x, e.y) * 0.7
        if abs(lx) <= hx + MARGINE + r and abs(ly) <= hy + MARGINE + r and abs(o.z - q.z) < 1500:
            s = scheda(a)
            s["locale_cm"] = [round(lx), round(ly)]
            (dentro if abs(lx) < hx and abs(ly) < hy else vicini).append(s)
    dati["dentro"] = dentro
    dati["vicini"] = vicini
    scrivi("DENTRO l'impronta nuova: %d cose (andranno rimesse o spostate)" % len(dentro))
    for s in dentro:
        scrivi("  %s (%s) locale %s" % (s["nome"], s["classe"], s["locale_cm"]))
    scrivi("VICINI a meno di 2 m dall'impronta nuova: %d" % len(vicini))
    for s in vicini:
        scrivi("  %s (%s) locale %s, ingombro %s" % (s["nome"], s["classe"], s["locale_cm"], s["ingombro_cm"]))
    scrivi("FATTO. Manda a Claude il file Saved/Claude/locanda_oggi.txt (o la foto di questo registro).")


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "locanda_oggi.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
    with open(os.path.join(cartella, "locanda_oggi.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)
