# Valdorso - Reimporta i suoni rifatti da Claude (13/11/2026).
#
# Prende da V:/Texture/valdorso/suoni i .wav fatti da Tools/Generatori/suoni_valle.py e li mette (o li rimette) in
# /Game/Valle/Suoni, senza toccare il resto della valle (crea_suoni_valle.py invece rimette anche le sorgenti e le campane).
# Il 13/11: gli uccelli più calmi (S_Uccelli) e le porte: S_Porta_Apri, S_Porta_Chiudi, S_Porta_Chiave (chiusa a chiave).
# Le porte li suonano da sole (AValdorsoPorta, ValdorsoInterazione.cpp): basta che ci siano.
# Scrive cosa ha fatto in Saved/Claude/aggiorna_suoni.txt.
# Si lancia dal menu Valdorso -> Strumenti -> "Suoni: reimporta i suoni rifatti da Claude".

import os
import unreal

CARTELLA = "V:/Texture/valdorso/suoni"
DEST = "/Game/Valle/Suoni"
# nome -> in giro (Looping)
SUONI = {"S_Uccelli": True, "S_Porta_Apri": False, "S_Porta_Chiudi": False, "S_Porta_Chiave": False}

RIGHE = []


def scrivi(testo):
    RIGHE.append(testo)
    unreal.log("[Valdorso] suoni: " + testo)


def principale():
    strumenti = unreal.AssetToolsHelpers.get_asset_tools()
    libreria = unreal.EditorAssetLibrary
    for nome, in_giro in SUONI.items():
        f = os.path.join(CARTELLA, nome + ".wav")
        if not os.path.exists(f):
            scrivi("ATTENZIONE: manca %s" % f)
            continue
        c = unreal.AssetImportTask()
        c.set_editor_property("filename", f)
        c.set_editor_property("destination_path", DEST)
        c.set_editor_property("automated", True)
        c.set_editor_property("replace_existing", True)
        c.set_editor_property("save", True)
        strumenti.import_asset_tasks([c])
        s = unreal.load_asset("%s/%s" % (DEST, nome))
        if s is None:
            scrivi("ATTENZIONE: non importato %s" % nome)
            continue
        s.set_editor_property("looping", in_giro)
        libreria.save_loaded_asset(s, only_if_is_dirty=False)
        try:
            durata = "%.1f secondi" % s.get_editor_property("duration")
        except Exception:  # noqa: BLE001
            durata = "importato"
        scrivi("%s: %s%s" % (nome, durata, " (in giro)" if in_giro else ""))
    scrivi("FATTO.")


def salva_resoconto():
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "aggiorna_suoni.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(RIGHE) + "\n")


try:
    principale()
finally:
    salva_resoconto()
