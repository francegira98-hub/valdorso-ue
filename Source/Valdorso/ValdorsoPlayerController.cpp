// Copyright Epic Games, Inc. All Rights Reserved.


#include "ValdorsoPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Valdorso.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "ValdorsoGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

namespace
{
	/** Secondi per entrare dopo il collegamento. */
	constexpr float SecondiAnticamera = 60.f;

	/** Richieste massime nell'anticamera prima di essere scollegati. */
	constexpr int32 RichiesteMassime = 5;
}

void AValdorsoPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogValdorso, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	// Valdorso, anticamera: appena arrivato sul server, il client manda la sua richiesta (solo su connessione cifrata).
	if (IsLocalPlayerController() && GetNetMode() == NM_Client)
	{
		UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
		FValdorsoRichiestaAccesso Richiesta;
		if (Istanza && Istanza->PrendiRichiesta(Richiesta))
		{
			if (!ConnessioneCifrata())
			{
				Istanza->RicordaMessaggio(TEXT("La connessione non è cifrata: la password non è partita."));
				ConsoleCommand(TEXT("disconnect"));
				return;
			}
			MostraSulloSchermo(TEXT("Il Cuore ti sta riconoscendo..."), FColor(232, 196, 120));
			if (Richiesta.Modo == EValdorsoModoAccesso::PrimoIngresso)
			{
				ServerPrimoIngresso(Richiesta.CodiceInvito, Richiesta.Nome, Richiesta.Password);
			}
			else
			{
				ServerAccedi(Richiesta.Nome, Richiesta.Password);
			}
		}
	}
}

void AValdorsoPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AValdorsoPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

// ------------------------------------------------------------------------------------------------
// Valdorso: l'anticamera
// ------------------------------------------------------------------------------------------------

bool AValdorsoPlayerController::ConnessioneCifrata() const
{
	if (GetNetMode() == NM_Client)
	{
		const UNetDriver* Driver = GetWorld() ? GetWorld()->GetNetDriver() : nullptr;
		return Driver && Driver->ServerConnection && Driver->ServerConnection->IsEncryptionEnabled();
	}
	const UNetConnection* Connessione = GetNetConnection();
	return Connessione && Connessione->IsEncryptionEnabled();
}

FString AValdorsoPlayerController::Indirizzo() const
{
	UNetConnection* Connessione = GetNetConnection();
	return Connessione ? Connessione->LowLevelGetRemoteAddress(false) : FString(TEXT("locale"));
}

void AValdorsoPlayerController::Accogli()
{
	// Chi gioca sul PC del server (gioco autonomo o server in ascolto) non passa dall'anticamera.
	if (IsLocalController())
	{
		bAutenticato = true;
		NomeAccount = TEXT("Locale");
		return;
	}

	bAutenticato = false;
	const UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();

	if (!ConnessioneCifrata())
	{
		// Nell'editor (Gioca come client) si entra liberi, per provare il gioco senza account.
		if (GetWorld() && GetWorld()->IsPlayInEditor() && Istanza && Istanza->bAccessoLiberoNellEditor)
		{
			static int32 Contatore = 0;
			bAutenticato = true;
			NomeAccount = FString::Printf(TEXT("Prova%d"), ++Contatore);
			AccountId = TEXT("prova-") + NomeAccount;
			Ruolo = EValdorsoRuolo::Amministratore;
			if (PlayerState)
			{
				PlayerState->SetPlayerName(NomeAccount);
			}
			UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: accesso libero dell'editor come %s"), *NomeAccount);
			return;
		}

		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Anticamera: connessione non cifrata da %s, scollego."), *Indirizzo());
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			Espelli(TEXT("Connessione non cifrata: aggiorna il gioco."));
		}));
		return;
	}

	GetWorldTimerManager().SetTimer(TimerAnticamera, this, &AValdorsoPlayerController::TempoScaduto, SecondiAnticamera, false);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: arrivato %s (connessione cifrata)"), *Indirizzo());
}

bool AValdorsoPlayerController::CanRestartPlayer()
{
	return bAutenticato && Super::CanRestartPlayer();
}

bool AValdorsoPlayerController::PuoChiedere(const FString& Nome, const FString& Segreto)
{
	if (bAutenticato || bRichiestaInCorso)
	{
		return false;
	}
	if (!ConnessioneCifrata())
	{
		Espelli(TEXT("Connessione non cifrata."));
		return false;
	}
	if (++RichiesteFatte > RichiesteMassime)
	{
		Espelli(TEXT("Troppe richieste: ricollegati."));
		return false;
	}
	if (Nome.Len() > 64 || Segreto.Len() > 256)
	{
		Espelli(TEXT("Richiesta non valida."));
		return false;
	}
	if (!UValdorsoArchivista::Di(this))
	{
		Espelli(TEXT("Il server non ha l'archivio degli account."));
		return false;
	}
	return true;
}

void AValdorsoPlayerController::ServerAccedi_Implementation(const FString& Nome, const FString& Password)
{
	if (!PuoChiedere(Nome, Password))
	{
		return;
	}
	bRichiestaInCorso = true;
	NomeAccount = Nome;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->Accedi(Nome, Password, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::ServerPrimoIngresso_Implementation(const FString& CodiceInvito, const FString& Nome, const FString& Password)
{
	if (CodiceInvito.Len() > 64 || !PuoChiedere(Nome, Password))
	{
		return;
	}
	bRichiestaInCorso = true;
	NomeAccount = Nome;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->CreaAccount(CodiceInvito, Nome, Password, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::ServerCambiaPassword_Implementation(const FString& Attuale, const FString& Nuova)
{
	// Si cambia qui solo la password temporanea, prima di entrare (il cambio normale arriverà nelle Impostazioni).
	if (!bDeveCambiarePassword || Nuova.Len() > 256 || !PuoChiedere(NomeAccount, Attuale))
	{
		return;
	}
	bRichiestaInCorso = true;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->CambiaPassword(NomeAccount, Attuale, Nuova, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::RispostaArchivista(const FValdorsoEsitoAccount& Esito)
{
	bRichiestaInCorso = false;
	if (!IsValid(this) || IsActorBeingDestroyed())
	{
		return;
	}

	switch (Esito.Esito)
	{
	case EValdorsoEsitoAccount::Ok:
		FaiEntrare(Esito);
		break;

	case EValdorsoEsitoAccount::OkDeveCambiarePassword:
		// Resta nell'anticamera finché non sceglie una password nuova.
		bDeveCambiarePassword = true;
		NomeAccount = Esito.Nome;
		ClientEsitoAccesso(Esito);
		break;

	case EValdorsoEsitoAccount::PasswordDebole:
	case EValdorsoEsitoAccount::CredenzialiSbagliate:
		if (bDeveCambiarePassword)
		{
			// Durante il cambio della password temporanea si può riprovare (entro i tentativi).
			ClientEsitoAccesso(Esito);
			break;
		}
		Espelli(Esito.Messaggio);
		break;

	default:
		Espelli(Esito.Messaggio);
		break;
	}
}

void AValdorsoPlayerController::FaiEntrare(const FValdorsoEsitoAccount& Esito)
{
	UWorld* Mondo = GetWorld();
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);

	// Un account, un collegamento: se era già dentro da un altro posto, quel collegamento si chiude.
	if (Archivista && !Archivista->SegnaCollegato(Esito.AccountId))
	{
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			AValdorsoPlayerController* Altro = Cast<AValdorsoPlayerController>(It->Get());
			if (Altro && Altro != this && Altro->AccountId == Esito.AccountId)
			{
				Altro->bAutenticato = false;
				Altro->AccountId.Empty();
				Altro->Espelli(TEXT("Il tuo account è entrato da un altro posto."));
			}
		}
		Archivista->SegnaScollegato(Esito.AccountId);
		Archivista->SegnaCollegato(Esito.AccountId);
	}

	bAutenticato = true;
	bDeveCambiarePassword = false;
	AccountId = Esito.AccountId;
	NomeAccount = Esito.Nome;
	Ruolo = Esito.Ruolo;
	GetWorldTimerManager().ClearTimer(TimerAnticamera);
	if (PlayerState)
	{
		PlayerState->SetPlayerName(Esito.Nome);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: %s è entrato nella valle"), *Esito.Nome);

	ClientEsitoAccesso(Esito);

	if (AGameModeBase* Modalita = Mondo ? Mondo->GetAuthGameMode() : nullptr)
	{
		if (Modalita->PlayerCanRestart(this))
		{
			Modalita->RestartPlayer(this);
		}
	}
}

void AValdorsoPlayerController::TempoScaduto()
{
	if (bAutenticato)
	{
		return;
	}
	if (bRichiestaInCorso)
	{
		// Il server sta ancora controllando: qualche secondo in più.
		GetWorldTimerManager().SetTimer(TimerAnticamera, this, &AValdorsoPlayerController::TempoScaduto, 10.f, false);
		return;
	}
	Espelli(TEXT("Tempo scaduto: ricollegati e riprova."));
}

void AValdorsoPlayerController::Espelli(const FString& Motivo)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: scollego %s (%s) - %s"), *NomeAccount, *Indirizzo(), *Motivo);
	AGameModeBase* Modalita = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
	if (Modalita && Modalita->GameSession)
	{
		Modalita->GameSession->KickPlayer(this, FText::FromString(Motivo));
	}
}

void AValdorsoPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerAnticamera);
	}
	Super::EndPlay(EndPlayReason);
}

void AValdorsoPlayerController::ClientEsitoAccesso_Implementation(const FValdorsoEsitoAccount& Esito)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Accesso: %s"), *Esito.Messaggio);
	switch (Esito.Esito)
	{
	case EValdorsoEsitoAccount::Ok:
		MostraSulloSchermo(Esito.Messaggio, FColor(232, 196, 120));
		break;
	case EValdorsoEsitoAccount::OkDeveCambiarePassword:
		// La finestra per cambiarla arriva con la schermata "Prima di entrare" (passo 2.2).
		MostraSulloSchermo(Esito.Messaggio + TEXT("  Per ora dalla console: Valdorso.CambiaPassword <attuale> <nuova>"), FColor::Yellow);
		break;
	default:
		MostraSulloSchermo(Esito.Messaggio, FColor(255, 140, 60));
		break;
	}
}

void AValdorsoPlayerController::ClientWasKicked_Implementation(const FText& KickReason)
{
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->RicordaMessaggio(KickReason.ToString());
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Scollegato dal server: %s"), *KickReason.ToString());
	Super::ClientWasKicked_Implementation(KickReason);
}

void AValdorsoPlayerController::MostraSulloSchermo(const FString& Testo, const FColor& Colore) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, Colore, Testo);
	}
}
