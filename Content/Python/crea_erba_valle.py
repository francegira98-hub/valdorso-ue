# Valdorso - L'erba vera della valle (passo R4 dei ritocchi). Scritto da Claude l'08/10/2026.
# Si lancia nell'editor, con L_Valle aperto: Strumenti -> Esegui script Python -> questo file. Si può rilanciare.
# Poi si rilancia crea_materiale_valle.py, che dice al paesaggio DOVE seminare (il prato, e le felci dove è bagnato).
#
# Cosa fa: crea in Content/Valle/Erba due "tipi d'erba" del paesaggio (LandscapeGrassType). Il paesaggio li semina
# da solo, a ciuffi, dove il materiale M_Valle dice che c'è prato; li toglie su roccia, ghiaia, terra e neve.
# Non si piantano a mano e non sono attori: si vedono solo intorno a chi guarda (fino a ~80 m) e si rifanno da soli.
#   LGT_Prato: l'erba da campo del Kite Demo di Epic, più i fiori (ranuncoli, vedovine, achillea), radi e a chiazze;
#   LGT_Felci: le felci, solo dove il prato è bagnato (lungo la Vena, sotto la cascata).
# I modelli vengono da Content/KiteDemo (migrati da V:\Prove\ProvaErba l'08/10; fuori da GitHub, nel .gitignore).
# Erba "mista" (decisa da Fra): qui è il pascolo medio; più bassa e calpestata vicino a villaggio e sentieri arriverà
# con la funzione dell'erba che si abbassa dove si passa spesso.

import unreal

CARTELLA = "/Game/Valle/Erba"
KITE = "/Game/KiteDemo/Environments/Foliage"

# nome, mesh, densità (ciuffi ogni 10 m x 10 m), scala min-max, distanza dove inizia a sparire e dove è sparita (cm)
PRATO = [
    ("Erba",      KITE + "/Grass/FieldGrass/SM_FieldGrass_01",             260.0, (0.8, 1.3), 4500, 8000),
    ("Ranuncoli", KITE + "/Flowers/Buttercup/SM_Buttercup_Patch_01",          4.0, (0.8, 1.2), 3000, 5000),
    ("Vedovine",  KITE + "/Flowers/FieldScabious/SM_FieldScabious_01",        2.0, (0.8, 1.1), 3000, 5000),
    ("Achillea",  KITE + "/Flowers/Yarrow/SM_Yarrow",                         2.0, (0.8, 1.2), 3000, 5000),
]
FELCI = [
    ("Felce 1",   KITE + "/Ferns/SM_Fern_01",                                 6.0, (0.7, 1.2), 3500, 6000),
    ("Felce 2",   KITE + "/Ferns/SM_Fern_02",                                 6.0, (0.7, 1.2), 3500, 6000),
    ("Felce 3",   KITE + "/Ferns/SM_Fern_03",                                 6.0, (0.7, 1.2), 3500, 6000),
]

libreria = unreal.EditorAssetLibrary


def scrivi(testo):
    unreal.log("[Valdorso] Erba: " + testo)


def metti(oggetto, nome, valore, *altre):
    """Prova il valore così com'è e poi le altre forme (in 5.x alcune proprietà sono 'per piattaforma')."""
    for v in (valore,) + altre:
        try:
            oggetto.set_editor_property(nome, v)
            return True
        except Exception:
            continue
    unreal.log_warning("[Valdorso] Erba: non riesco a impostare " + nome)
    return False


def per_piattaforma_float(v):
    try:
        p = unreal.PerPlatformFloat()
        p.set_editor_property("default", float(v))
        return p
    except Exception:
        return float(v)


def per_piattaforma_int(v):
    try:
        p = unreal.PerPlatformInt()
        p.set_editor_property("default", int(v))
        return p
    except Exception:
        return int(v)


def varieta(nome, mesh_percorso, densita, scala, inizio, fine):
    mesh = libreria.load_asset(mesh_percorso)
    if mesh is None:
        unreal.log_error("[Valdorso] Erba: manca " + mesh_percorso + " (la migrazione da ProvaErba è andata?)")
        return None
    v = unreal.GrassVariety()
    metti(v, "grass_mesh", mesh)
    metti(v, "grass_density", per_piattaforma_float(densita), float(densita))
    metti(v, "use_grid", False)                     # a caso, non in file: sembra più naturale
    metti(v, "start_cull_distance", per_piattaforma_int(inizio), int(inizio))
    metti(v, "end_cull_distance", per_piattaforma_int(fine), int(fine))
    metti(v, "scaling", unreal.GrassScaling.UNIFORM)
    intervallo = unreal.FloatInterval()
    intervallo.set_editor_property("min", scala[0])
    intervallo.set_editor_property("max", scala[1])
    metti(v, "scale_x", intervallo)
    metti(v, "random_rotation", True)
    metti(v, "align_to_surface", True)
    metti(v, "cast_dynamic_shadow", nome.startswith("Felce"))   # ombre solo sulle felci (sull'erba costano troppo)
    metti(v, "receives_decals", False)
    scrivi("  {}: {} ciuffi ogni 100 m², visibile fino a {:.0f} m".format(nome, densita, fine / 100))
    return v


def tipo_erba(nome, voci):
    percorso = CARTELLA + "/" + nome
    if libreria.does_asset_exist(percorso):
        tipo = libreria.load_asset(percorso)
    else:
        strumenti = unreal.AssetToolsHelpers.get_asset_tools()
        fabbrica = getattr(unreal, "LandscapeGrassTypeFactory", None)
        tipo = strumenti.create_asset(nome, CARTELLA, unreal.LandscapeGrassType, fabbrica() if fabbrica else None)
    if tipo is None:
        unreal.log_error("[Valdorso] Erba: non riesco a creare " + percorso)
        return
    scrivi(nome + ":")
    varieta_tutte = [v for v in (varieta(*voce) for voce in voci) if v is not None]
    tipo.set_editor_property("grass_varieties", varieta_tutte)
    libreria.save_loaded_asset(tipo)


def esegui():
    tipo_erba("LGT_Prato", PRATO)
    tipo_erba("LGT_Felci", FELCI)
    scrivi("fatto. Ora rilancia crea_materiale_valle.py: dice al paesaggio dove seminare.")


esegui()
