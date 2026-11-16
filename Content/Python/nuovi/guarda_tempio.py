# Valdorso - Guarda che cosa c'e' dentro il Tempio del Cuore (16/11/2026, scritto da Claude; v0.2.7 "Il tempio").
# NON CAMBIA NIENTE: legge il livello e scrive un resoconto per Claude.
#
# Perche': Fra vede un muro o una trave a mezz'aria dentro la pieve (fase 3). Nel modello della pieve ci sono le catene
# di legno delle capriate (travi da parete a parete a 9,5 m, una ogni 4 m): sono giuste. Ma potrebbe esserci altro:
# la chiesa della fase 2 (il suo muro di fondo cade dentro l'abside della pieve, le sue travi stanno 2 m piu' in basso),
# pezzi della cappella vecchia senza fase, alberi, case, lanterne, mobili...
# Questo script prende ogni cosa del livello che tocca l'ingombro della chiesa (fase 2) e della pieve (fase 3) e scrive:
# nome, classe, modello, etichette di fase, se c'e' nella fase 3 (come fa il gioco), se nell'editor e' nascosta, e dove
# sta nel sistema del tempio (x a destra, y verso la porta, z dal pavimento). Segna "SOSPESO" cio' che sta in aria.
#
# Voce nel menu Valdorso -> Villaggio: "Tempio: guarda cosa c'e' dentro (non cambia niente)".
# Resoconto: Saved/Claude/tempio_dentro.txt (e .json). Va lanciato con la zona del tempio caricata (L_Valle aperto,
# la telecamera vicino alla piazza), cosi' World Partition ha caricato tutto quello che sta li'.

import json
import math
import os

import unreal

PREFISSO = "ValdorsoFase "
NOME_FASE = {2: "Tempio del Cuore, fase 2", 3: "Tempio del Cuore, fase 3"}
# ingombri dalla scheda della chat 3D (Valdorso_mura_tempio.json, "tempio" -> "ingombri_cm"), sistema del tempio
INGOMBRO = {2: ((-1070.0, 650.0), (-1940.0, 1140.0), (0.0, 2350.0)),
            3: ((-1070.0, 800.0), (-2530.0, 1500.0), (0.0, 2550.0))}
# la navata libera (dove si cammina): x -520..520, y dalla porta (1020) al fondo; sopra i 150 cm e sotto le catene
NAVATA_X = 520.0
SOSPESO_DA = 150.0
TROPPO_GRANDE = 60000.0     # paesaggio, cielo, volumi: non interessano

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(t):
    righe.append(t)
    unreal.log("[Valdorso] Tempio: " + t)


def fase_di(a):
    for t in a.get_editor_property("tags"):
        s = str(t)
        if s.startswith(PREFISSO):
            try:
                da, fino = s[len(PREFISSO):].split("-")
                return int(da), int(fino)
            except ValueError:
                return None
    return None


def ce_nella_fase(f, fase):
    if f is None:
        return True
    return f[0] <= fase and (f[1] == 0 or fase <= f[1])


def locale(o, yaw, p):
    """Un punto del mondo nel sistema del tempio (o = origine, yaw del pezzo)."""
    a = math.radians(-yaw)
    dx, dy = p.x - o.x, p.y - o.y
    return (dx * math.cos(a) - dy * math.sin(a), dx * math.sin(a) + dy * math.cos(a), p.z - o.z)


def scatola_locale(a, o, yaw):
    centro, mezza = a.get_actor_bounds(False)
    if max(mezza.x, mezza.y, mezza.z) > TROPPO_GRANDE:
        return None
    pts = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            for sz in (-1, 1):
                pts.append(locale(o, yaw, unreal.Vector(centro.x + sx * mezza.x, centro.y + sy * mezza.y,
                                                        centro.z + sz * mezza.z)))
    return [[min(p[i] for p in pts), max(p[i] for p in pts)] for i in range(3)]


def tocca(s, ing):
    return all(s[i][0] < ing[i][1] and s[i][1] > ing[i][0] for i in range(3))


def modelli(a):
    nomi = []
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        m = c.get_editor_property("static_mesh")
        if m is not None:
            nomi.append(m.get_path_name().split(".")[0])
    return sorted(set(nomi))[:4]


def guarda():
    per_nome = {}
    for a in attori.get_all_level_actors():
        if a:
            per_nome.setdefault(a.get_actor_label(), a)
    tempio = {f: per_nome.get(n) for f, n in NOME_FASE.items()}
    cappella = per_nome.get("Tempio_del_Cuore")
    if tempio[3] is None:
        scrivi("ATTENZIONE: nel livello non c'e' 'Tempio del Cuore, fase 3' (lancia 'Mura: 3. il tempio che cresce')")
        return {}
    o = tempio[3].get_actor_location()
    yaw = tempio[3].get_actor_rotation().yaw
    scrivi("pieve a (%.0f, %.0f, %.0f), yaw %.1f; chiesa %s; cappella %s"
           % (o.x, o.y, o.z, yaw, "c'e'" if tempio[2] else "NON c'e'", "c'e'" if cappella else "NON c'e'"))
    for f, t in tempio.items():
        if t:
            scrivi("  %s: etichette %s, nascosta nell'editor: %s"
                   % (NOME_FASE[f], [str(x) for x in t.get_editor_property("tags")], t.is_temporarily_hidden_in_editor()))
    if cappella:
        scrivi("  cappella: etichette %s, nascosta nell'editor: %s"
               % ([str(x) for x in cappella.get_editor_property("tags")], cappella.is_temporarily_hidden_in_editor()))

    trovate = []
    for a in attori.get_all_level_actors():
        if a is None or a in (tempio[3],):
            continue
        s = scatola_locale(a, o, yaw)
        if s is None:
            continue
        dentro = [f for f in (2, 3) if tocca(s, INGOMBRO[f])]
        if not dentro:
            continue
        f = fase_di(a)
        in3 = ce_nella_fase(f, 3)
        nella_navata = abs((s[0][0] + s[0][1]) / 2.0) < NAVATA_X and -2530.0 < (s[1][0] + s[1][1]) / 2.0 < 1020.0
        sospeso = s[2][0] > SOSPESO_DA and nella_navata
        trovate.append({
            "nome": a.get_actor_label(),
            "classe": a.get_class().get_name(),
            "cartella": str(a.get_folder_path()),
            "modelli": modelli(a),
            "fase": ("%d-%d" % f) if f else "sempre",
            "nella_fase_3": in3,
            "nascosta_editor": a.is_temporarily_hidden_in_editor(),
            "x": [round(v) for v in s[0]], "y": [round(v) for v in s[1]], "z": [round(v) for v in s[2]],
            "nella_navata": nella_navata, "sospeso": sospeso, "dentro": dentro})

    # prima quelle che si vedono nella fase 3 dentro la navata, poi le altre
    trovate.sort(key=lambda d: (not d["nella_fase_3"], not d["nella_navata"], -d["z"][0]))
    scrivi("")
    scrivi("=== DENTRO LA PIEVE e LA CHIESA: %d cose (x a destra, y verso la porta a +1100, z dal pavimento; cm)" % len(trovate))
    for d in trovate:
        segni = []
        if d["nella_fase_3"] and d["nella_navata"]:
            segni.append("SI VEDE IN FASE 3")
        if d["sospeso"] and d["nella_fase_3"]:
            segni.append("SOSPESO")
        if d["nascosta_editor"]:
            segni.append("nascosta nell'editor")
        scrivi("%-44s %-24s fase %-6s x %6d..%-6d y %6d..%-6d z %5d..%-5d %s %s"
               % (d["nome"][:44], d["classe"][:24], d["fase"], d["x"][0], d["x"][1], d["y"][0], d["y"][1],
                  d["z"][0], d["z"][1], " ".join(segni), ("| " + ", ".join(d["modelli"])) if d["modelli"] else ""))
    sospese = [d for d in trovate if d["sospeso"] and d["nella_fase_3"]]
    scrivi("")
    scrivi("in aria nella navata e visibili in fase 3: %d -> %s" % (len(sospese), ", ".join(d["nome"] for d in sospese) or "nessuna"))
    return {"pieve": [o.x, o.y, o.z, yaw], "cose": trovate}


dati = {}
try:
    dati = guarda()
except Exception as e:  # noqa: BLE001
    scrivi("ATTENZIONE: errore: %s" % e)
    raise
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "tempio_dentro.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
    with open(os.path.join(cartella, "tempio_dentro.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)
    scrivi("FATTO: Saved/Claude/tempio_dentro.txt (non ho cambiato niente)")
