# Valdorso - I volti nuovi della valle e le loro giornate (v0.1.7 "Volti e giornate"). Scritto da Claude l'11/11/2026.
# Si lancia in L_Valle (dopo aver compilato: servono ValdorsoPNG con la Giornata e i dialoghi nuovi), dal menu
# Valdorso -> "Volti: Marta, Livia, Tobia, i mugnai, Elric", oppure Strumenti -> Esegui script Python -> questo file.
# Prima conviene lanciare "Gesti: importa da Mixamo" (importa_gesti.py): senza i gesti i PNG stanno solo fermi a respirare.
#
# Mette (etichetta "ValdorsoVolti", cartella Valle/PNG/Volti; li toglie e li rimette ogni volta):
# - Marta, la fabbra: la mattina e il pomeriggio all'incudine sotto la tettoia della fucina (martella); a mezzogiorno
#   al pozzo; la sera alla locanda, al tavolo grande (con Bartolo, un giorno); di notte a casa.
# - La capitana Livia Serra: al passo con le guardie la mattina, davanti alla casermetta a mezzogiorno, al fuoco della
#   Guardia il pomeriggio, la sera da Bruno, al bancone; di notte di nuovo al passo (una capitana non dorme).
# - Tobia, l'orfano (piu' piccolo: scala 0,72): la mattina spazza il tempio con Padre Anselmo, poi al mulino, poi davanti
#   alla fucina (Marta lo caccia), poi al pozzo; dopo il tramonto a letto in canonica.
# - Teodoro, il mugnaio: al mulino di giorno, la sera alla locanda nel suo angolo (lo "sgabello"); di notte a casa.
# - Agnese, la mugnaia: davanti al mulino (spazza), a meta' mattina al pozzo (le voci), poi di nuovo al mulino.
# - Maestro Elric, il mago di corte: alle Rovine degli Antichi a studiare; la sera al tempio. Arriva solo con l'Atto I
#   (AttoMinimo = 1: dopo il battito saltato; prima non si vede).
# Ogni posto si trova dagli attori del villaggio (crea_villaggio.py): "Fucina_di_Marta", "Locanda_di_Bruno",
# "Mulino", "Mulino porta", "Pozzo", "Tempio_del_Cuore", "Rovine degli Antichi", "Porta del passo", "Sigillo della
# Corona", "Casermetta_della_Guardia porta", "Luce del fuoco della Guardia". Se uno manca, quella tappa si salta (e lo
# dice). Fuori: i piedi sul paesaggio (una linea dall'alto); dentro: sul pavimento (come metti_png.py).
# Le ore sono quelle della valle (il cielo: AValdorsoCielo). Un PNG cambia posto solo quando nessun colono e' vicino.
# Mette anche la pietra dei segni (AValdorsoPietraSegni, ValdorsoRiti.h) davanti al Tempio del Cuore, a destra di chi
# entra: ogni colono ci incide il suo segno una volta (etichetta "ValdorsoVolti" anche lei).
# (11/11, v0.1.8) E il sasso dell'Orso sulla collina, quello del sogno di Tobia (ValdorsoStorie.h).
# (11/11, v0.2) E il pozzo che si usa con E (AValdorsoPozzo: il secchio si riempie d'acqua chiara).
# (12/11, v0.2.0) E il baule del colono nella locanda di Bruno (AValdorsoBaule: le cose che non porti con te).
# (12/11, v0.2.0) E Sibilla, la carovana (un giorno ogni tre: ValdorsoCarovana.h). E il magazzino del villaggio accanto alla bacheca della Gilda (le scorte: ValdorsoScorte.h).

import math

import unreal

ETICHETTA = "ValdorsoVolti"
CARTELLA = "Valle/PNG/Volti"
MEZZA_ALTEZZA = 92.0
SOPRA_PAVIMENTO = 150.0
SCALA_TOBIA = 0.72

# Le teste (UValdorsoAspettoComponent): -1 = quella "dei dadi" (sempre la stessa per lui/lei).
TESTA_DADI = -1

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Volti: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Volti: " + testo)


def mondo_editor():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def colpi_in_giu(x, y, z_alto, z_basso, ignora):
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
    """(come metti_png.py) La quota dei piedi fuori: il paesaggio, o un selciato appena sopra. (z, libero)."""
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


def pavimento(x, y, z_stima, ignora):
    """(come metti_png.py) La quota del pavimento dentro, vicino a z_stima. (z o None, libero)."""
    colpi = colpi_in_giu(x, y, z_stima + SOPRA_PAVIMENTO, z_stima - 300.0, ignora)
    vicini = [z for z, _ in colpi if abs(z - z_stima) <= 60.0]
    if not vicini:
        return None, False
    z = max(vicini)
    libero = not any(z + 30.0 < zz for zz, _ in colpi)
    return z, libero


def al_mondo(x, y, gradi, lx, ly):
    a = math.radians(gradi)
    return x + lx * math.cos(a) - ly * math.sin(a), y + lx * math.sin(a) + ly * math.cos(a)


def gradi_verso(da_x, da_y, a_x, a_y):
    return math.degrees(math.atan2(a_y - da_y, a_x - da_x))


class Posti:
    """I posti della valle, dagli attori del villaggio."""

    def __init__(self, tutti, ignora):
        self.per_nome = {}
        for a in tutti:
            self.per_nome.setdefault(a.get_actor_label(), a)
        self.ignora = ignora
        self.mancano = set()

    def attore(self, nome):
        a = self.per_nome.get(nome)
        if a is None and nome not in self.mancano:
            self.mancano.add(nome)
            avviso("non trovo \"%s\" nel livello (il villaggio e' messo? crea_villaggio.py): le tappe li' si saltano" % nome)
        return a

    def dentro(self, edificio, lx, ly, guarda_lx, guarda_ly):
        """Dentro un edificio, in centimetri locali (come villaggio.py), girato verso un altro punto locale."""
        e = self.attore(edificio)
        if e is None:
            return None
        q = e.get_actor_location()
        g = e.get_actor_rotation().yaw
        x, y = al_mondo(q.x, q.y, g, lx, ly)
        gx, gy = al_mondo(q.x, q.y, g, guarda_lx, guarda_ly)
        z, libero = pavimento(x, y, q.z, self.ignora)
        if z is None:
            avviso("%s (%.0f, %.0f): non trovo il pavimento, uso quello dell'edificio" % (edificio, lx, ly))
            z = q.z
        elif not libero:
            avviso("%s (%.0f, %.0f): sopra il pavimento c'e' qualcosa: controlla" % (edificio, lx, ly))
        return (x, y, z, gradi_verso(x, y, gx, gy))

    def sotto_tettoia(self, edificio, lx, ly, guarda_lx, guarda_ly):
        """Accanto a un edificio (la tettoia della fucina): il pavimento dell'edificio se c'e', se no il paesaggio."""
        e = self.attore(edificio)
        if e is None:
            return None
        q = e.get_actor_location()
        g = e.get_actor_rotation().yaw
        x, y = al_mondo(q.x, q.y, g, lx, ly)
        gx, gy = al_mondo(q.x, q.y, g, guarda_lx, guarda_ly)
        z, _ = pavimento(x, y, q.z, self.ignora)
        if z is None:
            z, _ = terra(x, y, q.z, self.ignora)
        if z is None:
            z = q.z
        return (x, y, z, gradi_verso(x, y, gx, gy))

    def intorno(self, nome, raggio, gradi_da=0.0, guarda_fuori=False):
        """Fuori, intorno a un attore: il primo posto libero sul cerchio (a partire da gradi_da), girato verso il centro."""
        a = self.attore(nome)
        if a is None:
            return None
        q = a.get_actor_location()
        primo = None
        for scarto in (0, 30, -30, 60, -60, 90, -90, 120, -120, 150, -150, 180):
            ang = math.radians(gradi_da + scarto)
            for r in (raggio, raggio * 1.4):
                x, y = q.x + math.cos(ang) * r, q.y + math.sin(ang) * r
                z, libero = terra(x, y, q.z, self.ignora + [a])
                if z is None:
                    continue
                g = gradi_verso(x, y, q.x, q.y) + (180.0 if guarda_fuori else 0.0)
                if libero:
                    return (x, y, z, g)
                if primo is None:
                    primo = (x, y, z, g)
        if primo is None:
            avviso("intorno a \"%s\": la linea dall'alto non trova il paesaggio (la zona e' caricata?)" % nome)
        return primo

    def fuori_porta(self, porta, edificio, avanti, lato):
        """Davanti alla porta di un edificio, verso fuori (avanti cm) e di lato; girato verso chi arriva."""
        p = self.attore(porta)
        e = self.attore(edificio)
        if p is None or e is None:
            return None
        pp, ce = p.get_actor_location(), e.get_actor_location()
        dx, dy = pp.x - ce.x, pp.y - ce.y
        lung = math.hypot(dx, dy) or 1.0
        dx, dy = dx / lung, dy / lung
        x = pp.x + dx * avanti - dy * lato
        y = pp.y + dy * avanti + dx * lato
        z, libero = terra(x, y, pp.z, self.ignora + [p])
        if z is None:
            z = pp.z
        elif not libero:
            avviso("davanti a \"%s\": sopra il posto c'e' qualcosa: controlla" % porta)
        return (x, y, z, math.degrees(math.atan2(dy, dx)))

    def passo(self, avanti, traverso):
        """Davanti alla porta del passo (metri verso la valle e di traverso, come le guardie in metti_png.py)."""
        porta = self.attore("Porta del passo")
        sigillo = self.attore("Sigillo della Corona")
        if porta is None or sigillo is None:
            return None
        pp, ps = porta.get_actor_location(), sigillo.get_actor_location()
        dx, dy = pp.x - ps.x, pp.y - ps.y
        lung = math.hypot(dx, dy) or 1.0
        dx, dy = dx / lung, dy / lung
        nx, ny = -dy, dx
        x = pp.x + (dx * avanti + nx * traverso) * 100.0
        y = pp.y + (dy * avanti + ny * traverso) * 100.0
        z, _ = terra(x, y, pp.z, self.ignora + [porta, sigillo])
        if z is None:
            z = pp.z
        return (x, y, z, math.degrees(math.atan2(dy, dx)))


def tappa(ora, posto, movenza="", scala=1.0, assente=False):
    """Una unreal.ValdorsoTappa (FValdorsoTappa): Dove e' il centro della capsula (i piedi + la meta' dell'altezza)."""
    t = unreal.ValdorsoTappa()
    t.set_editor_property("ora", float(ora))
    if posto is not None:
        x, y, z, g = posto
        t.set_editor_property("dove", unreal.Vector(x, y, z + MEZZA_ALTEZZA * scala))
        t.set_editor_property("gradi", float(g))
    if movenza:
        t.set_editor_property("movenza", unreal.Name(movenza))
    t.set_editor_property("assente", bool(assente))
    return t


def vuoto(v):
    return abs(v.x) + abs(v.y) + abs(v.z) < 1.0


def metti(classe, dialogo, nome, donna, giornata, scala=1.0, atto_minimo=0, ogni_giorni=0):
    buone = [t for t in giornata if t.get_editor_property("assente") or not vuoto(t.get_editor_property("dove"))]
    if not buone:
        avviso("%s: nessuna tappa trovata, non lo metto" % nome)
        return None
    # dove sta quando si apre il livello: la prima tappa con un posto (poi il gioco lo sposta all'ora giusta)
    con_posto = [t for t in buone if not vuoto(t.get_editor_property("dove"))]
    if not con_posto:
        avviso("%s: solo tappe \"a casa\", non lo metto" % nome)
        return None
    p = con_posto[0].get_editor_property("dove")
    g = con_posto[0].get_editor_property("gradi")
    a = attori.spawn_actor_from_class(classe, p, unreal.Rotator(roll=0.0, pitch=0.0, yaw=g))
    if a is None:
        unreal.log_error("[Valdorso] Volti: non sono riuscito a mettere %s" % nome)
        return None
    a.set_editor_property("dialogo", unreal.Name(dialogo))
    a.set_actor_label(nome)
    a.set_folder_path(CARTELLA)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA), unreal.Name("ValdorsoPNG_Volti")])
    if scala != 1.0:
        a.set_actor_scale3d(unreal.Vector(scala, scala, scala))
    for chiave, valore in (("donna", bool(donna)), ("testa", TESTA_DADI), ("giornata", buone), ("atto_minimo", int(atto_minimo)),
                          ("ogni_giorni", int(ogni_giorni))):
        try:
            a.set_editor_property(chiave, valore)
        except Exception as e:  # noqa: BLE001
            avviso("%s: non riesco a impostare %s (%s). Hai compilato?" % (nome, chiave, e))
    ore = ", ".join("%g%s" % (t.get_editor_property("ora"), " (a casa)" if t.get_editor_property("assente") else "") for t in buone)
    scrivi("%s: %d tappe (%s)" % (nome, len(buone), ore))
    return a


# (11/11, v0.1.8) Il sasso del sogno di Tobia: sulla collina dell'Orso (ValdorsoLuoghiDati.inl: la cima, in cm del
# mondo), qualche metro a est della cima. Un masso a forma d'orso che dorme (per ora una sfera schiacciata del motore,
# con la pietra del villaggio) e accanto l'iscrizione da leggere con E ("Il sasso dell'Orso": la cerca Tobia).
COLLINA = (-4080.0, -40800.0, 17877.0)
SASSO_SCARTO = (900.0, 300.0)
TITOLO_SASSO = "Il sasso dell'Orso"
RIGHE_SASSO = [
    "Un masso grande come un carro, con la forma di un orso che dorme: la testa sulle zampe, la schiena verso il vento.",
    "Sul fianco c'e' un segno inciso, consumato dalla pioggia: un cerchio, e dentro l'impronta di una zampa.",
    "Sotto, piu' piccolo e molto piu' vecchio, qualcuno ha scritto: \"Qui dorme chi veglia.\"",
]


def materiale_pietra():
    for dati in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant")):
        if str(dati.asset_name) == "MI_Villaggio_Pietra":
            return dati.get_asset()
    return None


def metti_sasso():
    x, y = COLLINA[0] + SASSO_SCARTO[0], COLLINA[1] + SASSO_SCARTO[1]
    z, _ = terra(x, y, COLLINA[2], [])
    if z is None:
        avviso("il sasso di Tobia: la linea dall'alto non trova la collina dell'Orso (e' caricata?): uso la quota dei dati")
        z = COLLINA[2]
    sfera = unreal.load_asset("/Engine/BasicShapes/Sphere")
    masso = attori.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z + 40.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=35.0))
    if masso and sfera:
        comp = masso.static_mesh_component
        comp.set_static_mesh(sfera)
        masso.set_actor_scale3d(unreal.Vector(3.4, 2.0, 1.5))
        mat = materiale_pietra()
        if mat:
            comp.set_material(0, mat)
        masso.set_actor_label("Il sasso dell'Orso (masso)")
        masso.set_folder_path(CARTELLA)
        masso.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    classe = getattr(unreal, "ValdorsoIscrizione", None)
    if classe is None:
        avviso("manca la classe ValdorsoIscrizione: niente iscrizione sul sasso")
        return
    a = attori.spawn_actor_from_class(classe, unreal.Vector(x, y - 190.0, z + 60.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0))
    if a is None:
        avviso("non riesco a mettere l'iscrizione del sasso")
        return
    a.set_editor_property("titolo", unreal.Text(TITOLO_SASSO))
    a.set_editor_property("avvisi", RIGHE_SASSO)
    a.set_actor_label(TITOLO_SASSO)
    a.set_folder_path(CARTELLA)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    scrivi("il sasso dell'Orso sulla collina, in (%.0f, %.0f, %.0f)" % (x, y, z))


def metti_pozzo(P):
    """(11/11, v0.2) Il pozzo: l'attore che si usa con E (AValdorsoPozzo), sopra il pozzo del villaggio."""
    classe = getattr(unreal, "ValdorsoPozzo", None)
    pozzo = P.attore("Pozzo")
    if classe is None or pozzo is None:
        if classe is None:
            avviso("manca la classe C++ ValdorsoPozzo (compila): niente acqua dal pozzo")
        return
    q = pozzo.get_actor_location()
    a = attori.spawn_actor_from_class(classe, unreal.Vector(q.x, q.y, q.z + 60.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
    if a is None:
        avviso("non riesco a mettere il pozzo")
        return
    a.set_actor_label("Il pozzo (acqua)")
    a.set_folder_path(CARTELLA)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    scrivi("il pozzo si usa con E, in (%.0f, %.0f, %.0f)" % (q.x, q.y, q.z))


def materiale(nome):
    for dati in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant")):
        if str(dati.asset_name) == nome:
            return dati.get_asset()
    return None


def metti_magazzino(P):
    """(11/11, v0.2.0) Il magazzino del villaggio (AValdorsoMagazzino, ValdorsoScorte.h): accanto alla bacheca della
    Gilda, con due casse e una catasta per farsi vedere."""
    classe = getattr(unreal, "ValdorsoMagazzino", None)
    if classe is None:
        avviso("manca la classe C++ ValdorsoMagazzino (compila): niente magazzino")
        return
    posto = P.intorno("Bacheca della Gilda", 320.0, 90.0)
    if posto is None:
        return
    x, y, z, g = posto
    a = attori.spawn_actor_from_class(classe, unreal.Vector(x, y, z + 60.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=g))
    if a is None:
        avviso("non riesco a mettere il magazzino")
        return
    a.set_actor_label("Il magazzino del villaggio")
    a.set_folder_path(CARTELLA)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    cubo = unreal.load_asset("/Engine/BasicShapes/Cube")
    legno = materiale("MI_Villaggio_Assi") or materiale("MI_Villaggio_Trave")
    for k, (dx, dy, sx, sy, sz) in enumerate(((0.0, 0.0, 0.9, 0.7, 0.7), (95.0, 10.0, 0.7, 0.7, 0.6), (-90.0, 0.0, 0.8, 1.4, 0.45))):
        c = attori.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x + dx, y + dy, z + sz * 50.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=g))
        if c and cubo:
            c.static_mesh_component.set_static_mesh(cubo)
            c.set_actor_scale3d(unreal.Vector(sx, sy, sz))
            if legno:
                c.static_mesh_component.set_material(0, legno)
            c.set_actor_label("Il magazzino del villaggio (cassa %d)" % (k + 1))
            c.set_folder_path(CARTELLA)
            c.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    scrivi("il magazzino del villaggio, in (%.0f, %.0f, %.0f)" % (x, y, z))


def metti_baule(P):
    """(12/11, v0.2.0) Il baule del colono (AValdorsoBaule, ValdorsoBaule.h): da Bruno, in un angolo della locanda
    lontano dalla porta, contro il muro. Uno solo, ma dentro ognuno trova le sue cose."""
    classe = getattr(unreal, "ValdorsoBaule", None)
    if classe is None:
        avviso("manca la classe C++ ValdorsoBaule (compila): niente baule")
        return
    locanda = P.attore("Locanda_di_Bruno")
    if locanda is None:
        return
    porta = P.per_nome.get("Locanda_di_Bruno porta")
    q = locanda.get_actor_location()
    g = locanda.get_actor_rotation().yaw
    # due angoli liberi a est (il bancone e' a nord-ovest, Teodoro a sud-ovest): quello piu' lontano dalla porta
    angoli = [(580.0, -380.0), (580.0, 380.0)]
    if porta is not None:
        pq = porta.get_actor_location()
        angoli.sort(key=lambda a: -math.hypot(al_mondo(q.x, q.y, g, a[0], a[1])[0] - pq.x, al_mondo(q.x, q.y, g, a[0], a[1])[1] - pq.y))
    lx, ly = angoli[0]
    posto = P.dentro("Locanda_di_Bruno", lx, ly, 0.0, ly)
    if posto is None:
        return
    x, y, z, gradi = posto
    a = attori.spawn_actor_from_class(classe, unreal.Vector(x, y, z + 40.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=gradi))
    if a is None:
        avviso("non riesco a mettere il baule")
        return
    a.set_actor_label("Il baule del colono")
    a.set_folder_path(CARTELLA)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    cubo = unreal.load_asset("/Engine/BasicShapes/Cube")
    legno = materiale("MI_Villaggio_Assi") or materiale("MI_Villaggio_Trave")
    ferro = materiale("MI_Villaggio_Ferro") or legno
    # la cassa, il coperchio un po' piu' largo e due bande di ferro (il baule guarda verso la stanza)
    pezzi = ((0.0, 0.0, 25.0, 0.55, 1.0, 0.5, legno), (0.0, 0.0, 53.0, 0.6, 1.05, 0.08, legno),
             (0.0, -30.0, 30.0, 0.57, 0.04, 0.5, ferro), (0.0, 30.0, 30.0, 0.57, 0.04, 0.5, ferro))
    for k, (dx, dy, dz, sx, sy, sz, mat) in enumerate(pezzi):
        px, py = al_mondo(x, y, gradi, dx, dy)
        c = attori.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(px, py, z + dz), unreal.Rotator(roll=0.0, pitch=0.0, yaw=gradi))
        if c and cubo:
            c.static_mesh_component.set_static_mesh(cubo)
            c.set_actor_scale3d(unreal.Vector(sx, sy, sz))
            if mat:
                c.static_mesh_component.set_material(0, mat)
            c.set_actor_label("Il baule del colono (pezzo %d)" % (k + 1))
            c.set_folder_path(CARTELLA)
            c.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    scrivi("il baule del colono, nella locanda in (%.0f, %.0f, %.0f)" % (x, y, z))


def metti_pietra(P):
    """(11/11) La pietra dei segni: davanti all'ingresso del tempio (-Y), a destra di chi entra, sul paesaggio."""
    classe = getattr(unreal, "ValdorsoPietraSegni", None)
    if classe is None:
        avviso("manca la classe C++ ValdorsoPietraSegni (compila): niente pietra dei segni")
        return
    posto = P.sotto_tettoia("Tempio_del_Cuore", 480.0, -1420.0, 0.0, -1420.0)
    if posto is None:
        return
    x, y, z, g = posto
    # il cilindro del motore ha l'origine nel mezzo: 1,8 m di pietra, meta' sopra e meta' sotto l'origine
    a = attori.spawn_actor_from_class(classe, unreal.Vector(x, y, z + 80.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=g))
    if a is None:
        avviso("non riesco a mettere la pietra dei segni")
        return
    a.set_actor_label("La pietra dei segni")
    a.set_folder_path(CARTELLA)
    a.set_editor_property("tags", [unreal.Name(ETICHETTA)])
    # il materiale della pietra del villaggio, se c'e'
    for dati in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant")):
        if str(dati.asset_name) == "MI_Villaggio_Pietra":
            mat = dati.get_asset()
            for c in a.get_components_by_class(unreal.StaticMeshComponent):
                c.set_material(0, mat)
            break
    scrivi("pietra dei segni davanti al tempio, in (%.0f, %.0f, %.0f)" % (x, y, z))


def principale():
    classe = getattr(unreal, "ValdorsoPNG", None)
    if classe is None or not hasattr(unreal, "ValdorsoTappa"):
        unreal.log_error("[Valdorso] Volti: manca la classe C++ ValdorsoPNG con la Giornata. Compila Valdorso e riapri l'editor.")
        return
    tutti = attori.get_all_level_actors()
    if not any(isinstance(a, unreal.Landscape) for a in tutti):
        unreal.log_error("[Valdorso] Volti: nel livello aperto non c'e' un Landscape. Apri L_Valle.")
        return
    tolti = 0
    for a in tutti:
        if a.actor_has_tag(ETICHETTA):
            attori.destroy_actor(a)
            tolti += 1
    if tolti:
        scrivi("tolti %d volti messi prima" % tolti)
    tutti = attori.get_all_level_actors()
    nostri = [a for a in tutti if a.get_class().get_name().startswith("Valdorso")]
    P = Posti(tutti, nostri)
    T = SCALA_TOBIA

    # La fucina (villaggio.py: W 1000, D 800; la tettoia davanti, verso +Y locale; l'incudine a x 150..230, y 600..640;
    # la forgia a x -150..50, y 550..700). Marta sta davanti all'incudine, girata verso di lei.
    incudine = P.sotto_tettoia("Fucina_di_Marta", 190.0, 700.0, 190.0, 620.0)
    davanti_fucina = P.sotto_tettoia("Fucina_di_Marta", 480.0, 820.0, 190.0, 620.0)
    # La locanda (W 1400, D 1000; il bancone a nord, y 320, x -580..-180; i tavoli a (0,-60) e (380,160)).
    bancone = P.dentro("Locanda_di_Bruno", -330.0, 175.0, -330.0, 400.0)
    tavolo_grande = P.dentro("Locanda_di_Bruno", 380.0, 20.0, 380.0, 160.0)
    angolo_teodoro = P.dentro("Locanda_di_Bruno", -560.0, -380.0, 0.0, 0.0)
    # Il tempio (W 1200, D 2200; l'ingresso a -Y, l'altare a +Y): Tobia spazza la navata vicino all'ingresso.
    navata = P.dentro("Tempio_del_Cuore", 120.0, -700.0, 120.0, 0.0)
    navata_sera = P.dentro("Tempio_del_Cuore", -200.0, -450.0, 0.0, 400.0)

    giornate = [
        ("Marta", "Marta, la fabbra", True, 1.0, 0, [
            tappa(7.0, incudine, "Martellare"),
            tappa(12.0, P.intorno("Pozzo", 230.0, 200.0)),
            tappa(13.0, incudine, "Martellare"),
            tappa(19.0, tavolo_grande),
            tappa(22.5, None, assente=True),
        ]),
        ("Livia", "La capitana Livia Serra", True, 1.0, 0, [
            tappa(6.5, P.passo(4.0, 0.0)),
            tappa(12.0, P.fuori_porta("Casermetta_della_Guardia porta", "Casermetta_della_Guardia", 260.0, 120.0)),
            tappa(15.0, P.intorno("Luce del fuoco della Guardia", 170.0, 90.0)),
            tappa(19.5, bancone),
            tappa(22.0, P.passo(3.0, 1.5)),
        ]),
        ("Tobia", "Tobia, l'orfano", False, T, 0, [
            tappa(7.5, navata, "Spazzare", scala=T),
            tappa(10.0, P.fuori_porta("Mulino porta", "Mulino", 320.0, -200.0), scala=T),
            tappa(13.5, davanti_fucina, scala=T),
            tappa(16.0, P.intorno("Pozzo", 260.0, 20.0), "Esultare", scala=T),
            tappa(19.0, None, assente=True),
        ]),
        ("Teodoro", "Teodoro, il mugnaio", False, 1.0, 0, [
            tappa(6.0, P.fuori_porta("Mulino porta", "Mulino", 260.0, 150.0)),
            tappa(18.0, angolo_teodoro),
            tappa(23.0, None, assente=True),
        ]),
        ("Agnese", "Agnese, la mugnaia", True, 1.0, 0, [
            tappa(6.0, P.fuori_porta("Mulino porta", "Mulino", 200.0, -120.0), "Spazzare"),
            tappa(10.0, P.intorno("Pozzo", 230.0, 110.0)),
            tappa(12.5, P.fuori_porta("Mulino porta", "Mulino", 200.0, -120.0)),
            tappa(21.0, None, assente=True),
        ]),
        ("Elric", "Maestro Elric, mago di corte", False, 1.0, 1, [
            tappa(8.0, P.intorno("Rovine degli Antichi", 900.0, 0.0), "Indicare"),
            tappa(18.0, navata_sera),
            tappa(21.5, None, assente=True),
        ]),
    ]

    metti_pietra(P)
    metti_sasso()
    metti_pozzo(P)
    metti_magazzino(P)
    metti_baule(P)

    messi = 0
    for dialogo, nome, donna, scala, atto, giornata in giornate:
        if metti(classe, dialogo, nome, donna, giornata, scala, atto):
            messi += 1
    # (12/11, v0.2.0) Sibilla e la carovana: al piazzale delle carovane un giorno ogni tre (veri), dalle 9 alle 20
    piazzale = P.intorno("Piazzale delle carovane", 400.0, 180.0)
    if metti(classe, "Sibilla", "Sibilla, la venditrice ambulante", True,
             [tappa(9.0, piazzale), tappa(20.0, None, assente=True)], 1.0, 0, 3):
        messi += 1
    scrivi("messi %d volti nella cartella %s" % (messi, CARTELLA))
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        scrivi("livello salvato")
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio non riuscito (%s): Ctrl+Maiusc+S" % e)
    scrivi("FATTO. In Play: Marta all'incudine, Agnese e Teodoro al mulino, Tobia al tempio, Livia al passo; Elric arriva "
           "con l'Atto I (prova: menu Staff -> atto, o console Valdorso.Atto.Battito). Si spostano con le ore del cielo.")


try:
    principale()
finally:
    import os
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "volti.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))
