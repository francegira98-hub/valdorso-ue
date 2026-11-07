// Valdorso - La modalità foto (vedi il .h).

#include "ValdorsoModoFoto.h"
#include "ValdorsoTemaUI.h"
#include "Valdorso.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoModoFoto"

namespace
{
	TAutoConsoleVariable<int32> CVarGrandezzaFoto(
		TEXT("Valdorso.Foto.Grandezza"),
		2,
		TEXT("Quante volte lo schermo è grande la foto della modalità foto (1-4; di serie 2: con 1920x1080 viene 3840x2160)."));

	/** Le modalità foto aperte (di solito una: quella del giocatore di questo PC). */
	struct FFotoAperta
	{
		TSharedPtr<SValdorsoFoto> Schermo;
		TWeakObjectPtr<AValdorsoCameraFoto> Camera;
	};
	TMap<TWeakObjectPtr<APlayerController>, FFotoAperta> FotoAperte;

	/** Sotto questo valore la levetta del controller non conta (le levette non tornano mai proprio a zero). */
	float ZonaMortaFoto(float Valore)
	{
		return FMath::Abs(Valore) < 0.15f ? 0.f : Valore;
	}

	void AlternaFotoDalComando(UWorld* Mondo)
	{
		if (!Mondo)
		{
			return;
		}
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Controllore = It->Get();
			if (Controllore && Controllore->IsLocalController())
			{
				if (ValdorsoFoto::Aperta(Controllore))
				{
					ValdorsoFoto::Chiudi(Controllore);
				}
				else
				{
					ValdorsoFoto::Apri(Controllore);
				}
				return;
			}
		}
	}

	FAutoConsoleCommandWithWorld ComandoFoto(
		TEXT("Valdorso.Foto"),
		TEXT("Apre o chiude la modalità foto (come il tasto P)."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&AlternaFotoDalComando));
}

// ------------------------------------------------------------------------------------------------
// Aprire e chiudere
// ------------------------------------------------------------------------------------------------

namespace ValdorsoFoto
{
	bool Aperta(const APlayerController* Controllore)
	{
		const FFotoAperta* Foto = FotoAperte.Find(TWeakObjectPtr<APlayerController>(const_cast<APlayerController*>(Controllore)));
		return Foto && Foto->Camera.IsValid() && Foto->Schermo.IsValid();
	}

	bool Apri(APlayerController* Controllore)
	{
		if (!Controllore || !Controllore->IsLocalController() || Aperta(Controllore))
		{
			return false;
		}
		APawn* Colono = Controllore->GetPawn();
		UWorld* Mondo = Controllore->GetWorld();
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		APlayerCameraManager* Occhio = Controllore->PlayerCameraManager;
		if (!Colono || !Viewport || !Occhio)
		{
			return false;
		}
		// una voce rimasta da una volta precedente (mondo cambiato) si chiude bene
		Chiudi(Controllore);

		FActorSpawnParameters Parametri;
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Parametri.ObjectFlags |= RF_Transient;
		const FVector Dove = Occhio->GetCameraLocation();
		const FRotator Verso = Occhio->GetCameraRotation();
		AValdorsoCameraFoto* Camera = Mondo->SpawnActor<AValdorsoCameraFoto>(AValdorsoCameraFoto::StaticClass(), Dove, Verso, Parametri);
		if (!Camera)
		{
			return false;
		}
		Camera->Prepara(Colono, Dove, Verso, Occhio->GetFOVAngle());
		Controllore->SetViewTargetWithBlend(Camera, 0.25f);

		TWeakObjectPtr<APlayerController> Debole(Controllore);
		TSharedRef<SValdorsoFoto> Schermo = SNew(SValdorsoFoto)
			.Camera(Camera)
			.OnEsci(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		Viewport->AddViewportWidgetContent(Schermo, 70);

		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Schermo);
		Modo.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = false;

		FFotoAperta Nuova;
		Nuova.Schermo = Schermo;
		Nuova.Camera = Camera;
		FotoAperte.Add(TWeakObjectPtr<APlayerController>(Controllore), Nuova);
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Modalità foto aperta"));
		return true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		if (!Controllore)
		{
			return;
		}
		const TWeakObjectPtr<APlayerController> Chiave(Controllore);
		FFotoAperta* Foto = FotoAperte.Find(Chiave);
		if (!Foto)
		{
			return;
		}
		const FFotoAperta Copia = *Foto;
		FotoAperte.Remove(Chiave);

		// prima si torna al colono (con la dissolvenza), poi la telecamera sparisce da sola dopo la dissolvenza
		if (APawn* Colono = Controllore->GetPawn())
		{
			Controllore->SetViewTargetWithBlend(Colono, 0.25f);
		}
		if (AValdorsoCameraFoto* Camera = Copia.Camera.Get())
		{
			Camera->MostraColono();
			Camera->Comandi = FValdorsoComandiFoto();
			Camera->SetLifeSpan(0.3f);
		}
		UWorld* Mondo = Controllore->GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			if (Copia.Schermo.IsValid())
			{
				Viewport->RemoveViewportWidgetContent(Copia.Schermo.ToSharedRef());
			}
		}
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture();
		}
		Controllore->SetInputMode(FInputModeGameOnly());
		Controllore->bShowMouseCursor = false;
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Modalità foto chiusa"));
	}

	FVector DentroIlRaggio(const FVector& Centro, const FVector& Voluto, float Raggio)
	{
		const FVector Distanza = Voluto - Centro;
		if (Raggio <= 0.f || Distanza.SizeSquared() <= FMath::Square(Raggio))
		{
			return Voluto;
		}
		return Centro + Distanza.GetSafeNormal() * Raggio;
	}

	float CampoDopoZoom(float Campo, float Scatti)
	{
		// ogni scatto stringe o allarga del 7%: a ogni livello di zoom lo scatto si sente uguale
		return FMath::Clamp(Campo * FMath::Pow(0.93f, Scatti), 15.f, 100.f);
	}
}

// ------------------------------------------------------------------------------------------------
// La telecamera
// ------------------------------------------------------------------------------------------------

AValdorsoCameraFoto::AValdorsoCameraFoto()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bReplicates = false;
	SetCanBeDamaged(false);
	if (UCameraComponent* Obiettivo = GetCameraComponent())
	{
		Obiettivo->bConstrainAspectRatio = false;
		Obiettivo->PostProcessBlendWeight = 1.f;
	}
}

void AValdorsoCameraFoto::Prepara(APawn* InColono, const FVector& Dove, const FRotator& Verso, float Campo)
{
	Colono = InColono;
	Imbardata = Verso.Yaw;
	Beccheggio = FMath::Clamp(FRotator::NormalizeAxis(Verso.Pitch), -85.f, 85.f);
	Rollio = 0.f;
	SetActorLocationAndRotation(Dove, FRotator(Beccheggio, Imbardata, Rollio));
	if (UCameraComponent* Obiettivo = GetCameraComponent())
	{
		Obiettivo->SetFieldOfView(FMath::Clamp(Campo, 15.f, 100.f));
	}
}

float AValdorsoCameraFoto::CampoVisivo() const
{
	const UCameraComponent* Obiettivo = GetCameraComponent();
	return Obiettivo ? Obiettivo->FieldOfView : 90.f;
}

void AValdorsoCameraFoto::RimettiDritto()
{
	Rollio = 0.f;
	if (UCameraComponent* Obiettivo = GetCameraComponent())
	{
		Obiettivo->SetFieldOfView(90.f);
	}
}

void AValdorsoCameraFoto::AlternaSfocatura()
{
	bSfocatura = !bSfocatura;
	if (UCameraComponent* Obiettivo = GetCameraComponent())
	{
		FPostProcessSettings& Effetti = Obiettivo->PostProcessSettings;
		Effetti.bOverride_DepthOfFieldFocalDistance = bSfocatura;
		Effetti.bOverride_DepthOfFieldFstop = bSfocatura;
		Effetti.DepthOfFieldFstop = 1.8f;
		if (bSfocatura)
		{
			MettiAFuoco();
		}
	}
}

void AValdorsoCameraFoto::MettiAFuoco()
{
	UCameraComponent* Obiettivo = GetCameraComponent();
	UWorld* Mondo = GetWorld();
	if (!Obiettivo || !Mondo)
	{
		return;
	}
	// a fuoco quello che c'è al centro dell'immagine (fino a 200 m); il fuoco si sposta piano, come un obiettivo vero
	const FVector Da = Obiettivo->GetComponentLocation();
	const FVector A = Da + Obiettivo->GetForwardVector() * 20000.f;
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoFotoFuoco), true, this);
	if (bColonoNascosto && Colono.IsValid())
	{
		Parametri.AddIgnoredActor(Colono.Get());
	}
	FHitResult Colpo;
	const float Distanza = Mondo->LineTraceSingleByChannel(Colpo, Da, A, ECC_Visibility, Parametri) ? FMath::Max(Colpo.Distance, 30.f) : 20000.f;
	float& Fuoco = Obiettivo->PostProcessSettings.DepthOfFieldFocalDistance;
	Fuoco = Fuoco <= 0.f ? Distanza : FMath::FInterpTo(Fuoco, Distanza, Mondo->GetDeltaSeconds(), 6.f);
}

void AValdorsoCameraFoto::AlternaColono()
{
	if (bColonoNascosto)
	{
		MostraColono();
		return;
	}
	APawn* Corpo = Colono.Get();
	if (!Corpo)
	{
		return;
	}
	// il colono e quello che ha attaccato (il corpo MetaHuman è un attore a parte): solo su questo PC
	Nascosti.Reset();
	TArray<AActor*> Attaccati;
	Corpo->GetAttachedActors(Attaccati, true, true);
	Attaccati.Add(Corpo);
	for (AActor* Attore : Attaccati)
	{
		if (Attore && !Attore->IsHidden())
		{
			Attore->SetActorHiddenInGame(true);
			Nascosti.Add(Attore);
		}
	}
	bColonoNascosto = true;
}

void AValdorsoCameraFoto::MostraColono()
{
	for (const TWeakObjectPtr<AActor>& Attore : Nascosti)
	{
		if (Attore.IsValid())
		{
			Attore->SetActorHiddenInGame(false);
		}
	}
	Nascosti.Reset();
	bColonoNascosto = false;
}

FString AValdorsoCameraFoto::Scatta()
{
	const int32 Volte = FMath::Clamp(CVarGrandezzaFoto.GetValueOnGameThread(), 1, 4);
	// come dalla console: il comando lo gestisce la finestra del gioco (UGameViewportClient)
	if (APlayerController* Controllore = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		Controllore->ConsoleCommand(FString::Printf(TEXT("HighResShot %d"), Volte), /*bWriteToLog*/ false);
	}
	return FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir());
}

void AValdorsoCameraFoto::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Muovi(DeltaSeconds);
	if (bSfocatura)
	{
		MettiAFuoco();
	}
}

void AValdorsoCameraFoto::Muovi(float DeltaSeconds)
{
	FValdorsoComandiFoto& K = Comandi;
	// lo sguardo: mouse (pixel) e levetta destra (velocità)
	Imbardata += K.Sguardo.X * SensibilitaMouse + K.LevettaGuarda.X * VelocitaLevetta * DeltaSeconds;
	Beccheggio = FMath::Clamp(Beccheggio - K.Sguardo.Y * SensibilitaMouse + K.LevettaGuarda.Y * VelocitaLevetta * DeltaSeconds, -85.f, 85.f);
	K.Sguardo = FVector2D::ZeroVector;
	if (!FMath::IsNearlyZero(K.Zoom))
	{
		if (UCameraComponent* Obiettivo = GetCameraComponent())
		{
			Obiettivo->SetFieldOfView(ValdorsoFoto::CampoDopoZoom(Obiettivo->FieldOfView, K.Zoom));
		}
		K.Zoom = 0.f;
	}
	const float Inclina = (K.bInclinaDestra ? 1.f : 0.f) - (K.bInclinaSinistra ? 1.f : 0.f);
	Rollio = FMath::Clamp(Rollio + Inclina * 30.f * DeltaSeconds, -45.f, 45.f);
	SetActorRotation(FRotator(Beccheggio, Imbardata, Rollio));

	// il movimento: avanti dove si guarda, di lato in piano, su e giù dritto
	const FVector Avanti = FRotator(Beccheggio, Imbardata, 0.f).Vector();
	const FVector Destra = FRotationMatrix(FRotator(0.f, Imbardata, 0.f)).GetUnitAxis(EAxis::Y);
	FVector Verso = Avanti * ((K.bAvanti ? 1.f : 0.f) - (K.bIndietro ? 1.f : 0.f) + K.LevettaMuovi.Y)
		+ Destra * ((K.bDestra ? 1.f : 0.f) - (K.bSinistra ? 1.f : 0.f) + K.LevettaMuovi.X)
		+ FVector::UpVector * ((K.bSu ? 1.f : 0.f) - (K.bGiu ? 1.f : 0.f) + K.GrillettoSu - K.GrillettoGiu);
	if (Verso.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}
	Verso = Verso.GetClampedToMaxSize(1.f);
	const float Passo = Velocita * (K.bVeloce ? 4.f : 1.f) * (K.bPiano ? 0.25f : 1.f) * DeltaSeconds;
	const FVector Da = GetActorLocation();
	FVector A = Da + Verso * Passo;
	if (const APawn* Corpo = Colono.Get())
	{
		A = ValdorsoFoto::DentroIlRaggio(Corpo->GetActorLocation(), A, RaggioMassimo);
	}
	// niente muri e rocce attraversati: una sfera di 15 cm si ferma dove tocca
	if (UWorld* Mondo = GetWorld())
	{
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoFotoMuovi), false, this);
		if (APawn* Corpo = Colono.Get())
		{
			Parametri.AddIgnoredActor(Corpo);
			TArray<AActor*> Attaccati;
			Corpo->GetAttachedActors(Attaccati, true, true);
			Parametri.AddIgnoredActors(Attaccati);
		}
		FHitResult Colpo;
		if (Mondo->SweepSingleByChannel(Colpo, Da, A, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(15.f), Parametri))
		{
			// partita dentro qualcosa (un cespuglio, una roccia): può sempre allontanarsene
			if (Colpo.bStartPenetrating)
			{
				A = FVector::DotProduct(A - Da, Colpo.Normal) > 0.f ? A : Da;
			}
			else
			{
				A = Colpo.Location;
			}
		}
	}
	SetActorLocation(A);
}

// ------------------------------------------------------------------------------------------------
// Lo schermo
// ------------------------------------------------------------------------------------------------

void SValdorsoFoto::Construct(const FArguments& InArgs)
{
	Camera = InArgs._Camera;
	OnEsci = InArgs._OnEsci;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Pieno = FSlateColorBrush(FLinearColor::White);
	Pannello = FSlateRoundedBoxBrush(ValdorsoTema::Pannelli().CopyWithNewOpacity(0.78f), 6.f, ValdorsoTema::Oro().CopyWithNewOpacity(0.45f), 1.f);

	FSlateFontInfo FontEtichetta = Caratteri->Titolo(15.f, TEXT("Bold"));
	FontEtichetta.LetterSpacing = 220;

	ChildSlot
	[
		SNew(SOverlay)

		// l'aiuto, in basso a sinistra
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(48.f, 0.f, 0.f, 44.f))
		[
			SNew(SBorder)
			.Visibility_Lambda([this]() { return bAiuto && Lampo <= 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.BorderImage(&Pannello)
			.Padding(FMargin(22.f, 16.f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Titolo", "MODALITÀ FOTO"))
					.Font(FontEtichetta)
					.ColorAndOpacity(ValdorsoTema::Oro())
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 10.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Aiuto1", "W A S D muoviti · Q E giù e su · mouse guarda · Maiusc veloce · Alt piano"))
					.Font(Caratteri->Testo(19.f))
					.ColorAndOpacity(ValdorsoTema::Pergamena())
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Aiuto2", "Rotella zoom · Z X inclina · R dritto · F sfoca lo sfondo · G griglia · C colono"))
					.Font(Caratteri->Testo(19.f))
					.ColorAndOpacity(ValdorsoTema::Pergamena())
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Aiuto3", "Spazio o clic scatta · H nasconde l'aiuto · Esc o P esce"))
					.Font(Caratteri->Testo(19.f, TEXT("SemiBold")))
					.ColorAndOpacity(ValdorsoTema::OroChiaro())
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 10.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const AValdorsoCameraFoto* C = Camera.Get();
						if (!C)
						{
							return FText::GetEmpty();
						}
						return FText::Format(LOCTEXT("Stato", "Campo {0}° · inclinazione {1}° · sfocatura {2} · colono {3}"),
							FText::AsNumber(FMath::RoundToInt(C->CampoVisivo())),
							FText::AsNumber(FMath::RoundToInt(C->Inclinazione())),
							C->Sfocatura() ? LOCTEXT("Si", "sì") : LOCTEXT("No", "no"),
							C->ColonoNascosto() ? LOCTEXT("Nascosto", "nascosto") : LOCTEXT("Visibile", "visibile"));
					})
					.Font(Caratteri->Testo(17.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::TestoSecondario())
				]
			]
		]

		// il messaggio, in alto al centro
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 46.f, 0.f, 0.f))
		[
			SNew(SBorder)
			.Visibility_Lambda([this]() { return FPlatformTime::Seconds() < FineMessaggio ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.BorderImage(&Pannello)
			.Padding(FMargin(22.f, 10.f))
			[
				SNew(SBox).MaxDesiredWidth(1100.f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return TestoMessaggio; })
					.Font(Caratteri->Testo(20.f))
					.ColorAndOpacity(ValdorsoTema::Pergamena())
					.AutoWrapText(true)
				]
			]
		]
	];

	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoFoto::Anima));
}

FValdorsoComandiFoto* SValdorsoFoto::Comandi() const
{
	AValdorsoCameraFoto* C = Camera.Get();
	return C ? &C->Comandi : nullptr;
}

FReply SValdorsoFoto::Cattura()
{
	// il mouse resta qui e si muove "grezzo" (senza accelerazione e senza fermarsi ai bordi dello schermo)
	return FReply::Handled().UseHighPrecisionMouseMovement(SharedThis(this));
}

void SValdorsoFoto::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	// Alt-Tab, la console, un avviso di Windows: i tasti non si rilasciano più qui, si azzera tutto
	if (FValdorsoComandiFoto* K = Comandi())
	{
		*K = FValdorsoComandiFoto();
	}
}

void SValdorsoFoto::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	if (FValdorsoComandiFoto* K = Comandi())
	{
		K->Sguardo = FVector2D::ZeroVector;
	}
}

FReply SValdorsoFoto::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	return Cattura();
}

void SValdorsoFoto::Messaggio(const FText& Testo)
{
	TestoMessaggio = Testo;
	FineMessaggio = FPlatformTime::Seconds() + 4.0;
}

void SValdorsoFoto::Scatta()
{
	AValdorsoCameraFoto* C = Camera.Get();
	if (!C)
	{
		return;
	}
	const FString Cartella = C->Scatta();
	Lampo = 1.f;
	Messaggio(FText::Format(LOCTEXT("Salvata", "Foto salvata in {0}"), FText::FromString(Cartella)));
}

FReply SValdorsoFoto::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FValdorsoComandiFoto* K = Comandi();
	if (!K)
	{
		OnEsci.ExecuteIfBound();
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	// i tasti che si tengono premuti
	if (Tasto == EKeys::W || Tasto == EKeys::Up) { K->bAvanti = true; return FReply::Handled(); }
	if (Tasto == EKeys::S || Tasto == EKeys::Down) { K->bIndietro = true; return FReply::Handled(); }
	if (Tasto == EKeys::A || Tasto == EKeys::Left) { K->bSinistra = true; return FReply::Handled(); }
	if (Tasto == EKeys::D || Tasto == EKeys::Right) { K->bDestra = true; return FReply::Handled(); }
	if (Tasto == EKeys::E || Tasto == EKeys::PageUp) { K->bSu = true; return FReply::Handled(); }
	if (Tasto == EKeys::Q || Tasto == EKeys::PageDown) { K->bGiu = true; return FReply::Handled(); }
	if (Tasto == EKeys::LeftShift || Tasto == EKeys::RightShift) { K->bVeloce = true; return FReply::Handled(); }
	if (Tasto == EKeys::LeftAlt || Tasto == EKeys::RightAlt) { K->bPiano = true; return FReply::Handled(); }
	if (Tasto == EKeys::Z || Tasto == EKeys::Gamepad_LeftShoulder) { K->bInclinaSinistra = true; return FReply::Handled(); }
	if (Tasto == EKeys::X || Tasto == EKeys::Gamepad_RightShoulder) { K->bInclinaDestra = true; return FReply::Handled(); }

	// i tasti che fanno una cosa sola (tenuti premuti non la ripetono)
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	AValdorsoCameraFoto* C = Camera.Get();
	if (Tasto == EKeys::Escape || Tasto == EKeys::P || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		OnEsci.ExecuteIfBound();
	}
	else if (Tasto == EKeys::SpaceBar || Tasto == EKeys::Enter || Tasto == EKeys::Gamepad_FaceButton_Bottom)
	{
		Scatta();
	}
	else if (Tasto == EKeys::R)
	{
		C->RimettiDritto();
	}
	else if (Tasto == EKeys::F || Tasto == EKeys::Gamepad_FaceButton_Left)
	{
		C->AlternaSfocatura();
		Messaggio(C->Sfocatura() ? LOCTEXT("SfocaSi", "Sfondo sfocato: a fuoco c'è quello che sta al centro.") : LOCTEXT("SfocaNo", "Tutto a fuoco."));
	}
	else if (Tasto == EKeys::G)
	{
		bGriglia = !bGriglia;
	}
	else if (Tasto == EKeys::C)
	{
		C->AlternaColono();
	}
	else if (Tasto == EKeys::H || Tasto == EKeys::Gamepad_FaceButton_Top)
	{
		bAiuto = !bAiuto;
	}
	else if (Tasto == EKeys::Gamepad_DPad_Up)
	{
		K->Zoom += 1.f;
	}
	else if (Tasto == EKeys::Gamepad_DPad_Down)
	{
		K->Zoom -= 1.f;
	}
	return FReply::Handled();
}

FReply SValdorsoFoto::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (FValdorsoComandiFoto* K = Comandi())
	{
		const FKey Tasto = InKeyEvent.GetKey();
		if (Tasto == EKeys::W || Tasto == EKeys::Up) { K->bAvanti = false; }
		else if (Tasto == EKeys::S || Tasto == EKeys::Down) { K->bIndietro = false; }
		else if (Tasto == EKeys::A || Tasto == EKeys::Left) { K->bSinistra = false; }
		else if (Tasto == EKeys::D || Tasto == EKeys::Right) { K->bDestra = false; }
		else if (Tasto == EKeys::E || Tasto == EKeys::PageUp) { K->bSu = false; }
		else if (Tasto == EKeys::Q || Tasto == EKeys::PageDown) { K->bGiu = false; }
		else if (Tasto == EKeys::LeftShift || Tasto == EKeys::RightShift) { K->bVeloce = false; }
		else if (Tasto == EKeys::LeftAlt || Tasto == EKeys::RightAlt) { K->bPiano = false; }
		else if (Tasto == EKeys::Z || Tasto == EKeys::Gamepad_LeftShoulder) { K->bInclinaSinistra = false; }
		else if (Tasto == EKeys::X || Tasto == EKeys::Gamepad_RightShoulder) { K->bInclinaDestra = false; }
	}
	return FReply::Handled();
}

FReply SValdorsoFoto::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	if (FValdorsoComandiFoto* K = Comandi())
	{
		const FKey Asse = InAnalogInputEvent.GetKey();
		const float Valore = ZonaMortaFoto(InAnalogInputEvent.GetAnalogValue());
		if (Asse == EKeys::Gamepad_LeftX) { K->LevettaMuovi.X = Valore; }
		else if (Asse == EKeys::Gamepad_LeftY) { K->LevettaMuovi.Y = Valore; }
		else if (Asse == EKeys::Gamepad_RightX) { K->LevettaGuarda.X = Valore; }
		else if (Asse == EKeys::Gamepad_RightY) { K->LevettaGuarda.Y = Valore; }
		else if (Asse == EKeys::Gamepad_RightTriggerAxis) { K->GrillettoSu = Valore; }
		else if (Asse == EKeys::Gamepad_LeftTriggerAxis) { K->GrillettoGiu = Valore; }
	}
	return FReply::Handled();
}

FReply SValdorsoFoto::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
	{
		Scatta();
	}
	return Cattura();
}

FReply SValdorsoFoto::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasMouseCapture())
	{
		if (FValdorsoComandiFoto* K = Comandi())
		{
			K->Sguardo += MouseEvent.GetCursorDelta();
		}
	}
	return FReply::Handled();
}

FReply SValdorsoFoto::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (FValdorsoComandiFoto* K = Comandi())
	{
		K->Zoom += MouseEvent.GetWheelDelta();
	}
	return FReply::Handled();
}

EActiveTimerReturnType SValdorsoFoto::Anima(double Tempo, float Delta)
{
	Lampo = FMath::Max(0.f, Lampo - FMath::Min(Delta, 0.1f) * 3.f);
	// se la telecamera è sparita (mondo cambiato), si esce
	if (!Camera.IsValid())
	{
		OnEsci.ExecuteIfBound();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

int32 SValdorsoFoto::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
	// la griglia dei terzi: due linee per lato, sottili, d'oro chiaro
	if (bGriglia)
	{
		const FLinearColor Linea = ValdorsoTema::OroChiaro().CopyWithNewOpacity(0.35f);
		for (int32 I = 1; I <= 2; ++I)
		{
			const float X = Misura.X * I / 3.f;
			const float Y = Misura.Y * I / 3.f;
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(1.f, Misura.Y), FSlateLayoutTransform(FVector2f(X, 0.f))), &Pieno, ESlateDrawEffect::None, Linea);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(Misura.X, 1.f), FSlateLayoutTransform(FVector2f(0.f, Y))), &Pieno, ESlateDrawEffect::None, Linea);
		}
	}
	int32 Strato = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 1, InWidgetStyle, bParentEnabled);
	// il lampo dello scatto
	if (Lampo > 0.f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, Strato + 1, AllottedGeometry.ToPaintGeometry(), &Pieno, ESlateDrawEffect::None,
			FLinearColor(1.f, 0.97f, 0.9f, 0.55f * Lampo * Lampo));
		Strato += 1;
	}
	return Strato;
}

#undef LOCTEXT_NAMESPACE
