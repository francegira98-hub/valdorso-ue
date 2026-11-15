# Valdorso - L'inventario dei pacchetti di Fab nel progetto (14/11/2026, scritto da Claude, per la v0.2.5 "La valle piu'
# bella"). Fra aggiunge i pacchetti con Add to Project; questo script li guarda tutti senza aprirli uno per uno e scrive
# Saved/Claude/inventario_fab.txt (e .json), che Claude legge da solo per decidere cosa usare e dove.
#
# Per ogni pacchetto (le cartelle di /Game che non sono del gioco, e quelle dentro /Game/Fab): quanto pesa sul disco, quando
# e' arrivato, quanti modelli, modelli animati, texture, materiali, animazioni (e di quale scheletro), effetti, livelli di
# prova; per i modelli le misure, i triangoli e se usano Nanite; per le texture la grandezza piu' grande.
# Non cambia niente nel progetto: legge e basta. Si lancia dal menu Valdorso -> Strumenti -> "Fab: inventario dei pacchetti".

import json
import os
import time
from collections import Counter, defaultdict

import unreal

# Le cartelle del gioco (non sono pacchetti da guardare).
DEL_GIOCO = {"__ExternalActors__", "__ExternalObjects__", "Audio", "Collections", "Developers", "Input", "Legami", "Menu",
             "MetaHumans", "Mixamo", "Peli", "Prove", "Python", "Splash", "ThirdPerson", "UI", "Unpacked", "Valle", "Volto",
             "Characters", "LevelPrototyping", "Variant_Combat", "Variant_Platforming", "Variant_SideScrolling", "Fab"}
# Dentro /Game/Fab: le cartelle fatte dal gioco (gli altri sono pacchetti).
DEL_GIOCO_FAB = {"Armi", "MaterialFunctions", "MaterialParameterCollection", "Materials", "Textures"}
MODELLI_PER_PACCHETTO = 400

registro = unreal.AssetRegistryHelpers.get_asset_registry()
contenuto = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
RIGHE = []


def scrivi(testo=""):
    RIGHE.append(testo)


def pacchetti():
    fuori = []
    for nome in sorted(os.listdir(contenuto)):
        if os.path.isdir(os.path.join(contenuto, nome)) and nome not in DEL_GIOCO:
            fuori.append(("/Game/" + nome, os.path.join(contenuto, nome)))
    fab = os.path.join(contenuto, "Fab")
    if os.path.isdir(fab):
        for nome in sorted(os.listdir(fab)):
            if os.path.isdir(os.path.join(fab, nome)) and nome not in DEL_GIOCO_FAB:
                fuori.append(("/Game/Fab/" + nome, os.path.join(fab, nome)))
    return fuori


def disco(cartella):
    totale, ultimo, file = 0, 0.0, 0
    for radice, _, nomi in os.walk(cartella):
        for n in nomi:
            try:
                s = os.stat(os.path.join(radice, n))
            except OSError:
                continue
            totale += s.st_size
            ultimo = max(ultimo, s.st_mtime)
            file += 1
    return totale, ultimo, file


def etichetta(dati, nome):
    try:
        v = dati.get_tag_value(nome)
        return str(v) if v is not None else ""
    except Exception:
        return ""


def classe(dati):
    try:
        return str(dati.asset_class_path.asset_name)
    except Exception:
        return str(getattr(dati, "asset_class", ""))


def guarda(percorso):
    filtro = unreal.ARFilter(package_paths=[percorso], recursive_paths=True)
    tutti = registro.get_assets(filtro)
    conta = Counter(classe(d) for d in tutti)
    modelli, texture, scheletri, livelli = [], [], Counter(), []
    for d in tutti:
        c = classe(d)
        nome = str(d.package_name)
        if c == "StaticMesh" and len(modelli) < MODELLI_PER_PACCHETTO:
            modelli.append({"nome": nome.split("/")[-1], "percorso": nome,
                            "misure": etichetta(d, "ApproxSize"), "triangoli": etichetta(d, "Triangles"),
                            "vertici": etichetta(d, "Vertices"), "nanite": etichetta(d, "NaniteEnabled") or etichetta(d, "Nanite"),
                            "materiali": etichetta(d, "Materials")})
        elif c == "Texture2D":
            texture.append(etichetta(d, "Dimensions"))
        elif c in ("AnimSequence", "AnimMontage", "BlendSpace"):
            sk = etichetta(d, "Skeleton")
            scheletri[sk.split(".")[-1].strip("'\"") if sk else "?"] += 1
        elif c == "World":
            livelli.append(nome)
    return tutti, conta, modelli, texture, scheletri, livelli


def lato_massimo(dimensioni):
    massimo = 0
    for t in dimensioni:
        try:
            a, b = t.lower().split("x")[:2]
            massimo = max(massimo, int(float(a)), int(float(b)))
        except Exception:
            pass
    return massimo


def principale():
    adesso = time.time()
    tutti_i_dati = []
    elenco = pacchetti()
    scrivi("Inventario dei pacchetti di Fab nel progetto Valdorso - %s" % time.strftime("%d/%m/%Y %H:%M"))
    scrivi("(%d cartelle guardate; 'nuovo' = arrivato negli ultimi 2 giorni)" % len(elenco))
    scrivi()
    for percorso, cartella in elenco:
        peso, ultimo, file = disco(cartella)
        tutti, conta, modelli, texture, scheletri, livelli = guarda(percorso)
        nuovo = adesso - ultimo < 2 * 86400
        scrivi("=" * 100)
        scrivi("%s%s   %.1f MB su %d file, ultimo arrivo %s" % (percorso, "   [NUOVO]" if nuovo else "", peso / 1048576.0, file,
                                                             time.strftime("%d/%m %H:%M", time.localtime(ultimo)) if ultimo else "-"))
        scrivi("  asset: " + ", ".join("%s %d" % (k, v) for k, v in conta.most_common()))
        if texture:
            scrivi("  texture: %d, la piu' grande %d px" % (len(texture), lato_massimo(texture)))
        if scheletri:
            scrivi("  animazioni per scheletro: " + ", ".join("%s %d" % (k, v) for k, v in scheletri.most_common()))
        for l in livelli[:10]:
            scrivi("  livello di prova: " + l)
        for m in modelli:
            scrivi("    %-60s misure %-18s tri %-8s nanite %-5s" % (m["nome"][:60], m["misure"], m["triangoli"], m["nanite"]))
        tutti_i_dati.append({"percorso": percorso, "mb": round(peso / 1048576.0, 1), "file": file, "nuovo": nuovo,
                             "asset": dict(conta), "texture": len(texture), "texture_max": lato_massimo(texture),
                             "scheletri": dict(scheletri), "livelli": livelli, "modelli": modelli})
    cartella = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Claude")
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, "inventario_fab.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(RIGHE) + "\n")
    with open(os.path.join(cartella, "inventario_fab.json"), "w", encoding="utf-8") as f:
        json.dump(tutti_i_dati, f, ensure_ascii=False, indent=1)
    unreal.log("[Valdorso] Inventario di Fab: %d pacchetti, scritto in %s" % (len(tutti_i_dati), cartella))


principale()
