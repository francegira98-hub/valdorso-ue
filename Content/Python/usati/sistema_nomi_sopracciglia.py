# Valdorso - Mette a posto i nomi dei legami delle sopracciglia rifatti da rifai_sopracciglia.py. Scritto da Claude
# il 02/11/2026.
#
# Per 7 teste Unreal teneva ancora in memoria il legame vecchio e ha salvato quello nuovo con un "1" in fondo
# (Eyebrows_M_Natural1). Il gioco cerca il nome senza "1". Questo script:
#   - se ci sono tutti e due (vecchio e nuovo "1"): cancella il vecchio;
#   - se c'e' solo il nuovo "1": lo rinomina col nome giusto.
# Va lanciato DUE volte, chiudendo e riaprendo l'editor in mezzo (la prima volta cancella, la seconda rinomina).

import unreal

CARTELLA = "/Game/Peli"
TESTE = ["Prova", "Walter", "Lorenzo", "Dominic", "Mikel", "Omari", "Jelena", "Grace", "Etta", "Celeste", "Sunita"]
NOMI = ["Eyebrows_M_Natural", "Eyebrows_M_Thick", "Eyebrows_M_Thin", "Eyebrows_M_Unruly", "Eyebrows_M_Full",
        "Eyebrows_M_SlightArch", "Eyebrows_M_Fine", "Eyebrows_M_Swept"]


def principale():
    cancellati, rinominati, problemi = 0, 0, 0
    for testa in TESTE:
        for nome in NOMI:
            giusto = "%s/%s/%s" % (CARTELLA, testa, nome)
            nuovo = giusto + "1"
            if not unreal.EditorAssetLibrary.does_asset_exist(nuovo):
                continue
            if unreal.EditorAssetLibrary.does_asset_exist(giusto):
                if unreal.EditorAssetLibrary.delete_asset(giusto):
                    cancellati += 1
                else:
                    problemi += 1
            elif unreal.EditorAssetLibrary.rename_asset(nuovo, giusto):
                rinominati += 1
            else:
                problemi += 1
    resta = 0
    for testa in TESTE:
        for nome in NOMI:
            if unreal.EditorAssetLibrary.does_asset_exist("%s/%s/%s1" % (CARTELLA, testa, nome)):
                resta += 1
    unreal.log("[Valdorso] Nomi sopracciglia: cancellati %d vecchi, rinominati %d, problemi %d. Ne restano col \"1\": %d%s" % (
        cancellati, rinominati, problemi, resta,
        " (chiudi e riapri l'editor e rilancia lo script)" if resta else " (tutto a posto)"))


principale()
