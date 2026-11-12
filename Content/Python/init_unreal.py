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

# (09/11) crea_bosco.py con la densita' (1 = tutti gli alberi)
COMANDO_BOSCO = (
    "import os, unreal; "
    "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'crea_bosco.py'); "
    "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'DENSITA_BOSCO': %s})"
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
                                      ["Rovine", "Grotta", "Grotta nord", "Belvedere"]),
                                     # (09/11) le cose intorno alle case, le insegne, i carri, le lanterne delle strade
                                     ("VillaggioVita", "Villaggio: rifai solo la vita nelle strade", ["Vita"]),
                                     # (09/11) il posto di guardia del passo (con la pietra del sigillo e la runa Ur)
                                     ("VillaggioGuardia", "Villaggio: rifai solo il posto di guardia del passo", ["Guardia"])):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_VILLAGGIO % repr(sezioni))
        valdorso.add_menu_entry("Villaggio", voce)

    # (07/11) la pietra di Val d'Orso sull'altare del tempio, senza rifare il villaggio
    voce = unreal.ToolMenuEntry(name="PietraTempio", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Tempio: metti la pietra di Val d'Orso")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "metti_pietra_tempio.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (07/11) i primi PNG: Gaspare al margine del bosco, le guardie Tebaldo e Fulvio davanti alla porta del passo
    voce = unreal.ToolMenuEntry(name="MettiPNG", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("PNG: metti Gaspare, le guardie del passo, Nonna Edda, Bruno e Padre Anselmo")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "metti_png.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (09/11) il bosco: tutti gli alberi della valle (crea_bosco.py); la seconda voce ne mette sei su dieci
    for nome, etichetta, densita in (("Bosco", "Bosco: pianta gli alberi", 1.0),
                                     ("BoscoRado", "Bosco: pianta gli alberi (6 su 10, se gli fps calano)", 0.6)):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_BOSCO % densita)
        valdorso.add_menu_entry("Villaggio", voce)

    # (10/11) i banchi delle botteghe (fucina, conceria, falegname, locanda): crea_botteghe.py
    voce = unreal.ToolMenuEntry(name="Botteghe", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Botteghe: metti i banchi (fucina, conceria, falegname, locanda)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_botteghe.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (10/11) la valle si vede anche da lontano (terreno, acqua e cose grandi sempre caricati): sistema_lontananza.py
    voce = unreal.ToolMenuEntry(name="Lontananza", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Valle: si vede anche da lontano (terreno, acqua, cose grandi)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "sistema_lontananza.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (10/11) i materiali del fumo dei camini e delle lucciole (AValdorsoVitaValle): crea_vita_valle.py
    voce = unreal.ToolMenuEntry(name="VitaValle", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Valle: fumo dei camini e lucciole (materiali)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_vita_valle.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (10/11) la Gilda: la bacheca degli incarichi dentro la Gilda, Lucia al bancone, la bacheca del Balivo: crea_gilda.py
    voce = unreal.ToolMenuEntry(name="Gilda", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Gilda: bacheca degli incarichi, Lucia, Mastro Ettore e bacheca del Balivo")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_gilda.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (09/11) i pericoli: i pezzi del lupo, le armi e i suoni dei colpi (crea_pericoli.py)
    voce = unreal.ToolMenuEntry(name="Pericoli", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Pericoli: importa animali, armi e suoni")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_pericoli.py")
    valdorso.add_menu_entry("Pericoli", voce)

    # (11/11, v0.1.6) il gatto della locanda, quello che sa le cose (crea_gatto.py)
    voce = unreal.ToolMenuEntry(name="Gatto", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Gatto della locanda: importa e mettilo da Bruno")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_gatto.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (11/11, v0.1.7) la grotta del nord: le gallerie dei nani, gli scheletri, il forziere, la frana (crea_grotta_nord.py)
    voce = unreal.ToolMenuEntry(name="GrottaNord", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Grotta del nord: gallerie, scheletri, forziere e frana")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_grotta_nord.py")
    valdorso.add_menu_entry("Pericoli", voce)

    # (11/11, v0.1.7) i gesti (Mixamo + Martellare e Spazzare provvisori) e i volti nuovi con le loro giornate
    voce = unreal.ToolMenuEntry(name="Gesti", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Gesti: importa da Mixamo (V:\\Mixamo\\Gesti) e fai Martellare e Spazzare")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "importa_gesti.py")
    valdorso.add_menu_entry("Villaggio", voce)

    voce = unreal.ToolMenuEntry(name="Musica", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Musica: importa i brani (V:\\Texture\\valdorso\\musica)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "importa_musica.py")
    valdorso.add_menu_entry("Villaggio", voce)

    voce = unreal.ToolMenuEntry(name="Volti", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Volti: Marta, Livia, Tobia, i mugnai, Elric (con le giornate) e la pietra dei segni")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_volti.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (12/11, v0.2.1) il .pak a pezzi: le etichette che dividono il gioco in cinque scatole (crea_scatole.py)
    voce = unreal.ToolMenuEntry(name="Scatole", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Pacchetto: prepara le scatole (il .pak a pezzi)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_scatole.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (07/11 sera) i piedi in piazza, il materiale delle rocce, Nanite sui trasparenti
    voce = unreal.ToolMenuEntry(name="SistemaValle", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Valle: sistema (piedi, materiali, Nanite)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "sistema_valle.py")
    valdorso.add_menu_entry("Villaggio", voce)

    menu_tool.refresh_all_widgets()
    unreal.log("Valdorso: menu pronto")


crea_menu_valdorso()