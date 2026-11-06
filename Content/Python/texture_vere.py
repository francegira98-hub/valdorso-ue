# Valdorso - Le texture vere del villaggio (passo C, 06/11/2026, scritto da Claude).
#
# Il villaggio usa 9 superfici fatte da Claude da zero (Pietra, Intonaco, Assi, Trave, Coppi, Paglia, Ciottoli, Terra,
# e dal 06/11 Roccia, la roccia viva della grotta est),
# ognuna in un'istanza /Game/Valle/Villaggio/Materiali/MI_Villaggio_<Nome>. Per passare a texture vere (Megascans o
# altre di Fab) non si tocca niente nel livello: si cambiano le due texture (colore e rilievo) di quell'istanza, e
# tutte le case cambiano insieme.
#
# Come si fa:
#   1. Fab (nell'editor) -> Aggiungi al progetto le superfici scelte. Finiscono in Content/Fab/...
#   2. Menu Valdorso -> "Villaggio: elenca le superfici": scrive Saved/Claude/superfici.txt con le cartelle trovate
#      (Fra lo manda a Claude, che sceglie i nomi giusti).
#   3. In Content/Python/texture_vere_villaggio.json, per ogni superficie, la cartella e la scala, per esempio
#        {"Pietra": {"cartella": "/Game/Fab/Megascans/Surfaces/Rough_Stone_Wall", "scala": 0.5}}
#      (scala: quante volte si ripete; 0.5 = più grande, 2 = più piccola). Le superfici che non ci sono restano le nostre.
#   4. Menu Valdorso -> "Villaggio: metti le texture vere". Per tornare indietro: "Villaggio: rimetti le texture di
#      Claude". Anche crea_villaggio.py, quando si rilancia, rimette le texture vere scritte nel file.
# Le texture di Fab non vanno mai su GitHub (Content/Fab è nel .gitignore); il file .json sì (sono solo nomi).

import json
import os

import unreal

DEST = "/Game/Valle/Villaggio"
SUPERFICI = ["Pietra", "Intonaco", "Assi", "Trave", "Coppi", "Paglia", "Ciottoli", "Terra", "Roccia"]
COLORE = ("_d", "_basecolor", "_base_color", "_albedo", "_diffuse", "_diff", "_col", "_color", "_bc")
RILIEVO = ("_n", "_normal", "_nor", "_nor_dx", "_normaldx", "_nrm")

libreria = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def _file_scelte():
    return os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), "Python",
                        "texture_vere_villaggio.json")


def _texture_di(cartella):
    """Colore e rilievo dentro una cartella di Fab, dal nome (T_..._D, ..._BaseColor, ..._N, ..._Normal...)."""
    colore = rilievo = None
    for p in libreria.list_assets(cartella, recursive=True, include_folder=False):
        nome = p.rsplit(".", 1)[-1].lower()
        if colore is None and nome.endswith(COLORE):
            a = unreal.load_asset(p)
            if isinstance(a, unreal.Texture2D):
                colore = a
        elif rilievo is None and nome.endswith(RILIEVO):
            a = unreal.load_asset(p)
            if isinstance(a, unreal.Texture2D):
                rilievo = a
    return colore, rilievo


def _istanza(nome):
    return unreal.load_asset("%s/Materiali/MI_Villaggio_%s" % (DEST, nome))


def applica():
    """Mette le texture vere scritte in texture_vere_villaggio.json. Restituisce le righe del resoconto."""
    righe = []
    if not os.path.exists(_file_scelte()):
        return ["nessuna scelta (texture_vere_villaggio.json non c'è): restano le texture di Claude"]
    with open(_file_scelte(), "r", encoding="utf-8") as f:
        scelte = json.load(f)
    for nome, voce in scelte.items():
        if nome not in SUPERFICI or not isinstance(voce, dict) or not voce.get("cartella"):
            continue
        mi = _istanza(nome)
        if mi is None:
            righe.append("%s: manca MI_Villaggio_%s (lanciare prima crea_villaggio.py)" % (nome, nome))
            continue
        colore, rilievo = _texture_di(voce["cartella"])
        if colore is None:
            righe.append("%s: nella cartella %s non trovo la texture del colore" % (nome, voce["cartella"]))
            continue
        # le texture "virtuali" (Virtual Texture) non vanno in questi materiali: si salta e si dice
        if colore.get_editor_property("virtual_texture_streaming") or (rilievo and rilievo.get_editor_property("virtual_texture_streaming")):
            righe.append("%s: texture virtuale (Virtual Texture Streaming acceso): spegnerlo nella texture, poi riprovare" % nome)
            continue
        mel.set_material_instance_texture_parameter_value(mi, "Colore", colore)
        if rilievo is not None:
            rilievo.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            rilievo.set_editor_property("srgb", False)
            libreria.save_loaded_asset(rilievo)
            mel.set_material_instance_texture_parameter_value(mi, "Rilievo", rilievo)
        mel.set_material_instance_scalar_parameter_value(mi, "Scala", float(voce.get("scala", 1.0)))
        mel.update_material_instance(mi)
        libreria.save_loaded_asset(mi)
        righe.append("%s: %s%s (scala %.2f)" % (nome, colore.get_name(), (" + " + rilievo.get_name()) if rilievo else "",
                                                float(voce.get("scala", 1.0))))
    return righe or ["nessuna superficie nel file"]


def rimetti_claude():
    """Rimette le texture fatte da Claude (T_<Nome>, T_<Nome>_N) e la scala 1 in tutte le istanze."""
    for nome in SUPERFICI:
        mi = _istanza(nome)
        c = unreal.load_asset("%s/Texture/T_%s" % (DEST, nome))
        n = unreal.load_asset("%s/Texture/T_%s_N" % (DEST, nome))
        if mi is None or c is None or n is None:
            continue
        mel.set_material_instance_texture_parameter_value(mi, "Colore", c)
        mel.set_material_instance_texture_parameter_value(mi, "Rilievo", n)
        mel.set_material_instance_scalar_parameter_value(mi, "Scala", 1.0)
        mel.update_material_instance(mi)
        libreria.save_loaded_asset(mi)
    unreal.log("[Valdorso] Texture del villaggio: rimesse quelle di Claude")


def metti():
    for r in applica():
        unreal.log("[Valdorso] Texture vere: " + r)


def elenca():
    """Scrive Saved/Claude/superfici.txt: le cartelle di Content/Fab che hanno una texture di colore e una di rilievo."""
    trovate = {}
    for p in libreria.list_assets("/Game/Fab", recursive=True, include_folder=False):
        nome = p.rsplit(".", 1)[-1].lower()
        cartella = p.rsplit("/", 1)[0]
        if nome.endswith(COLORE):
            trovate.setdefault(cartella, set()).add("colore")
        elif nome.endswith(RILIEVO):
            trovate.setdefault(cartella, set()).add("rilievo")
    righe = ["Superfici trovate in /Game/Fab (cartelle con colore e rilievo):"]
    righe += sorted(c for c, k in trovate.items() if {"colore", "rilievo"} <= k)
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "superfici.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe))
    unreal.log("[Valdorso] Superfici: %d cartelle, elenco in Saved/Claude/superfici.txt" % (len(righe) - 1))
