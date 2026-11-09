# Valdorso - I pericoli: lupi, armi e suoni dei colpi (v0.1.4, 09/11/2026). Scritto da Claude.
# Si lancia dal menu Valdorso -> Pericoli -> "Pericoli: importa lupi, armi e suoni" (o Strumenti -> Esegui script).
# Si puo' rilanciare (rimpiazza quello che c'e').
#
# Legge V:/Texture/valdorso/pericoli (o pericoli.zip accanto), fatta da Tools/Generatori/pericoli.py:
#   SM_Lupo_*.glb   -> /Game/Valle/Pericoli/Lupo   (corpo, testa, coda, zampa: li monta AValdorsoLupo, C++)
#   SM_Arma_*.glb   -> /Game/Valle/Pericoli/Armi   (coltello, pugnale, falcetto, martello, bastone, spada, scudo)
#   S_*.wav         -> /Game/Valle/Pericoli/Suoni  (i colpi, il lupo, le bolle e il respiro sott'acqua)
# I materiali sono quelli del villaggio (MI_Villaggio_Canna: il colore dei vertici; MI_Villaggio_Assi: il legno):
# serve aver fatto il villaggio almeno una volta. Senza collisione (i lupi hanno la loro capsula, le armi non toccano).
# I lupi non si mettono nel livello: escono da soli dalle tane (AValdorsoTaneLupi, C++). Resoconto in
# Saved/Claude/pericoli.txt.

import os
import zipfile

import unreal

CARTELLA = "V:/Texture/valdorso/pericoli"
DEST = "/Game/Valle/Pericoli"
MATERIALI = "/Game/Valle/Villaggio/Materiali"

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()
righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Pericoli: " + testo)


def avviso(testo):
    righe.append("ATTENZIONE: " + testo)
    unreal.log_warning("[Valdorso] Pericoli: " + testo)


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "pericoli.txt"), "w", encoding="utf-8") as uscita:
        uscita.write("\n".join(righe))


def scompatta():
    zf = CARTELLA + ".zip"
    segno = CARTELLA + "/SM_Lupo_Corpo.glb"
    if os.path.exists(zf) and (not os.path.exists(segno) or os.path.getmtime(zf) > os.path.getmtime(segno)):
        os.makedirs(CARTELLA, exist_ok=True)
        with zipfile.ZipFile(zf) as z:
            z.extractall(CARTELLA)
        if os.path.exists(segno):
            os.utime(segno, None)
        scrivi("scompattato %s" % zf)


def compito(file, destinazione):
    c = unreal.AssetImportTask()
    c.set_editor_property("filename", file)
    c.set_editor_property("destination_path", destinazione)
    c.set_editor_property("automated", True)
    c.set_editor_property("replace_existing", True)
    c.set_editor_property("save", True)
    return c


def cerca(cartella, nome, classe):
    a = unreal.load_asset("%s/%s.%s" % (cartella, nome, nome))
    if isinstance(a, classe):
        return a
    for percorso in libreria.list_assets(cartella, recursive=True, include_folder=False):
        if percorso.rsplit("/", 1)[-1].split(".")[0] == nome:
            a = unreal.load_asset(percorso)
            if isinstance(a, classe):
                return a
    return None


def materiali():
    mat = {}
    for n in ("Canna", "Assi"):
        m = unreal.load_asset("%s/MI_Villaggio_%s" % (MATERIALI, n))
        if m is None:
            avviso("manca %s/MI_Villaggio_%s: prima fai il villaggio (Valdorso -> Villaggio: rifai tutto)" % (MATERIALI, n))
        else:
            mat[n] = m
    return mat


def veste(mesh, mat):
    """I materiali negli slot (il nome dello slot e' MV_<Materiale>), Nanite spento (pezzi piccoli che si muovono: meglio
    i triangoli normali). La collisione la spegne il C++ (i pezzi non toccano niente)."""
    for i, sm in enumerate(mesh.get_editor_property("static_materials")):
        nome = str(sm.get_editor_property("material_slot_name"))
        mi = sm.get_editor_property("material_interface")
        if (not nome or nome == "None") and mi is not None:
            nome = mi.get_name()
        chiave = nome.split("MV_", 1)[-1].split("_")[0] if "MV_" in nome else None
        if chiave in mat:
            mesh.set_material(i, mat[chiave])
        else:
            avviso("%s: slot %s senza materiale" % (mesh.get_name(), nome))
    try:
        ns = mesh.get_editor_property("nanite_settings")
        if ns.get_editor_property("enabled"):
            ns.set_editor_property("enabled", False)
            mesh.set_editor_property("nanite_settings", ns)
    except Exception as e:  # noqa: BLE001
        avviso("%s: Nanite non controllato (%s)" % (mesh.get_name(), e))
    try:
        bs = mesh.get_editor_property("body_setup")
        if bs is not None:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
            mesh.set_editor_property("body_setup", bs)
    except Exception as e:  # noqa: BLE001
        avviso("%s: collisione non impostata (%s)" % (mesh.get_name(), e))
    libreria.save_loaded_asset(mesh)


def importa_modelli(prefisso, cartella_dest, mat):
    nomi = sorted(f[:-4] for f in os.listdir(CARTELLA) if f.startswith(prefisso) and f.endswith(".glb"))
    if not nomi:
        avviso("nessun %s*.glb in %s" % (prefisso, CARTELLA))
        return 0
    strumenti.import_asset_tasks([compito("%s/%s.glb" % (CARTELLA, n), cartella_dest) for n in nomi])
    fatti = 0
    for n in nomi:
        m = cerca(cartella_dest, n, unreal.StaticMesh)
        if m is None:
            avviso("non importato: %s" % n)
            continue
        veste(m, mat)
        b = m.get_bounding_box()
        scrivi("%s: da (%.0f, %.0f, %.0f) a (%.0f, %.0f, %.0f)" % (n, b.min.x, b.min.y, b.min.z, b.max.x, b.max.y, b.max.z))
        fatti += 1
    return fatti


def importa_suoni():
    nomi = sorted(f[:-4] for f in os.listdir(CARTELLA) if f.startswith("S_") and f.endswith(".wav"))
    cartella = DEST + "/Suoni"
    strumenti.import_asset_tasks([compito("%s/%s.wav" % (CARTELLA, n), cartella) for n in nomi])
    fatti = 0
    for n in nomi:
        s = cerca(cartella, n, unreal.SoundWave)
        if s is None:
            avviso("suono non importato: %s" % n)
            continue
        # le bolle e il respiro sott'acqua girano; il resto si suona una volta
        s.set_editor_property("looping", n in ("S_Bolle", "S_Respiro"))
        libreria.save_loaded_asset(s)
        fatti += 1
    return fatti


def esegui():
    scompatta()
    if not os.path.isdir(CARTELLA):
        avviso("manca la cartella %s (e %s.zip)" % (CARTELLA, CARTELLA))
        salva_resoconto()
        return
    mat = materiali()
    lupo = importa_modelli("SM_Lupo_", DEST + "/Lupo", mat)
    armi = importa_modelli("SM_Arma_", DEST + "/Armi", mat)
    suoni = importa_suoni()
    scrivi("fatto: %d pezzi del lupo, %d armi, %d suoni" % (lupo, armi, suoni))
    # controllo: il lupo guarda +X (la testa ha il muso a +X)
    testa = cerca(DEST + "/Lupo", "SM_Lupo_Testa", unreal.StaticMesh)
    if testa is not None:
        b = testa.get_bounding_box()
        if b.max.x < 40:
            avviso("la testa del lupo non guarda +X (max x %.0f): mandare a Claude il resoconto" % b.max.x)
    salva_resoconto()
    unreal.EditorDialog.show_message("Valdorso - Pericoli", "\n".join(righe[-12:]), unreal.AppMsgType.OK)


esegui()
