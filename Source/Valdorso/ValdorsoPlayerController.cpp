// Copyright Epic Games, Inc. All Rights Reserved.


#include "ValdorsoPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Valdorso.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "ValdorsoGameInstance.h"
#include "SValdorsoAccesso.h"
#include "SValdorsoPersonaggi.h"
#include "ValdorsoRegole.h"
#include "ValdorsoAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
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
			MostraAnticamera();
			if (Richiesta.Modo == EValdorsoModoAccesso::PrimoIngresso)
			{
				ServerPrimoIngresso(Richiesta.CodiceInvito, Richiesta.Nome, Richiesta.Password);
			}
			else if (Richiesta.Modo == EValdorsoModoAccesso::Rientro)
			{
				ServerRientra(Richiesta.Biglietto);
			}
			else if (Richiesta.Modo == EValdorsoModoAccesso::Recupero)
			{
				ServerRecupera(Richiesta.Nome, Richiesta.CodiceRecupero, Richiesta.Password);
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
		bSenzaPersonaggio = true;
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
			bSenzaPersonaggio = true;
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
	return bAutenticato && !bAttendeCodici && (bPersonaggioScelto || bSenzaPersonaggio) && Super::CanRestartPlayer();
}

void AValdorsoPlayerController::ServerCodiciScritti_Implementation()
{
	if (!bAttendeCodici || !bAutenticato)
	{
		return;
	}
	bAttendeCodici = false;
	if (UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this))
	{
		Archivista->ConfermaCodiciRecupero(AccountId);
	}
	InviaSceltaPersonaggio();
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

void AValdorsoPlayerController::ServerRecupera_Implementation(const FString& Nome, const FString& Codice, const FString& Nuova)
{
	if (Codice.Len() > 64)
	{
		Espelli(TEXT("Richiesta non valida."));
		return;
	}
	if (!PuoChiedere(Nome, Nuova))
	{
		return;
	}
	bRichiestaInCorso = true;
	NomeAccount = Nome;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->RecuperaConCodice(Nome, Codice, Nuova, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::ServerRientra_Implementation(const FString& Biglietto)
{
	if (Biglietto.Len() > 128 || !PuoChiedere(FString(), Biglietto))
	{
		return;
	}
	bRichiestaInCorso = true;
	bRientro = true;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->RientraConBiglietto(Biglietto, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
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
	if (!Esito.Riuscito())
	{
		bRientro = false;
	}
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
				// Prima si salva dov'era il personaggio del collegamento vecchio (al rientro è il caso normale).
				Altro->SalvaPersonaggio();
				Altro->bPersonaggioScelto = false;
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
	// Codici di recupero nuovi: il personaggio nasce dopo che il giocatore li ha scritti.
	bAttendeCodici = Esito.CodiciRecupero.Num() > 0;
	AccountId = Esito.AccountId;
	NomeAccount = Esito.Nome;
	Ruolo = Esito.Ruolo;
	GetWorldTimerManager().ClearTimer(TimerAnticamera);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: %s è entrato nella valle"), *Esito.Nome);

	ClientEsitoAccesso(Esito);

	// Il biglietto per rientrare senza password se il collegamento si interrompe.
	if (Archivista)
	{
		const FString Biglietto = Archivista->CreaBiglietto(AccountId);
		if (!Biglietto.IsEmpty())
		{
			ClientBiglietto(Biglietto, NomeAccount);
		}
	}

	// Poi la scelta del personaggio (dopo i codici di recupero, se ce ne sono di nuovi da scrivere).
	if (!bAttendeCodici)
	{
		InviaSceltaPersonaggio();
	}
}

// ------------------------------------------------------------------------------------------------
// La scelta del personaggio (v0.1.2, passo 3)
// ------------------------------------------------------------------------------------------------

void AValdorsoPlayerController::InviaSceltaPersonaggio(const FString& Messaggio, bool bErrore)
{
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	if (!Archivista || !bAutenticato || bPersonaggioScelto)
	{
		return;
	}
	// Al rientro senza password si torna subito con l'ultimo personaggio.
	if (bRientro)
	{
		bRientro = false;
		const FString Ultimo = Archivista->UltimoPersonaggio(AccountId);
		if (!Ultimo.IsEmpty())
		{
			FaiNascere(Ultimo);
			return;
		}
	}
	ClientSceltaPersonaggio(Archivista->ElencoPersonaggi(AccountId), Messaggio, bErrore);
}

bool AValdorsoPlayerController::PuoChiederePersonaggi()
{
	if (!bAutenticato || bPersonaggioScelto || bAttendeCodici || !UValdorsoArchivista::Di(this))
	{
		return false;
	}
	if (++RichiestePersonaggi > 40)
	{
		Espelli(TEXT("Troppe richieste: ricollegati."));
		return false;
	}
	return true;
}

void AValdorsoPlayerController::ServerScegliPersonaggio_Implementation(const FString& Id)
{
	if (Id.Len() > 64 || !PuoChiederePersonaggi())
	{
		return;
	}
	FaiNascere(Id);
}

void AValdorsoPlayerController::ServerCreaPersonaggio_Implementation(const FString& Nome)
{
	if (Nome.Len() > 64 || !PuoChiederePersonaggi())
	{
		return;
	}
	FString NuovoId;
	const FString Problema = UValdorsoArchivista::Di(this)->CreaPersonaggio(AccountId, Nome, NuovoId);
	if (!Problema.IsEmpty())
	{
		InviaSceltaPersonaggio(Problema, true);
		return;
	}
	InviaSceltaPersonaggio(FString::Printf(TEXT("%s è scritto nel registro."), *ValdorsoRegole::NormalizzaNomePersonaggio(Nome)), false);
}

void AValdorsoPlayerController::ServerCancellaPersonaggio_Implementation(const FString& Id, const FString& Conferma)
{
	if (Id.Len() > 64 || Conferma.Len() > 64 || !PuoChiederePersonaggi())
	{
		return;
	}
	const FString Problema = UValdorsoArchivista::Di(this)->CancellaPersonaggio(AccountId, Id, Conferma, NomeAccount);
	InviaSceltaPersonaggio(Problema.IsEmpty() ? FString(TEXT("Il personaggio è stato cancellato.")) : Problema, !Problema.IsEmpty());
}

void AValdorsoPlayerController::ServerChiediRegistro_Implementation(const FString& IdRegistro)
{
	if (IdRegistro.Len() > 64 || !PuoChiederePersonaggi())
	{
		return;
	}
	const FValdorsoPersonaggio* Dati = UValdorsoArchivista::Di(this)->TrovaPersonaggio(AccountId, IdRegistro);
	if (!Dati)
	{
		InviaSceltaPersonaggio(TEXT("Questo personaggio non c'è più."), true);
		return;
	}
	ClientRegistro(Dati->Id, Dati->Nome, Dati->Registro);
}

void AValdorsoPlayerController::ServerSalvaRegistro_Implementation(const FString& IdRegistro, const FValdorsoRegistro& Proposto, bool bFirma)
{
	if (IdRegistro.Len() > 64 || !bAutenticato || bPersonaggioScelto || bAttendeCodici || !UValdorsoArchivista::Di(this))
	{
		return;
	}
	if (++RichiesteRegistro > 300)
	{
		Espelli(TEXT("Troppe richieste: ricollegati."));
		return;
	}
	FValdorsoRegistro Salvato;
	const FString Errore = UValdorsoArchivista::Di(this)->SalvaRegistro(AccountId, IdRegistro, Proposto, bFirma, Salvato);
	ClientEsitoRegistro(IdRegistro, Errore, Errore.IsEmpty(), Salvato);
}

void AValdorsoPlayerController::ClientRegistro_Implementation(const FString& IdRegistro, const FString& NomePersonaggio, const FValdorsoRegistro& Registro)
{
	// La schermata del Registro arriva al passo 4.2; per ora si scrive nel registro di Unreal.
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Registro di %s: %s"), *NomePersonaggio, Registro.bFirmato ? TEXT("firmato") : TEXT("da compilare"));
}

void AValdorsoPlayerController::ClientEsitoRegistro_Implementation(const FString& IdRegistro, const FString& Messaggio, bool bRiuscito, const FValdorsoRegistro& Salvato)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Registro: %s"), bRiuscito ? (Salvato.bFirmato ? TEXT("firmato") : TEXT("bozza salvata")) : *Messaggio);
}

void AValdorsoPlayerController::FaiNascere(const FString& Id)
{
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	FValdorsoPersonaggio* Dati = Archivista ? Archivista->TrovaPersonaggio(AccountId, Id) : nullptr;
	if (!Dati)
	{
		InviaSceltaPersonaggio(TEXT("Questo personaggio non c'è più."), true);
		return;
	}

	PersonaggioId = Dati->Id;
	bPersonaggioScelto = true;
	UltimoSalvataggio = FPlatformTime::Seconds();
	Dati->UltimoGioco = FDateTime::UtcNow().ToUnixTimestamp();
	Archivista->SalvaPersonaggio(*Dati);
	if (PlayerState)
	{
		// Gli altri vedono il nome del personaggio, mai quello dell'account.
		PlayerState->SetPlayerName(Dati->Nome);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] %s entra come %s"), *NomeAccount, *Dati->Nome);

	// Dove era rimasto (solo se è la stessa mappa e il punto è sensato), altrimenti al punto di partenza.
	UWorld* Mondo = GetWorld();
	AGameModeBase* Modalita = Mondo ? Mondo->GetAuthGameMode() : nullptr;
	const FString MappaQui = UGameplayStatics::GetCurrentLevelName(this, true);
	const bool bDovEra = Dati->bHaPosizione && Dati->Mappa == MappaQui && Dati->Posizione.Z > -50000.f && !Dati->Posizione.ContainsNaN();
	if (Modalita && Modalita->PlayerCanRestart(this))
	{
		if (bDovEra)
		{
			Modalita->RestartPlayerAtTransform(this, FTransform(FRotator(0.f, Dati->Direzione, 0.f), Dati->Posizione));
		}
		if (!GetPawn())
		{
			Modalita->RestartPlayer(this);
		}
	}
	if (!GetPawn())
	{
		// Non è nato: si torna alla scelta invece di restare senza corpo.
		bPersonaggioScelto = false;
		PersonaggioId.Empty();
		InviaSceltaPersonaggio(TEXT("Il personaggio non è riuscito a entrare nella valle: riprova."), true);
		return;
	}

	// Le statistiche com'erano (le prime volte restano quelle di partenza).
	IAbilitySystemInterface* ConAbilita = Cast<IAbilitySystemInterface>(GetPawn());
	UAbilitySystemComponent* Abilita = ConAbilita ? ConAbilita->GetAbilitySystemComponent() : nullptr;
	if (Abilita)
	{
		if (Dati->Salute > 0.f)
		{
			Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(), Dati->Salute);
		}
		if (Dati->Stamina >= 0.f)
		{
			Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute(), Dati->Stamina);
		}
		if (Dati->Mana >= 0.f)
		{
			Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetManaAttribute(), Dati->Mana);
		}
	}

	ClientPersonaggioScelto(Dati->Nome);
	GetWorldTimerManager().SetTimer(TimerSalvataggio, this, &AValdorsoPlayerController::SalvaPersonaggio, 60.f, true);
}

void AValdorsoPlayerController::SalvaPersonaggio()
{
	if (!HasAuthority() || !bPersonaggioScelto)
	{
		return;
	}
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	FValdorsoPersonaggio* Dati = Archivista ? Archivista->TrovaPersonaggio(AccountId, PersonaggioId) : nullptr;
	if (!Dati)
	{
		return;
	}
	const double Ora = FPlatformTime::Seconds();
	Dati->TempoDiGioco += static_cast<int64>(FMath::Max(0.0, Ora - UltimoSalvataggio));
	UltimoSalvataggio = Ora;
	Dati->UltimoGioco = FDateTime::UtcNow().ToUnixTimestamp();

	if (APawn* Corpo = GetPawn())
	{
		Dati->Mappa = UGameplayStatics::GetCurrentLevelName(this, true);
		Dati->Posizione = Corpo->GetActorLocation();
		Dati->Direzione = Corpo->GetActorRotation().Yaw;
		Dati->bHaPosizione = true;
		IAbilitySystemInterface* ConAbilita = Cast<IAbilitySystemInterface>(Corpo);
		if (UAbilitySystemComponent* Abilita = ConAbilita ? ConAbilita->GetAbilitySystemComponent() : nullptr)
		{
			Dati->Salute = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute());
			Dati->Stamina = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute());
			Dati->Mana = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetManaAttribute());
		}
	}
	Archivista->SalvaPersonaggio(*Dati);
}

void AValdorsoPlayerController::PawnLeavingGame()
{
	SalvaPersonaggio();
	Super::PawnLeavingGame();
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
	if (Anticamera.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Anticamera.ToSharedRef());
		}
		Anticamera.Reset();
	}
	ChiudiCodiciRecupero();
	ChiudiSceltaPersonaggio();
	// Server che si spegne o cambia mappa: l'ultimo minuto non va perso.
	SalvaPersonaggio();
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerAnticamera);
		GetWorldTimerManager().ClearTimer(TimerSalvataggio);
	}
	Super::EndPlay(EndPlayReason);
}

void AValdorsoPlayerController::ClientEsitoAccesso_Implementation(const FValdorsoEsitoAccount& Esito)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Accesso: %s"), *Esito.Messaggio);
	switch (Esito.Esito)
	{
	case EValdorsoEsitoAccount::Ok:
		// Il personaggio sta nascendo: via l'anticamera, mouse e tastiera al gioco.
		if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
		{
			Istanza->Entrato();
		}
		ChiudiAnticamera();
		MostraSulloSchermo(Esito.Messaggio, FColor(232, 196, 120));
		if (Esito.CodiciRecupero.Num() > 0)
		{
			MostraCodiciRecupero(Esito.CodiciRecupero);
		}
		break;

	case EValdorsoEsitoAccount::OkDeveCambiarePassword:
	case EValdorsoEsitoAccount::PasswordDebole:
	case EValdorsoEsitoAccount::CredenzialiSbagliate:
		MostraAnticamera();
		if (Anticamera.IsValid())
		{
			Anticamera->MostraCambioPassword(FText::FromString(Esito.Messaggio), Esito.Esito != EValdorsoEsitoAccount::OkDeveCambiarePassword);
			FInputModeUIOnly Modo;
			Modo.SetWidgetToFocus(Anticamera->CampoIniziale());
			SetInputMode(Modo);
			bShowMouseCursor = true;
		}
		break;

	default:
		if (Anticamera.IsValid())
		{
			Anticamera->MostraAttesa(FText::FromString(Esito.Messaggio));
		}
		break;
	}
}

void AValdorsoPlayerController::MostraAnticamera()
{
	if (Anticamera.IsValid())
	{
		return;
	}
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	SAssignNew(Anticamera, SValdorsoAnticamera)
		.OnCambia(FValdorsoSuCambioPassword::CreateLambda([Debole](const FString& Attuale, const FString& Nuova)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ServerCambiaPassword(Attuale, Nuova);
			}
		}));
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->AddViewportWidgetContent(Anticamera.ToSharedRef(), 50);
	}
	SetInputMode(FInputModeUIOnly());
}

void AValdorsoPlayerController::ChiudiAnticamera()
{
	if (!Anticamera.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(Anticamera.ToSharedRef());
	}
	Anticamera.Reset();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AValdorsoPlayerController::MostraCodiciRecupero(const TArray<FString>& Codici)
{
	ChiudiCodiciRecupero();
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	TSharedRef<SValdorsoCodiciRecupero> Schermata = SNew(SValdorsoCodiciRecupero)
		.Codici(Codici)
		.OnFatto(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ChiudiCodiciRecupero();
				Controllore->ServerCodiciScritti();
			}
		}));
	SchermataCodici = Schermata;
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->AddViewportWidgetContent(Schermata, 60);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Schermata->PulsanteIniziale());
	SetInputMode(Modo);
	bShowMouseCursor = true;
}

void AValdorsoPlayerController::ChiudiCodiciRecupero()
{
	if (!SchermataCodici.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SchermataCodici.ToSharedRef());
	}
	SchermataCodici.Reset();
	if (IsLocalController() && !IsActorBeingDestroyed())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void AValdorsoPlayerController::ClientSceltaPersonaggio_Implementation(const TArray<FValdorsoPersonaggioBreve>& Elenco, const FString& Messaggio, bool bErrore)
{
	ChiudiAnticamera();
	MostraSceltaPersonaggio();
	if (SceltaPersonaggio.IsValid())
	{
		SceltaPersonaggio->Aggiorna(Elenco, FText::FromString(Messaggio), bErrore);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(SceltaPersonaggio->FuocoIniziale());
		SetInputMode(Modo);
		bShowMouseCursor = true;
	}
}

void AValdorsoPlayerController::ClientPersonaggioScelto_Implementation(const FString& Nome)
{
	ChiudiSceltaPersonaggio();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	MostraSulloSchermo(FString::Printf(TEXT("%s entra nella valle."), *Nome), FColor(232, 196, 120));
}

void AValdorsoPlayerController::MostraSceltaPersonaggio()
{
	if (SceltaPersonaggio.IsValid())
	{
		return;
	}
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	SAssignNew(SceltaPersonaggio, SValdorsoSceltaPersonaggio)
		.OnScegli(FValdorsoSuPersonaggio::CreateLambda([Debole](const FString& Id)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->SceltaPersonaggio->Attendi(NSLOCTEXT("ValdorsoPersonaggi", "Entrando", "Il Cuore ti riconosce..."));
				Controllore->ServerScegliPersonaggio(Id);
			}
		}))
		.OnCrea(FValdorsoSuPersonaggio::CreateLambda([Debole](const FString& Nome)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->SceltaPersonaggio->Attendi(NSLOCTEXT("ValdorsoPersonaggi", "Scrivendo", "Il sacerdote scrive nel registro..."));
				Controllore->ServerCreaPersonaggio(Nome);
			}
		}))
		.OnCancella(FValdorsoSuCancellaPersonaggio::CreateLambda([Debole](const FString& Id, const FString& Conferma)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->SceltaPersonaggio->Attendi(NSLOCTEXT("ValdorsoPersonaggi", "Cancellando", "Il sacerdote cancella il nome..."));
				Controllore->ServerCancellaPersonaggio(Id, Conferma);
			}
		}))
		.OnEsci(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				// Uscita voluta: niente rientro automatico.
				if (UValdorsoGameInstance* Istanza = Controllore->GetGameInstance<UValdorsoGameInstance>())
				{
					Istanza->DimenticaBiglietto();
				}
				Controllore->ConsoleCommand(TEXT("disconnect"));
			}
		}));
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->AddViewportWidgetContent(SceltaPersonaggio.ToSharedRef(), 55);
	}
}

void AValdorsoPlayerController::ChiudiSceltaPersonaggio()
{
	if (!SceltaPersonaggio.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SceltaPersonaggio.ToSharedRef());
	}
	SceltaPersonaggio.Reset();
}

void AValdorsoPlayerController::ClientBiglietto_Implementation(const FString& Biglietto, const FString& Nome)
{
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->RicordaBiglietto(Biglietto, Nome);
	}
}

void AValdorsoPlayerController::ClientWasKicked_Implementation(const FText& KickReason)
{
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->RicordaMessaggio(KickReason.ToString());
		// Espulsi dal server: niente rientro automatico (si torna dalla schermata, con la password).
		Istanza->DimenticaBiglietto();
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
