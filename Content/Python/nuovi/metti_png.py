# Valdorso - Mette i primi PNG della valle, senza rifare il villaggio (v0.1.3). Scritto da Claude il 07/11/2026.
# Si lancia in L_Valle (dopo aver compilato: serve la classe C++ ValdorsoPNG): Strumenti -> Esegui script Python ->
# questo file, oppure dal menu Valdorso -> "PNG: metti Gaspare e le guardie del passo".
#
# Mette:
# - Gaspare, il vecchio cacciatore, al margine del bosco grande: accanto alla strada che dal villaggio va alle Rovine
#   degli Antichi, dove il bosco comincia a farsi fitto (lo dice lo strato Bosco della valle), girato verso il
#   villaggio, a guardare chi arriva. Se nel livello c'è un attore con l'etichetta "Posto di Gaspare" (per esempio
#   un Target Point messo a mano), Gaspare va lì, girato come lui.
# - Corrado e Ottone, le guardie della Corona, davanti alla porta del passo, uno per parte, girati verso la valle.
#   Li mette rispetto alla "Porta del passo" e al "Sigillo della Corona" messi da crea_villaggio.py.
# Ognuno sta a terra (una linea dall'alto: il paesaggio, o la strada se c'è). Toglie i PNG messi prima da questo script
# (etichetta "ValdorsoPNG"); crea_villaggio.py non li tocca. Alla fine salva il livello.

import math

import unreal

ETICHETTA = "ValdorsoPNG"
CARTELLA = "Valle/PNG"
QUOTA_MAX_CM = 80000.0          # come crea_villaggio.py: i 16 bit delle altezze coprono 0-800 m
MEZZA_ALTEZZA = 92.0            # la capsula del PNG: l'origine sta 92 cm sopra i piedi
POSTO_GASPARE = "Posto di Gaspare"

# Gaspare: un punto della strada per le rovine al margine del bosco (cm dall'angolo del paesaggio, come
# Villaggio_3000.json), la direzione della strada verso le rovine e la quota del terreno lì (solo se la linea
# dall'alto non trova niente). Si mette un poco di lato, nel primo posto libero.
GASPARE_STRADA = (110240.0, 145945.0)
GASPARE_VERSO_ROVINE = (-0.80, -0.60)
GASPARE_QUOTA = 5400.0
GASPARE_LATI = (450.0, 350.0, 550.0, -450.0, -350.0, 650.0)

# Le guardie: metri verso la valle davanti alla porta, metri di traverso (+ verso la bacheca del passo).
GUARDIE = (
    ("GuardiaCorrado", "Corrado, guardia della Corona", 2.0, -4.3),
    ("GuardiaOttone", "Ottone, guardia della Corona", 2.0, 4.3),
)
NOME_PORTA = "Porta del passo"
NOME_SIGILLO = "Sigillo della Corona"

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def scrivi(testo):
    unreal.log("[Valdorso] PNG: " + testo)


def avviso(testo):
    unreal.log_warning("[Valdorso] PNG: " + testo)


def mondo_editor():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def colpi_in_giu(x, y, z_alto, z_basso, ignora):
    """Gli urti di una linea verticale (dall'alto in basso), come (z, attore), in ordine (come sistema_valle.py)."""
    r = unreal.SystemLibrary.line_trace_multi_for_objects(
        mondo_editor(), unreal.Vector(x, y, z_alto), unreal.Vector(x, y, z_basso),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2], True, ignora,
        unreal.DrawDebugTrace.NONE, True)
    colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
    fuori = []
    for hit in colpi or []:
        t = hit.to_tuple()
        fuori.append((t[5].z if t[5] is not None else t[4].z, t[9]))
    fuori.sort(key=lambda c: -c[0])
    return fuori


def e_paesaggio(attore):
    return attore is not None and "Landscape" in attore.get_class().get_name()


def terra(x, y, z_vicino, ignora):
    """La quota dei piedi in (x, y): il paesaggio, o una strada/selciato appoggiato sopra (fino a 60 cm più su).
    Restituisce (z, libero): libero è falso se sopra i piedi, fino a 2,5 m, c'è altro (un tronco, un muro)."""
    colpi = colpi_in_giu(x, y, z_vicino + 3000.0, z_vicino - 3000.0, ignora)
    suolo = [z for z, a in colpi if e_paesaggio(a)]
    if not suolo:
        return None, False
    z = suolo[0]
    sopra = [zz for zz, a in colpi if not e_paesaggio(a) and zz > z]
    appoggiati = [zz for zz in sopra if zz <= z + 60.0]
    if appoggiati:
        z = max(appoggiati)
    libero = not any(z + 60.0 < zz <= z + 250.0 for zz in sopra)
    return z, libero


def metti(classe, dialogo, nome, x, y, z_piedi, gradi):
    p = unreal.Vector(x, y, z_piedi + MEZZA_ALTEZZA)
    a = attori.spawn_actor_from_class(classe, p, unreal.Rotator(roll=0.0, pitch=0.0, yaw=gradi))
    if a is None:
        unreal.log_error("[Valdorso] PNG: non sono riuscito a mettere %s" % nome)
        return None
    a.set_editor_property("dialogo", unreal.Name(dialogo))
    a.set_actor_label(nome)
    a.set_folder_path(CARTELLA)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    scrivi("%s in (%.0f, %.0f, %.0f), girato a %.0f gradi" % (nome, p.x, p.y, p.z, gradi))
    return a


def principale():
    classe = getattr(unreal, "ValdorsoPNG", None)
    if classe is None:
        unreal.log_error("[Valdorso] PNG: manca la classe C++ ValdorsoPNG. Compila Valdorso e riapri l'editor.")
        return
    tutti = attori.get_all_level_actors()
    paesaggi = [a for a in tutti if isinstance(a, unreal.Landscape)]
    if not paesaggi:
        unreal.log_error("[Valdorso] PNG: nel livello aperto non c'è un Landscape. Apri L_Valle.")
        return
    angolo = paesaggi[0].get_actor_location()
    scala = paesaggi[0].get_actor_scale3d()

    def nel_mondo(px, py, quota):
        valore = quota / QUOTA_MAX_CM * 65535.0
        return unreal.Vector(angolo.x + px, angolo.y + py, angolo.z + (valore - 32768.0) * scala.z / 128.0)

    # 1. via quelli messi prima
    tolti = 0
    for a in tutti:
        if a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            tolti += 1
    if tolti:
        scrivi("tolti %d PNG messi prima" % tolti)
    tutti = attori.get_all_level_actors()
    per_nome = {}
    for a in tutti:
        per_nome.setdefault(a.get_actor_label(), a)
    messi = 0

    # 2. Gaspare
    posto = per_nome.get(POSTO_GASPARE)
    if posto is not None:
        q = posto.get_actor_location()
        z, _ = terra(q.x, q.y, q.z, [posto])
        if z is None:
            z = q.z
        if metti(classe, "Gaspare", "Gaspare, il vecchio cacciatore", q.x, q.y, z, posto.get_actor_rotation().yaw):
            messi += 1
    else:
        s = nel_mondo(GASPARE_STRADA[0], GASPARE_STRADA[1], GASPARE_QUOTA)
        vx, vy = GASPARE_VERSO_ROVINE
        lato = (-vy, vx)                      # di traverso alla strada
        verso_villaggio = math.degrees(math.atan2(-vy, -vx))
        scelto = None
        for k in GASPARE_LATI:
            x, y = s.x + lato[0] * k, s.y + lato[1] * k
            z, libero = terra(x, y, s.z, [])
            if z is None:
                continue
            if libero:
                scelto = (x, y, z)
                break
            if scelto is None:
                scelto = (x, y, z)            # ricordato, se non se ne trova uno libero
        if scelto is None:
            avviso("la linea dall'alto non trova il paesaggio vicino al bosco (la zona è caricata nel World Partition?): "
                   "uso la quota dei dati")
            scelto = (s.x + lato[0] * GASPARE_LATI[0], s.y + lato[1] * GASPARE_LATI[0], s.z)
        if metti(classe, "Gaspare", "Gaspare, il vecchio cacciatore", scelto[0], scelto[1], scelto[2], verso_villaggio):
            messi += 1

    # 3. Le guardie del passo
    porta = per_nome.get(NOME_PORTA)
    sigillo = per_nome.get(NOME_SIGILLO)
    if porta is None or sigillo is None:
        avviso("non trovo \"%s\" o \"%s\" (il posto di guardia c'è? crea_villaggio.py): niente guardie" % (NOME_PORTA, NOME_SIGILLO))
    else:
        pp = porta.get_actor_location()
        ps = sigillo.get_actor_location()
        dx, dy = pp.x - ps.x, pp.y - ps.y           # dal sigillo alla porta: verso la valle
        lung = math.hypot(dx, dy)
        if lung < 1.0:
            f = sigillo.get_actor_forward_vector()   # la X del sigillo guarda verso la valle
            dx, dy, lung = f.x, f.y, max(1e-6, math.hypot(f.x, f.y))
        dx, dy = dx / lung, dy / lung
        nx, ny = -dy, dx                              # di traverso al passo (come villaggio.py)
        verso_valle = math.degrees(math.atan2(dy, dx))
        for dialogo, nome, avanti, traverso in GUARDIE:
            x = pp.x + (dx * avanti + nx * traverso) * 100.0
            y = pp.y + (dy * avanti + ny * traverso) * 100.0
            z, libero = terra(x, y, pp.z, [porta, sigillo])
            if z is None:
                avviso("%s: la linea dall'alto non trova il paesaggio, uso la quota della porta" % nome)
                z = pp.z
            elif not libero:
                avviso("%s: sopra il posto c'è qualcosa (un palo? una botte?): controlla e spostalo a mano se serve" % nome)
            if metti(classe, dialogo, nome, x, y, z, verso_valle):
                messi += 1

    scrivi("messi %d PNG nella cartella %s" % (messi, CARTELLA))
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): Ctrl+Maiusc+S" % e)
    scrivi("FATTO. In Play: Gaspare sulla strada per le rovine, dove comincia il bosco; Corrado e Ottone davanti alla "
           "porta del passo. Avvicinati e premi E.")


principale()
