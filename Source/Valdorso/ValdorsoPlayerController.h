// Copyright Epic Games, Inc. All Rights Reserved.
// Modificato per Valdorso (03/10/2026, v0.1.2 passo 2.3): l'anticamera. Chi arriva sul server non ha ancora
// un personaggio: manda nome e password (o il codice d'invito) solo se la connessione è cifrata, il server li
// controlla con l'archivista e, se vanno bene, lo fa entrare. Se dopo 60 secondi non è entrato, viene scollegato.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoPlayerController.generated.h"

class UInputMappingContext;
class SValdorsoAnticamera;
class SValdorsoSceltaPersonaggio;
class SValdorsoRegistroColono;
class AValdorsoPalcoRegistro;
class UUserWidget;

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
};
