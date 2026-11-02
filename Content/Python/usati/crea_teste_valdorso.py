# Valdorso - Crea da sole le teste base dei coloni, direttamente in Valdorso. Scritto da Claude il 28/10/2026.
#
# Per ogni nome qui sotto: copia il preset del MetaHuman Creator in /Game/MH_<Nome>, chiede il rig "solo giunzioni"
# e le texture al servizio di Epic (come i due pulsanti del Creator), poi monta la testa (Cinematic) in
# /Game/Unpacked/MH_<Nome>. Le teste già montate si saltano: lo script si può rilanciare.
# Serve essere collegati al servizio MetaHuman (lo si è già, se nel Creator i pulsanti hanno funzionato).
# Ogni testa richiede alcuni minuti e l'editor sembra fermo: è normale. Alla fine, nel Registro output, l'elenco.
# Le teste servono al C++ (ValdorsoAspettoComponent.cpp, TesteUomo e TesteDonna): Claude le aggiunge lì.

import unreal

UOMINI = ["Lorenzo", "Dominic", "Mikel", "Omari"]
DONNE = ["Grace", "Etta", "Celeste", "Sunita"]


def scrivi(testo):
    unreal.log("[Valdorso] Teste: " + testo)


def errore(testo):
    unreal.log_error("[Valdorso] Teste: " + testo)


def monta(sottosistema, nome):
    pulito = nome.replace("-", "")
    montato = "/Game/Unpacked/MH_%s/BP_MH_%s" % (pulito, pulito)
    if unreal.EditorAssetLibrary.does_asset_exist(montato):
        scrivi("%s c'è già (saltata)" % nome)
        return True

    sorgente = "/MetaHumanCharacter/Optional/Presets/%s" % nome
    destinazione = "/Game/MH_%s" % pulito
    if unreal.EditorAssetLibrary.does_asset_exist(destinazione):
        personaggio = unreal.EditorAssetLibrary.load_asset(destinazione)
    elif unreal.EditorAssetLibrary.does_asset_exist(sorgente):
        personaggio = unreal.EditorAssetLibrary.duplicate_asset(sorgente, destinazione)
    else:
        errore("non trovo il preset " + sorgente)
        return False
    if personaggio is None:
        errore("non riesco a preparare " + destinazione)
        return False

    if not sottosistema.try_add_object_to_edit(personaggio):
        errore("non riesco ad aprire " + destinazione)
        return False
    try:
        if not sottosistema.can_build_meta_human(personaggio, False):
            scrivi("%s: chiedo il rig (solo giunzioni)..." % nome)
            rig = unreal.MetaHumanCharacterAutoRiggingRequestParams()
            rig.set_editor_property("rig_type", unreal.MetaHumanRigType.JOINTS_ONLY)
            rig.set_editor_property("blocking", True)
            rig.set_editor_property("report_progress", True)
            sottosistema.request_auto_rigging(personaggio, rig)

            scrivi("%s: chiedo le texture..." % nome)
            texture = unreal.MetaHumanCharacterTextureRequestParams()
            texture.set_editor_property("blocking", True)
            texture.set_editor_property("report_progress", True)
            sottosistema.request_texture_sources(personaggio, texture)
            unreal.EditorAssetLibrary.save_loaded_asset(personaggio)

        if not sottosistema.can_build_meta_human(personaggio, True):
            errore("%s non si può ancora montare (guarda il messaggio sopra)" % nome)
            return False

        scrivi("%s: monto la testa..." % nome)
        parametri = unreal.MetaHumanCharacterEditorBuildParameters()
        parametri.set_editor_property("pipeline_type", unreal.MetaHumanDefaultPipelineType.CINEMATIC)
        sottosistema.build_meta_human(personaggio, parametri)
        unreal.EditorAssetLibrary.save_loaded_asset(personaggio)
    finally:
        sottosistema.remove_object_to_edit(personaggio)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    if unreal.EditorAssetLibrary.does_asset_exist(montato):
        scrivi("%s pronta: %s" % (nome, montato))
        return True
    errore("%s: montata, ma non trovo %s" % (nome, montato))
    return False


def principale():
    sottosistema = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    fatte = {"uomo": [], "donna": []}
    for sesso, nomi in (("uomo", UOMINI), ("donna", DONNE)):
        for nome in nomi:
            try:
                if monta(sottosistema, nome):
                    fatte[sesso].append(nome)
            except Exception as problema:
                errore("%s: %s" % (nome, problema))
    scrivi("FINITO. Uomini: %s. Donne: %s." % (", ".join(fatte["uomo"]) or "nessuno", ", ".join(fatte["donna"]) or "nessuna"))


principale()
