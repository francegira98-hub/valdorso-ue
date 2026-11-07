# Avvio di Valdorso nell'editor.
# Unreal esegue questo file da solo a ogni apertura del progetto:
# crea il menu "Valdorso" nella barra in alto, con gli strumenti scritti da Claude.

import unreal

# Comando della voce: ricarica lo script ogni volta, cosi' le modifiche valgono senza riavviare.
COMANDO_INVENTARIO = (
    "import importlib, inventario_per_claude as inv; "
    "importlib.reload(inv); inv.esegui()"
)

# (06/11, sera) crea_villaggio.py con le sezioni da rifare (SOLO_VILLAGGIO; [] = tutto)
COMANDO_VILLAGGIO = (
    "import os, unreal; "
    "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'crea_villaggio.py'); "
    "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'SOLO_VILLAGGIO': %s})"
)

# (07/11) un altro script di Content/Python/nuovi, lanciato così com'è
COMANDO_SCRIPT = (
    "import os, unreal; "
    "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', '%s'); "
    "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__'})"
)


def crea_menu_valdorso():
    menu_tool = unreal.ToolMenus.get()
    barra = menu_tool.find_menu("LevelEditor.MainMenu")
    if barra is None:
        unreal.log_warning("Valdorso: barra dei menu non trovata, menu non creato")
        return

    valdorso = barra.add_sub_menu(barra.get_name(), "Valdorso", "Valdorso", "Valdorso")

    voce = unreal.ToolMenuEntry(name="InventarioPerClaude", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Inventario per Claude")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_INVENTARIO)
    valdorso.add_menu_entry("Strumenti", voce)

    # (06/11) il villaggio a fasi: nell'editor si guarda una fase alla volta
    for fase in (1, 2, 3):
        voce = unreal.ToolMenuEntry(name="VillaggioFase%d" % fase, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label("Villaggio: mostra la fase %d" % fase)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "",
                                "import importlib, fasi_villaggio as fv; importlib.reload(fv); fv.mostra(%d)" % fase)
        valdorso.add_menu_entry("Villaggio", voce)

    # (06/11) le texture vere del villaggio (passo C)
    for nome, etichetta, funzione in (("TextureElenca", "Villaggio: elenca le superfici", "elenca"),
                                      ("TextureVere", "Villaggio: metti le texture vere", "metti"),
                                      ("TextureClaude", "Villaggio: rimetti le texture di Claude", "rimetti_claude")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "",
                                "import importlib, texture_vere as tv; importlib.reload(tv); tv.%s()" % funzione)
        valdorso.add_menu_entry("Villaggio", voce)

    # (06/11, sera) rifare solo alcune sezioni del villaggio (più veloce: il resto non si tocca e non si reimporta)
    for nome, etichetta, sezioni in (("VillaggioTutto", "Villaggio: rifai tutto", []),
                                     ("VillaggioRovineGrotte", "Villaggio: rifai solo rovine, grotte e belvedere",
                                      ["Rovine", "Grotta", "Grotta nord", "Belvedere"])):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_VILLAGGIO % repr(sezioni))
        valdorso.add_menu_entry("Villaggio", voce)

    # (07/11) la pietra di Val d'Orso sull'altare del tempio, senza rifare il villaggio
    voce = unreal.ToolMenuEntry(name="PietraTempio", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Tempio: metti la pietra di Val d'Orso")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "metti_pietra_tempio.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (07/11) i primi PNG: Gaspare al margine del bosco, Corrado e Ottone davanti alla porta del passo
    voce = unreal.ToolMenuEntry(name="MettiPNG", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("PNG: metti Gaspare e le guardie del passo")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "metti_png.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (07/11 sera) i piedi in piazza, il materiale delle rocce, Nanite sui trasparenti
    voce = unreal.ToolMenuEntry(name="SistemaValle", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Valle: sistema (piedi, materiali, Nanite)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "sistema_valle.py")
    valdorso.add_menu_entry("Villaggio", voce)

    menu_tool.refresh_all_widgets()
    unreal.log("Valdorso: menu pronto")


crea_menu_valdorso()