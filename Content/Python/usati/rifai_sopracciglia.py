# Valdorso - Rifa' i legami delle sopracciglia di tutte le teste partendo da un'altra testa modello.
# Scritto da Claude il 02/11/2026.
#
# Perche': con la testa modello SKM_Face alcune sopracciglia finiscono fuori posto (le "Arcuate" ai lati della bocca,
# "Naturali" e "Folte" sparite). I capelli da donna stanno perfetti partendo da SKM_Groom_Head_Legacy01: i peli della
# libreria sono fatti su quella testa, quindi si prova la stessa cosa per le sopracciglia.
# Se il risultato non va, cambia MODELLO qui sotto ("Legacy02" o "Face", cioe' come prima) e rilancia.
# Cancella e rifa' solo /Game/Peli/<Testa>/Eyebrows_*; i capelli, le barbe e i baffi non si toccano.

import unreal

MODELLO = "Legacy01"   # "Legacy01", "Legacy02" oppure "Face" (come prima)

MODELLI = {
    "Legacy01": "/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy01",
    "Legacy02": "/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy02",
    "Face": "/MetaHumanCharacter/Face/SKM_Face",
}
LIBRERIA = "/MetaHumanCharacter/Optional/Grooms/GroomAssets/Eyebrows"
CARTELLA = "/Game/Peli"

# Le stesse liste di crea_legami_peli.py e del C++ (ValdorsoAspettoComponent.cpp).
TESTE_UOMO = ["Prova", "Walter", "Lorenzo", "Dominic", "Mikel", "Omari"]
TESTE_DONNA = ["Jelena", "Grace", "Etta", "Celeste", "Sunita"]
SOPRACCIGLIA_UOMO = ["Eyebrows_M_Natural", "Eyebrows_M_Thick", "Eyebrows_M_Thin", "Eyebrows_M_Unruly", "Eyebrows_M_Full"]
SOPRACCIGLIA_DONNA = ["Eyebrows_M_Natural", "Eyebrows_M_Thin", "Eyebrows_M_SlightArch", "Eyebrows_M_Fine", "Eyebrows_M_Swept"]


def volto(testa):
    for p in ("/Game/Unpacked/MH_%s/Face/SKM_MH_%s_FaceMesh" % (testa, testa),
              "/Game/MetaHumans/MH_%s/Face/SKM_MH_%s_FaceMesh" % (testa, testa)):
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return p
    return None


def principale():
    strumenti = getattr(unreal, "ValdorsoStrumenti", None)
    modello = unreal.EditorAssetLibrary.load_asset(MODELLI[MODELLO])
    if strumenti is None or modello is None:
        unreal.log_error("[Valdorso] Sopracciglia: manca ValdorsoStrumenti o la testa modello " + MODELLI[MODELLO])
        return
    fatti, errori = 0, 0
    for teste, peli in ((TESTE_UOMO, SOPRACCIGLIA_UOMO), (TESTE_DONNA, SOPRACCIGLIA_DONNA)):
        for testa in teste:
            faccia_percorso = volto(testa)
            faccia = unreal.EditorAssetLibrary.load_asset(faccia_percorso) if faccia_percorso else None
            for nome in peli:
                groom = unreal.EditorAssetLibrary.load_asset("%s/%s/%s" % (LIBRERIA, nome, nome))
                destinazione = "%s/%s/%s" % (CARTELLA, testa, nome)
                if groom is None or faccia is None:
                    unreal.log_error("[Valdorso] Sopracciglia: manca %s" % (nome if groom is None else "il volto di " + testa))
                    errori += 1
                    continue
                if unreal.EditorAssetLibrary.does_asset_exist(destinazione):
                    unreal.EditorAssetLibrary.delete_asset(destinazione)
                legame = unreal.GroomLibrary.create_new_groom_binding_asset_with_path(
                    destinazione, groom, faccia, 100, modello, 0)
                if legame is None:
                    errori += 1
                    continue
                strumenti.costruisci_subito(legame)
                unreal.EditorAssetLibrary.save_loaded_asset(legame, False)
                fatti += 1
    unreal.SystemLibrary.collect_garbage()
    unreal.log("[Valdorso] Sopracciglia: FINITO con la testa modello %s. Rifatti %d legami, errori %d." % (MODELLO, fatti, errori))


principale()
