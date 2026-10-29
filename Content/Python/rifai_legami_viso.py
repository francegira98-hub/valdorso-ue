# Valdorso - Rifà i legami dei peli (barbe, pizzetti, baffi, sopracciglia, capelli) con una testa modello scelta.
# (29/10) TESTE: se non è vuota, rifà solo quelle teste (per le prove sulle donne).
# Scritto da Claude il 29/10/2026.
#
# I capelli della libreria MetaHuman sono fatti sulla testa MH_Groom_Head, ma barbe, baffi e sopracciglia no: con quella
# testa finivano sopra il capo. Qui i loro legami (già esistenti in /Game/Peli) prendono come testa di partenza il volto
# modello del Creator (SKM_Face) e si ricostruiscono. Se il risultato non va, prova la seconda testa modello cambiando
# MODELLO_VISO qui sotto. Serve il C++ di Valdorso compilato (ValdorsoStrumenti).

import os
import unreal

MODELLO_VISO = "/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy02"
TESTE = ["Jelena", "Grace", "Etta", "Celeste", "Sunita"]
CARTELLA = "/Game/Peli"
VISO = ("Hair_M_Layered",)  # (29/10) prova: lo "Scalati" delle donne con la seconda testa modello


def scrivi(testo):
    unreal.log("[Valdorso] Legami viso: " + testo)


def principale():
    strumenti = getattr(unreal, "ValdorsoStrumenti", None)
    modello = unreal.EditorAssetLibrary.load_asset(MODELLO_VISO)
    if strumenti is None or modello is None:
        unreal.log_error("[Valdorso] Legami viso: manca %s" % ("ValdorsoStrumenti (compila)" if strumenti is None else MODELLO_VISO))
        return
    percorsi = [p.split(".")[0] for p in unreal.EditorAssetLibrary.list_assets(CARTELLA, recursive=True)]
    fatti, errori = 0, 0
    for percorso in sorted(set(percorsi)):
        nome = percorso.rsplit("/", 1)[-1]
        testa = percorso.rsplit("/", 2)[-2]
        if TESTE and testa not in TESTE:
            continue
        if not nome.startswith(VISO):
            continue
        legame = unreal.EditorAssetLibrary.load_asset(percorso)
        if legame is None:
            errori += 1
            continue
        try:
            legame.set_editor_property("source_skeletal_mesh", modello)
        except Exception as problema:
            unreal.log_error("[Valdorso] Legami viso: %s: %s" % (percorso, problema))
            errori += 1
            continue
        strumenti.costruisci_subito(legame)
        unreal.EditorAssetLibrary.save_loaded_asset(legame, False)
        fatti += 1
        if fatti % 20 == 0:
            scrivi("%d rifatti..." % fatti)
            unreal.SystemLibrary.collect_garbage()
    scrivi("FINITO. Rifatti %d, errori %d (testa modello %s)" % (fatti, errori, MODELLO_VISO))


principale()
