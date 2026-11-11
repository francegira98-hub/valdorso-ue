# Valdorso - Porta in Valdorso i brani della musica (provvisori, li fa Fra). Scritto da Claude l'11/11/2026.
# Si lancia dal menu Valdorso -> "Musica: importa i brani", oppure Strumenti -> Esegui script Python -> questo file.
#
# Prende i file .wav da V:\Texture\valdorso\musica e li mette in /Game/Valle/Musica con i nomi che cerca il gioco
# (UValdorsoMusicaComponent, ValdorsoMusica.h):
#   giorno.wav (o M_Giorno.wav)                  -> M_Giorno           di giorno, fuori
#   notte.wav (o M_Notte.wav)                    -> M_Notte            dalle 20 alle 6
#   tempio.wav (o M_Tempio.wav)                  -> M_Tempio           dentro il Tempio del Cuore
#   combattimento.wav (o M_Combattimento.wav)    -> M_Combattimento    quando si combatte
# Anche .ogg, .flac o .mp3 se il motore li legge (il .wav va sempre). Un brano che manca: quel momento resta in
# silenzio. Il brano non si ripete da solo: ci pensa il gioco (una pausa tra un giro e l'altro). Si puo' rilanciare
# quando i brani cambiano: sostituisce quelli di prima.

import os

import unreal

CARTELLA = r"V:\Texture\valdorso\musica"
DEST = "/Game/Valle/Musica"
BRANI = {
    "M_Giorno": ("giorno", "m_giorno", "day"),
    "M_Notte": ("notte", "m_notte", "night"),
    "M_Tempio": ("tempio", "m_tempio", "temple"),
    "M_Combattimento": ("combattimento", "m_combattimento", "battaglia", "combat"),
}
ESTENSIONI = (".wav", ".ogg", ".flac", ".mp3")

libreria = unreal.EditorAssetLibrary
strumenti = unreal.AssetToolsHelpers.get_asset_tools()


def trova(nomi):
    if not os.path.isdir(CARTELLA):
        return None
    for f in sorted(os.listdir(CARTELLA)):
        base, est = os.path.splitext(f)
        if est.lower() in ESTENSIONI and base.lower() in nomi:
            return os.path.join(CARTELLA, f)
    return None


def principale():
    if not os.path.isdir(CARTELLA):
        os.makedirs(CARTELLA, exist_ok=True)
        unreal.log_warning("[Valdorso] Musica: ho creato la cartella %s: mettici giorno.wav, notte.wav, tempio.wav, "
                           "combattimento.wav e rilancia" % CARTELLA)
        return
    fatti = []
    for nome, alias in BRANI.items():
        f = trova(alias)
        if not f:
            unreal.log_warning("[Valdorso] Musica: manca %s (cerco %s in %s)" % (nome, "/".join(alias), CARTELLA))
            continue
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", DEST)
        c.set_editor_property("destination_name", nome)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", True)
        strumenti.import_asset_tasks([c])
        s = unreal.load_asset("%s/%s" % (DEST, nome))
        if s is None:
            unreal.log_warning("[Valdorso] Musica: %s non importato (il formato va? prova con .wav)" % f)
            continue
        try:
            s.set_editor_property("looping", False)
        except Exception:  # noqa: BLE001
            pass
        libreria.save_loaded_asset(s)
        fatti.append(nome)
    unreal.log("[Valdorso] Musica: importati %d brani in %s (%s). In Play: console Valdorso.Musica.Stato" % (
        len(fatti), DEST, ", ".join(fatti) or "nessuno"))


principale()
