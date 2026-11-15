# Valdorso - Gli alberi in citta' (15/11/2026, scritto da Claude; idea di Fra: "mettere alberi anche in citta' per non
# farla sembrare spoglia, anche una bella quercia alta").
#
# Legge la pianta del villaggio (V:/Texture/valdorso/villaggio/Pianta_3000.json e Villaggio_3000.json, tutte e tre le
# fasi insieme: un albero non deve mai trovarsi dove in una fase c'e' una casa, una strada, un muro o una panca) e cerca
# i posti liberi dentro le mura:
#   - l'albero grande: nel posto piu' largo e libero vicino al centro (la piazza), un faggio o una quercia grande,
#     scalato perche' sia il piu' alto del villaggio;
#   - gli altri: nei cortili, negli slarghi e lungo le strade, ben distanziati (almeno 18 m l'uno dall'altro),
#     mai a meno di 6 m da una casa (le fronde) e mai sulla strada.
# Gli alberi sono quelli dei pacchetti (EuropeanBeech, poi i nostri del bosco se mancano), girati e scalati a caso
# (sempre uguali se si rilancia). Etichetta "ValdorsoAlberoCitta": "PNG: panche, letti e porte" li conosce e i PNG ci
# girano intorno (rilanciarlo dopo questo script).
# Voci nel menu Valdorso -> Villaggio:
#   "Alberi in citta': pianta"   (ALBERI_MODO = "fai": toglie quelli di prima e li rimette)
#   "Alberi in citta': togli"    (ALBERI_MODO = "togli")
# Resoconto in Saved/Claude/alberi_citta.txt.

import json
import math
import os

PIANTA = "V:/Texture/valdorso/villaggio/Pianta_3000.json"
VILLAGGIO = "V:/Texture/valdorso/villaggio/Villaggio_3000.json"
ETICHETTA = "ValdorsoAlberoCitta"
CONTORNO = "Valle/Alberi in citta"

PASSO = 200.0               # griglia per cercare i posti (cm)
DA_CASA = 600.0             # le fronde non toccano le case
DA_STRADA = 250.0           # oltre il bordo della strada
DA_MURO = 450.0             # dalle mura, palizzate, recinti (le fronde non scavalcano le mura)
DA_COSA = 350.0             # da panche, pozzi, bancarelle, porte, luci
DISTANZA = 1800.0           # tra un albero e l'altro
QUANTI = 26                 # quanti alberi piccoli al massimo
GRANDE_LIBERO = 1100.0      # l'albero grande vuole almeno 11 m liberi intorno
GRANDI = 3                  # quanti alberi grandi intorno alla piazza (Fra, 15/11: tre)


# ---------------------------------------------------------------- la scelta dei posti (senza Unreal)

def _dist_seg(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    l2 = dx * dx + dy * dy
    t = 0.0 if l2 <= 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / l2))
    return math.hypot(px - (ax + dx * t), py - (ay + dy * t))


def _dentro(px, py, poli):
    dentro = False
    j = len(poli) - 1
    for i in range(len(poli)):
        xi, yi = poli[i]
        xj, yj = poli[j]
        if (yi > py) != (yj > py) and px < (xj - xi) * (py - yi) / ((yj - yi) or 1e-9) + xi:
            dentro = not dentro
        j = i
    return dentro


def _inviluppo(punti):
    """L'inviluppo convesso (per sapere cosa sta 'dentro le mura')."""
    p = sorted(set(punti))
    if len(p) < 3:
        return p

    def giro(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    basso, alto = [], []
    for q in p:
        while len(basso) >= 2 and giro(basso[-2], basso[-1], q) <= 0:
            basso.pop()
        basso.append(q)
    for q in reversed(p):
        while len(alto) >= 2 and giro(alto[-2], alto[-1], q) <= 0:
            alto.pop()
        alto.append(q)
    return basso[:-1] + alto[:-1]


def scegli(pianta, cose, ok=lambda x, y: True):
    """[(x, y, 'grande'|'piccolo', raggio_fronde)] in coordinate della pianta, e qualche numero per il resoconto.
    ok(x, y): un controllo in piu' (in Unreal: sotto c'e' il prato e non l'acqua o una roccia)."""
    villaggio = [c for c in pianta if c.get("sezione") == "Villaggio"]
    case = [c["punti"] for c in villaggio if c.get("tipo") == "edificio"]
    strade = [(c["punti"], float(c.get("larga") or 300) / 2.0) for c in villaggio if c.get("tipo") == "strada"]
    linee = [c["punti"] for c in villaggio if c.get("tipo") in ("muro", "palizzata", "linea")]
    oggetti = [c["pos"] for c in cose if c.get("sezione") == "Villaggio" and c.get("pos")
               and c.get("tipo") in ("fab", "panca", "bacheca", "letto", "porta", "luce", "scala", "iscrizione", "mesh")]
    # dentro le mura: l'inviluppo di mura e palizzate (tutte le fasi), stretto di 4 m
    bordo = [tuple(p) for c in villaggio if c.get("tipo") in ("muro", "palizzata") for p in c["punti"]]
    if len(bordo) < 3:
        bordo = [tuple(p) for q in case for p in q]
    giro = _inviluppo(bordo)
    xs = [p[0] for p in giro]
    ys = [p[1] for p in giro]
    box = (min(xs), min(ys), max(xs), max(ys))
    cx = sum(xs) / len(xs)
    cy = sum(ys) / len(ys)

    def libero(x, y):
        """Quanto spazio c'e' intorno (cm), o -1 se il punto non va bene."""
        if not _dentro(x, y, giro):
            return -1.0
        chiuso = giro + [giro[0]]
        if min(_dist_seg(x, y, chiuso[k][0], chiuso[k][1], chiuso[k + 1][0], chiuso[k + 1][1])
               for k in range(len(giro))) < 400.0:
            return -1.0
        spazio = 1e9
        for q in case:
            if _dentro(x, y, q):
                return -1.0
            qq = q + [q[0]]
            d = min(_dist_seg(x, y, qq[k][0], qq[k][1], qq[k + 1][0], qq[k + 1][1]) for k in range(len(q)))
            if d < DA_CASA:
                return -1.0
            spazio = min(spazio, d)
        for punti, mezza in strade:
            for k in range(len(punti) - 1):
                d = _dist_seg(x, y, punti[k][0], punti[k][1], punti[k + 1][0], punti[k + 1][1]) - mezza
                if d < DA_STRADA:
                    return -1.0
                spazio = min(spazio, d + 300.0)
        for punti in linee:
            for k in range(len(punti) - 1):
                d = _dist_seg(x, y, punti[k][0], punti[k][1], punti[k + 1][0], punti[k + 1][1])
                if d < DA_MURO:
                    return -1.0
                spazio = min(spazio, d + 300.0)
        for p in oggetti:
            d = math.hypot(x - p[0], y - p[1])
            if d < DA_COSA:
                return -1.0
        return spazio

    posti = []
    x = box[0]
    while x <= box[2]:
        y = box[1]
        while y <= box[3]:
            s = libero(x, y)
            if s > 0:
                posti.append((x, y, s))
            y += PASSO
        x += PASSO
    alberi = []
    # l'albero grande: sul bordo della piazza (vicino al pozzo), nel posto piu' libero entro 30 m dal suo centro
    piazza = next((c["pos"] for c in cose if c.get("sezione") == "Villaggio" and c.get("nome") == "Piazza"), None)
    if piazza is None:
        piazza = (cx, cy)
    vicini = sorted([p for p in posti if math.hypot(p[0] - piazza[0], p[1] - piazza[1]) < 3000.0 and p[2] >= 700.0],
                    key=lambda p: -(min(p[2], 2000.0) - 0.2 * math.hypot(p[0] - piazza[0], p[1] - piazza[1])))
    for g in vicini:
        if ok(g[0], g[1]):
            alberi.append((g[0], g[1], "grande", min(g[2], 1400.0)))
            break
    # (15/11 sera, Fra: "la quercia la voglio anche sul lato opposto", poi "un'altra anche da quel lato") gli altri
    # grandi intorno alla piazza: ogni volta il posto piu' lontano dai grandi gia' messi (cosi' si spargono sui lati
    # liberi), entro 40 m dal centro della piazza e almeno 25 m l'uno dall'altro
    intorno = [p for p in posti if math.hypot(p[0] - piazza[0], p[1] - piazza[1]) < 4000.0 and p[2] >= 700.0]
    while alberi and len(alberi) < GRANDI:
        intorno.sort(key=lambda p: min(math.hypot(p[0] - a[0], p[1] - a[1]) for a in alberi) + 0.3 * min(p[2], 1500.0))
        messo = False
        while intorno:
            g = intorno.pop()
            if min(math.hypot(g[0] - a[0], g[1] - a[1]) for a in alberi) < 2500.0:
                break
            if ok(g[0], g[1]):
                alberi.append((g[0], g[1], "grande", min(g[2], 1400.0)))
                messo = True
                break
        if not messo:
            break
    # gli altri: solo vicino a case e strade (entro 15 m: niente boschetti nei prati vuoti), sparsi per tutta la citta'
    # (ogni volta il posto piu' lontano dagli alberi gia' scelti), mai a meno di DISTANZA l'uno dall'altro
    buoni = [p for p in posti if p[2] <= 1500.0]
    while len(alberi) < QUANTI + 1 and buoni:
        if alberi:
            lontano = lambda p: min(math.hypot(p[0] - a[0], p[1] - a[1]) for a in alberi)  # noqa: E731
            buoni.sort(key=lontano)
            p = buoni.pop()
            if lontano(p) < DISTANZA:
                break
        else:
            p = buoni.pop()
        if ok(p[0], p[1]):
            alberi.append((p[0], p[1], "piccolo", min(p[2], 700.0)))
    info = {"box": box, "posti_liberi": len(posti), "alberi": len(alberi)}
    return alberi, info


# ---------------------------------------------------------------- in Unreal

def _in_unreal():
    import unreal

    modo = str(globals().get("ALBERI_MODO", "fai"))
    attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    righe = []

    def scrivi(t):
        righe.append(t)
        unreal.log("[Valdorso] Alberi in citta': " + t)

    def avviso(t):
        righe.append("ATTENZIONE: " + t)
        unreal.log_warning("[Valdorso] Alberi in citta': " + t)

    def cerca_mesh(cartelle, parole):
        registro = unreal.AssetRegistryHelpers.get_asset_registry()
        trovati = []
        for cartella in cartelle:
            filtro = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")],
                                     package_paths=[cartella], recursive_paths=True)
            for d in registro.get_assets(filtro):
                n = str(d.asset_name).lower()
                if any(p in n for p in parole) and not any(s in n for s in ("sapling", "bush", "shrub", "branch",
                                                                             "leaf", "twig", "debris", "log",
                                                                             "stump", "root", "small", "young")):
                    trovati.append(str(d.package_name))
        return sorted(set(trovati))

    def principale():
        via = 0
        for a in attori.get_all_level_actors():
            if a and a.actor_has_tag(ETICHETTA):
                attori.destroy_actor(a)
                via += 1
        if via:
            scrivi("tolti %d alberi di prima" % via)
        if modo == "togli":
            unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
            scrivi("FATTO: alberi tolti (rilancia 'PNG: panche, letti e porte' perche' i PNG lo sappiano)")
            return
        tutti = attori.get_all_level_actors()
        paesaggi = [a for a in tutti if isinstance(a, unreal.Landscape)]
        if not paesaggi:
            avviso("nel livello non c'e' il paesaggio (apri L_Valle)")
            return
        angolo = paesaggi[0].get_actor_location()
        with open(PIANTA, "r", encoding="utf-8") as f:
            pianta = json.load(f)
        with open(VILLAGGIO, "r", encoding="utf-8") as f:
            cose = json.load(f).get("cose", [])
        mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        quote = {}
        # l'acqua (fiume, lago, stagni): le sue scatole; un albero non va dove il terreno sta sotto il pelo dell'acqua
        # (15/11 sera, prima prova: 0 alberi) l'acqua: solo i pezzi d'acqua piccoli (stagni, lavatoio, tratti di
        # fiume); le "zone" d'acqua grandi come la valle non dicono niente e scartavano tutto
        acque = []
        for a in tutti:
            try:
                nome = (a.get_actor_label() + " " + a.get_class().get_name()).lower()
            except Exception:  # noqa: BLE001
                continue
            if any(p in nome for p in ("water", "acqua", "fiume", "torrente", "lago", "stagno", "ruscello")):
                if "zone" in nome or "zona" in nome:
                    continue
                o, e = a.get_actor_bounds(False)
                if 50.0 < e.x < 15000.0 and 50.0 < e.y < 15000.0:
                    acque.append((o.x - e.x - 300.0, o.y - e.y - 300.0, o.x + e.x + 300.0, o.y + e.y + 300.0,
                                  o.z + e.z + 50.0, a.get_actor_label()))
        scrivi("pezzi d'acqua considerati: %d" % len(acque))
        perche = {}

        def conta(motivo):
            perche[motivo] = perche.get(motivo, 0) + 1

        def leggero(attore):
            """Cose che non contano (l'erba, i fiori, il fogliame): si guarda sotto."""
            try:
                n = (attore.get_class().get_name() + " " + attore.get_actor_label()).lower()
            except Exception:  # noqa: BLE001
                return False
            return any(p in n for p in ("foliage", "grass", "erba", "prato", "fiori", "volume", "decal", "fog",
                                        "nebbia", "sky", "cielo", "luce", "light", "trigger", "sound", "suono"))

        def sul_prato(px, py):
            """Scendendo dall'alto, la prima cosa solida dev'essere il paesaggio: non un ponte, una roccia, un tetto,
            la piazza lastricata. (L'erba e il fogliame si saltano.) E non dentro uno specchio d'acqua."""
            x, y = px + angolo.x, py + angolo.y
            try:
                r = unreal.SystemLibrary.line_trace_multi(mondo, unreal.Vector(x, y, angolo.z + 200000.0),
                                                          unreal.Vector(x, y, angolo.z - 200000.0),
                                                          unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                          unreal.DrawDebugTrace.NONE, True)
                colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
            except Exception:  # noqa: BLE001
                colpi = []
            zt = None
            for hit in colpi or []:
                t = hit.to_tuple()
                if t[9] is None:
                    continue
                if "Landscape" in t[9].get_class().get_name():
                    zt = t[4].z
                    break
                if leggero(t[9]):
                    continue
                conta("sopra c'e' " + t[9].get_actor_label())
                return False
            if zt is None:
                conta("niente paesaggio")
                return False
            for x0, y0, x1, y1, pelo, nome in acque:
                if x0 <= x <= x1 and y0 <= y <= y1 and zt < pelo:
                    conta("acqua: " + nome)
                    return False
            quote[(px, py)] = zt
            return True

        alberi, info = scegli(pianta, cose, sul_prato)
        scrivi("posti scartati: %d" % sum(perche.values()))
        for motivo, n in sorted(perche.items(), key=lambda kv: -kv[1])[:15]:
            scrivi("  %5d  %s" % (n, motivo))
        scrivi("posti liberi trovati: %d; alberi da mettere: %d (con i grandi della piazza)" % (info["posti_liberi"], len(alberi)))
        grandi = [p for p in cerca_mesh(["/Game/EuropeanBeech", "/Game/Megascans", "/Game/Fab"], ("beech", "oak", "quercia"))
                  if "seedling" not in p.lower()]
        if any(not p.endswith("_PP") for p in grandi):
            grandi = [p for p in grandi if not p.endswith("_PP")]
        piccoli = grandi[:]
        if not grandi:
            grandi = piccoli = cerca_mesh(["/Game/Valle/Bosco"], ("faggio", "quercia", "castagno", "tiglio", "betulla"))
        if not grandi:
            avviso("non trovo modelli di alberi (EuropeanBeech o il nostro bosco)")
            return
        scrivi("modelli: %s" % ", ".join(p.rsplit("/", 1)[-1] for p in grandi))
        modelli = {}

        def modello(p):
            if p not in modelli:
                modelli[p] = unreal.EditorAssetLibrary.load_asset(p)
            return modelli[p]

        # l'altezza vera di ogni modello, per scalare il grande a ~24 m e i piccoli a 9-14 m
        def alto(m):
            b = m.get_bounding_box()
            return max(1.0, b.max.z - b.min.z)

        seme = [17]

        def caso():
            seme[0] = (seme[0] * 9301 + 49297) % 233280
            return seme[0] / 233280.0

        messi = 0
        for k, (px, py, tipo, fronde) in enumerate(alberi):
            x, y = px + angolo.x, py + angolo.y
            z = quote.get((px, py))
            if z is None:
                continue
            if tipo == "grande":
                # (15/11 sera) il faggio "di campo" (chioma larga, da albero solitario) se c'e': quelli "di bosco"
                # sono alti e magri, belli in un bosco ma spogli da soli in piazza
                campo = [q for q in grandi if "field" in q.lower()]
                p = campo[0] if campo else max(grandi, key=lambda q: alto(modello(q)))
                m = modello(p)
                s = 2400.0 / alto(m)
            else:
                p = piccoli[int(caso() * len(piccoli)) % len(piccoli)]
                m = modello(p)
                s = (900.0 + caso() * 500.0) / alto(m)
            s = max(0.3, min(3.0, s))
            a = attori.spawn_actor_from_object(m, unreal.Vector(x, y, z - 15.0),
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=caso() * 360.0))
            if not a:
                continue
            a.set_actor_scale3d(unreal.Vector(s, s, s))
            a.set_actor_label(("La quercia della piazza %d" % (k + 1)) if tipo == "grande" else "Albero in citta %d" % k)
            a.set_folder_path(CONTORNO)
            a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
            messi += 1
            scrivi("  %s: %s a (%.0f, %.0f), alto %.0f m, spazio libero intorno %.0f m"
                   % ("GRANDE" if tipo == "grande" else "albero", p.rsplit("/", 1)[-1], x, y,
                      alto(m) * s / 100.0, fronde / 100.0))
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
        scrivi("messi %d alberi, livello salvato" % messi)
        scrivi("FATTO. Ora rilancia 'PNG: panche, letti e porte': i PNG impareranno a girare intorno agli alberi.")

    try:
        with unreal.ScopedEditorTransaction("Valdorso: alberi in citta'"):
            principale()
    except Exception as e:  # noqa: BLE001
        avviso("errore: %s" % e)
        raise
    finally:
        cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
        os.makedirs(cartella, exist_ok=True)
        with open(os.path.join(cartella, "alberi_citta.txt"), "w", encoding="utf-8") as f:
            f.write("\n".join(righe) + "\n")


if __name__ == "__main__":
    _in_unreal()
