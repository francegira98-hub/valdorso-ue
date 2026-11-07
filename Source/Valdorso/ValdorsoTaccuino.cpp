// Valdorso - Bussola e taccuino di viaggio (vedi il .h). Scritto da Claude il 07/11/2026.

#include "ValdorsoTaccuino.h"
#include "SValdorsoTaccuino.h"
#include "Valdorso.h"
#include "ValdorsoCielo.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "ValdorsoTaccuino"

namespace
{
	/** Ogni quanto (secondi) il server guarda se il colono è entrato in un luogo nuovo. */
	constexpr double TaccuinoSecondiControllo = 1.0;

	/** Quante richieste di testi al secondo accetta il server da un giocatore (chi ne manda di più sta barando). */
	constexpr int32 TaccuinoRichiesteAlSecondo = 8;

	/** Su ogni controllore con autorità (server, o da soli) del mondo: Fai(taccuino). Restituisce quanti ne ha trovati. */
	int32 TaccuinoPerOgniGiocatore(UWorld* Mondo, TFunctionRef<void(UValdorsoTaccuinoComponent&)> Fai)
	{
		int32 Trovati = 0;
		if (!Mondo)
		{
			return 0;
		}
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Controllore = It->Get();
			UValdorsoTaccuinoComponent* Diario = UValdorsoTaccuinoComponent::Di(Controllore);
			if (Diario && Controllore->HasAuthority())
			{
				Fai(*Diario);
				++Trovati;
			}
		}
		return Trovati;
	}

	void TaccuinoScopriTutto(UWorld* Mondo)
	{
		const int32 Trovati = TaccuinoPerOgniGiocatore(Mondo, [](UValdorsoTaccuinoComponent& Diario)
		{
			for (const FValdorsoLuogo& Luogo : ValdorsoLuoghi::Tutti())
			{
				Diario.Scopri(Luogo.Nome, false);
			}
		});
		UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Taccuino: %s"), Trovati > 0
			? TEXT("tutti i luoghi della valle sono scoperti")
			: TEXT("nessun giocatore (il comando vale sul server, o giocando da soli)"));
	}

	void TaccuinoDimentica(UWorld* Mondo)
	{
		const int32 Trovati = TaccuinoPerOgniGiocatore(Mondo, [](UValdorsoTaccuinoComponent& Diario) { Diario.Dimentica(); });
		UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Taccuino: %s"), Trovati > 0
			? TEXT("svuotato")
			: TEXT("nessun giocatore (il comando vale sul server, o giocando da soli)"));
	}

	FAutoConsoleCommandWithWorld ComandoTaccuinoScopriTutto(
		TEXT("Valdorso.Taccuino.ScopriTutto"),
		TEXT("Per le prove: segna come scoperti tutti i luoghi della valle nel taccuino di ogni giocatore (sul server, o da soli)."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&TaccuinoScopriTutto));

	FAutoConsoleCommandWithWorld ComandoTaccuinoDimentica(
		TEXT("Valdorso.Taccuino.Dimentica"),
		TEXT("Per le prove: svuota il taccuino di ogni giocatore (luoghi e iscrizioni; sul server, o da soli)."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&TaccuinoDimentica));
}

// ------------------------------------------------------------------------------------------------- il componente

UValdorsoTaccuinoComponent::UValdorsoTaccuinoComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(true);
}

UValdorsoTaccuinoComponent* UValdorsoTaccuinoComponent::Di(const APlayerController* Controllore)
{
	return Controllore ? Controllore->FindComponentByClass<UValdorsoTaccuinoComponent>() : nullptr;
}

void UValdorsoTaccuinoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// solo al giocatore che ne è il padrone: gli altri non sanno dove è stato né cosa ha letto
	DOREPLIFETIME_CONDITION(UValdorsoTaccuinoComponent, Luoghi, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UValdorsoTaccuinoComponent, Titoli, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UValdorsoTaccuinoComponent, Revisione, COND_OwnerOnly);
}

void UValdorsoTaccuinoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	if (!Controllore || !Mondo)
	{
		return;
	}
	if (Controllore->HasAuthority())
	{
		const double Adesso = Mondo->GetTimeSeconds();
		if (Adesso >= ProssimoControllo)
		{
			ProssimoControllo = Adesso + TaccuinoSecondiControllo;
			ControllaLuoghi();
		}
	}
	// la bussola si mette quando c'è lo schermo (sul server dedicato mai)
	if (!Bussola.IsValid() && Controllore->IsLocalController())
	{
		MettiBussola();
	}
}

void UValdorsoTaccuinoComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// il mondo se ne va: via la pergamena e la bussola dallo schermo (il modo dei tasti lo rimette chi resta)
	if (Schermata.IsValid())
	{
		UWorld* Mondo = GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Schermata.ToSharedRef());
		}
		Schermata.Reset();
	}
	TogliBussola();
	Super::EndPlay(EndPlayReason);
}

// --- Sul server --------------------------------------------------------------------------------------

void UValdorsoTaccuinoComponent::Carica(const FValdorsoTaccuino& Salvato)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	FValdorsoTaccuino Pulito = Salvato;
	ValdorsoTaccuino::Sistema(Pulito);
	Luoghi = MoveTemp(Pulito.Luoghi);
	Iscrizioni = MoveTemp(Pulito.Iscrizioni);
	AggiornaTitoli();
	// i luoghi dove si trova adesso si guardano subito
	ProssimoControllo = 0.0;
}

FValdorsoTaccuino UValdorsoTaccuinoComponent::Dati() const
{
	FValdorsoTaccuino Fuori;
	Fuori.Luoghi = Luoghi;
	Fuori.Iscrizioni = Iscrizioni;
	return Fuori;
}

void UValdorsoTaccuinoComponent::AnnotaIscrizione(const FString& Titolo, const TArray<FString>& Righe)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	FValdorsoTaccuino Lavoro;
	Lavoro.Iscrizioni = MoveTemp(Iscrizioni);
	const EValdorsoLettura Esito = ValdorsoTaccuino::AnnotaIscrizione(Lavoro, Titolo, Righe, FDateTime::UtcNow().ToUnixTimestamp());
	Iscrizioni = MoveTemp(Lavoro.Iscrizioni);
	if (Esito != EValdorsoLettura::Ignorata)
	{
		AggiornaTitoli();
		Salva();
	}
}

bool UValdorsoTaccuinoComponent::Scopri(const FString& Nome, bool bAvvisa)
{
	AActor* Padrone = GetOwner();
	if (!Padrone || !Padrone->HasAuthority() || !ValdorsoLuoghi::Trova(Nome))
	{
		return false;
	}
	// l'ora della valle, per scrivere "al tramonto" nel taccuino
	float Ora = -1.f;
	if (UWorld* Mondo = GetWorld())
	{
		for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
		{
			Ora = (*It)->Ora();
			break;
		}
	}
	FValdorsoTaccuino Lavoro;
	Lavoro.Luoghi = MoveTemp(Luoghi);
	const bool bNuovo = ValdorsoTaccuino::AggiungiLuogo(Lavoro, Nome, FDateTime::UtcNow().ToUnixTimestamp(), Ora);
	Luoghi = MoveTemp(Lavoro.Luoghi);
	if (!bNuovo)
	{
		return false;
	}
	++NumeroCambiamenti;
	AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(Padrone);
	APawn* Colono = Controllore ? Controllore->GetPawn() : nullptr;
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Taccuino: %s scopre %s"), Controllore && Controllore->PlayerState
		? *Controllore->PlayerState->GetPlayerName() : TEXT("?"), *Nome);
	if (Colono)
	{
		ValdorsoRegistroEventi::Annota(TEXT("ScopreLuogo"), Colono, nullptr, Colono->GetActorLocation(), Nome);
	}
	if (bAvvisa && Controllore)
	{
		Controllore->ClientAvviso(FText::Format(LOCTEXT("Scoperto", "Luogo scoperto: {0}"), FText::FromString(Nome)), 4.f, 0.f);
	}
	Salva();
	return true;
}

void UValdorsoTaccuinoComponent::Dimentica()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Luoghi.Reset();
	Iscrizioni.Reset();
	AggiornaTitoli();
	Salva();
}

void UValdorsoTaccuinoComponent::ControllaLuoghi()
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Colono = Controllore ? Controllore->GetPawn() : nullptr;
	if (!Colono)
	{
		return;
	}
	// il server guarda dov'è davvero il corpo: il client non dice mai "ho scoperto"
	for (const FValdorsoLuogo* Luogo : ValdorsoLuoghi::Intorno(Colono->GetActorLocation()))
	{
		Scopri(Luogo->Nome, true);
	}
}

void UValdorsoTaccuinoComponent::Salva()
{
	// Il personaggio si salva con il taccuino dentro (AValdorsoPlayerController::SalvaPersonaggio lo prende da Dati);
	// chi gioca senza account non ha un file: il taccuino resta finché resta nella valle.
	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(GetOwner()))
	{
		Controllore->SalvaPersonaggio();
	}
}

void UValdorsoTaccuinoComponent::AggiornaTitoli()
{
	Titoli.Reset(Iscrizioni.Num());
	for (const FValdorsoIscrizioneLetta& Iscrizione : Iscrizioni)
	{
		Titoli.Add(Iscrizione.Titolo);
	}
	++Revisione;
	++NumeroCambiamenti;
}

void UValdorsoTaccuinoComponent::ServerChiediIscrizione_Implementation(int32 Indice)
{
	// al massimo qualche richiesta al secondo, e solo per un'iscrizione che c'è
	const double Adesso = FPlatformTime::Seconds();
	if (Adesso - InizioRichieste > 1.0)
	{
		InizioRichieste = Adesso;
		RichiesteFatte = 0;
	}
	if (++RichiesteFatte > TaccuinoRichiesteAlSecondo || !Iscrizioni.IsValidIndex(Indice))
	{
		return;
	}
	ClientIscrizione(Indice, Revisione, Iscrizioni[Indice].Righe);
}

// --- Sul PC del giocatore ----------------------------------------------------------------------------

const TArray<FString>* UValdorsoTaccuinoComponent::RigheIscrizione(int32 Indice)
{
	if (!Titoli.IsValidIndex(Indice))
	{
		return nullptr;
	}
	// giocando da soli (o sul server con lo schermo) il testo è già qui
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return Iscrizioni.IsValidIndex(Indice) ? &Iscrizioni[Indice].Righe : nullptr;
	}
	if (const FTestoRicevuto* Ricevuto = TestiRicevuti.Find(Indice))
	{
		if (Ricevuto->Revisione == Revisione)
		{
			return &Ricevuto->Righe;
		}
	}
	// una richiesta sola per volta; se la risposta non arriva (il server ne accetta poche al secondo) si richiede
	const double Adesso = FPlatformTime::Seconds();
	const double* Chiesto = TestiChiesti.Find(Indice);
	if (!Chiesto || Adesso - *Chiesto > 2.0)
	{
		TestiChiesti.Add(Indice, Adesso);
		ServerChiediIscrizione(Indice);
	}
	return nullptr;
}

void UValdorsoTaccuinoComponent::ClientIscrizione_Implementation(int32 Indice, int32 RevisioneTesto, const TArray<FString>& Righe)
{
	FTestoRicevuto& Ricevuto = TestiRicevuti.FindOrAdd(Indice);
	Ricevuto.Revisione = RevisioneTesto;
	Ricevuto.Righe = Righe;
	TestiChiesti.Remove(Indice);
	++NumeroCambiamenti;
}

void UValdorsoTaccuinoComponent::OnRep_Taccuino()
{
	// il taccuino è cambiato sul server: i testi vecchi non valgono più (si richiedono quando servono)
	TestiChiesti.Reset();
	++NumeroCambiamenti;
}

void UValdorsoTaccuinoComponent::MettiBussola()
{
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
	if (!Controllore || !Viewport)
	{
		return;
	}
	Bussola = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 16.f, 0.f, 0.f))
		.Visibility(EVisibility::HitTestInvisible)
		[
			SNew(SValdorsoBussola)
			.Controllore(Controllore)
			.Taccuino(this)
		];
	// sotto il suggerimento (40) e l'avviso (45), sotto tutte le schermate
	Viewport->AddViewportWidgetContent(Bussola.ToSharedRef(), 35);
}

void UValdorsoTaccuinoComponent::TogliBussola()
{
	if (!Bussola.IsValid())
	{
		return;
	}
	UWorld* Mondo = GetWorld();
	if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(Bussola.ToSharedRef());
	}
	Bussola.Reset();
}

// ------------------------------------------------------------------------------------------------- aprire e chiudere

namespace ValdorsoTaccuino
{
	bool Aperto(const APlayerController* Controllore)
	{
		const UValdorsoTaccuinoComponent* Diario = UValdorsoTaccuinoComponent::Di(Controllore);
		return Diario && Diario->Schermata.IsValid();
	}

	bool Apri(APlayerController* Controllore)
	{
		UValdorsoTaccuinoComponent* Diario = UValdorsoTaccuinoComponent::Di(Controllore);
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (!Diario || !Viewport || !Controllore->IsLocalController() || !Controllore->GetPawn() || Diario->Schermata.IsValid()
			|| ValdorsoFoto::Aperta(Controllore))
		{
			return false;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Diario->Schermata, SValdorsoTaccuino)
			.Controllore(Controllore)
			.Taccuino(Diario)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		Viewport->AddViewportWidgetContent(Diario->Schermata.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Diario->Schermata);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
		return true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoTaccuinoComponent* Diario = UValdorsoTaccuinoComponent::Di(Controllore);
		if (!Diario || !Diario->Schermata.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoTaccuino> Schermo = Diario->Schermata.ToSharedRef();
		Diario->Schermata.Reset();
		UWorld* Mondo = Controllore->GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Schermo);
		}
		// il taccuino si apre solo quando non c'è nient'altro: chiudendolo si torna al gioco
		Controllore->SetInputMode(FInputModeGameOnly());
		Controllore->bShowMouseCursor = false;
	}

	void Alterna(APlayerController* Controllore)
	{
		if (Aperto(Controllore))
		{
			Chiudi(Controllore);
		}
		else
		{
			Apri(Controllore);
		}
	}

	void AnnotaLettura(APlayerController* Controllore, const FString& Titolo, const TArray<FString>& Righe)
	{
		if (UValdorsoTaccuinoComponent* Diario = UValdorsoTaccuinoComponent::Di(Controllore))
		{
			Diario->AnnotaIscrizione(Titolo, Righe);
		}
	}
}

#undef LOCTEXT_NAMESPACE
