# Valdorso - Le quote del terreno intorno alle mura, per la chat 3D (16/11/2026 sera, scritto da Claude; v0.2.6 "Le mura").
# NON cambia niente nel livello: guarda e scrive.
#
# La chat 3D deve fare le strade che portano alle quattro porte (SM_Fase2_Strade_Porte) e le servono le quote vere del
# terreno. Questo script legge il paesaggio ogni 2 m nel rettangolo delle mura piu' 150 m intorno e scrive
# Saved/Claude/quote_mura.json, in coordinate della Pianta (cm, l'angolo del Landscape = 0,0), come il percorso delle mura.
# Menu Valdorso -> Villaggio -> "Strade: esporta le quote intorno alle mura (per la chat 3D)". Ci mette circa un minuto.

import json
import os

import unreal

PASSO = 200.0          # 2 m
MARGINE = 15000.0      # 150 m intorno alle mura
POSIZIONI = "V:/Texture/valdorso/mura/posizioni_mura_valdorso.json"

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
righe = []


def scrivi(t):
    righe.append(t)
    unreal.log("[Valdorso] Quote: " + t)


def avviso(t):
    righe.append("ATTENZIONE: " + t)
    unreal.log_warning("[Valdorso] Quote: " + t)


def terreno(x, y):
    """La quota del paesaggio in (x, y): il raggio scende da +2 km a -2 km e salta le cose che lo coprono (mura,
    palizzate, alberi, acqua), come in crea_mura.py."""
    ignora = []
    for _ in range(12):
        trovate = []
        fermato = None
        try:
            r = unreal.SystemLibrary.line_trace_multi(mondo, unreal.Vector(x, y, 200000.0), unreal.Vector(x, y, -200000.0),
                                                      unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora,
                                                      unreal.DrawDebugTrace.NONE, True)
            colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
        except Exception:  # noqa: BLE001
            colpi = []
        for hit in colpi or []:
            t = hit.to_tuple()
            if t[9] is None:
                continue
            if "Landscape" in t[9].get_class().get_name():
                trovate.append(t[4].z)
            elif t[0]:
                fermato = t[9]
        if trovate:
            return max(trovate)
        if fermato is None or fermato in ignora:
            return None
        ignora.append(fermato)
    return None


def principale():
    angolo = None
    for a in attori.get_all_level_actors():
        if isinstance(a, unreal.Landscape):
            angolo = a.get_actor_location()
            break
    if angolo is None:
        avviso("nel livello non c'e' il paesaggio (apri L_Valle)")
        return
    with open(POSIZIONI, "r", encoding="utf-8") as f:
        pezzi = json.load(f)["pezzi"]
    xs = [p["posizione_cm"][0] for p in pezzi]
    ys = [p["posizione_cm"][1] for p in pezzi]
    x0, x1 = min(xs) - MARGINE, max(xs) + MARGINE
    y0, y1 = min(ys) - MARGINE, max(ys) + MARGINE
    colonne = int((x1 - x0) / PASSO) + 1
    file_righe = int((y1 - y0) / PASSO) + 1
    scrivi("rettangolo (Pianta, cm): x %.0f..%.0f, y %.0f..%.0f; %d x %d punti ogni %.0f cm"
           % (x0, x1, y0, y1, colonne, file_righe, PASSO))
    quote = []
    mancano = 0
    with unreal.ScopedSlowTask(file_righe, "Valdorso: quote del terreno intorno alle mura") as compito:
        compito.make_dialog(True)
        for j in range(file_righe):
            if compito.should_cancel():
                avviso("fermato a meta' (riga %d di %d)" % (j, file_righe))
                break
            compito.enter_progress_frame(1)
            riga = []
            for i in range(colonne):
                z = terreno(angolo.x + x0 + i * PASSO, angolo.y + y0 + j * PASSO)
                if z is None:
                    mancano += 1
                    riga.append(None)
                else:
                    riga.append(round(z))
            quote.append(riga)
    if mancano:
        avviso("%d punti senza terreno (fuori dal paesaggio o non caricati): sono null" % mancano)
    dati = {
        "_come_si_legge": [
            "Quote del terreno di Valdorso intorno alle mura, lette in Unreal il 16/11/2026 (Saved/Claude/quote_mura.json).",
            "Coordinate della Pianta in cm (angolo del Landscape = 0,0), come posizioni_mura_valdorso.json e Pianta_3000.json.",
            "z_cm[riga][colonna] = quota del terreno in cm (Z del mondo, come 'terreno' in Villaggio_3000.json) nel punto "
            "x = da_cm[0] + colonna * passo_cm, y = da_cm[1] + riga * passo_cm. null = nessun terreno.",
            "T_SenzaErba.png (4096 x 4096) copre 300000 x 300000 cm a partire dall'angolo del Landscape (73,24 cm per pixel)."],
        "angolo_landscape_mondo_cm": [round(angolo.x), round(angolo.y), round(angolo.z)],
        "da_cm": [round(x0), round(y0)], "passo_cm": PASSO, "colonne": colonne, "righe": len(quote),
        "z_cm": quote}
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "quote_mura.json"), "w", encoding="utf-8") as f:
        json.dump(dati, f, separators=(",", ":"))
    scrivi("FATTO: Saved/Claude/quote_mura.json (%d righe x %d colonne). Mandalo alla chat 3D." % (len(quote), colonne))


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "quote_mura.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
