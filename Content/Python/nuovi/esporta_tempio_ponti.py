# Valdorso - I dati per il sagrato del tempio e per i ponti, per la chat 3D (17/11/2026, scritto da Claude; v0.2.7).
# NON cambia niente nel livello: guarda e scrive. Menu Valdorso -> Villaggio ->
# "Tempio e ponti: esporta i dati per la chat 3D". Si lancia in L_Valle, non in Play.
#
# Scrive Saved/Claude/dati_tempio_ponti.json (e .txt da leggere), in cm della Pianta (mondo meno l'angolo del
# paesaggio; Z = quota vera del mondo):
#   - tempio: origine, yaw e porta dei tre modelli (cappella, chiesa, pieve); dalla porta verso la piazza, ogni 25 cm
#     per 40 m: la quota del terreno e che cosa c'e' sopra (terreno, strada, piazza), e a che distanza comincia la piazza;
#   - corsi d'acqua: la linea di mezzo di ogni corso (AValdorsoAcque.Corsi) con il pelo dell'acqua e la larghezza;
#   - ponti ("Ponte del mulino", "Ponte a nord"): dove sono oggi (attore, yaw, ingombro), la linea della Pianta, e lungo
#     quella linea allungata di 15 m per parte, ogni 50 cm: quota del terreno, pelo dell'acqua se li' c'e' acqua, e le
#     due sponde (dove il terreno passa sopra il pelo).

import json
import math
import os

import unreal

PIANTA = "V:/Texture/valdorso/villaggio/Pianta_3000.json"
PONTI = ("Ponte del mulino", "Ponte a nord")
TEMPLI = (("cappella (fase 1)", "Tempio_del_Cuore"), ("chiesa (fase 2)", "Tempio del Cuore, fase 2"),
          ("pieve (fase 3)", "Tempio del Cuore, fase 3"))

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
righe = []


def scrivi(t):
    righe.append(t)


def colpi(a, b, ignora):
    """Tutte le cose che una linea tocca (canale della vista), come liste di tuple."""
    try:
        r = unreal.SystemLibrary.line_trace_multi(mondo, a, b, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora,
                                                  unreal.DrawDebugTrace.NONE, True)
        r = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
    except Exception:  # noqa: BLE001
        r = []
    return [h.to_tuple() for h in (r or [])]


def terreno(x, y):
    """La quota del paesaggio in (x, y) del mondo: il raggio salta tutto cio' che non e' paesaggio. None se non c'e'."""
    ignora = []
    for _ in range(16):
        fermato = None
        for t in colpi(unreal.Vector(x, y, 200000.0), unreal.Vector(x, y, -200000.0), ignora):
            a = t[9]
            if a is None:
                continue
            if "Landscape" in a.get_class().get_name():
                return t[4].z
            if t[0]:
                fermato = a
        if fermato is None or fermato in ignora:
            return None
        ignora.append(fermato)
    return None


def sopra(x, y, vicino, ignora):
    """La prima cosa che si incontra scendendo da 6 m sopra 'vicino': (quota, nome). None se niente."""
    for t in colpi(unreal.Vector(x, y, vicino + 600.0), unreal.Vector(x, y, vicino - 2000.0), ignora):
        if t[0] and t[9] is not None:
            a = t[9]
            n = "terreno" if "Landscape" in a.get_class().get_name() else a.get_actor_label()
            return t[4].z, n
    return None


def pelo_qui(corsi, x, y):
    """Il pelo dell'acqua in (x, y) del mondo e il nome del corso, se il punto e' sull'acqua (entro mezza larghezza)."""
    meglio = None
    for c in corsi:
        p = c["punti_mondo"]
        for i in range(len(p) - 1):
            ax, ay, az = p[i]
            bx, by, bz = p[i + 1]
            dx, dy = bx - ax, by - ay
            l2 = dx * dx + dy * dy
            if l2 < 1e-6:
                continue
            u = max(0.0, min(1.0, ((x - ax) * dx + (y - ay) * dy) / l2))
            d = math.hypot(x - (ax + dx * u), y - (ay + dy * u))
            la = c["larghezze"][i] if i < len(c["larghezze"]) else 0.0
            lb = c["larghezze"][i + 1] if i + 1 < len(c["larghezze"]) else la
            larga = la + (lb - la) * u
            if d <= larga / 2.0 and (meglio is None or d < meglio[0]):
                meglio = (d, az + (bz - az) * u, c["nome"], larga)
    return None if meglio is None else (meglio[1], meglio[2], meglio[3])


def principale():
    tutti = [a for a in attori.get_all_level_actors() if a]
    angolo = None
    per_nome = {}
    acque = None
    for a in tutti:
        per_nome.setdefault(a.get_actor_label(), a)
        if isinstance(a, unreal.Landscape) and angolo is None:
            angolo = a.get_actor_location()
        if a.get_class().get_name() == "ValdorsoAcque":
            acque = a
    if angolo is None:
        scrivi("ERRORE: non trovo il paesaggio (sei in L_Valle?)")
        return
    ax, ay = angolo.x, angolo.y

    def pianta(x, y):
        return [round(x - ax, 1), round(y - ay, 1)]

    dati = {"_come_si_legge": "cm della Pianta (mondo meno l'angolo del paesaggio); z = quota vera del mondo in cm; yaw in gradi di Unreal",
            "angolo_paesaggio_mondo": [ax, ay, angolo.z]}

    # ---------------------------------------------------------------- i corsi d'acqua
    corsi = []
    if acque is not None:
        try:
            for c in acque.get_editor_property("corsi"):
                punti = [(p.x, p.y, p.z) for p in c.get_editor_property("punti")]
                corsi.append({"nome": str(c.get_editor_property("nome")), "punti_mondo": punti,
                              "larghezze": [float(v) for v in c.get_editor_property("larghezze")]})
        except Exception as e:  # noqa: BLE001
            scrivi("ATTENZIONE: non leggo i corsi d'acqua (%s)" % e)
    dati["corsi_acqua"] = [{"nome": c["nome"],
                            "punti": [[round(x - ax, 1), round(y - ay, 1), round(z, 1)] for x, y, z in c["punti_mondo"]],
                            "larghezze_cm": [round(v, 1) for v in c["larghezze"]]} for c in corsi]
    scrivi("corsi d'acqua: %d (%s)" % (len(corsi), ", ".join("%s: %d punti" % (c["nome"], len(c["punti_mondo"])) for c in corsi)))

    # ---------------------------------------------------------------- il tempio e la piazza
    tempio = {"modelli": []}
    piazza = per_nome.get("Piazza")
    if piazza is not None:
        o, e = piazza.get_actor_bounds(False)
        tempio["piazza_ingombro"] = {"min": pianta(o.x - e.x, o.y - e.y), "max": pianta(o.x + e.x, o.y + e.y),
                                     "z_min": round(o.z - e.z, 1), "z_max": round(o.z + e.z, 1)}
    cappella = per_nome.get("Tempio_del_Cuore")
    templi = [per_nome[n] for _, n in TEMPLI if n in per_nome]
    # (secondo controllo) nell'editor le cose delle altre fasi ci sono tutte insieme (anche nascoste hanno la collisione):
    # per ogni modello si guardano solo le cose della sua fase
    fasi = {}
    for a in tutti:
        for t in a.get_editor_property("tags"):
            x = str(t)
            if x.startswith("ValdorsoFase "):
                try:
                    da, fino = [int(v) for v in x[len("ValdorsoFase "):].split("-")]
                    fasi[a] = (da, fino)
                except ValueError:
                    pass
                break
    for numero, (titolo, nome) in enumerate(TEMPLI, start=1):
        fuori_fase = [a for a, (da, fino) in fasi.items() if not (da <= numero and (fino == 0 or numero <= fino))]
        a = per_nome.get(nome)
        if a is None:
            scrivi("ATTENZIONE: nel livello non c'e' '%s'" % nome)
            continue
        o = a.get_actor_location()
        yaw = a.get_actor_rotation().yaw
        # la cappella ha la porta a (0, -1100) suoi, i modelli nuovi a (0, +1100): vedi crea_mura.py, fai_tempio
        ly = -1100.0 if a == cappella else 1100.0
        r = math.radians(yaw)
        px, py = o.x - math.sin(r) * ly, o.y + math.cos(r) * ly
        lung = math.hypot(px - o.x, py - o.y)
        ux, uy = (px - o.x) / lung, (py - o.y) / lung
        bo, be = a.get_actor_bounds(False)
        m = {"quale": titolo, "attore": nome, "origine": pianta(o.x, o.y) + [round(o.z, 1)], "yaw": round(yaw, 1),
             "porta": pianta(px, py), "porta_z_terreno": terreno(px, py), "verso_fuori": [round(ux, 4), round(uy, 4)],
             "ingombro": {"min": pianta(bo.x - be.x, bo.y - be.y), "max": pianta(bo.x + be.x, bo.y + be.y),
                          "z_min": round(bo.z - be.z, 1), "z_max": round(bo.z + be.z, 1)}}
        profilo = []
        inizio_piazza = None
        s = 0.0
        while s <= 4000.0:
            x, y = px + ux * s, py + uy * s
            zt = terreno(x, y)
            su = sopra(x, y, zt if zt is not None else o.z, templi + fuori_fase)
            profilo.append([round(s), None if zt is None else round(zt, 1), None if su is None else round(su[0], 1),
                            "" if su is None else su[1]])
            if inizio_piazza is None and su is not None and su[1] == "Piazza":
                inizio_piazza = s
            s += 25.0
        m["dalla_porta_verso_fuori"] = {"colonne": ["distanza_cm", "z_terreno", "z_sopra", "che_cosa_c_e_sopra"],
                                        "punti": profilo}
        m["la_piazza_comincia_a_cm"] = inizio_piazza
        tempio["modelli"].append(m)
        scrivi("%s: origine Pianta (%.0f, %.0f) z %.0f, yaw %.0f, porta (%.0f, %.0f); la piazza comincia a %s cm dalla porta"
               % (titolo, o.x - ax, o.y - ay, o.z, yaw, px - ax, py - ay,
                  "?" if inizio_piazza is None else "%.0f" % inizio_piazza))
    dati["tempio"] = tempio

    # ---------------------------------------------------------------- i ponti
    linee = {}
    try:
        with open(PIANTA, "r", encoding="utf-8-sig") as f:
            for o in json.load(f):
                if o.get("nome") in PONTI and o.get("tipo") == "linea":
                    linee[o["nome"]] = o["punti"]
    except Exception as e:  # noqa: BLE001
        scrivi("ATTENZIONE: non leggo %s (%s)" % (PIANTA, e))
    ponti = []
    for nome in PONTI:
        p = {"nome": nome}
        a = per_nome.get(nome)
        if a is not None:
            o = a.get_actor_location()
            bo, be = a.get_actor_bounds(False)
            p["oggi"] = {"origine": pianta(o.x, o.y) + [round(o.z, 1)], "yaw": round(a.get_actor_rotation().yaw, 1),
                         "ingombro": {"min": pianta(bo.x - be.x, bo.y - be.y), "max": pianta(bo.x + be.x, bo.y + be.y),
                                      "z_min": round(bo.z - be.z, 1), "z_max": round(bo.z + be.z, 1)}}
            try:
                sm = a.get_component_by_class(unreal.StaticMeshComponent)
                p["oggi"]["modello"] = sm.get_editor_property("static_mesh").get_name()
            except Exception:  # noqa: BLE001
                pass
        linea = linee.get(nome)
        if linea and len(linea) >= 2:
            (x0, y0), (x1, y1) = linea[0], linea[-1]
            x0, y0, x1, y1 = x0 + ax, y0 + ay, x1 + ax, y1 + ay
            lung = math.hypot(x1 - x0, y1 - y0)
            ux, uy = (x1 - x0) / lung, (y1 - y0) / lung
            p["linea_pianta"] = [pianta(x0, y0), pianta(x1, y1)]
            p["lungo_cm"] = round(lung)
            profilo = []
            sponde = []
            s = -1500.0
            while s <= lung + 1500.0:
                x, y = x0 + ux * s, y0 + uy * s
                zt = terreno(x, y)
                w = pelo_qui(corsi, x, y)
                profilo.append([round(s)] + pianta(x, y) + [None if zt is None else round(zt, 1),
                                                           None if w is None else round(w[0], 1), "" if w is None else w[1]])
                s += 50.0
            # (secondo controllo) le sponde: dal centro dell'acqua si va verso i due capi finche' il terreno resta sotto
            # il pelo (anche oltre la larghezza scritta nei dati); i punti senza terreno si saltano
            acqua_i = [i for i, r in enumerate(profilo) if r[4] is not None and r[3] is not None]
            if acqua_i:
                mezzo = acqua_i[len(acqua_i) // 2]
                pelo = profilo[mezzo][4]
                p["pelo_acqua"] = pelo
                for passo in (-1, 1):
                    i = mezzo
                    while 0 <= i + passo < len(profilo):
                        r = profilo[i + passo]
                        if r[3] is not None and r[3] >= pelo:
                            sponde.append({"distanza_cm": r[0], "punto": [r[1], r[2]], "z_terreno": r[3]})
                            break
                        i += passo
                sponde.sort(key=lambda sp: sp["distanza_cm"])
            p["profilo"] = {"colonne": ["distanza_cm (0 = primo punto della linea)", "x", "y", "z_terreno", "pelo_acqua", "corso"],
                            "punti": profilo}
            p["sponde"] = sponde
            acqua = [r for r in profilo if r[4] is not None]
            scrivi("%s: linea lunga %.0f cm; acqua in %d punti su %d (pelo %s); sponde a %s cm"
                   % (nome, lung, len(acqua), len(profilo),
                      "?" if not acqua else "%.0f" % acqua[len(acqua) // 2][4],
                      ", ".join(str(sp["distanza_cm"]) for sp in sponde) or "?"))
        else:
            scrivi("ATTENZIONE: nella Pianta non trovo la linea '%s'" % nome)
        ponti.append(p)
    dati["ponti"] = ponti

    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "dati_tempio_ponti.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, indent=1)
    scrivi("scritto Saved/Claude/dati_tempio_ponti.json")


try:
    principale()
except Exception as e:  # noqa: BLE001
    scrivi("ERRORE: %s" % e)
    raise
finally:
    _c = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(_c, exist_ok=True)
    with open(os.path.join(_c, "dati_tempio_ponti.txt"), "w", encoding="utf-8") as _f:
        _f.write("\n".join(righe) + "\n")
    unreal.log("[Valdorso] Tempio e ponti: fatto. Resoconto in Saved/Claude/dati_tempio_ponti.txt (%d righe)" % len(righe))
