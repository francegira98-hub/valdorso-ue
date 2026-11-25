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

# (14/11, v0.2.3) rifai_sopracciglia_nuove.py con le teste e "torna indietro" (i capelli da uomo sono fatti: lo script
# rifai_capelli_uomo.py e' in usati/)
COMANDO_SOPRACCIGLIA = (
    "import os, unreal; "
    "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'rifai_sopracciglia_nuove.py'); "
    "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'SOLO_TESTE': %s, 'TORNA_INDIETRO': %s})"
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

    # (15/11, v0.2.5) il sentiero dei nani: la mulattiera a tornanti fino alla grotta del nord (nuovi/crea_sentiero_nani.py)
    for nome, etichetta, modo in (("SentieroNani", "Sentiero dei nani: costruisci (grotta del nord)", "fai"),
                                  ("SentieroNaniVia", "Sentiero dei nani: togli", "togli")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", (
            "import os, unreal; "
            "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'crea_sentiero_nani.py'); "
            "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'SENTIERO_MODO': '%s'})") % modo)
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

    # (15/11, v0.2.5) le case vere con il kit del Medieval Village, sulla stessa pianta (nuovi/costruisci_case.py)
    for nome, etichetta, modo in (("CaseVereProva", "Case vere: casa di prova (Casa_Sud_1)", "prova"),
                                  ("CaseVereTutte", "Case vere: tutte le case", "tutte"),
                                  ("CaseVereIndietro", "Case vere: torna come prima", "indietro")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", (
            "import os, unreal; "
            "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'costruisci_case.py'); "
            "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'CASE_MODO': '%s'})") % modo)
        valdorso.add_menu_entry("Villaggio", voce)

    # (16/11, v0.2.6 "Le mura" e v0.2.7 "Il tempio") i pezzi della chat 3D nel livello (nuovi/crea_mura.py)
    for nome, etichetta, modo in (("MuraAssi", "Mura: 1. prova degli assi", "assi"),
                                  ("MuraCostruisci", "Mura: 2. costruisci le mura (basse in fase 2, alte in fase 3)", "mura"),
                                  ("MuraTempio", "Mura: 3. il tempio che cresce (fasi 2 e 3)", "tempio"),
                                  ("MuraStrade", "Mura: 4. strade, case spostate e niente erba", "strade"),
                                  ("MuraCaserma", "Mura: 5. il recinto della caserma (fase 3)", "caserma"),
                                  ("MuraAlloggi", "Mura: 6. la caserma dentro: camerata, ufficio, prigione (fase 3)", "alloggi"),
                                  ("MuraGradini", "Mura: 7. i gradini davanti alle porte (dal collaudo)", "gradini"),
                                  ("MuraScale", "Mura: 8. le scale di fuori (mulino, casermetta, torre del passo)", "scale"),
                                  ("MuraPonti", "Mura: 9. i ponti veri sul fiume", "ponti"),
                                  ("MuraLocanda", "Mura: 10. la locanda nuova di Bruno", "locanda"),
                                  ("MuraPasso", "Mura: 11. il posto di guardia reale del passo", "passo"),
                                  ("MuraRonda", "Mura: 12. i registri della ronda", "ronda"),
                                  ("MuraTexture", "Mura: 13. texture vere degli interni e pietra da castello", "texture"),
                                  ("MuraTextureVia", "Mura: 14. texture vere: torna a quelle di prima", "texture_via"),
                                  ("MuraIndietro", "Mura: torna come prima", "indietro")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", (
            "import os, unreal; "
            "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'crea_mura.py'); "
            "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'MURA_MODO': '%s'})") % modo)
        valdorso.add_menu_entry("Villaggio", voce)

    # (24/11) PROVA: le Terre di Montalto, la mappa oltre il passo, in un livello a parte (nuovi/crea_montalto.py).
    # Non tocca L_Valle e non toglie il sigillo.
    for nome, etichetta, modo in (("MontaltoLivello", "Montalto (prova): 1. crea il livello", "livello"),
                                  ("MontaltoVesti", "Montalto (prova): 2. vesti il terreno (materiale e fiumi)", "vesti"),
                                  ("MontaltoPonte", "Montalto (prova): 3. il ponte sull'Aure", "ponte"),
                                  ("MontaltoRocca", "Montalto (prova): 4. la rocca del duca", "rocca"),
                                  ("MontaltoPorto", "Montalto (prova): 5. il porto e i magazzini della lana", "porto"),
                                  ("MontaltoGranaio", "Montalto (prova): 6. il Granaio della Corona", "granaio"),
                                  ("MontaltoCase", "Montalto (prova): 7. le case di citta'", "case"),
                                  ("MontaltoMura", "Montalto (prova): 8. le mura e la porta nord", "mura"),
                                  ("MontaltoCimitero", "Montalto (prova): 9. il cimitero vecchio", "cimitero")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", (
            "import os, unreal; "
            "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'crea_montalto.py'); "
            "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'MONTALTO_MODO': '%s'})") % modo)
        valdorso.add_menu_entry("Villaggio", voce)

    # (17/11, v0.2.7) i dati per il sagrato e per i ponti, per la chat 3D (nuovi/esporta_tempio_ponti.py; non cambia niente)
    voce = unreal.ToolMenuEntry(name="EsportaTempioPonti", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Tempio e ponti: esporta i dati per la chat 3D")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "esporta_tempio_ponti.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (18/11, v0.2.7) le misure del posto di guardia del passo, per rifarlo per la Guardia Reale (nuovi/esporta_passo.py)
    voce = unreal.ToolMenuEntry(name="EsportaPasso", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Passo: esporta le misure del posto di guardia per la chat 3D")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "esporta_passo.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (16/11 sera, v0.2.7 "Il tempio") che cosa c'e' dentro la chiesa e la pieve (nuovi/guarda_tempio.py; non cambia niente)
    voce = unreal.ToolMenuEntry(name="GuardaTempio", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Tempio: guarda cosa c'e' dentro (non cambia niente)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "guarda_tempio.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (16/11, v0.2.6 "Le mura") la grata di prova con la leva e una porta chiusa a chiave (nuovi/metti_grata_prova.py)
    for nome, etichetta, modo in (("GrataProva", "Mura: metti una grata di prova davanti alla telecamera", "metti"),
                                  ("GrataProvaVia", "Mura: togli la grata di prova", "togli")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", (
            "import os, unreal; "
            "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'metti_grata_prova.py'); "
            "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'GRATA_MODO': '%s'})") % modo)
        valdorso.add_menu_entry("Villaggio", voce)

    # (25/11) quale pezzo e', e dove sta: scrive i pezzi selezionati e fa una foto (nuovi/dimmi_selezione.py; non cambia niente)
    voce = unreal.ToolMenuEntry(name="DimmiSelezione", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Dimmi cosa ho selezionato (per Claude e la chat 3D)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "dimmi_selezione.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (16/11, v0.2.10 "Casa mia") misura la locanda di Bruno di oggi per la chat 3D (nuovi/misura_locanda.py; non cambia niente)
    voce = unreal.ToolMenuEntry(name="MisuraLocanda", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Locande: misura la locanda di oggi")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "misura_locanda.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (16/11 sera, v0.2.6) le quote del terreno intorno alle mura per le strade delle porte (nuovi/esporta_quote.py)
    voce = unreal.ToolMenuEntry(name="EsportaQuote", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Strade: esporta le quote intorno alle mura (per la chat 3D)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "esporta_quote.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (15/11, v0.2.5) la pelle nuova delle case, primo passo: solo guardare i materiali (nuovi/guarda_case.py)
    voce = unreal.ToolMenuEntry(name="GuardaCase", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Case: guarda i materiali (non cambia niente)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "guarda_case.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (15/11, v0.2.5) gli alberi in citta': la quercia della piazza e gli alberi nei cortili (nuovi/crea_alberi_citta.py)
    for nome, etichetta, modo in (("AlberiCitta", "Alberi in citta': pianta (poi rilancia 'PNG: panche, letti e porte')", "fai"),
                                  ("AlberiCittaVia", "Alberi in citta': togli", "togli")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", (
            "import os, unreal; "
            "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'crea_alberi_citta.py'); "
            "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'ALBERI_MODO': '%s'})") % modo)
        valdorso.add_menu_entry("Villaggio", voce)

    # (14/11) i PNG che si siedono sulle panche, dormono nei letti e aprono le porte (nuovi/giornate_vive.py)
    voce = unreal.ToolMenuEntry(name="GiornateVive", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("PNG: panche, letti e porte")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "giornate_vive.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (12/11, v0.2.1) il .pak a pezzi: le etichette che dividono il gioco in cinque scatole (crea_scatole.py)
    voce = unreal.ToolMenuEntry(name="Scatole", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Pacchetto: prepara le scatole (il .pak a pezzi)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "crea_scatole.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (12/11) le spade di Paragon: Greystone nel nostro colono (porta_armi_greystone.py) e il controllo delle
    # animazioni in numeri per Claude (controlla_animazioni.py: scrive Saved/Claude/animazioni.txt)
    voce = unreal.ToolMenuEntry(name="ArmiGreystone", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Armi: porta le spade di Greystone nel colono")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "porta_armi_greystone.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (17/11, v0.2.7) il collaudo dei passaggi: un manichino attraverso tutte le porte, nelle tre fasi
    # (collauda_passaggi.py: scrive Saved/Claude/collaudo_passaggi.txt e .json)
    voce = unreal.ToolMenuEntry(name="CollaudaPassaggi", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Per Claude: collauda i passaggi (porte e soglie, tre fasi)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "collauda_passaggi.py")
    valdorso.add_menu_entry("Strumenti", voce)

    voce = unreal.ToolMenuEntry(name="ControllaAnimazioni", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Per Claude: controlla le animazioni (numeri)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "controlla_animazioni.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (07/11 sera) i piedi in piazza, il materiale delle rocce, Nanite sui trasparenti
    voce = unreal.ToolMenuEntry(name="SistemaValle", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Valle: sistema (piedi, materiali, Nanite)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "sistema_valle.py")
    valdorso.add_menu_entry("Villaggio", voce)

    # (13/11) dopo la grande prova: le cose che si usano (porte, PNG, pozzo, pietre) sempre caricate, con lo stesso nome
    # nel server e nel gioco (nuovi/sempre_caricati.py; lo fanno da sole anche crea_villaggio e metti_png)
    voce = unreal.ToolMenuEntry(name="SempreCaricati", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Rete: le cose che si usano sempre caricate (dopo ogni generatore)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "sempre_caricati.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (13/11) dopo la prova della 0.2.1: i materiali che nel gioco pacchettizzato uscivano grigi sugli oggetti ripetuti
    # (nuovi/sistema_materiali.py) e i suoni rifatti da Claude (nuovi/aggiorna_suoni.py: uccelli più calmi, le porte)
    voce = unreal.ToolMenuEntry(name="SistemaMateriali", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Pacchetto: sistema i materiali (oggetti ripetuti)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "sistema_materiali.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (13/11) le icone ad acquerello degli oggetti (nuovi/importa_icone.py; le fa Tools/Generatori/icone/icone.py)
    voce = unreal.ToolMenuEntry(name="ImportaIcone", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Icone: importa le icone ad acquerello")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "importa_icone.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (14/11, v0.2.3) le sopracciglia rifatte dalla testa modello Legacy01, e le pulizie dei doppioni con l'"1"
    # (nuovi/rifai_sopracciglia_nuove.py)
    for nome, etichetta, teste, indietro in (
            ("SopraccigliaProva", "Sopracciglia: prova su Padre Anselmo", "['Walter']", "False"),
            ("SopraccigliaTutte", "Sopracciglia: rifai tutte (e togli i doppioni)", "[]", "False"),
            ("SopraccigliaIndietro", "Sopracciglia: torna come prima", "[]", "True")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SOPRACCIGLIA % (teste, indietro))
        valdorso.add_menu_entry("Strumenti", voce)

    # (14/11, v0.2.3 "Armi vere") per controllare le animazioni dei colpi (nuovi/controlla_animazioni.py)
    voce = unreal.ToolMenuEntry(name="ControllaAnimazioniArmi", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Armi: controlla le animazioni dei colpi")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "controlla_animazioni.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (14/11, v0.2.5) l'inventario dei pacchetti di Fab aggiunti al progetto (nuovi/inventario_fab.py), per Claude
    voce = unreal.ToolMenuEntry(name="InventarioFab", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Fab: inventario dei pacchetti (per Claude)")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "inventario_fab.py")
    valdorso.add_menu_entry("Strumenti", voce)

    # (15/11, v0.2.5) la dieta della memoria: i tetti alle texture dei pacchetti (nuovi/dieta_texture.py)
    for nome, etichetta, fai in (("MemoriaGuarda", "Memoria: guarda quanto pesano le texture (non cambia niente)", "False"),
                                 ("MemoriaDieta", "Memoria: metti a dieta le texture dei pacchetti", "True")):
        voce = unreal.ToolMenuEntry(name=nome, type=unreal.MultiBlockType.MENU_ENTRY)
        voce.set_label(etichetta)
        voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", (
            "import os, unreal; "
            "p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), 'Python', 'nuovi', 'dieta_texture.py'); "
            "exec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__name__': '__main__', 'DIETA_FAI': %s})") % fai)
        valdorso.add_menu_entry("Strumenti", voce)

    # (14/11) i colpi di Grux (asce, mazze) e di Aurora (pugnali) nel nostro colono (nuovi/porta_armi_paragon.py)
    voce = unreal.ToolMenuEntry(name="ArmiParagon", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Armi: porta le animazioni di Grux e Aurora")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "porta_armi_paragon.py")
    valdorso.add_menu_entry("Strumenti", voce)

    voce = unreal.ToolMenuEntry(name="AggiornaSuoni", type=unreal.MultiBlockType.MENU_ENTRY)
    voce.set_label("Suoni: reimporta i suoni rifatti da Claude")
    voce.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", COMANDO_SCRIPT % "aggiorna_suoni.py")
    valdorso.add_menu_entry("Strumenti", voce)

    menu_tool.refresh_all_widgets()
    unreal.log("Valdorso: menu pronto")


crea_menu_valdorso()