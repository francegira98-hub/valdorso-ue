// Copyright Epic Games, Inc. All Rights Reserved.
// Modificato per Valdorso (03/10/2026, v0.1.2 passo 2.3): l'anticamera. Chi arriva sul server non ha ancora
// un personaggio: manda nome e password (o il codice d'invito) solo se la connessione è cifrata, il server li
// controlla con l'archivista e, se vanno bene, lo fa entrare. Se dopo 60 secondi non è entrato, viene scollegato.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoDialoghi.h"
#include "ValdorsoPlayerController.generated.h"

class UInputMappingContext;
class SValdorsoAnticamera;
class SValdorsoSceltaPersonaggio;
class SValdorsoRegistroColono;
class AValdorsoPalcoRegistro;
class UUserWidget;
class SValdorsoPausa;
class SValdorsoImpostazioni;
class SValdorsoBacheca;
class SValdorsoMappa;
class SValdorsoDialogo;
class UValdorsoTaccuinoComponent;
class UValdorsoBarreComponent;
class UValdorsoChatComponent;
class UValdorsoZainoComponent;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AValdorsoPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

public:

	AValdorsoPlayerController();

	/** (07/11) Il taccuino di viaggio e la bussola (ValdorsoTaccuino.h): lo tiene il server, arriva solo a questo giocatore. */
	UPROPERTY(VisibleAnywhere, Category = "Taccuino")
	TObjectPtr<UValdorsoTaccuinoComponent> Taccuino;

	/** (08/11, v0.1.4) Le barre di salute, stamina, mana ed equilibrio e la barra delle azioni (ValdorsoBarre.h): solo su questo PC. */
	UPROPERTY(VisibleAnywhere, Category = "Pericoli")
	TObjectPtr<UValdorsoBarreComponent> Barre;

	/** (08/11, v0.1.4) La chat di prossimità (ValdorsoChat.h). */
	UPROPERTY(VisibleAnywhere, Category = "Pericoli")
	TObjectPtr<UValdorsoChatComponent> Chat;

	/** (09/11, v0.1.4 passo 4.4) L'aggancio del bersaglio e la barra di chi hai colpito (ValdorsoCombattimento.h). */
	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<class UValdorsoAggancioComponent> Aggancio;

	/** (08/11, v0.1.4) Lo zaino, con il peso e le monete (ValdorsoZaino.h): lo tiene il server, arriva solo a questo giocatore. */
	UPROPERTY(VisibleAnywhere, Category = "Pericoli")
	TObjectPtr<UValdorsoZainoComponent> Zaino;

	/** (10/11, v0.1.4) La Gilda del colono: i punti e gli incarichi presi (ValdorsoGilda.h). */
	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<class UValdorsoGildaComponent> Gilda;

	/** (10/11, v0.1.4) La finestra dello staff (F9) e i suoi permessi (ValdorsoStaff.h). */
	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<class UValdorsoStaffComponent> Staff;

	/** (10/11, scheletro della v0.1.5) Le abilita' che salgono usandole e la scheda del personaggio, K (ValdorsoCrescita.h). */
	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<class UValdorsoCrescitaComponent> Crescita;

	/**
	 * (28/10) Solo per le prove nell'editor (console): apre il Registro di Val d'Orso su un colono di prova, con il
	 * palco e la pagina del volto, senza account. Le risposte non si salvano (il server non conosce quel colono).
	 */
	UFUNCTION(Exec)
	void ProvaRegistro();

	/** Chiude il Registro di prova e torna al gioco. */
	void ChiudiProvaRegistro();
	// --- Anticamera: dal client al server ---------------------------------------------------------

	UFUNCTION(Server, Reliable)
	void ServerAccedi(const FString& Nome, const FString& Password);

	UFUNCTION(Server, Reliable)
	void ServerPrimoIngresso(const FString& CodiceInvito, const FString& Nome, const FString& Password);

	// --- La scelta del personaggio (v0.1.2, passo 3) ------------------------------------------------

	UFUNCTION(Server, Reliable)
	void ServerScegliPersonaggio(const FString& IdScelto);

	UFUNCTION(Server, Reliable)
	void ServerCreaPersonaggio(const FString& Nome);

	UFUNCTION(Server, Reliable)
	void ServerCancellaPersonaggio(const FString& IdDaCancellare, const FString& Conferma);

	/** L'elenco dei propri personaggi (dopo l'accesso, e dopo ogni creazione o cancellazione), con un messaggio. */
	UFUNCTION(Client, Reliable)
	void ClientSceltaPersonaggio(const TArray<FValdorsoPersonaggioBreve>& Elenco, const FString& Messaggio, bool bErrore);

	// --- Il Registro di Val d'Orso (v0.1.2, passo 4) -------------------------------------------------

	/** Il registro di un proprio personaggio (per aprirlo, anche a metà). */
	UFUNCTION(Server, Reliable)
	void ServerChiediRegistro(const FString& IdRegistro);

	/** Salva le risposte (bozza) o, con bFirma, firma il registro. */
	UFUNCTION(Server, Reliable)
	void ServerSalvaRegistro(const FString& IdRegistro, const FValdorsoRegistro& Proposto, bool bFirma);

	UFUNCTION(Client, Reliable)
	void ClientRegistro(const FString& IdRegistro, const FString& NomePersonaggio, const FValdorsoRegistro& Registro);

	UFUNCTION(Client, Reliable)
	void ClientEsitoRegistro(const FString& IdRegistro, const FString& Messaggio, bool bRiuscito, const FValdorsoRegistro& Salvato);

	/** Dal Registro (passo 4.2a): "Torna ai personaggi", il server rimanda l'elenco. */
	UFUNCTION(Server, Reliable)
	void ServerTornaAllaScelta();

	/** Il personaggio sta nascendo: via la scelta. */
	UFUNCTION(Client, Reliable)
	void ClientPersonaggioScelto(const FString& Nome);

	/** Server: salva luogo e statistiche del personaggio in gioco (ogni minuto, e quando si esce). */
	void SalvaPersonaggio();

	/** Il giocatore ha scritto i codici di recupero: valgono, e il personaggio nasce. */
	UFUNCTION(Server, Reliable)
	void ServerCodiciScritti();

	/** Password dimenticata: un codice di recupero e la password nuova. */
	UFUNCTION(Server, Reliable)
	void ServerRecupera(const FString& Nome, const FString& Codice, const FString& Nuova);

	/** Rientro senza password dopo un collegamento interrotto, con il biglietto avuto dal server. */
	UFUNCTION(Server, Reliable)
	void ServerRientra(const FString& Biglietto);

	/** Dopo un reset dell'Amministratrice: la password temporanea va cambiata prima di entrare. */
	UFUNCTION(Server, Reliable)
	void ServerCambiaPassword(const FString& Attuale, const FString& Nuova);

	// --- La pausa e l'uscita con attesa (v0.1.2, passo 6) ---------------------------------------------

	/** Il giocatore vuole uscire (0 = ai personaggi, 1 = al menu, 2 = chiude il gioco): il server fa partire l'attesa. */
	UFUNCTION(Server, Reliable)
	void ServerChiediUscita(uint8 Dove);

	/** Il giocatore resta nella valle. */
	UFUNCTION(Server, Reliable)
	void ServerAnnullaUscita();

	/** L'uscita è partita: tra quanti secondi il colono lascia la valle. */
	UFUNCTION(Client, Reliable)
	void ClientContoUscita(uint8 Dove, int32 Secondi);

	/** L'uscita è annullata, con il motivo (vuoto = l'ha annullata il giocatore). */
	UFUNCTION(Client, Reliable)
	void ClientUscitaAnnullata(const FString& Motivo);

	/** L'attesa è finita e il personaggio è salvato: il client va dove aveva chiesto. */
	UFUNCTION(Client, Reliable)
	void ClientUscitaFatta(uint8 Dove);

	// --- Le interazioni (v0.1.3): il tasto E -----------------------------------------------------------

	/** Il colono vuole usare una cosa (porta, poi panche, letti...): il server controlla la distanza e decide. */
	UFUNCTION(Server, Reliable)
	void ServerInteragisci(AActor* Cosa);

	virtual void PlayerTick(float DeltaTime) override;

	/** Il server manda gli avvisi della bacheca letta: il PC li mostra su una pergamena. */
	UFUNCTION(Client, Reliable)
	void ClientMostraBacheca(const FString& Titolo, const TArray<FString>& Avvisi);

	/** (06/11) Un avviso breve in alto sullo schermo ("Il sigillo della Corona ti respinge."); con SecondiFermo > 0 il
	 *  colono resta fermo per quei secondi (un breve stordimento). */
	UFUNCTION(Client, Reliable)
	void ClientAvviso(const FText& Testo, float Secondi, float SecondiFermo);

	/** (08/11) Vero se c'è una schermata aperta (anche la modalità foto, il taccuino, lo zaino, la tessera o la riga
	 *  della chat). (09/11) Pubblica: la usa anche la tessera (ValdorsoTesseraSchermo::Apri). */
	bool SchermataAperta() const;

	/** (07/11) Il dialogo con un PNG: il server manda la battuta di adesso (bChiudi = via la pergamena). */
	UFUNCTION(Client, Reliable)
	void ClientMostraDialogo(AActor* Chi, const FValdorsoBattuta& Battuta);

	/** (07/11) La risposta scelta alla battuta numero Passo (INDEX_NONE = il colono se ne va). */
	UFUNCTION(Server, Reliable)
	void ServerScegliDialogo(AActor* Chi, int32 Passo, int32 Scelta);

	// --- Anticamera: dal server al client ---------------------------------------------------------

	UFUNCTION(Client, Reliable)
	void ClientEsitoAccesso(const FValdorsoEsitoAccount& Esito);

	/** Il biglietto per rientrare senza password se il collegamento si interrompe (il gioco lo tiene solo in memoria). */
	UFUNCTION(Client, Reliable)
	void ClientBiglietto(const FString& Biglietto, const FString& Nome);

	virtual void ClientWasKicked_Implementation(const FText& KickReason) override;

	// --- Server ------------------------------------------------------------------------------------

	/** Chiamata dalla modalità di gioco quando il giocatore arriva: anticamera, accesso libero di prova o espulsione. */
	void Accogli();

	/** Scollega il giocatore con un motivo che vedrà nel menu. */
	void Espelli(const FString& Motivo);

	bool EAutenticato() const { return bAutenticato; }
	const FString& GetAccountId() const { return AccountId; }
	EValdorsoRuolo GetRuolo() const { return Ruolo; }
	/** (10/11) Il nome dell'account (per i permessi dello staff: ValdorsoStaff.h). */
	const FString& GetNomeAccount() const { return NomeAccount; }

	/** Il personaggio nasce solo dopo l'accesso (e la scelta del personaggio). */
	virtual bool CanRestartPlayer() override;

	/** Prima che il personaggio lasci il mondo si salva dov'era. */
	virtual void PawnLeavingGame() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool ConnessioneCifrata() const;
	FString Indirizzo() const;
	bool PuoChiedere(const FString& Nome, const FString& Segreto);
	void RispostaArchivista(const FValdorsoEsitoAccount& Esito);
	void FaiEntrare(const FValdorsoEsitoAccount& Esito);
	void TempoScaduto();
	void MostraSulloSchermo(const FString& Testo, const FColor& Colore) const;

	/** Client: la schermata dell'anticamera (attesa e password nuova), finché il personaggio non nasce. */
	void MostraAnticamera();
	void ChiudiAnticamera();
	TSharedPtr<SValdorsoAnticamera> Anticamera;

	/** Client: i codici di recupero appena nati, da scrivere (si mostrano una volta sola). */
	void MostraCodiciRecupero(const TArray<FString>& Codici);
	void ChiudiCodiciRecupero();
	TSharedPtr<SWidget> SchermataCodici;

	/** Vero per chi gioca in locale; falso per chi arriva da fuori finché non entra (lo decide Accogli). */
	bool bAutenticato = true;
	bool bRichiestaInCorso = false;
	bool bDeveCambiarePassword = false;
	/** Server: si aspetta che il giocatore scriva i codici di recupero prima di far nascere il personaggio. */
	bool bAttendeCodici = false;

	/** Server: il personaggio scelto (passo 3). Chi gioca in locale o nell'editor non ne ha bisogno. */
	FString PersonaggioId;
	bool bPersonaggioScelto = false;
	bool bSenzaPersonaggio = false;
	/** Server: entrato con il biglietto del rientro (torna con l'ultimo personaggio, senza scelta). */
	bool bRientro = false;
	int32 RichiestePersonaggi = 0;
	/** Le bozze del registro si salvano spesso (una per pagina): hanno il loro conto. */
	int32 RichiesteRegistro = 0;
	double UltimoSalvataggio = 0.0;
	FTimerHandle TimerSalvataggio;

	/** Server: manda l'elenco dei personaggi (o, al rientro, fa nascere subito l'ultimo). */
	void InviaSceltaPersonaggio(const FString& Messaggio = FString(), bool bErrore = false);
	void FaiNascere(const FString& Id);
	bool PuoChiederePersonaggi();

	/** Client: la schermata della scelta. */
	void MostraSceltaPersonaggio();
	void ChiudiSceltaPersonaggio();
	TSharedPtr<SValdorsoSceltaPersonaggio> SceltaPersonaggio;

	/** Client: il Registro di Val d'Orso a schermo (passo 4.2a) e l'Id del personaggio che si sta scrivendo. */
	void MostraRegistro(const FString& IdRegistro, const FString& NomePersonaggio, const FValdorsoRegistro& Risposte);
	void ChiudiRegistro();
	TSharedPtr<SValdorsoRegistroColono> SchermataRegistro;
	FString RegistroAperto;
	/** Client: il palco con il ritratto del colono (passo 4.2b), solo su questo PC; lo usano Registro e scelta. */
	TWeakObjectPtr<AValdorsoPalcoRegistro> Palco;
	AValdorsoPalcoRegistro* AssicuraPalco();
	/** Il palco si toglie quando non lo usa più nessuna schermata. */
	void ForseTogliPalco();
	int32 RichiesteFatte = 0;
	FString AccountId;
	FString NomeAccount;
	EValdorsoRuolo Ruolo = EValdorsoRuolo::Giocatore;
	FTimerHandle TimerAnticamera;

	/** Client: la pausa (Esc, F10 nell'editor, Start sul controller). */
	void TastoPausa();
	void ApriPausa();
	void ChiudiPausa();
	TSharedPtr<SValdorsoPausa> Pausa;
	/** Client: il tasto E e il suggerimento in basso ("E · Apri la porta"). */
	void TastoInteragisci();
	void AggiornaSuggerimento();
	TWeakObjectPtr<AActor> CosaVicina;
	FText TestoSuggerimento;
	TSharedPtr<SWidget> Suggerimento;
	void ChiudiBacheca();
	/** (06/11) Client: la mappa di carta (tasto M). */
	void TastoMappa();
	/** (07/11) Client: la modalità foto (tasto P; ValdorsoModoFoto.h). */
	void TastoFoto();
	/** (07/11) R: il quaderno delle rune (l'alfabeto e le parole delle Cronache), nella bacheca. */
	void TastoQuaderno();
	/** (07/11) J: il taccuino di viaggio (luoghi, iscrizioni, rune). */
	void TastoTaccuino();
	/** (08/11) I: lo zaino. */
	void TastoZaino();
	/** (09/11) Tab, il tasto centrale del mouse, la levetta destra premuta: aggancia chi hai davanti, o sgancia. */
	void TastoAggancio();
	/** (09/11) Il tasto Z: ripone le armi (spada al fianco, scudo sulla schiena) o le riprende in mano. */
	void TastoRiponi();
	/** (10/11, v0.1.4) Sott'acqua: Ctrl scende, Spazio risale (ogni fotogramma, solo su questo PC). */
	void Immersione();
	/** (10/11) F9: la finestra dello staff. */
	void TastoStaff();
	/** (10/11) K: la scheda del personaggio (le abilita'). */
	void TastoScheda();
	/** (11/11, v0.1.5) G: la guardia dopo (ValdorsoScherma.h). */
	void TastoGuardia();
	/** (11/11, v0.1.5) I tasti del colono (da ValdorsoTasti.h): li collega SetupInputComponent, e li rifa' quando cambiano. */
	void CollegaTasti();
	void RifaiTasti();
	FDelegateHandle ManigliaTasti;
	/** I contesti dei tasti del corpo messi (quelli di serie, o le copie con i tasti cambiati). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputMappingContext>> ContestiMessi;
	/** (10/11) La telecamera che segue morbida e si allarga appena di corsa (solo su questo PC). */
	void Telecamera(float DeltaTime);
	TWeakObjectPtr<APawn> CorpoTelecamera;
	float CampoBase = 90.f;
	/** (08/11) I tasti 1-8 della barra delle azioni (Indice da 0 a 7) e Invio per la chat: solo in gioco, senza schermate aperte. */
	void TastoAzione(int32 Indice);
	void TastoChat();
	void ChiudiMappa();
	TSharedPtr<SValdorsoMappa> SchermataMappa;
	TSharedPtr<SValdorsoBacheca> SchermataBacheca;
	/** (07/11) Client: il dialogo con un PNG (SValdorsoDialogo); bAvvisaServer = il colono se ne va. */
	void ChiudiDialogo(bool bAvvisaServer);
	TSharedPtr<SValdorsoDialogo> SchermataDialogo;
	TWeakObjectPtr<AActor> DialogoCon;
	int32 DialogoPasso = 0;
	/** Client: l'avviso breve in alto (ClientAvviso) e lo stordimento. */
	void ChiudiAvviso();
	TSharedPtr<SWidget> Avviso;
	FText TestoAvviso;
	FTimerHandle TimerAvviso;
	FTimerHandle TimerFermo;
	bool bFermoPerAvviso = false;
	double ProssimaRicerca = 0.0;
	int32 RichiesteInterazione = 0;
	double InizioConteggioInterazioni = 0.0;
	/** Client: le impostazioni, aperte dalla pausa (passo 6, seconda parte). */
	void ApriImpostazioni();
	void ChiudiImpostazioni();
	TSharedPtr<SValdorsoImpostazioni> SchermataImpostazioni;
	/** Client: il colono è entrato con un account (dalla scelta dei personaggi): si può tornare ai personaggi. */
	bool bColonoDelRegistro = false;

	/** Server: l'uscita con attesa. Si annulla se il colono viene spostato o perde salute. */
	void ControllaUscita();
	void AnnullaUscita(const FString& Motivo);
	void CompletaUscita();
	float SaluteAttuale() const;
	bool bInUscita = false;
	uint8 UscitaDove = 0;
	double UscitaFine = 0.0;
	FVector UscitaPartenza = FVector::ZeroVector;
	float UscitaSalute = 0.f;
	int32 RichiesteUscita = 0;
	FTimerHandle TimerUscita;
};
