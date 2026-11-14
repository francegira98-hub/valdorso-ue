# Valdorso - Rifa' i legami dei CAPELLI delle teste da uomo partendo dalla testa modello Legacy01 (come i capelli da donna
# e le sopracciglia, che stanno al posto giusto). Scritto da Claude il 14/11/2026.
#
# Perche': nella prova del 14/11 i capelli di Padre Anselmo (testa Walter, Hair_L_MessyClumps) stanno piu' bassi della
# testa, come da sempre i capelli dei coloni. La diagnosi (Valdorso.Capelli.Diagnosi nel registro) dice che il pelo e'
# attaccato bene al volto: il problema e' il legame. I capelli da uomo erano fatti da SKM_Face; quelli da donna e le
# sopracciglia, fatti da Legacy01, stanno perfetti (02/11).
#
# (14/11, secondo giro) I legami nuovi vanno in /Game/Peli/Nuovi/<Testa>/ e il gioco li preferisce a quelli di sempre
# (che restano dove sono): il primo giro provava a sovrascriverli, ma l'editor non li cancella mentre sono caricati e
# creava i nuovi con un "1" in fondo, che il gioco non usava. "Torna come prima" toglie solo quelli nuovi.
# Si lancia dal menu Valdorso:
#   "Capelli da uomo: prova su Padre Anselmo"   solo la testa Walter (pochi secondi), per vedere se va
#   "Capelli da uomo: rifai tutti"              le 6 teste da uomo (qualche minuto)
# Barbe, baffi e sopracciglia non si toccano. Il risultato va anche in Saved/Claude/capelli_uomo.txt.

import os
import unreal

try:
    SOLO_TESTE  # noqa: F821 (lo passa il menu)
except NameError:
    SOLO_TESTE = ["Walter"]
try:
    TORNA_INDIETRO  # noqa: F821
except NameError:
    TORNA_INDIETRO = False

MODELLI = {
    "Legacy01": "/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy01",
    "Legacy02": "/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy02",
}
LIBRERIA = "/MetaHumanCharacter/Optional/Grooms/GroomAssets/Hair"
CARTELLA = "/Game/Peli"
PRIMA = "/Game/Peli/_Prima"      # (solo il primo giro del 14/11: si toglie)
NUOVI = "/Game/Peli/Nuovi"

# Le stesse liste di crea_legami_peli.py e del C++ (ValdorsoAspettoComponent.cpp).
TESTE_UOMO = ["Prova", "Walter", "Lorenzo", "Dominic", "Mikel", "Omari"]
CAPELLI_UOMO = ["Hair_S_BuzzCut", "Hair_S_BrushCut", "Hair_S_Casual", "Hair_S_Messy", "Hair_S_SlickBack",
                "Hair_S_PulledBack", "Hair_S_SideSweptFringe", "Hair_M_Layered", "Hair_L_MessyClumps",
                "Hair_S_RecedeMessy", "Hair_S_BaldingStubble"]

RIGHE = []


def scrivi(testo):
    RIGHE.append(testo)
    unreal.log("[Valdorso] Capelli da uomo: " + testo)


def volto(testa):
    for p in ("/Game/Unpacked/MH_%s/Face/SKM_MH_%s_FaceMesh" % (testa, testa),
              "/Game/MetaHumans/MH_%s/Face/SKM_MH_%s_FaceMesh" % (testa, testa)):
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return p
    return None


def modello_per(nome):
    # come per le donne: lo "Scalati" va con Legacy02, gli altri con Legacy01
    return MODELLI["Legacy02"] if nome == "Hair_M_Layered" else MODELLI["Legacy01"]


def pulisci(teste):
    """Toglie quello che ha lasciato il primo giro (14/11): i legami con il "1" in fondo e le copie in _Prima."""
    tolti = 0
    for testa in teste:
        for nome in CAPELLI_UOMO:
            uno = "%s/%s/%s1" % (CARTELLA, testa, nome)
            if not unreal.EditorAssetLibrary.does_asset_exist(uno):
                continue        # (se non c'e' il "1", quello di sempre e' stato sostituito: la copia in _Prima resta)
            for resto in (uno, "%s/%s/%s" % (PRIMA, testa, nome)):
                if unreal.EditorAssetLibrary.does_asset_exist(resto) and unreal.EditorAssetLibrary.delete_asset(resto):
                    tolti += 1
    if tolti:
        scrivi("tolti %d avanzi del primo giro" % tolti)


def torna_indietro(teste):
    tolti = 0
    for testa in teste:
        for nome in CAPELLI_UOMO:
            nuovo = "%s/%s/%s" % (NUOVI, testa, nome)
            if unreal.EditorAssetLibrary.does_asset_exist(nuovo) and unreal.EditorAssetLibrary.delete_asset(nuovo):
                tolti += 1
    scrivi("TORNATO COME PRIMA: tolti %d legami nuovi (il gioco usa di nuovo quelli di sempre)." % tolti)


def principale():
    teste = [t for t in SOLO_TESTE if t in TESTE_UOMO] or TESTE_UOMO
    pulisci(teste)
    if TORNA_INDIETRO:
        torna_indietro(teste)
        return
    strumenti = getattr(unreal, "ValdorsoStrumenti", None)
    if strumenti is None:
        scrivi("ERRORE: manca ValdorsoStrumenti (il C++ non e' caricato?)")
        return
    fatti, errori = 0, 0
    for testa in teste:
        faccia_percorso = volto(testa)
        faccia = unreal.EditorAssetLibrary.load_asset(faccia_percorso) if faccia_percorso else None
        if faccia is None:
            scrivi("manca il volto di %s" % testa)
            errori += 1
            continue
        for nome in CAPELLI_UOMO:
            groom = unreal.EditorAssetLibrary.load_asset("%s/%s/%s" % (LIBRERIA, nome, nome))
            modello = unreal.EditorAssetLibrary.load_asset(modello_per(nome))
            destinazione = "%s/%s/%s" % (NUOVI, testa, nome)
            if groom is None or modello is None:
                scrivi("manca %s" % (nome if groom is None else modello_per(nome)))
                errori += 1
                continue
            # i legami di sempre (/Game/Peli/<Testa>) non si toccano: i nuovi vanno in /Game/Peli/Nuovi e il gioco li
            # preferisce (ValdorsoAspettoComponent.cpp). Uno nuovo gia' fatto si rifa'.
            if unreal.EditorAssetLibrary.does_asset_exist(destinazione) and not unreal.EditorAssetLibrary.delete_asset(destinazione):
                scrivi("non riesco a rifare %s di %s (e' aperto?)" % (nome, testa))
                errori += 1
                continue
            legame = unreal.GroomLibrary.create_new_groom_binding_asset_with_path(destinazione, groom, faccia, 100, modello, 0)
            if legame is None:
                scrivi("non riesco a creare %s per %s" % (nome, testa))
                errori += 1
                continue
            percorso = legame.get_path_name().split(".")[0]
            if percorso != destinazione:
                # l'editor l'ha chiamato con un altro nome (succede se quello vecchio e' ancora caricato)
                scrivi("ATTENZIONE: %s e' finito in %s invece di %s: chiudi e riapri l'editor e rilancia" % (nome, percorso, destinazione))
                errori += 1
            strumenti.costruisci_subito(legame)
            unreal.EditorAssetLibrary.save_loaded_asset(legame, False)
            fatti += 1
        scrivi("%s: fatto" % testa)
        unreal.SystemLibrary.collect_garbage()
    scrivi("FINITO: fatti %d legami nuovi in %s (teste: %s), errori %d." % (fatti, NUOVI, ", ".join(teste), errori))
    scrivi("Ora: Play, e guarda %s." % ("Padre Anselmo nel tempio" if teste == ["Walter"] else "i coloni e i PNG da uomo"))


principale()
cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
os.makedirs(cartella, exist_ok=True)
with open(os.path.join(cartella, "capelli_uomo.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(RIGHE) + "\n")
