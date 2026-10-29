# Valdorso - Crea i "legami" (groom binding) tra i peli della libreria MetaHuman (capelli, barbe, baffi, sopracciglia)
# e le teste dei coloni, così il gioco può cambiare acconciatura e barba. Scritto da Claude il 29/10/2026 (terza versione).
#
# I peli della libreria sono fatti su una testa modello: il legame li "trasferisce" su ogni volto. Trovate per prova
# (29/10): barbe, baffi, sopracciglia e capelli da uomo vanno con SKM_Face; i capelli da donna con Legacy01, tranne lo
# "Scalati" (Hair_M_Layered) che va con Legacy02. MH_Groom_Head non va bene per nessuno (peli sopra la testa).
# Un legame appena creato non è "costruito": le versioni di prima lo salvavano vuoto (2 KB) e i capelli restavano
# sospesi sopra la testa. Ora lo fa costruire subito il C++ (UValdorsoStrumenti::CostruisciSubito), poi lo salva
# (circa 7 KB). I legami vanno in /Game/Peli/<Testa>/<Pelo> (solo sul PC). Quelli già pronti si saltano: lo script
# si può rilanciare. Ne fa al massimo MASSIMO_PER_VOLTA per lancio (la memoria). Alla fine, quando sono tutti pronti,
# cancella la vecchia cartella /Game/Legami.
# Le liste qui sotto sono le stesse del C++ (ValdorsoAspettoComponent.cpp): se cambiano, vanno cambiate insieme.

import os
import unreal

CARTELLA = "/Game/Peli"
VECCHIA = "/Game/Legami"
PRONTO = 4000          # byte: un legame costruito pesa circa 7 KB, uno vuoto circa 2 KB
MASSIMO_PER_VOLTA = 120
MODELLO = "/MetaHumanCharacter/Face/SKM_Face"
MODELLO_DONNA = "/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy01"
MODELLO_DONNA_SCALATI = "/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy02"


def modello_per(testa, nome):
    """La testa modello da cui trasferire un pelo su una testa (vedi sopra)."""
    if testa in TESTE_DONNA and nome.startswith("Hair_"):
        return MODELLO_DONNA_SCALATI if nome == "Hair_M_Layered" else MODELLO_DONNA
    return MODELLO
LIBRERIA = "/MetaHumanCharacter/Optional/Grooms/GroomAssets"

TESTE_UOMO = ["Prova", "Walter", "Lorenzo", "Dominic", "Mikel", "Omari"]
TESTE_DONNA = ["Jelena", "Grace", "Etta", "Celeste", "Sunita"]

CAPELLI_UOMO = ["Hair_S_BuzzCut", "Hair_S_BrushCut", "Hair_S_Casual", "Hair_S_Messy", "Hair_S_SlickBack",
                "Hair_S_PulledBack", "Hair_S_SideSweptFringe", "Hair_M_Layered", "Hair_L_MessyClumps",
                "Hair_S_RecedeMessy", "Hair_S_BaldingStubble"]
CAPELLI_DONNA = ["Hair_L_Straight", "Hair_L_StraightBangs", "Hair_S_LowPonytail", "Hair_S_UpdoBraids", "Hair_S_Updo",
                 "Hair_S_UpdoBuns", "Hair_M_TwistedBraids", "Hair_M_BobMessy", "Hair_S_BobLayered",
                 "Hair_L_MessyClumps", "Hair_M_Layered"]
BARBE = ["Beard_S_Stubble", "Beard_S_Full", "Beard_L_Full", "Beard_L_Messy", "Goatee_M_Pointed", "Beard_M_MuttonChops"]
BAFFI = ["Mustache_S_Stubble", "Mustache_S_Full", "Mustache_L_Full", "Mustache_L_Messy", "Mustache_L_Handlebar",
         "Mustache_M_Gunslinger"]
SOPRACCIGLIA_UOMO = ["Eyebrows_M_Natural", "Eyebrows_M_Thick", "Eyebrows_M_Thin", "Eyebrows_M_Unruly", "Eyebrows_M_Full"]
SOPRACCIGLIA_DONNA = ["Eyebrows_M_Natural", "Eyebrows_M_Thin", "Eyebrows_M_SlightArch", "Eyebrows_M_Fine", "Eyebrows_M_Swept"]

CARTELLE = {"Hair": "Hair", "Beard": "Beards", "Goatee": "Beards", "Mustache": "Mustaches", "Eyebrows": "Eyebrows"}


def scrivi(testo):
    unreal.log("[Valdorso] Legami: " + testo)


def avvisa(testo):
    unreal.log_warning("[Valdorso] Legami: " + testo)


def pelo(nome):
    return "%s/%s/%s/%s" % (LIBRERIA, CARTELLE[nome.split("_")[0]], nome, nome)


def volto(testa):
    for p in ("/Game/Unpacked/MH_%s/Face/SKM_MH_%s_FaceMesh" % (testa, testa),
              "/Game/MetaHumans/MH_%s/Face/SKM_MH_%s_FaceMesh" % (testa, testa)):
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return p
    return None


def file_di(destinazione):
    contenuti = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
    return os.path.join(contenuti, destinazione[len("/Game/"):] + ".uasset")


def dimensione(destinazione):
    percorso = file_di(destinazione)
    return os.path.getsize(percorso) if os.path.exists(percorso) else 0



def principale():
    strumenti = getattr(unreal, "ValdorsoStrumenti", None)
    if strumenti is None:
        unreal.log_error("[Valdorso] Legami: manca ValdorsoStrumenti nel C++ (compila Valdorso prima)")
        return
    modello = unreal.EditorAssetLibrary.load_asset(MODELLO)
    if modello is None:
        unreal.log_error("[Valdorso] Legami: non trovo la testa modello " + MODELLO)
        return
    da_fare = []
    for teste, peli in ((TESTE_UOMO, CAPELLI_UOMO + BARBE + BAFFI + SOPRACCIGLIA_UOMO),
                        (TESTE_DONNA, CAPELLI_DONNA + SOPRACCIGLIA_DONNA)):
        for testa in teste:
            for nome in peli:
                destinazione = "%s/%s/%s" % (CARTELLA, testa, nome)
                if dimensione(destinazione) < PRONTO:
                    da_fare.append((testa, nome, destinazione))
    totale = 248
    scrivi("mancano %d legami su %d; in questo giro ne faccio al massimo %d" % (len(da_fare), totale, MASSIMO_PER_VOLTA))

    fatti, vuoti, errori = 0, 0, 0
    for testa, nome, destinazione in da_fare[:MASSIMO_PER_VOLTA]:
        faccia_percorso = volto(testa)
        groom = unreal.EditorAssetLibrary.load_asset(pelo(nome))
        faccia = unreal.EditorAssetLibrary.load_asset(faccia_percorso) if faccia_percorso else None
        if groom is None or faccia is None:
            unreal.log_error("[Valdorso] Legami: manca %s" % (pelo(nome) if groom is None else "il volto di " + testa))
            errori += 1
            continue
        legame = None
        if unreal.EditorAssetLibrary.does_asset_exist(destinazione):
            legame = unreal.EditorAssetLibrary.load_asset(destinazione)
        if legame is None:
            try:
                legame = unreal.GroomLibrary.create_new_groom_binding_asset_with_path(
                    destinazione, groom, faccia, 100, unreal.EditorAssetLibrary.load_asset(modello_per(testa, nome)), 0)
            except Exception as problema:
                unreal.log_error("[Valdorso] Legami: %s su %s: %s" % (nome, testa, problema))
        if legame is None:
            errori += 1
            continue
        strumenti.costruisci_subito(legame)
        unreal.EditorAssetLibrary.save_loaded_asset(legame, False)
        peso = dimensione(destinazione)
        if peso >= PRONTO:
            fatti += 1
            scrivi("%s su %s pronto (%d byte)" % (nome, testa, peso))
        else:
            vuoti += 1
            avvisa("%s su %s ancora vuoto (%d byte)" % (nome, testa, peso))
        if (fatti + vuoti) % 10 == 0:
            unreal.SystemLibrary.collect_garbage()

    mancano = len(da_fare) - fatti
    cancellata = ""
    if mancano == 0 and unreal.EditorAssetLibrary.does_directory_exist(VECCHIA):
        if unreal.EditorAssetLibrary.delete_directory(VECCHIA):
            cancellata = " Cancellata la vecchia cartella %s." % VECCHIA
    scrivi("FINITO. Pronti %d, vuoti %d, errori %d, ne mancano %d%s%s" % (
        fatti, vuoti, errori, mancano, " (chiudi e riapri l'editor e rilancia lo script)" if mancano else "", cancellata))


principale()
