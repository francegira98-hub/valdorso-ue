# Valdorso - Rifa' i legami delle SOPRACCIGLIA dalla testa modello Legacy01, nella cartella /Game/Peli/Nuovi, come i
# capelli da uomo del 14/11. Scritto da Claude il 14/11/2026 (v0.2.3 "Armi vere", la pulizia).
#
# Perche': il 02/11 rifai_sopracciglia.py (ora in usati/) provo' a rifare le sopracciglia da Legacy01, ma in 7 teste
# (Celeste, Dominic, Etta, Jelena, Lorenzo, Mikel, Walter) l'editor non riusci' a sovrascrivere i legami vecchi (erano
# caricati) e creo' quelli nuovi con un "1" in fondo (Eyebrows_M_Natural1...), che il gioco non usa: in quelle teste le
# sopracciglia sono ancora quelle vecchie (fatte da SKM_Face, a volte fuori posto). Grace, Omari, Sunita e la testa di prova
# sono gia' a posto, ma si rifanno lo stesso: cosi' tutte le teste hanno le sopracciglia nello stesso posto.
# Il gioco cerca prima in /Game/Peli/Nuovi/<Testa> per ogni pelo (ValdorsoAspettoComponent.cpp): niente C++ da cambiare.
#
# Fa anche le pulizie: toglie i doppioni con l'"1" (Eyebrows_*1 nelle 7 teste, e i 10 Hair_*1 rimasti in Walter dal primo
# giro dei capelli) e la copia di sicurezza /Game/Peli/_Prima. Se l'editor non riesce a toglierne uno (perche' e' caricato),
# lo scrive: si chiude e si riapre l'editor e si rilancia, prima di aprire il livello.
#
# Si lancia dal menu Valdorso -> Strumenti:
#   "Sopracciglia: prova su Padre Anselmo"   solo la testa Walter (pochi secondi), per vedere se va
#   "Sopracciglia: rifai tutte"              le 11 teste (un paio di minuti)
#   "Sopracciglia: torna come prima"         toglie solo quelle nuove (il gioco usa di nuovo quelle di sempre)
# Il risultato va anche in Saved/Claude/sopracciglia.txt.

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

MODELLO = "/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy01"
LIBRERIA = "/MetaHumanCharacter/Optional/Grooms/GroomAssets/Eyebrows"
CARTELLA = "/Game/Peli"
NUOVI = "/Game/Peli/Nuovi"
PRIMA = "/Game/Peli/_Prima"

# Le stesse liste di crea_legami_peli.py e del C++ (ValdorsoAspettoComponent.cpp).
TESTE_UOMO = ["Prova", "Walter", "Lorenzo", "Dominic", "Mikel", "Omari"]
TESTE_DONNA = ["Jelena", "Grace", "Etta", "Celeste", "Sunita"]
SOPRACCIGLIA_UOMO = ["Eyebrows_M_Natural", "Eyebrows_M_Thick", "Eyebrows_M_Thin", "Eyebrows_M_Unruly", "Eyebrows_M_Full"]
SOPRACCIGLIA_DONNA = ["Eyebrows_M_Natural", "Eyebrows_M_Thin", "Eyebrows_M_SlightArch", "Eyebrows_M_Fine", "Eyebrows_M_Swept"]
CAPELLI_UOMO = ["Hair_S_BuzzCut", "Hair_S_BrushCut", "Hair_S_Casual", "Hair_S_Messy", "Hair_S_SlickBack",
                "Hair_S_PulledBack", "Hair_S_SideSweptFringe", "Hair_M_Layered", "Hair_L_MessyClumps",
                "Hair_S_RecedeMessy", "Hair_S_BaldingStubble"]

RIGHE = []
libreria = unreal.EditorAssetLibrary


def scrivi(testo):
    RIGHE.append(testo)
    unreal.log("[Valdorso] Sopracciglia: " + testo)


def volto(testa):
    for p in ("/Game/Unpacked/MH_%s/Face/SKM_MH_%s_FaceMesh" % (testa, testa),
              "/Game/MetaHumans/MH_%s/Face/SKM_MH_%s_FaceMesh" % (testa, testa)):
        if libreria.does_asset_exist(p):
            return p
    return None


def sopracciglia_di(testa):
    return SOPRACCIGLIA_UOMO if testa in TESTE_UOMO else SOPRACCIGLIA_DONNA


def pulisci():
    """Toglie i doppioni con l'"1" in fondo (sopracciglia del 02/11, capelli del primo giro del 14/11) e _Prima."""
    tolti, restati = 0, []
    da_togliere = []
    for testa in TESTE_UOMO + TESTE_DONNA:
        for nome in SOPRACCIGLIA_UOMO + SOPRACCIGLIA_DONNA + CAPELLI_UOMO:
            da_togliere.append("%s/%s/%s1" % (CARTELLA, testa, nome))
    for p in sorted(set(da_togliere)):
        if not libreria.does_asset_exist(p):
            continue
        if libreria.delete_asset(p) and not libreria.does_asset_exist(p):
            tolti += 1
        else:
            restati.append(p)
    if libreria.does_directory_exist(PRIMA):
        if libreria.delete_directory(PRIMA):
            scrivi("tolta la copia di sicurezza %s (i capelli da uomo nuovi vanno bene)" % PRIMA)
        else:
            restati.append(PRIMA)
    scrivi("pulizia: tolti %d doppioni con l'1" % tolti)
    for p in restati:
        scrivi("ATTENZIONE: non riesco a togliere %s (e' caricato): chiudi e riapri l'editor e rilancia, prima di aprire il livello" % p)
    return len(restati)


def torna_indietro(teste):
    tolti = 0
    for testa in teste:
        for nome in sopracciglia_di(testa):
            nuovo = "%s/%s/%s" % (NUOVI, testa, nome)
            if libreria.does_asset_exist(nuovo) and libreria.delete_asset(nuovo):
                tolti += 1
    scrivi("TORNATO COME PRIMA: tolte %d sopracciglia nuove (il gioco usa di nuovo quelle di sempre)." % tolti)


def principale():
    tutte = TESTE_UOMO + TESTE_DONNA
    scelte = [t for t in SOLO_TESTE if t in tutte]
    teste = scelte or tutte
    # le pulizie solo con "rifai tutte" (la prova e "torna come prima" non toccano niente d'altro)
    restati = pulisci() if not scelte and not TORNA_INDIETRO else 0
    if TORNA_INDIETRO:
        torna_indietro(teste)
        return
    strumenti = getattr(unreal, "ValdorsoStrumenti", None)
    modello = libreria.load_asset(MODELLO)
    if strumenti is None or modello is None:
        scrivi("ERRORE: manca %s" % ("ValdorsoStrumenti (il C++ non e' caricato?)" if strumenti is None else MODELLO))
        return
    fatti, errori = 0, 0
    for testa in teste:
        faccia_percorso = volto(testa)
        faccia = libreria.load_asset(faccia_percorso) if faccia_percorso else None
        if faccia is None:
            scrivi("manca il volto di %s" % testa)
            errori += 1
            continue
        for nome in sopracciglia_di(testa):
            groom = libreria.load_asset("%s/%s/%s" % (LIBRERIA, nome, nome))
            destinazione = "%s/%s/%s" % (NUOVI, testa, nome)
            if groom is None:
                scrivi("manca %s nella libreria MetaHuman" % nome)
                errori += 1
                continue
            # quelle di sempre (/Game/Peli/<Testa>) non si toccano: le nuove vanno in Nuovi e il gioco le preferisce
            if libreria.does_asset_exist(destinazione) and not libreria.delete_asset(destinazione):
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
                scrivi("ATTENZIONE: %s e' finito in %s invece di %s: chiudi e riapri l'editor e rilancia" % (nome, percorso, destinazione))
                errori += 1
            strumenti.costruisci_subito(legame)
            libreria.save_loaded_asset(legame, False)
            fatti += 1
        scrivi("%s: fatto" % testa)
        unreal.SystemLibrary.collect_garbage()
    scrivi("FINITO: fatte %d sopracciglia nuove in %s (teste: %s), errori %d, doppioni rimasti %d." % (
        fatti, NUOVI, ", ".join(teste), errori, restati))
    scrivi("Ora: Play, e guarda %s." % ("Padre Anselmo nel tempio" if teste == ["Walter"] else "i coloni e i PNG da vicino (anche nel Registro)"))


principale()
cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
os.makedirs(cartella, exist_ok=True)
with open(os.path.join(cartella, "sopracciglia.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(RIGHE) + "\n")
