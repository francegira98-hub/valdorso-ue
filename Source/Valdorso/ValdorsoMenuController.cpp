// Valdorso - Il controllore del menu principale (vedi il .h).

#include "ValdorsoMenuController.h"
#include "SValdorsoMenuPrincipale.h"
#include "ValdorsoScenaMenu.h"
#include "ValdorsoGameInstance.h"
#include "SValdorsoAccesso.h"
#include "Engine/Engine.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Valdorso.h"

AValdorsoMenuController::AValdorsoMenuController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
}

void AValdorsoMenuController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// La telecamera della scena: quella con l'etichetta MenuCamera, altrimenti la prima che c'è.
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MenuCamera")) || !Telecamera.IsValid())
		{
			Telecamera = *It;
			if (It->ActorHasTag(TEXT("MenuCamera")))
			{
				break;
			}
		}
	}
	if (Telecamera.IsValid())
	{
		PosizioneBase = Telecamera->GetActorLocation();
		RotazioneBase = Telecamera->GetActorRotation();
		SetViewTarget(Telecamera.Get());
	}
	else
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Menu: nessuna telecamera MenuCamera nel livello"));
	}

	// Il menu, sopra la scena.
	SAssignNew(Menu, SValdorsoMenuPrincipale)
		.OnEntra(FSimpleDelegate::CreateWeakLambda(this, [this]() { MostraAccesso(FText::GetEmpty()); }))
		.OnEsci(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::Esci));

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Menu.ToSharedRef(), 10);
	}

	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Menu->GetPrimoPulsante());
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
	bShowMouseCursor = true;

	// Tornando dal server (password sbagliata, espulsione, server spento) si riapre "Prima di entrare" con il motivo.
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		const FString Messaggio = Istanza->PrendiMessaggio();
		if (!Messaggio.IsEmpty())
		{
			UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: %s"), *Messaggio);
			MostraAccesso(FText::FromString(Messaggio));
		}
	}
}

void AValdorsoMenuController::MostraAccesso(const FText& Messaggio)
{
	if (bInTransizione)
	{
		return;
	}
	ChiudiAccesso();

	const UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	SAssignNew(Accesso, SValdorsoPrimaDiEntrare)
		.NomeIniziale(Istanza ? Istanza->UltimoNome : FString())
		.Messaggio(Messaggio)
		.OnRichiesta(FValdorsoSuRichiestaAccesso::CreateUObject(this, &AValdorsoMenuController::SuRichiestaAccesso))
		.OnIndietro(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::ChiudiAccesso))
		.OnProvaLocale(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::ProvaLocale));

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Accesso.ToSharedRef(), 20);
	}
	if (Menu.IsValid())
	{
		Menu->SetEnabled(false);
	}

	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Accesso->CampoIniziale());
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
}

void AValdorsoMenuController::ChiudiAccesso()
{
	if (Accesso.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Accesso.ToSharedRef());
		}
		Accesso.Reset();
	}
	if (Menu.IsValid() && !bInTransizione)
	{
		Menu->SetEnabled(true);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Menu->GetPrimoPulsante());
		Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Modo);
	}
}

void AValdorsoMenuController::SuRichiestaAccesso(const FValdorsoRichiestaAccesso& Richiesta)
{
	UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	FString Errore;
	if (!Istanza || !Istanza->Collegati(Richiesta, Errore))
	{
		if (Accesso.IsValid())
		{
			Accesso->MostraMessaggio(FText::FromString(Errore.IsEmpty() ? TEXT("Il gioco non è pronto: riprova.") : Errore), true);
		}
		return;
	}
	// Il collegamento è partito: mentre Unreal apre la strada verso il server, la telecamera vola verso il frammento.
	bVersoIlServer = true;
	ChiudiAccesso();
	EntraNellaValle();
}

void AValdorsoMenuController::ProvaLocale()
{
	bVersoIlServer = false;
	ChiudiAccesso();
	EntraNellaValle();
}

void AValdorsoMenuController::Arriva()
{
	bLivelloAperto = true;
	// Verso il server il viaggio è già partito (Collegati): il livello lo manda il server.
	if (!bVersoIlServer)
	{
		UGameplayStatics::OpenLevel(this, LivelloValle);
	}
}

void AValdorsoMenuController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ChiudiAccesso();
	if (Menu.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Menu.ToSharedRef());
		}
		Menu.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void AValdorsoMenuController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bInTransizione)
	{
		AvanzaTransizione(DeltaSeconds);
		return;
	}

	// La telecamera respira: si sposta e gira di pochissimo, lentamente.
	if (Telecamera.IsValid())
	{
		Tempo += DeltaSeconds;
		const FVector Spostamento(0.f, FMath::Sin(Tempo * 0.13f) * Ondeggio, FMath::Sin(Tempo * 0.09f) * Ondeggio * 0.4f);
		const FRotator Giro(FMath::Sin(Tempo * 0.11f) * 0.4f, FMath::Sin(Tempo * 0.07f) * 1.2f, 0.f);
		Telecamera->SetActorLocationAndRotation(PosizioneBase + Spostamento, RotazioneBase + Giro);
	}
}

void AValdorsoMenuController::EntraNellaValle()
{
	if (bInTransizione)
	{
		return;
	}
	bInTransizione = true;
	TempoTransizione = 0.f;

	// Si restituiscono mouse e tastiera al gioco; il menu non risponde più e sfuma.
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	if (Menu.IsValid())
	{
		Menu->SetVisibility(EVisibility::HitTestInvisible);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: entra nella valle (%s)"), *LivelloValle.ToString());

	// Il frammento si risveglia e la telecamera gli vola incontro.
	FVector Cuore(0.f, 0.f, 165.f);
	for (TActorIterator<AValdorsoFrammentoCuore> It(GetWorld()); It; ++It)
	{
		Cuore = It->GetActorLocation() + FVector(0.f, 0.f, 75.f);
		It->Risveglia();
		break;
	}

	if (!Telecamera.IsValid())
	{
		Arriva();
		return;
	}
	Partenza = Telecamera->GetActorLocation();
	GiroPartenza = Telecamera->GetActorQuat();
	const FVector Direzione = (Partenza - Cuore).GetSafeNormal2D();
	Arrivo = Cuore + Direzione * 230.f + FVector(0.f, 0.f, 15.f);
	GiroArrivo = (Cuore - Arrivo).Rotation().Quaternion();
}

void AValdorsoMenuController::AvanzaTransizione(float DeltaSeconds)
{
	TempoTransizione += DeltaSeconds;
	const float Durata = FMath::Max(DurataTransizione, 0.5f);
	const float A = FMath::Clamp(TempoTransizione / Durata, 0.f, 1.f);

	// Parte piano e accelera, come chi si avvicina a qualcosa che lo chiama.
	const float Volo = A * A * A;
	const float Sguardo = FMath::InterpEaseInOut(0.f, 1.f, FMath::Min(A * 1.6f, 1.f), 2.f);
	if (Telecamera.IsValid())
	{
		Telecamera->SetActorLocationAndRotation(FMath::Lerp(Partenza, Arrivo, Volo), FQuat::Slerp(GiroPartenza, GiroArrivo, Sguardo));
	}

	// Il menu sfuma nel primo mezzo secondo.
	if (Menu.IsValid())
	{
		Menu->SetRenderOpacity(1.f - FMath::Clamp(TempoTransizione / 0.5f, 0.f, 1.f));
	}

	// Dopo un po' tutto diventa nero.
	if (!bBuio && A >= 0.45f)
	{
		bBuio = true;
		if (PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraFade(0.f, 1.f, Durata * 0.5f, FLinearColor::Black, false, true);
		}
	}

	if (!bLivelloAperto && A >= 1.f)
	{
		Arriva();
	}
}

void AValdorsoMenuController::Esci()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
