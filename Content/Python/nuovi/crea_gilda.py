# Valdorso - La Gilda e il Balivo nel villaggio (v0.1.4, 10/11/2026). Scritto da Claude.
# Si lancia in L_Valle dal menu Valdorso -> Villaggio -> "Gilda: bacheca degli incarichi, Lucia e bacheca del Balivo"
# (o Strumenti -> Esegui script), dopo aver compilato e dopo crea_villaggio.py. Si puo' rilanciare: toglie quello che ha
# messo prima (etichetta "ValdorsoGilda").
#
# Fa tre cose (vedi Source/Valdorso/ValdorsoGilda.h):
#   1. "Bacheca della Gilda" (messa da crea_villaggio.py davanti alla Gilda) diventa la bacheca degli incarichi (Tipo
#      Gilda) e va dentro, sulla parete di fondo, a destra del bancone, girata verso la porta.
#   2. Lucia (il PNG con il dialogo "Lucia") dietro il bancone della Gilda, girata verso la porta.
#   2b. (10/11) Mastro Ettore (il PNG con il dialogo "CapoGilda"), il capo della filiale: in fondo a sinistra, girato verso
#      la porta. I gradi li da' lo staff a nome suo (finestra dello staff, F9).
#   3. "Bacheca del Balivo": una bacheca nuova con la tettoia, fuori dal cancello del cortile del Balivo, girata verso
#      la piazza (Tipo Balivo: i proclami dello staff).
# Resoconto in Saved/Claude/gilda.txt. Alla fine salva.

import math
import os

import unreal

ETICHETTA = "ValdorsoGilda"
CARTELLA = "Valle/Gilda"
MEZZA_ALTEZZA = 92.0                   # la capsula del PNG: l'origine sta 92 cm sopra i piedi
SOPRA_PAVIMENTO = 150.0

NOME_GILDA = "Gilda"                   # l'edificio (Tools/Generatori/villaggio.py: 1200 x 900, la porta sulla facciata -Y)
NOME_BACHECA_GILDA = "Bacheca della Gilda"
BACHECA_GILDA_LOCALE = (380.0, 395.0)  # a destra del bancone (che va da -200 a 200 in x, a y 85..155), contro la parete di fondo
LUCIA_LOCALE = (0.0, 240.0)            # dietro il bancone
ETTORE_LOCALE = (-330.0, 300.0)        # in fondo a sinistra (lo scaffale degli incarichi e' contro la parete a -540)
PORTA_LOCALE = (0.0, -450.0)

NOME_MURO_BALIVO = "Muro del cortile del Balivo"   # l'origine e' l'angolo L(28, -4) del cortile
BALIVO_DAL_MURO = (950.0, 170.0)                   # L(37.5, -2.3): fuori dal cancello (fra L(28,-4) e L(35,-4)), a destra
BALIVO_GRADI = 90.0                                # la faccia della bacheca (la sua X) verso +Y, cioe' verso la piazza

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Gilda: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Gilda: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "gilda.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))


def ricostruisci(a):
    """(10/11) Rifa' la bacheca (tettoia, fogli). In 5.8 rerun_construction_scripts non c'e' per gli attori C++: basta
    gia' set_editor_property (l'editor la rifa' da solo), quindi se manca si va avanti."""
    rifai = getattr(a, "rerun_construction_scripts", None)
    if rifai is not None:
        try:
            rifai()
        except Exception:  # noqa: BLE001
            pass


def tipi_oggetti():
    """WorldStatic e WorldDynamic (i nomi nuovi di 5.8, o quelli vecchi)."""
    q = unreal.ObjectTypeQuery
    fisso = getattr(q, "ECC_WORLD_STATIC", None)
    mobile = getattr(q, "ECC_WORLD_DYNAMIC", None)
    return [q.OBJECT_TYPE_QUERY1 if fisso is None else fisso, q.OBJECT_TYPE_QUERY2 if mobile is None else mobile]


def mondo_editor():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def colpi_in_giu(x, y, z_alto, z_basso, ignora):
    """Gli urti di una linea verticale (dall'alto in basso), come (z, attore), in ordine (come metti_png.py)."""
    r = unreal.SystemLibrary.line_trace_multi_for_objects(
        mondo_editor(), unreal.Vector(x, y, z_alto), unreal.Vector(x, y, z_basso),
        tipi_oggetti(), True, ignora,
        unreal.DrawDebugTrace.NONE, True)
    colpi = r[0] if isinstance(r, tuple) and r and isinstance(r[0], (list, tuple)) else r
    fuori = []
    for hit in colpi or []:
        t = hit.to_tuple()
        fuori.append((t[5].z if t[5] is not None else t[4].z, t[9]))
    fuori.sort(key=lambda c: -c[0])
    return fuori


def pavimento(x, y, z_stima, ignora):
    """La quota del pavimento dentro un edificio vicino a z_stima (z None se non lo trova) e se sopra e' libero."""
    colpi = colpi_in_giu(x, y, z_stima + SOPRA_PAVIMENTO, z_stima - 300.0, ignora)
    vicini = [z for z, _ in colpi if abs(z - z_stima) <= 60.0]
    if not vicini:
        return None, False
    z = max(vicini)
    libero = not any(z + 30.0 < zz for zz, _ in colpi)
    return z, libero


def terra(x, y, z_vicino, ignora):
    """La quota del terreno (o della strada) all'aperto: il primo urto dall'alto, fino a 30 m sopra e sotto."""
    colpi = colpi_in_giu(x, y, z_vicino + 3000.0, z_vicino - 3000.0, ignora)
    terreno = [z for z, a in colpi if a is not None and "Landscape" in a.get_class().get_name()]
    if not terreno:
        return None
    z = terreno[0]
    sopra = [zz for zz, a in colpi if zz > z and zz <= z + 60.0]
    return max(sopra) if sopra else z


def al_mondo(x, y, gradi, lx, ly):
    a = math.radians(gradi)
    return x + lx * math.cos(a) - ly * math.sin(a), y + lx * math.sin(a) + ly * math.cos(a)


def gradi_verso(da_x, da_y, a_x, a_y):
    return math.degrees(math.atan2(a_y - da_y, a_x - da_x))


def esegui():
    bacheca = getattr(unreal, "ValdorsoBacheca", None)
    png = getattr(unreal, "ValdorsoPNG", None)
    tipi = getattr(unreal, "ValdorsoTipoBacheca", None)
    if bacheca is None or png is None or tipi is None:
        unreal.log_error("[Valdorso] Gilda: mancano le classi C++ (ValdorsoBacheca con il Tipo, ValdorsoPNG): compila Valdorso")
        return
    tutti = attori.get_all_level_actors()
    tolti = 0
    for a in tutti:
        if a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            tolti += 1
    if tolti:
        scrivi("tolti %d attori messi prima" % tolti)
    tutti = attori.get_all_level_actors()
    per_nome = {}
    for a in tutti:
        per_nome.setdefault(a.get_actor_label(), a)
    nostri = [a for a in tutti if a.get_class().get_name().startswith("Valdorso")]

    # --- 1 e 2: dentro la Gilda
    gilda = per_nome.get(NOME_GILDA)
    if gilda is None:
        avviso("non trovo l'edificio \"%s\" (crea_villaggio.py e' stato lanciato?): niente Lucia, la bacheca resta dov'e'" % NOME_GILDA)
    else:
        q = gilda.get_actor_location()
        cx, cy, pav, gradi = q.x, q.y, q.z, gilda.get_actor_rotation().yaw
        porta = per_nome.get(NOME_GILDA + " porta")
        px, py = (porta.get_actor_location().x, porta.get_actor_location().y) if porta else al_mondo(cx, cy, gradi, *PORTA_LOCALE)

        b = per_nome.get(NOME_BACHECA_GILDA)
        if b is None:
            avviso("non trovo \"%s\": la metto nuova" % NOME_BACHECA_GILDA)
            b = attori.spawn_actor_from_class(bacheca, unreal.Vector(cx, cy, pav), unreal.Rotator(0, 0, 0))
            b.set_actor_label(NOME_BACHECA_GILDA)
            b.set_folder_path(CARTELLA)
            b.set_editor_property("tags", [unreal.Name(ETICHETTA)])
        x, y = al_mondo(cx, cy, gradi, *BACHECA_GILDA_LOCALE)
        z, libero = pavimento(x, y, pav, nostri)
        if z is None:
            avviso("bacheca della Gilda: non trovo il pavimento (la Gilda e' caricata?): uso la quota dell'edificio")
            z = pav
        elif not libero:
            avviso("bacheca della Gilda: sopra il posto c'e' qualcosa (la scala? lo scaffale?): controlla e spostala a mano se serve")
        b.set_actor_location(unreal.Vector(x, y, z), False, False)
        b.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=gradi_verso(x, y, px, py)), False)
        b.set_editor_property("tipo", tipi.GILDA)
        b.set_editor_property("titolo", unreal.Text("Bacheca della Gilda"))
        ricostruisci(b)
        scrivi("bacheca della Gilda a (%.0f, %.0f, %.0f), girata verso la porta" % (x, y, z))

        x, y = al_mondo(cx, cy, gradi, *LUCIA_LOCALE)
        z, libero = pavimento(x, y, pav, nostri + [b])
        if z is None:
            avviso("Lucia: non trovo il pavimento della Gilda: uso la quota dell'edificio")
            z = pav
        elif not libero:
            avviso("Lucia: sopra il posto c'e' qualcosa: controlla e spostala a mano se serve")
        lucia = attori.spawn_actor_from_class(png, unreal.Vector(x, y, z + MEZZA_ALTEZZA),
                                              unreal.Rotator(roll=0.0, pitch=0.0, yaw=gradi_verso(x, y, px, py)))
        if lucia is None:
            avviso("non sono riuscito a mettere Lucia")
        else:
            lucia.set_editor_property("dialogo", unreal.Name("Lucia"))
            lucia.set_actor_label("Lucia, della Gilda")
            lucia.set_folder_path(CARTELLA)
            lucia.set_editor_property("tags", [unreal.Name(ETICHETTA)])
            for nome, valore in (("donna", True), ("testa", -1)):
                try:
                    lucia.set_editor_property(nome, valore)
                except Exception as e:  # noqa: BLE001
                    avviso("Lucia: non riesco a impostare %s (%s)" % (nome, e))
            scrivi("Lucia dietro il bancone a (%.0f, %.0f, %.0f)" % (x, y, z))

        # (10/11) Mastro Ettore, il capo della filiale
        x, y = al_mondo(cx, cy, gradi, *ETTORE_LOCALE)
        z, libero = pavimento(x, y, pav, nostri + [b])
        if z is None:
            avviso("Mastro Ettore: non trovo il pavimento della Gilda: uso la quota dell'edificio")
            z = pav
        elif not libero:
            avviso("Mastro Ettore: sopra il posto c'e' qualcosa (la scala? lo scaffale?): controlla e spostalo a mano se serve")
        ettore = attori.spawn_actor_from_class(png, unreal.Vector(x, y, z + MEZZA_ALTEZZA),
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=gradi_verso(x, y, px, py)))
        if ettore is None:
            avviso("non sono riuscito a mettere Mastro Ettore")
        else:
            ettore.set_editor_property("dialogo", unreal.Name("CapoGilda"))
            ettore.set_actor_label("Mastro Ettore, capo della filiale")
            ettore.set_folder_path(CARTELLA)
            ettore.set_editor_property("tags", [unreal.Name(ETICHETTA)])
            for nome, valore in (("donna", False), ("testa", -1)):
                try:
                    ettore.set_editor_property(nome, valore)
                except Exception as e:  # noqa: BLE001
                    avviso("Mastro Ettore: non riesco a impostare %s (%s)" % (nome, e))
            scrivi("Mastro Ettore in fondo alla Gilda a (%.0f, %.0f, %.0f)" % (x, y, z))

    # --- 3: la bacheca del Balivo, fuori dal cancello del cortile
    muro = per_nome.get(NOME_MURO_BALIVO)
    if muro is None:
        avviso("non trovo \"%s\" (crea_villaggio.py?): niente bacheca del Balivo" % NOME_MURO_BALIVO)
    else:
        q = muro.get_actor_location()
        x, y = q.x + BALIVO_DAL_MURO[0], q.y + BALIVO_DAL_MURO[1]
        z = terra(x, y, q.z, nostri + [muro])
        if z is None:
            avviso("bacheca del Balivo: la linea dall'alto non trova il terreno (la piazza e' caricata?): uso la quota del muro")
            z = q.z
        a = attori.spawn_actor_from_class(bacheca, unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=BALIVO_GRADI))
        a.set_actor_label("Bacheca del Balivo")
        a.set_folder_path(CARTELLA)
        a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
        a.set_editor_property("tipo", tipi.BALIVO)
        a.set_editor_property("titolo", unreal.Text("Proclami del Balivo"))
        a.set_editor_property("avvisi", ["Per ordine del Balivo: chi trova armi o cose degli Antichi le porti alla casa del Balivo. Saranno pagate."])
        try:
            a.set_editor_property("is_spatially_loaded", False)
        except Exception:  # noqa: BLE001
            pass
        ricostruisci(a)
        scrivi("bacheca del Balivo a (%.0f, %.0f, %.0f), con la tettoia, girata verso la piazza" % (x, y, z))

    scrivi("FATTO. In Play: alla Gilda, E sulla bacheca della parete; poi parla con Lucia, al bancone, e con Mastro Ettore per i gradi.")
    salva_resoconto()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


esegui()
