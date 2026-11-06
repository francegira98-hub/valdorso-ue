# Avvio di Valdorso nell'editor.
# Unreal esegue questo file da solo a ogni apertura del progetto:
# crea il menu "Valdorso" nella barra in alto, con gli strumenti scritti da Claude.

import unreal

# Comando della voce: ricarica lo script ogni volta, cosi' le modifiche valgono senza riavviare.
COMANDO_INVENTARIO = (
    "import importlib, inventario_per_claude as inv; "
    "importlib.reload(inv); inv.esegui()"
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

    menu_tool.refresh_all_widgets()
    unreal.log("Valdorso: menu pronto")


crea_menu_valdorso()