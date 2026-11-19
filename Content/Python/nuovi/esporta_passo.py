# Valdorso - Le misure del posto di guardia del passo, per la chat 3D (18/11/2026, scritto da Claude; v0.2.7).
# NON cambia niente nel livello: guarda e scrive. Menu Valdorso -> Villaggio ->
# "Passo: esporta le misure del posto di guardia per la chat 3D". Si lancia in L_Valle, non in Play.
#
# Serve a rifare il posto di guardia per la Guardia Reale (deciso da Fra il 18/11). Scrive
# Saved/Claude/dati_passo.json (e .txt da leggere). Tutto in cm; "mondo" = coordinate di Unreal, "pianta" = mondo meno
# l'angolo del paesaggio; "lungo" e "traverso" = il sistema del passo, come in Tools/Generatori/villaggio.py, guardia():
# lungo cresce verso la valle (vettore d), traverso di lato (vettore n), con l'origine G nella "Porta del passo".
#   a) G, d, n, gr_traverso (e lo yaw vero dell'attore, per controllo);
#   b) il terreno: lungo da -15 a +30 m, traverso da -35 a +35 m, ogni 50 cm (quota del paesaggio vero);
#   c) nella stessa griglia, ogni 50 cm, che cosa c'e' sopra il terreno (strada, edifici...): la strada si vede li';
#   d) tutti gli attori entro 60 m da G: nome, classe, modello, posizione (anche lungo/traverso), yaw, scala, ingombro,
#      fase; il sigillo con le sue misure; le persone (PNG) con il posto dove stanno adesso.

import json
import math
import os

import unreal

NOME_PORTA = "Porta del passo"
D = (-0.80105, -0.59860)            # la strada, verso la valle (villaggio.py, guardia())
N = (0.59860, -0.80105)             # di traverso al passo (-d.y, d.x)
LUNGO = (-1500.0, 3000.0)
TRAVERSO = (-3500.0, 3500.0)
PASSO = 50.0
RAGGIO = 6000.0

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
righe = []


def scrivi(t):
    righe.append(t)


def colpi(a, b, ignora):
    try:
        r = unreal.SystemLibrary.line_trace_multi(mondo, a, b, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora,
                                                  unreal.DrawDebugTrace.NONE, True)
        r = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
    except Exception:  # noqa: BLE001
        r = []
    return [h.to_tuple() for h in (r or [])]


def terreno_e_sopra(x, y, vicino):
    """(quota del paesaggio piu' vicina a 'vicino', nome della prima cosa piena sopra il paesaggio o '').
    Il raggio salta una alla volta le cose piene che non sono paesaggio (al massimo 16)."""
    ignora = []
    primo = ""
    for _ in range(16):
        fermato = None
        trovate = []
        for t in colpi(unreal.Vector(x, y, vicino + 20000.0), unreal.Vector(x, y, vicino - 20000.0), ignora):
            a = t[9]
            if a is None:
                continue
            if "Landscape" in a.get_class().get_name():
                trovate.append(t[4].z)
            elif t[0]:
                fermato = a
        if trovate:
            return min(trovate, key=lambda z: abs(z - vicino)), primo
        if fermato is None or fermato in ignora:
            return None, primo
        # (secondo controllo) fra le cose sopra il terreno conta la strada, se c'e' (sotto un tetto, la sbarra, un albero)
        n = fermato.get_actor_label()
        if not primo or ("trad" in n and "trad" not in primo):
            primo = n
        ignora.append(fermato)
    return None, primo


def principale():
    tutti = [a for a in attori.get_all_level_actors() if a]
    angolo = None
    porta = None
    for a in tutti:
        if isinstance(a, unreal.Landscape) and angolo is None:
            angolo = a.get_actor_location()
        if porta is None and a.get_actor_label() == NOME_PORTA:
            porta = a
    if angolo is None or porta is None:
        scrivi("ERRORE: non trovo il paesaggio o '%s' (sei in L_Valle?)" % NOME_PORTA)
        return
    g = porta.get_actor_location()
    gr_traverso = math.degrees(math.atan2(N[1], N[0]))
    yaw_porta = porta.get_actor_rotation().yaw

    def al_mondo(lungo, traverso):
        return g.x + D[0] * lungo + N[0] * traverso, g.y + D[1] * lungo + N[1] * traverso

    def al_passo(x, y):
        dx, dy = x - g.x, y - g.y
        return round(dx * D[0] + dy * D[1], 1), round(dx * N[0] + dy * N[1], 1)

    dati = {"_come_si_legge": "cm. mondo = Unreal; pianta = mondo meno l'angolo del paesaggio (solo x e y); lungo/traverso = "
                              "sistema del passo: mondo = G + d*lungo + n*traverso; z sempre quota vera del mondo; yaw in gradi di Unreal",
            "angolo_paesaggio_mondo": [angolo.x, angolo.y, angolo.z],
            "G_mondo": [round(g.x, 1), round(g.y, 1), round(g.z, 1)],
            "G_pianta": [round(g.x - angolo.x, 1), round(g.y - angolo.y, 1)],
            "d": list(D), "n": list(N), "gr_traverso": round(gr_traverso, 3),
            "porta_del_passo": {"yaw_attore": round(yaw_porta, 3), "scala": [porta.get_actor_scale3d().x, porta.get_actor_scale3d().y,
                                                                        porta.get_actor_scale3d().z],
                                "nota": "il portale di oggi e' largo 600 cm fra i pali; la sbarra e' 300 cm davanti, verso la valle "
                                        "(nel modello: y -300), lunga 720, a 100 cm da terra (villaggio.py, guardia())"}}
    if abs(((yaw_porta - gr_traverso + 180.0) % 360.0) - 180.0) > 1.0:
        scrivi("ATTENZIONE: lo yaw della Porta del passo (%.1f) non e' gr_traverso (%.1f): d e n potrebbero non valere piu'"
               % (yaw_porta, gr_traverso))
    scrivi("G (Porta del passo): mondo (%.0f, %.0f, %.0f), pianta (%.0f, %.0f); yaw dell'attore %.1f, gr_traverso %.1f"
           % (g.x, g.y, g.z, g.x - angolo.x, g.y - angolo.y, yaw_porta, gr_traverso))

    # ---------------------------------------------------------------- b, c) il terreno e che cosa c'e' sopra
    lunghi = [LUNGO[0] + PASSO * i for i in range(int((LUNGO[1] - LUNGO[0]) / PASSO) + 1)]
    traversi = [TRAVERSO[0] + PASSO * i for i in range(int((TRAVERSO[1] - TRAVERSO[0]) / PASSO) + 1)]
    nomi = [""]
    quote = []
    sopra = []
    buchi = 0
    with unreal.ScopedSlowTask(len(lunghi), "Passo: misuro il terreno") as lavoro:
        lavoro.make_dialog(False)
        for lu in lunghi:
            lavoro.enter_progress_frame(1)
            rq, rs = [], []
            for tr in traversi:
                x, y = al_mondo(lu, tr)
                q, chi = terreno_e_sopra(x, y, g.z)
                if q is None:
                    buchi += 1
                rq.append(None if q is None else int(round(q)))
                if chi not in nomi:
                    nomi.append(chi)
                rs.append(nomi.index(chi))
            quote.append(rq)
            sopra.append(rs)
    dati["terreno"] = {"lungo_cm": [LUNGO[0], LUNGO[1]], "traverso_cm": [TRAVERSO[0], TRAVERSO[1]], "passo_cm": PASSO,
                       "come": "quote_z[i][j]: i = lungo (dal primo), j = traverso (dal primo); null = paesaggio non trovato. "
                               "sopra[i][j] = indice in sopra_nomi della prima cosa piena sopra il terreno (0 = niente)",
                       "quote_z": quote, "sopra_nomi": nomi, "sopra": sopra}
    piene = [q for r in quote for q in r if q is not None]
    scrivi("terreno: %d x %d punti, quota da %d a %d, %d senza paesaggio" % (len(lunghi), len(traversi), min(piene) if piene else 0,
                                                                          max(piene) if piene else 0, buchi))
    scrivi("sopra il terreno: " + ", ".join(n for n in nomi if n))
    # la strada: dove la prima cosa sopra il terreno ha "Strad" nel nome, la striscia per ogni 'lungo'
    indici_strada = [i for i, n in enumerate(nomi) if "trad" in n]
    strada = []
    for i, lu in enumerate(lunghi[::4]):
        r = sopra[i * 4]
        js = [j for j, v in enumerate(r) if v in indici_strada]
        if js:
            strada.append({"lungo": lu, "traverso_da": traversi[js[0]], "traverso_a": traversi[js[-1]],
                           "mezzo": (traversi[js[0]] + traversi[js[-1]]) / 2.0, "larga": traversi[js[-1]] - traversi[js[0]] + PASSO})
    dati["strada"] = {"come": "ogni 2 m lungo il passo: dove comincia e finisce la strada (il modello delle strade sopra il "
                              "terreno). Se e' vuoto la strada li' e' solo dipinta sul terreno: passa per G lungo d",
                      "strisce": strada}
    scrivi("strada: %d strisce misurate%s" % (len(strada), "" if strada else " (solo dipinta: passa per G lungo d)"))

    # ---------------------------------------------------------------- d) gli attori intorno
    cose = []
    for a in tutti:
        if isinstance(a, unreal.Landscape) or "Landscape" in a.get_class().get_name():
            continue
        p = a.get_actor_location()
        if math.hypot(p.x - g.x, p.y - g.y) > RAGGIO:
            continue
        classe = a.get_class().get_name()
        if classe in ("InstancedFoliageActor", "WorldDataLayers", "WorldPartitionMiniMap"):
            continue
        lu, tr = al_passo(p.x, p.y)
        c = {"nome": a.get_actor_label(), "classe": classe, "mondo": [round(p.x, 1), round(p.y, 1), round(p.z, 1)],
             "lungo": lu, "traverso": tr, "yaw": round(a.get_actor_rotation().yaw, 2),
             "scala": [round(v, 3) for v in (a.get_actor_scale3d().x, a.get_actor_scale3d().y, a.get_actor_scale3d().z)],
             "cartella": str(a.get_folder_path()),
             "etichette": [str(t) for t in a.get_editor_property("tags")]}
        m = a.get_component_by_class(unreal.StaticMeshComponent)
        sm = m.get_editor_property("static_mesh") if m else None
        if sm is not None:
            c["modello"] = sm.get_name()
        try:
            o, e = a.get_actor_bounds(False)
            c["ingombro_mondo"] = {"centro": [round(o.x), round(o.y), round(o.z)], "mezzo": [round(e.x), round(e.y), round(e.z)]}
        except Exception:  # noqa: BLE001
            pass
        if classe == "ValdorsoSigillo":
            for campo in ("larghezza", "altezza", "base", "centro_traverso"):
                try:
                    c[campo] = float(a.get_editor_property(campo))
                except Exception:  # noqa: BLE001
                    pass
        cose.append(c)
    cose.sort(key=lambda c: (c["lungo"], c["traverso"]))
    dati["attori_entro_60_m"] = cose
    scrivi("attori entro 60 m da G: %d" % len(cose))
    for c in cose:
        scrivi("  %-44s %-26s lungo %+7.0f traverso %+7.0f z %6.0f yaw %6.1f %s"
               % (c["nome"][:44], c["classe"][:26], c["lungo"], c["traverso"], c["mondo"][2], c["yaw"], c.get("modello", "")))

    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "dati_passo.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, ensure_ascii=False, separators=(",", ":"))
    scrivi("scritto Saved/Claude/dati_passo.json")


try:
    principale()
except Exception as e:  # noqa: BLE001
    scrivi("ERRORE: %s" % e)
    raise
finally:
    _c = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(_c, exist_ok=True)
    with open(os.path.join(_c, "dati_passo.txt"), "w", encoding="utf-8") as _f:
        _f.write("\n".join(righe) + "\n")
    unreal.log("[Valdorso] Passo: fatto. Resoconto in Saved/Claude/dati_passo.txt (%d righe)" % len(righe))
