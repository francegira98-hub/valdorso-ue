# Valdorso - Mette i primi PNG della valle, senza rifare il villaggio (v0.1.3). Scritto da Claude il 07/11/2026.
# Si lancia in L_Valle (dopo aver compilato: serve la classe C++ ValdorsoPNG): Strumenti -> Esegui script Python ->
# questo file, oppure dal menu Valdorso -> "PNG: metti Gaspare e le guardie del passo".
# (08/11) Mette anche Nonna Edda, Bruno e Padre Anselmo (vedi sotto).
#
# Mette:
# - Gaspare, il vecchio cacciatore, al margine del bosco grande: accanto alla strada che dal villaggio va alle Rovine
#   degli Antichi, dove il bosco comincia a farsi fitto (lo dice lo strato Bosco della valle), girato verso il
#   villaggio, a guardare chi arriva. Se nel livello c'è un attore con l'etichetta "Posto di Gaspare" (per esempio
#   un Target Point messo a mano), Gaspare va lì, girato come lui.
# - Corrado e Ottone, le guardie della Corona, davanti alla porta del passo, uno per parte, girati verso la valle.
#   Li mette rispetto alla "Porta del passo" e al "Sigillo della Corona" messi da crea_villaggio.py.
# - (08/11) Nonna Edda alla palude: accanto al fuoco del calderone, dove la passerella arriva alla scaletta della
#   capanna, girata verso chi arriva dalla passerella ("Fuoco del calderone", "Passerella della palude").
# - (08/11) Bruno dentro la Locanda di Bruno, in fondo al bancone, girato verso la porta ("Locanda_di_Bruno",
#   "Locanda_di_Bruno porta"). Dentro le case conta il pavimento: la linea parte 1,5 m sopra il pavimento (sotto il
#   soffitto del piano terra, così non trova il piano di sopra) e scende.
# - (08/11) Padre Anselmo nel Tempio del Cuore, 2,2 m davanti all'altare e un poco di lato (per non coprire la pietra),
#   girato verso l'ingresso ("Altare del Cuore (luce)" o "La pietra di Val d'Orso", e "Tempio_del_Cuore").
#   Se un attore non c'è, si usano le posizioni dei dati (Villaggio_3000.json) dall'angolo del paesaggio.
#   Nonna Edda ha la testa da donna anziana (Etta), Padre Anselmo quella da uomo anziano (Walter); Bruno quella "dei
#   dadi" (sempre la stessa per lui).
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

# (08/11) Nonna Edda, Bruno e Padre Anselmo. Le posizioni dei dati (cm dall'angolo del paesaggio e quota vera, come
# Villaggio_3000.json) servono solo se nel livello manca l'attore con quell'etichetta.
# Le teste (UValdorsoAspettoComponent): da donna 2 = Etta, da uomo 1 = Walter; -1 = quella "dei dadi".
TESTA_EDDA = 2
TESTA_ANSELMO = 1
NOME_FUOCO_EDDA = "Fuoco del calderone"
NOME_PASSERELLA = "Passerella della palude"
DATI_FUOCO_EDDA = (70935.9, 230108.8, 3048.9)
DATI_PASSERELLA = (71231.5, 230173.8, 3046.5)
PASSERELLA_VERSO_LAGO = (1808.5, -773.8)    # dall'inizio della passerella verso il suo secondo punto (chi arriva)
EDDA_FRA_FUOCO_E_PASSERELLA = 0.65          # dal fuoco (0) all'inizio della passerella (1)

NOME_LOCANDA = "Locanda_di_Bruno"
NOME_PORTA_LOCANDA = "Locanda_di_Bruno porta"
DATI_LOCANDA = (162200.0, 192428.0, 5569.2, 90.0)   # x, y, pavimento, gradi
BRUNO_LOCALE = (-110.0, 300.0)              # in fondo al bancone (che va da -585 a -175 in x, a y 275..365)
PORTA_LOCANDA_LOCALE = (-150.0, -480.0)

NOME_ALTARE = "Altare del Cuore (luce)"     # 260 cm sopra il pavimento del tempio
NOME_PIETRA = "La pietra di Val d'Orso"     # 118 cm sopra il pavimento (sull'altare)
NOME_TEMPIO = "Tempio_del_Cuore"
DATI_ALTARE = (165300.0, 188058.0, 5773.0)
DATI_TEMPIO = (165300.0, 188878.0, 5513.0)
ANSELMO_AVANTI = 220.0                      # cm dall'altare verso l'ingresso (la pedana finisce a 150)
ANSELMO_LATO = 150.0                        # cm di lato (le panche stanno a 260, la prima fila a 390 dall'altare)

SOPRA_PAVIMENTO = 150.0                     # dentro: la linea parte da qui (sotto il soffitto, sopra i tavoli no)

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


def volto(a, donna, testa):
    """(08/11) Donna o uomo e la testa MetaHuman (bDonna e Testa del PNG)."""
    if a is None:
        return
    for nome, valore in (("donna", bool(donna)), ("testa", int(testa))):
        try:
            a.set_editor_property(nome, valore)
        except Exception as e:  # noqa: BLE001
            avviso("%s: non riesco a impostare %s (%s)" % (a.get_actor_label(), nome, e))


def pavimento(x, y, z_stima, ignora):
    """(08/11) La quota del pavimento dentro un edificio, vicino a z_stima: una linea da SOPRA_PAVIMENTO sopra la stima
    (sotto il soffitto) in giù, il primo urto vicino alla stima. Restituisce (z, libero): z None se non trova niente
    (allora si usa la stima); libero è falso se sopra il pavimento c'era altro (un tavolo, una panca)."""
    colpi = colpi_in_giu(x, y, z_stima + SOPRA_PAVIMENTO, z_stima - 300.0, ignora)
    vicini = [z for z, _ in colpi if abs(z - z_stima) <= 60.0]
    if not vicini:
        return None, False
    z = max(vicini)
    libero = not any(z + 30.0 < zz for zz, _ in colpi)
    return z, libero


def gradi_verso(da_x, da_y, a_x, a_y):
    return math.degrees(math.atan2(a_y - da_y, a_x - da_x))


def al_mondo(x, y, gradi, lx, ly):
    """Da centimetri locali di un edificio (come Tools/Generatori/villaggio.py) al mondo."""
    a = math.radians(gradi)
    return x + lx * math.cos(a) - ly * math.sin(a), y + lx * math.sin(a) + ly * math.cos(a)


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

    # 4. (08/11) Nonna Edda, Bruno, Padre Anselmo. Le linee verso il basso non guardano gli attori C++ della valle
    #    (porte, panche, la pietra): hanno zone invisibili che la linea troverebbe prima del pavimento.
    nostri = [a for a in tutti if a.get_class().get_name().startswith("Valdorso")]
    messi += metti_edda(classe, per_nome, nel_mondo, nostri)
    messi += metti_bruno(classe, per_nome, nel_mondo, nostri)
    messi += metti_anselmo(classe, per_nome, nel_mondo, nostri)

    scrivi("messi %d PNG nella cartella %s" % (messi, CARTELLA))
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): Ctrl+Maiusc+S" % e)
    scrivi("FATTO. In Play: Gaspare sulla strada per le rovine, dove comincia il bosco; Corrado e Ottone davanti alla "
           "porta del passo; Nonna Edda al fuoco della palude; Bruno al bancone della locanda; Padre Anselmo davanti "
           "all'altare del Tempio del Cuore. Avvicinati e premi E.")


def dove(per_nome, nome, dati, nel_mondo):
    """(08/11) Dov'è l'attore con quell'etichetta; se non c'è, il punto dei dati (x, y, quota) nel mondo."""
    a = per_nome.get(nome)
    if a is not None:
        return a.get_actor_location(), a
    avviso("non trovo \"%s\" nel livello: uso la posizione dei dati (il villaggio è stato messo? crea_villaggio.py)" % nome)
    return nel_mondo(dati[0], dati[1], dati[2]), None


def metti_edda(classe, per_nome, nel_mondo, nostri):
    """(08/11) Nonna Edda accanto al fuoco del calderone, verso l'inizio della passerella, girata verso chi arriva."""
    fuoco, _ = dove(per_nome, NOME_FUOCO_EDDA, DATI_FUOCO_EDDA, nel_mondo)
    passerella, _ = dove(per_nome, NOME_PASSERELLA, DATI_PASSERELLA, nel_mondo)
    f = EDDA_FRA_FUOCO_E_PASSERELLA
    x = fuoco.x + (passerella.x - fuoco.x) * f
    y = fuoco.y + (passerella.y - fuoco.y) * f
    z, libero = terra(x, y, fuoco.z, nostri)
    if z is None:
        avviso("Nonna Edda: la linea dall'alto non trova il paesaggio (la palude è caricata?): uso la quota del fuoco")
        z = fuoco.z
    elif not libero:
        avviso("Nonna Edda: sopra il posto c'è qualcosa (la scaletta? il treppiede?): controlla e spostala a mano se serve")
    verso = gradi_verso(x, y, passerella.x + PASSERELLA_VERSO_LAGO[0], passerella.y + PASSERELLA_VERSO_LAGO[1])
    a = metti(classe, "NonnaEdda", "Nonna Edda, la vecchia della palude", x, y, z, verso)
    volto(a, True, TESTA_EDDA)
    return 1 if a else 0


def metti_bruno(classe, per_nome, nel_mondo, nostri):
    """(08/11) Bruno in fondo al bancone della locanda, girato verso la porta."""
    locanda = per_nome.get(NOME_LOCANDA)
    if locanda is not None:
        q = locanda.get_actor_location()
        cx, cy, pav, gradi = q.x, q.y, q.z, locanda.get_actor_rotation().yaw
    else:
        avviso("non trovo \"%s\" nel livello: uso la posizione dei dati" % NOME_LOCANDA)
        q = nel_mondo(DATI_LOCANDA[0], DATI_LOCANDA[1], DATI_LOCANDA[2])
        cx, cy, pav, gradi = q.x, q.y, q.z, DATI_LOCANDA[3]
    x, y = al_mondo(cx, cy, gradi, *BRUNO_LOCALE)
    porta = per_nome.get(NOME_PORTA_LOCANDA)
    if porta is not None:
        px, py = porta.get_actor_location().x, porta.get_actor_location().y
    else:
        px, py = al_mondo(cx, cy, gradi, *PORTA_LOCANDA_LOCALE)
    z, libero = pavimento(x, y, pav, nostri)
    if z is None:
        avviso("Bruno: non trovo il pavimento della locanda (la locanda è caricata?): uso la quota dei dati")
        z = pav
    elif not libero:
        avviso("Bruno: sopra il pavimento c'è qualcosa (il bancone? una botte?): controlla e spostalo a mano se serve")
    a = metti(classe, "Bruno", "Bruno, il locandiere", x, y, z, gradi_verso(x, y, px, py))
    volto(a, False, -1)
    return 1 if a else 0


def metti_anselmo(classe, per_nome, nel_mondo, nostri):
    """(08/11) Padre Anselmo davanti all'altare, un poco di lato, girato verso l'ingresso del tempio."""
    luce = per_nome.get(NOME_ALTARE)
    pietra = per_nome.get(NOME_PIETRA)
    if luce is not None:
        q = luce.get_actor_location()
        ax, ay, pav = q.x, q.y, q.z - 260.0
    elif pietra is not None:
        q = pietra.get_actor_location()
        ax, ay, pav = q.x, q.y, q.z - 118.0
    else:
        avviso("non trovo \"%s\" né \"%s\": uso la posizione dei dati" % (NOME_ALTARE, NOME_PIETRA))
        q = nel_mondo(DATI_ALTARE[0], DATI_ALTARE[1], DATI_ALTARE[2])
        ax, ay, pav = q.x, q.y, q.z - 260.0
    tempio, _ = dove(per_nome, NOME_TEMPIO, DATI_TEMPIO, nel_mondo)
    # dall'altare (in fondo alla navata) verso il centro del tempio = verso l'ingresso
    dx, dy = tempio.x - ax, tempio.y - ay
    lung = math.hypot(dx, dy)
    if lung < 1.0:
        dx, dy, lung = 0.0, 1.0, 1.0                 # come nei dati: l'ingresso del tempio guarda a sud (+Y)
    dx, dy = dx / lung, dy / lung
    lx, ly = -dy, dx                                  # di lato
    x = ax + dx * ANSELMO_AVANTI + lx * ANSELMO_LATO
    y = ay + dy * ANSELMO_AVANTI + ly * ANSELMO_LATO
    ignora = nostri + [p for p in (pietra,) if p is not None]
    z, libero = pavimento(x, y, pav, ignora)
    if z is None:
        avviso("Padre Anselmo: non trovo il pavimento del tempio (è caricato?): uso la quota dell'altare")
        z = pav
    elif not libero:
        avviso("Padre Anselmo: sopra il pavimento c'è qualcosa (una panca?): controlla e spostalo a mano se serve")
    a = metti(classe, "PadreAnselmo", "Padre Anselmo, sacerdote del Tempio del Cuore", x, y, z, math.degrees(math.atan2(dy, dx)))
    volto(a, False, TESTA_ANSELMO)
    return 1 if a else 0


principale()
