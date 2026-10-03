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

	/** Dopo un reset dell'Amministratrice: la password temporanea va cambiata prima di entrare. */
	UFUNCTION(Server, Reliable)
	void ServerCambiaPassword(const FString& Attuale, const FString& Nuova);

	// --- Anticamera: dal server al client ---------------------------------------------------------

	UFUNCTION(Client, Reliable)
	void ClientEsitoAccesso(const FValdorsoEsitoAccount& Esito);

	virtual void ClientWasKicked_Implementation(const FText& KickReason) override;

	// --- Server ------------------------------------------------------------------------------------

	/** Chiamata dalla modalità di gioco quando il giocatore arriva: anticamera, accesso libero di prova o espulsione. */
	void Accogli();

	/** Scollega il giocatore con un motivo che vedrà nel menu. */
	void Espelli(const FString& Motivo);

	bool EAutenticato() const { return bAutenticato; }
	const FString& GetAccountId() const { return AccountId; }
	EValdorsoRuolo GetRuolo() const { return Ruolo; }

	/** Il personaggio nasce solo dopo l'accesso. */
	virtual bool CanRestartPlayer() override;

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

	/** Vero per chi gioca in locale; falso per chi arriva da fuori finché non entra (lo decide Accogli). */
	bool bAutenticato = true;
	bool bRichiestaInCorso = false;
	bool bDeveCambiarePassword = false;
	int32 RichiesteFatte = 0;
	FString AccountId;
	FString NomeAccount;
	EValdorsoRuolo Ruolo = EValdorsoRuolo::Giocatore;
	FTimerHandle TimerAnticamera;
};
