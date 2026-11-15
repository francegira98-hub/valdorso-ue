# Valdorso - La dieta della memoria (15/11/2026, scritto da Claude: primo passo della v0.2.5 "La valle piu' bella",
# approvato da Fra). Il crash del 14/11 era la scheda video piena (7,9 GB su 6,8): i pacchetti di Fab arrivano con
# texture enormi (le scogliere Megascans "Raw" a 8K: 140 MB l'una). Questo script mette un tetto alle texture dei
# pacchetti, senza toccare i file originali: Unreal le carica e le mette nel pacchetto del gioco al massimo a quella
# grandezza (Max Texture Size). Si torna indietro mettendo il tetto a 0.
#
# (15/11, dopo la domanda di Fra: "voglio che sia un signor gioco") I tetti sono quelli della qualita' EPICA: piu' su
# non si vede differenza sullo schermo (un 8K serve solo a un primo piano al cinema). Sotto ci pensa l'impostazione
# Qualita' del gioco (Basso / Medio / Alto / Epico): Unreal scala da solo le texture e la memoria che usa.
#   - scogliere, rocce grandi, terreno, alberi: 4096 (gli 8K scendono a 4K);
#   - case, mobili, tende, letti, oggetti grandi della locanda: 2048;
#   - cose piccole (piatti, posate, cibo, sacchi, candele): 1024;
#   - i pacchetti che non conosciamo: solo gli 8K, a 4096;
#   - i personaggi (Paragon, MetaHuman), la nostra valle, l'interfaccia e le icone: non si toccano.
# Due voci nel menu Valdorso -> Strumenti:
#   "Memoria: guarda quanto pesano le texture (non cambia niente)"  -> solo il resoconto;
#   "Memoria: metti a dieta le texture dei pacchetti"                -> mette i tetti e salva.
# Il resoconto in Saved/Claude/memoria.txt: quanto pesano (stima nella scheda video) prima e dopo, per cartella, e le
# 40 texture piu' pesanti. Si puo' rilanciare quando arrivano pacchetti nuovi.

import os

import unreal

FAI = bool(globals().get("DIETA_FAI", False))

# cartella (inizio del percorso) -> tetto. La prima che combacia vince (le piu' precise prima).
REGOLE = [
    ("/Game/Fab/Megascans/3D", 4096),
    ("/Game/EuropeanBeech", 4096),
    ("/Game/MSPresets", 4096),
    ("/Game/Fab/PolyHaven", 4096),
    ("/Game/KiteDemo", 2048),
    ("/Game/Fab/Case_Medievali", 2048),
    ("/Game/Fab/Casa_Camino", 2048),
    ("/Game/Fab/Taverna", 2048),
    ("/Game/Fab/Mercato_Tenda1", 2048),
    ("/Game/Fab/Mercato_Tenda2", 2048),
    ("/Game/Fab/Letto", 2048),
    ("/Game/Fab/Letto2", 2048),
    ("/Game/Fab/Megascans", 2048),
    ("/Game/Medieval_House_Props_V2", 2048),
    ("/Game/Scene_Banquet", 1024),
    ("/Game/Pack", 1024),
    ("/Game/Burlap_Sacks", 1024),
    ("/Game/SampleAnimationPack", 1024),
    ("/Game/FreeAnimationLibrary", 1024),
    # (15/11) il kit del Medieval Village (Migrate): Epic stesso aveva messo 1024 alle sue texture da 4K; si torna li'
    ("/Game/Megascans", 1024),
    ("/Game/Meshes", 1024),
    ("/Game/Materials", 1024),
]
# mai toccare
MAI = ("/Game/Paragon", "/Game/MetaHumans", "/Game/Valle", "/Game/UI", "/Game/Menu", "/Game/Splash", "/Game/Volto",
       "/Game/Peli", "/Game/Legami", "/Game/Characters", "/Game/Fab/Armi", "/Game/Unpacked", "/Game/Mixamo")
# (15/11) cartelle dove i tetti messi da chi ha fatto il pacchetto si rispettano (non si tolgono mai)
RISPETTA = ("/Game/Megascans", "/Game/Meshes", "/Game/Materials")
# oggetti piccoli: anche dentro le cartelle da 2048 o 4096 scendono a 1024
PICCOLI = ("plate", "spoon", "fork", "knife", "cup", "mug", "bowl", "goblet", "bottle", "jar", "candle", "apple",
           "grape", "bread", "cheese", "fish", "chicken", "banana", "sack", "coin", "piatto", "cucchiaio", "candela")

righe = []


def scrivi(testo):
    righe.append(testo)
    unreal.log("[Valdorso] Memoria: " + testo)


def tetto_per(percorso):
    if percorso.startswith(MAI):
        return None
    for inizio, tetto in REGOLE:
        if percorso.startswith(inizio):
            nome = percorso.rsplit("/", 1)[-1].lower()
            if tetto > 1024 and any(p in nome for p in PICCOLI):
                return 1024
            return tetto
    # gli altri pacchetti nuovi (non del gioco) che non conosciamo: solo gli 8K, a 4096
    return -1


def peso_mb(lato_x, lato_y, compressa):
    """Stima nella scheda video: byte per pixel (BC1 0,5; BC5/BC7/BC3 1) con le mip (+33%)."""
    bpp = 0.5 if compressa == "bc1" else 1.0
    return lato_x * lato_y * bpp * 1.333 / 1048576.0


def principale():
    registro = unreal.AssetRegistryHelpers.get_asset_registry()
    filtro = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "Texture2D")],
                             package_paths=["/Game"], recursive_paths=True)
    dati = registro.get_assets(filtro)
    scrivi("%s: %d texture nel progetto" % ("DIETA" if FAI else "SOLO GUARDARE", len(dati)))
    per_cartella = {}
    pesanti = []
    prima_tot = dopo_tot = 0.0
    cambiate = 0
    for d in dati:
        percorso = str(d.package_name)
        tetto = tetto_per(percorso)
        if tetto is None:
            continue
        # (senza caricarla, se si puo': la grandezza e' scritta nel registro) le piccole non cambiano mai
        # (15/11) la grandezza vera del file (dal registro): mentre Unreal prepara le texture, quella "caricata" e' piccola
        try:
            dim = str(d.get_tag_value("Dimensions") or "")
            reg_x, reg_y = [int(float(v)) for v in dim.lower().split("x")[:2]]
            lato_reg = max(reg_x, reg_y)
        except Exception:  # noqa: BLE001
            reg_x = reg_y = lato_reg = 0
        # (mettendo a dieta si guardano tutte: un tetto piu' basso messo prima, sotto quello nuovo, si toglie)
        if not FAI and lato_reg and ((tetto == -1 and lato_reg < 8192) or (tetto > 0 and lato_reg <= tetto)):
            continue
        t = unreal.EditorAssetLibrary.load_asset(percorso)
        if not isinstance(t, unreal.Texture2D):
            continue
        if lato_reg:
            x, y = reg_x, reg_y
        else:
            try:
                x, y = t.blueprint_get_size_x(), t.blueprint_get_size_y()
            except Exception:  # noqa: BLE001
                continue
        if x <= 0 or y <= 0:
            continue
        comp = str(t.get_editor_property("compression_settings")).lower()
        compressa = "bc1" if "default" in comp else "bc7"
        vecchio_tetto = int(t.get_editor_property("max_texture_size") or 0)
        sconosciuta = tetto == -1
        if sconosciuta:
            tetto = 4096 if max(x, y) >= 8192 else 0
        # (15/11) nel kit del Medieval Village solo le 4K scendono a 1024 (come le aveva Epic); le altre restano come sono
        if percorso.startswith(RISPETTA) and max(x, y) < 4096:
            tetto = 0
        lato = max(x, y)
        efficace_prima = min(lato, vecchio_tetto) if vecchio_tetto > 0 else lato
        if tetto and lato > tetto:
            nuovo = tetto
        elif tetto and vecchio_tetto and vecchio_tetto < min(lato, tetto) and not percorso.startswith(RISPETTA):
            nuovo = 0                                   # un tetto vecchio troppo basso: via
        else:
            nuovo = vecchio_tetto
        efficace_dopo = min(lato, nuovo) if nuovo > 0 else lato
        f_prima = efficace_prima / float(lato)
        f_dopo = efficace_dopo / float(lato)
        mb_prima = peso_mb(x * f_prima, y * f_prima, compressa)
        mb_dopo = peso_mb(x * f_dopo, y * f_dopo, compressa)
        prima_tot += mb_prima
        dopo_tot += mb_dopo
        cartella = "/".join(percorso.split("/")[:3])
        c = per_cartella.setdefault(cartella, [0, 0.0, 0.0])
        c[0] += 1
        c[1] += mb_prima
        c[2] += mb_dopo
        pesanti.append((mb_prima, mb_dopo, percorso, x, y))
        if FAI and nuovo != vecchio_tetto:
            t.set_editor_property("max_texture_size", int(nuovo))
            unreal.EditorAssetLibrary.save_loaded_asset(t, only_if_is_dirty=False)
            cambiate += 1
    scrivi("(contate solo le texture piu' grandi del loro tetto: quelle gia' piccole non cambiano)")
    scrivi("stima nella scheda video, se fosse tutto caricato: prima %.0f MB, dopo %.0f MB (-%.0f MB)"
           % (prima_tot, dopo_tot, prima_tot - dopo_tot))
    if FAI:
        scrivi("texture cambiate: %d" % cambiate)
    scrivi("")
    scrivi("per cartella (texture, MB prima, MB dopo):")
    for cartella, (n, a, b) in sorted(per_cartella.items(), key=lambda kv: -kv[1][1]):
        scrivi("  %-40s %5d  %8.0f  %8.0f" % (cartella, n, a, b))
    scrivi("")
    scrivi("le 40 piu' pesanti (MB prima -> dopo, grandezza):")
    for a, b, p, x, y in sorted(pesanti, reverse=True)[:40]:
        scrivi("  %7.1f -> %6.1f  %dx%d  %s" % (a, b, x, y, p))
    scrivi("FATTO")


try:
    principale()
finally:
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "memoria.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(righe) + "\n")
