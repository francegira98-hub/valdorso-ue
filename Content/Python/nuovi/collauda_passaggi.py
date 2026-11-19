# Valdorso - Il collaudo dei passaggi (17/11/2026, scritto da Claude; v0.2.7 "Il tempio", approvato da Fra il 17/11).
# Menu Valdorso -> Strumenti -> "Per Claude: collauda i passaggi (porte e soglie, tre fasi)". Si lancia in L_Valle,
# NON in Play. Non cambia niente nel livello e non serve salvare.
#
# Che cosa fa: per ogni porta vera del livello (AValdorsoPorta: case, botteghe, torri, porticine, ante delle porte
# della citta', celle) e per ogni fase in cui la porta c'e', fa passare un "manichino" grande come il colono
# (capsula di raggio 42 cm, alta 192 cm; sale gradini fino a 45 cm) da 1,5 m prima a 1,5 m dopo la soglia, a passi di
# 10 cm, con la porta aperta. A ogni passo:
#   - cerca il pavimento sotto i piedi (una linea verso il basso) e misura il gradino rispetto al passo prima;
#   - controlla che il corpo ci stia (una capsula dal ginocchio in su: i gradini bassi non contano).
# Segna:  BLOCCO   il corpo tocca qualcosa a meno di 1 m dalla soglia, o un gradino sopra i 45 cm, o il vuoto;
#         GRADINO  un gradino tra 25 e 45 cm (si passa, ma si inciampa: da sistemare);
#         VICINO   il corpo tocca qualcosa tra 1 e 1,5 m dalla soglia (un mobile davanti alla porta: da guardare);
#         OK.
# Le cose delle altre fasi (che nell'editor ci sono tutte insieme) non contano: il manichino le ignora, come nel gioco.
# Non guarda: le imposte (AValdorsoImposta), le grate, le scale a pioli (AValdorsoScala) e le scale dentro le torri
# (quelle si collaudano con un secondo giro, quando le porte sono a posto).
#
# Resoconto: Saved/Claude/collaudo_passaggi.txt (per Claude e per la chat 3D) e collaudo_passaggi.json (i numeri).
# Le posizioni sono in cm del mondo e in cm della Pianta (mondo meno l'angolo del paesaggio).

import datetime
import json
import math
import os

import unreal

RAGGIO = 42.0            # la capsula del colono (ValdorsoCharacter.cpp: InitCapsuleSize(42, 96))
MEZZA = 96.0
SALE = 45.0              # MaxStepHeight del colono (quello di serie di Unreal)
GRADINO = 25.0           # sopra questo un gradino si segna (17/11: era 20; le alzate normali delle scale sono 18-22)
TRATTO = 150.0           # quanto prima e quanto dopo la soglia
PASSO = 10.0
SOGLIA = 100.0           # entro questa distanza dalla soglia un tocco e' un BLOCCO, oltre e' solo VICINO
                         # (18/11: era 110; un corridoio largo 140, come quello delle camere della locanda, si tocca
                         # a 110 dal filo della porta di fronte e non e' un blocco: ci si gira e si passa)
PROFILO = "Pawn"
PREFISSO = "ValdorsoFase "

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []
RIPIEGO = set()           # se Unreal non ha la prova "per profilo" si usa il canale della vista: lo si dice una volta


def scrivi(t):
    righe.append(t)


def fase_di(a):
    """(da, fino) dall'etichetta "ValdorsoFase da-fino"; None se la cosa c'e' sempre."""
    for t in a.get_editor_property("tags"):
        x = str(t)
        if x.startswith(PREFISSO):
            try:
                da, fino = x[len(PREFISSO):].split("-")
                return int(da), int(fino)
            except ValueError:
                return None
    return None


def nella_fase(f, fase):
    return f is None or (f[0] <= fase and (f[1] == 0 or fase <= f[1]))


def colpo(r):
    """Il risultato di una linea o di una capsula di Unreal, in qualunque forma arrivi: la tupla del colpo o None."""
    if r is None:
        return None
    if isinstance(r, tuple):
        # (vero/falso, colpo) oppure gia' la tupla del colpo
        if len(r) == 2 and isinstance(r[0], bool):
            if not r[0] or r[1] is None:
                return None
            r = r[1]
        else:
            return r if r and r[0] else None
    try:
        t = r.to_tuple()
    except Exception:  # noqa: BLE001
        return None
    return t if t and t[0] else None


def linea(mondo, a, b, ignora):
    try:
        return colpo(unreal.SystemLibrary.line_trace_single_by_profile(
            mondo, a, b, PROFILO, False, ignora, unreal.DrawDebugTrace.NONE, True))
    except Exception:  # noqa: BLE001
        RIPIEGO.add("linea")
        return colpo(unreal.SystemLibrary.line_trace_single(
            mondo, a, b, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora, unreal.DrawDebugTrace.NONE, True))


def capsula(mondo, centro, raggio, mezza, ignora):
    su = unreal.Vector(centro.x, centro.y, centro.z + 1.0)
    try:
        return colpo(unreal.SystemLibrary.capsule_trace_single_by_profile(
            mondo, centro, su, raggio, mezza, PROFILO, False, ignora, unreal.DrawDebugTrace.NONE, True))
    except Exception:  # noqa: BLE001
        RIPIEGO.add("capsula")
        return colpo(unreal.SystemLibrary.capsule_trace_single(
            mondo, centro, su, raggio, mezza, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignora,
            unreal.DrawDebugTrace.NONE, True))


def nome_di(t):
    """Chi e' stato toccato: l'etichetta dell'attore (e il modello, se c'e')."""
    a = t[9] if len(t) > 9 else None
    if a is None:
        return "qualcosa senza nome"
    n = a.get_actor_label()
    c = t[10] if len(t) > 10 else None
    try:
        m = c.get_editor_property("static_mesh") if c is not None else None
        if m is not None and m.get_name() not in n:
            n += " [%s]" % m.get_name()
    except Exception:  # noqa: BLE001
        pass
    return n


def apertura(porta):
    """Il centro dell'apertura (a terra), la direzione in cui si passa e la larghezza dell'anta, dalla scatola del
    modello dell'anta: il lato sottile dell'anta e' la direzione del passaggio. None se la porta non ha un'anta."""
    try:
        anta = porta.get_editor_property("anta")
    except Exception:  # noqa: BLE001
        anta = None
    if anta is None:
        return None
    mesh = anta.get_editor_property("static_mesh")
    if mesh is None:
        return None
    scatola = mesh.get_bounding_box()
    lo, hi = scatola.min, scatola.max
    tr = anta.get_world_transform()
    s = tr.scale3d
    lx = abs((hi.x - lo.x) * s.x)
    ly = abs((hi.y - lo.y) * s.y)
    centro = unreal.MathLibrary.transform_location(
        tr, unreal.Vector((lo.x + hi.x) / 2.0, (lo.y + hi.y) / 2.0, (lo.z + hi.z) / 2.0))
    base = min(unreal.MathLibrary.transform_location(tr, unreal.Vector(0.0, 0.0, lo.z)).z,
               unreal.MathLibrary.transform_location(tr, unreal.Vector(0.0, 0.0, hi.z)).z)
    asse = unreal.Vector(1.0, 0.0, 0.0) if lx <= ly else unreal.Vector(0.0, 1.0, 0.0)
    d = unreal.MathLibrary.transform_direction(tr, asse)
    lung = math.hypot(d.x, d.y)
    if lung < 1e-3:
        return None
    return (centro.x, centro.y, base), (d.x / lung, d.y / lung), max(lx, ly), min(lx, ly)


def tratto_libero(mondo, x, y, piedi, dx, dy, lungo, ignora):
    """Dal punto (x, y, piedi) verso (dx, dy) per 'lungo' cm: vero se il manichino ci cammina (pavimento a ogni passo,
    nessun gradino oltre SALE, corpo libero)."""
    mezza = (2.0 * MEZZA - SALE) / 2.0
    s = PASSO
    while s <= lungo:
        px, py = x + dx * s, y + dy * s
        t = linea(mondo, unreal.Vector(px, py, piedi + 60.0), unreal.Vector(px, py, piedi - 200.0), ignora)
        if t is None or t[3] < 1.0 or abs(t[4].z - piedi) > SALE:
            return False
        piedi = t[4].z
        if capsula(mondo, unreal.Vector(px, py, piedi + SALE + mezza), RAGGIO, mezza, ignora) is not None:
            return False
        s += PASSO
    return True


def di_lato(mondo, x, y, piedi, dire, ignora):
    """Quando davanti il pavimento finisce (un ballatoio, una scala che gira): prova a uscire di lato.
    Ritorna l'angolo (gradi, rispetto al passaggio) della prima direzione dove si cammina per 1,5 m, o None."""
    for ang in (90.0, -90.0, 45.0, -45.0, 135.0, -135.0):
        a = math.radians(ang)
        dx = dire[0] * math.cos(a) - dire[1] * math.sin(a)
        dy = dire[0] * math.sin(a) + dire[1] * math.cos(a)
        if tratto_libero(mondo, x, y, piedi, dx, dy, TRATTO, ignora):
            return ang
    return None


def luce_della_porta(mondo, cx, cy, piedi, ignora):
    """Quanto spazio c'e' sopra la soglia (cm), fino a 4 m: una linea verso l'alto dal pavimento."""
    t = linea(mondo, unreal.Vector(cx, cy, piedi + 5.0), unreal.Vector(cx, cy, piedi + 400.0), ignora)
    if t is None or t[3] < 1.0:
        return None          # niente sopra entro 4 m, oppure la linea parte dentro qualcosa: non si giudica
    return t[4].z - piedi


def cammina(mondo, centro, dire, ignora):
    """Fa passare il manichino. Ritorna (voto, note, gradino piu' alto in cm)."""
    cx, cy, base = centro
    note = []
    voto = "OK"
    peggio = {"OK": 0, "VICINO": 1, "GRADINO": 2, "BLOCCO": 3}

    def segna(v, testo):
        nonlocal voto
        if peggio[v] > peggio[voto]:
            voto = v
        if testo not in note:
            note.append(testo)

    passi = int(round(TRATTO / PASSO))
    piedi = None
    alto = 0.0
    toccati = set()
    for i in range(-passi, passi + 1):
        s = i * PASSO
        x = cx + dire[0] * s
        y = cy + dire[1] * s
        if piedi is None:
            su, giu = base + 120.0, base - 300.0
        else:
            su, giu = piedi + 60.0, piedi - 200.0
        t = linea(mondo, unreal.Vector(x, y, su), unreal.Vector(x, y, giu), ignora)
        if t is None:
            if piedi is None:
                # il primo punto e' fuori (un muro, il vuoto): si riparte dal passo dopo senza segnare
                if abs(s) <= SOGLIA:
                    segna("BLOCCO", "a %+.0f cm dalla soglia non c'e' pavimento" % s)
                continue
            if s <= 0:
                # prima della soglia: un buco nel pavimento di qua dalla porta; si segna e si va avanti
                segna("BLOCCO" if abs(s) <= SOGLIA else "VICINO",
                      "a %+.0f cm dalla soglia non c'e' pavimento entro 2 m sotto i piedi" % s)
                continue
            # (17/11, secondo giro) il pavimento finisce: quanto si cade, e se si esce di lato (ballatoio, scala che gira)
            px, py = cx + dire[0] * (s - PASSO), cy + dire[1] * (s - PASSO)
            fondo = linea(mondo, unreal.Vector(x, y, piedi + 60.0), unreal.Vector(x, y, piedi - 3000.0), ignora)
            caduta = ("si cade di %.0f cm su %s" % (piedi - fondo[4].z, nome_di(fondo))) if fondo is not None \
                else "sotto non c'e' niente per 30 m"
            ang = di_lato(mondo, px, py, piedi, dire, ignora)
            if ang is not None:
                segna("OK", "a %+.0f cm dalla soglia il pavimento finisce (%s), ma si esce di lato (a %+.0f gradi)"
                      % (s, caduta, ang))
            else:
                segna("BLOCCO" if abs(s) <= SOGLIA else "VICINO",
                      "a %+.0f cm dalla soglia il pavimento finisce (%s) e non si esce nemmeno di lato" % (s, caduta))
            break
        z = t[4].z
        if t[3] < 1.0 and piedi is None:
            continue          # il primo punto e' dentro qualcosa: si parte dal passo dopo
        if t[3] < 1.0:
            # la linea parte gia' dentro qualcosa: un muro o un gradino piu' alto di 60 cm
            chi = nome_di(t)
            if (chi, abs(s) <= SOGLIA) not in toccati:
                toccati.add((chi, abs(s) <= SOGLIA))
                segna("BLOCCO" if abs(s) <= SOGLIA else "VICINO",
                      "a %+.0f cm dalla soglia c'e' un muro o un gradino oltre 60 cm: %s" % (s, chi))
            continue
        if piedi is not None:
            salto = z - piedi
            if abs(salto) > abs(alto):
                alto = salto
            if salto > SALE:
                segna("BLOCCO" if abs(s) <= SOGLIA else "VICINO",
                      "a %+.0f cm dalla soglia un gradino di %.0f cm (il colono sale al massimo %.0f): %s"
                      % (s, salto, SALE, nome_di(t)))
            elif abs(salto) > GRADINO:
                segna("GRADINO", "a %+.0f cm dalla soglia un gradino di %+.0f cm: %s" % (s, salto, nome_di(t)))
        piedi = z
        # il corpo dal ginocchio in su: una capsula da piedi+SALE a piedi+2*MEZZA
        mezza = (2.0 * MEZZA - SALE) / 2.0
        c = unreal.Vector(x, y, piedi + SALE + mezza)
        t2 = capsula(mondo, c, RAGGIO, mezza, ignora)
        if t2 is not None:
            chi = nome_di(t2)
            if (chi, abs(s) <= SOGLIA) not in toccati:
                toccati.add((chi, abs(s) <= SOGLIA))
                quota = t2[5].z - piedi
                segna("BLOCCO" if abs(s) <= SOGLIA else "VICINO",
                      "a %+.0f cm dalla soglia il corpo tocca %s (a %.0f cm da terra)" % (s, chi, quota))
    if piedi is None:
        segna("BLOCCO", "non trovo il pavimento in nessun punto del passaggio")
    return voto, note, alto


def luce_al_centro(mondo, centro, ignora):
    """La luce della porta misurata al centro della soglia (cm), o None se non c'e' pavimento o niente sopra."""
    cx, cy, base = centro
    t = linea(mondo, unreal.Vector(cx, cy, base + 120.0), unreal.Vector(cx, cy, base - 150.0), ignora)
    if t is None:
        return None
    return luce_della_porta(mondo, cx, cy, t[4].z, ignora)


VICINANZA = 500.0         # si ignorano solo le cose delle altre fasi entro 5 m dalla porta (le altre non c'entrano)


def scatola_di(a):
    """(x, y, mezza x, mezza y) dell'ingombro di un attore; None se non ne ha."""
    try:
        o, e = a.get_actor_bounds(False)
        return o.x, o.y, e.x, e.y
    except Exception:  # noqa: BLE001
        return None


def vicini(lista, scatole, cx, cy):
    """Gli attori della lista il cui ingombro arriva entro VICINANZA dal punto (chi non ha ingombro si tiene)."""
    r = []
    for a in lista:
        b = scatole.get(a)
        if b is None or (abs(b[0] - cx) - b[2] <= VICINANZA and abs(b[1] - cy) - b[3] <= VICINANZA):
            r.append(a)
    return r


def principale():
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    classe_porta = getattr(unreal, "ValdorsoPorta", None)
    classe_imposta = getattr(unreal, "ValdorsoImposta", None)
    if mondo is None or classe_porta is None:
        scrivi("ERRORE: manca il mondo o la classe C++ ValdorsoPorta (compila e riapri l'editor).")
        return
    tutti = [a for a in attori.get_all_level_actors() if a]
    angolo = None
    for a in tutti:
        if isinstance(a, unreal.Landscape):
            angolo = a.get_actor_location()
            break
    fasi = {}
    porte = []
    non_contano = set()       # porte, imposte e grate: nel collaudo sono tutte aperte
    for a in tutti:
        fasi[a] = fase_di(a)
        n = a.get_class().get_name()
        if isinstance(a, classe_porta) or n in ("ValdorsoGrata", "ValdorsoCella"):
            non_contano.add(a)
            if isinstance(a, classe_porta) and not (classe_imposta and isinstance(a, classe_imposta)):
                porte.append(a)
    scrivi("Valdorso - collaudo dei passaggi (%s)" % datetime.datetime.now().strftime("%d/%m/%Y %H:%M"))
    scrivi("porte nel livello: %d; manichino raggio %.0f, alto %.0f, sale %.0f cm; tratto %.0f cm prima e dopo"
           % (len(porte), RAGGIO, 2 * MEZZA, SALE, TRATTO))
    if angolo is not None:
        scrivi("angolo del paesaggio (mondo): %.0f, %.0f, %.0f" % (angolo.x, angolo.y, angolo.z))
    scrivi("")
    risultati = []
    senza_anta = []
    scatole = {}
    for a in tutti:
        if fasi[a] is not None or a in non_contano:
            scatole[a] = scatola_di(a)
    for fase in (1, 2, 3):
        fuori = set(a for a in tutti if not nella_fase(fasi[a], fase))
        da_ignorare = list(fuori) + [a for a in non_contano if a not in fuori]
        conti = {"OK": 0, "VICINO": 0, "GRADINO": 0, "BLOCCO": 0}
        della_fase = []
        for p in porte:
            if not nella_fase(fasi[p], fase):
                continue
            ap = apertura(p)
            if ap is None:
                if p.get_actor_label() not in senza_anta:
                    senza_anta.append(p.get_actor_label())
                continue
            centro, dire, larga, spessa = ap
            ignora = vicini(da_ignorare, scatole, centro[0], centro[1])
            voto, note, alto = cammina(mondo, centro, dire, ignora)
            luce = luce_al_centro(mondo, centro, ignora)
            if luce is not None and luce < 2.0 * MEZZA + 8.0:
                # il corpo che urta e' alto 192 cm per tutti i coloni (il piu' alto misura 189 cm, il piu' basso 155)
                note.append("porta bassa: %.0f cm di luce (servono almeno 200)" % luce)
                if luce < 2.0 * MEZZA:
                    voto = "BLOCCO"
            if larga < 2.0 * RAGGIO + 10.0:
                note.append("anta stretta: %.0f cm (il colono e' largo %.0f)" % (larga, 2.0 * RAGGIO))
            conti[voto] += 1
            r = {"fase": fase, "porta": p.get_actor_label(), "cartella": str(p.get_folder_path()),
                 "voto": voto, "note": note, "gradino_cm": round(alto, 1),
                 "mondo_cm": [round(centro[0]), round(centro[1]), round(centro[2])],
                 "passaggio_yaw": round(math.degrees(math.atan2(dire[1], dire[0])), 1),
                 "anta_larga_cm": round(larga), "anta_spessa_cm": round(spessa),
                 "luce_cm": None if luce is None else round(luce)}
            if angolo is not None:
                r["pianta_cm"] = [round(centro[0] - angolo.x), round(centro[1] - angolo.y)]
            try:
                r["anta"] = p.get_editor_property("anta").get_editor_property("static_mesh").get_name()
            except Exception:  # noqa: BLE001
                r["anta"] = ""
            della_fase.append(r)
        risultati.extend(della_fase)
        scrivi("=== FASE %d: %d porte provate - BLOCCO %d, GRADINO %d, VICINO %d, OK %d"
               % (fase, len(della_fase), conti["BLOCCO"], conti["GRADINO"], conti["VICINO"], conti["OK"]))
        for v in ("BLOCCO", "GRADINO", "VICINO"):
            for r in sorted((x for x in della_fase if x["voto"] == v), key=lambda x: x["porta"]):
                dove = "mondo (%d, %d, %d)" % tuple(r["mondo_cm"])
                if "pianta_cm" in r:
                    dove += ", Pianta (%d, %d)" % tuple(r["pianta_cm"])
                scrivi("%s  %s  [%s, anta %s larga %d]  %s" % (v, r["porta"], r["cartella"], r["anta"],
                                                               r["anta_larga_cm"], dove))
                for n in r["note"]:
                    scrivi("        - " + n)
        scrivi("")
    if RIPIEGO:
        scrivi("NOTA: usato il canale della vista al posto del profilo del colono per: %s (i risultati possono "
               "essere un poco diversi dal gioco)" % ", ".join(sorted(RIPIEGO)))
    if senza_anta:
        scrivi("Porte senza il modello dell'anta (non provate): %s" % ", ".join(sorted(senza_anta)))
    # le stesse porte con lo stesso difetto in piu' fasi: un riassunto corto in fondo
    per_porta = {}
    for r in risultati:
        if r["voto"] in ("BLOCCO", "GRADINO"):
            per_porta.setdefault((r["porta"], r["voto"]), []).append(r["fase"])
    scrivi("RIASSUNTO: %d porte con BLOCCO, %d con GRADINO (contate una volta sola)"
           % (len({k[0] for k in per_porta if k[1] == "BLOCCO"}), len({k[0] for k in per_porta if k[1] == "GRADINO"})))
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "collaudo_passaggi.json"), "w", encoding="utf-8") as f:
        json.dump({"quando": datetime.datetime.now().isoformat(timespec="seconds"), "porte": risultati}, f, ensure_ascii=False, indent=1)


try:
    principale()
except Exception as e:  # noqa: BLE001
    scrivi("ERRORE: %s" % e)
    raise
finally:
    _cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(_cartella, exist_ok=True)
    with open(os.path.join(_cartella, "collaudo_passaggi.txt"), "w", encoding="utf-8") as _f:
        _f.write("\n".join(righe) + "\n")
    unreal.log("[Valdorso] Collaudo dei passaggi: fatto. Resoconto in Saved/Claude/collaudo_passaggi.txt (%d righe)"
               % len(righe))
