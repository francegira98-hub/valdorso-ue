// Valdorso - La modalità foto (vedi il .h). (08/11) Con lo strumento per il trailer (K, L, B, O, 1-5; ValdorsoTrailer.h).

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
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
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

	/** (08/11) Le cose che la sfera della telecamera non deve sentire: lei stessa, il colono e quello che ha attaccato. */
	FCollisionQueryParams FotoParametriSfera(const AActor* Camera, const APawn* Corpo)
	{
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoFotoMuovi), false, Camera);
		if (Corpo)
		{
			Parametri.AddIgnoredActor(Corpo);
			TArray<AActor*> Attaccati;
			Corpo->GetAttachedActors(Attaccati, true, true);
			Parametri.AddIgnoredActors(Attaccati);
		}
		return Parametri;
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
		// (08/11) il percorso del trailer: la prima volta si rilegge Saved/Trailer/ultimo.json
		ValdorsoTrailer::CaricaUltimoSeServe();

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
			// (08/11) il percorso si ferma (e il tempo del gioco torna normale, se c'erano i fotogrammi)
			Camera->FermaPercorso();
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
		// (08/11) il percorso del trailer si tiene: Saved/Trailer/ultimo.json
		ValdorsoTrailer::SalvaUltimo();
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
	if (bInPercorso)
	{
		MuoviSulPercorso(DeltaSeconds);
	}
	else
	{
		Muovi(DeltaSeconds);
	}
	// (08/11) se il percorso porta il suo fuoco, il fuoco lo mette lui (MuoviSulPercorso)
	if (bSfocatura && !(bInPercorso && bFuocoDalPercorso))
	{
		MettiAFuoco();
	}
}

void AValdorsoCameraFoto::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// (08/11) mai lasciare il tempo del gioco a passi fissi (mondo chiuso a metà percorso)
	FinisciFotogrammi();
	Super::EndPlay(EndPlayReason);
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
	SetActorLocation(PassoSicuro(Da, A));
}

FVector AValdorsoCameraFoto::PassoSicuro(const FVector& Da, const FVector& Voluto) const
{
	// niente muri e rocce attraversati: una sfera di 15 cm si ferma dove tocca
	FVector A = Voluto;
	if (UWorld* Mondo = GetWorld())
	{
		const FCollisionQueryParams Parametri = FotoParametriSfera(this, Colono.Get());
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
	return A;
}

bool AValdorsoCameraFoto::LiberoIn(const FVector& A) const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return false;
	}
	const FCollisionQueryParams Parametri = FotoParametriSfera(this, Colono.Get());
	return !Mondo->OverlapAnyTestByChannel(A, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(15.f), Parametri);
}

// ------------------------------------------------------------------------------------------------
// (08/11) Il percorso del trailer
// ------------------------------------------------------------------------------------------------

bool AValdorsoCameraFoto::AggiungiTappa(int32& OutNumero)
{
	FValdorsoPercorsoTrailer& Percorso = ValdorsoTrailer::Corrente();
	OutNumero = Percorso.Tappe.Num();
	if (Percorso.Tappe.Num() >= ValdorsoTrailer::MassimoTappe)
	{
		return false;
	}
	FValdorsoTappaTrailer Tappa;
	Tappa.Posizione = GetActorLocation();
	Tappa.Rotazione = FRotator(Beccheggio, FRotator::NormalizeAxis(Imbardata), Rollio);
	Tappa.Campo = CampoVisivo();
	const UCameraComponent* Obiettivo = GetCameraComponent();
	Tappa.Fuoco = (bSfocatura && Obiettivo) ? FMath::Max(0.f, Obiettivo->PostProcessSettings.DepthOfFieldFocalDistance) : 0.f;
	Percorso.Tappe.Add(Tappa);
	OutNumero = Percorso.Tappe.Num();
	return true;
}

bool AValdorsoCameraFoto::TogliTappa(int32& OutRimaste)
{
	FValdorsoPercorsoTrailer& Percorso = ValdorsoTrailer::Corrente();
	const bool bTolta = Percorso.Tappe.Num() > 0;
	if (bTolta)
	{
		Percorso.Tappe.RemoveAt(Percorso.Tappe.Num() - 1);
	}
	OutRimaste = Percorso.Tappe.Num();
	return bTolta;
}

void AValdorsoCameraFoto::RifaiTabella()
{
	Tabella.Prepara(ValdorsoTrailer::Corrente());
	DistanzaPercorso = FMath::Clamp(DistanzaPercorso, 0.f, Tabella.Lunghezza());
}

bool AValdorsoCameraFoto::AvviaPercorso(FText& OutMessaggio)
{
	const FValdorsoPercorsoTrailer& Percorso = ValdorsoTrailer::Corrente();
	OutMessaggio = FText::GetEmpty();
	if (Percorso.Tratti() <= 0)
	{
		OutMessaggio = LOCTEXT("PercorsoCorto", "Servono almeno due tappe: premi K per metterle.");
		return false;
	}
	Tabella.Prepara(Percorso);
	if (!Tabella.Pronta() || Tabella.Lunghezza() < 1.f)
	{
		OutMessaggio = LOCTEXT("PercorsoFermo", "Le tappe sono tutte nello stesso punto e guardano dalla stessa parte.");
		return false;
	}

	// le tappe oltre il limite (per esempio da un file fatto in un'altra parte della valle): lì la telecamera si ferma
	if (const APawn* Corpo = Colono.Get())
	{
		int32 Lontane = 0;
		for (const FValdorsoTappaTrailer& Tappa : Percorso.Tappe)
		{
			Lontane += FVector::Dist(Tappa.Posizione, Corpo->GetActorLocation()) > RaggioMassimo + 1.f ? 1 : 0;
		}
		if (Lontane > 0)
		{
			OutMessaggio = FText::Format(LOCTEXT("TappeLontane", "{0} tappe sono a più di {1} m dal colono: lì la telecamera si ferma al limite."),
				FText::AsNumber(Lontane), FText::AsNumber(FMath::RoundToInt(RaggioMassimo / 100.f)));
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Trailer: %d tappe oltre il limite dal colono"), Lontane);
		}
	}

	// il fuoco: se qualche tappa l'ha messo (con F), lo decide il percorso
	bFuocoDalPercorso = false;
	for (const FValdorsoTappaTrailer& Tappa : Percorso.Tappe)
	{
		bFuocoDalPercorso |= Tappa.Fuoco > 0.f;
	}
	if (UCameraComponent* Obiettivo = GetCameraComponent())
	{
		if (bFuocoDalPercorso)
		{
			FPostProcessSettings& Effetti = Obiettivo->PostProcessSettings;
			Effetti.bOverride_DepthOfFieldFocalDistance = true;
			Effetti.bOverride_DepthOfFieldFstop = true;
			Effetti.DepthOfFieldFstop = 1.8f;
		}
	}

	bInPercorso = true;
	bPrimoPasso = true;
	TempoPercorso = 0.f;
	DistanzaPercorso = 0.f;
	VelocitaUsata = Percorso.Velocita;
	GenerazionePercorso = ValdorsoTrailer::Generazione();
	Comandi = FValdorsoComandiFoto();

	if (ValdorsoTrailer::Fotogrammi())
	{
		if (GetNetMode() == NM_Standalone)
		{
			IniziaFotogrammi();
		}
		else
		{
			// collegati a un server, il tempo a passi fissi farebbe andare l'orologio di questo PC diverso da quello del server
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Trailer: i fotogrammi si salvano solo giocando senza server (Prova senza server)"));
		}
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Trailer: parte il percorso, %d tappe, %.0f m, %.0f m/s%s"), Percorso.Tappe.Num(),
		Tabella.Lunghezza() / 100.f, Percorso.Velocita / 100.f, Percorso.bGiro ? TEXT(", ad anello") : TEXT(""));
	return true;
}

void AValdorsoCameraFoto::FermaPercorso()
{
	FinisciFotogrammi();
	if (!bInPercorso)
	{
		return;
	}
	bInPercorso = false;
	// da qui si vola di nuovo a mano: lo sguardo riparte da com'è adesso
	const FRotator SguardoAdesso = GetActorRotation();
	Imbardata = SguardoAdesso.Yaw;
	Beccheggio = FMath::Clamp(FRotator::NormalizeAxis(SguardoAdesso.Pitch), -85.f, 85.f);
	Rollio = FMath::Clamp(FRotator::NormalizeAxis(SguardoAdesso.Roll), -45.f, 45.f);
	SetActorRotation(FRotator(Beccheggio, Imbardata, Rollio));
	if (bFuocoDalPercorso)
	{
		RimettiSfocatura();
	}
	bFuocoDalPercorso = false;
	Comandi = FValdorsoComandiFoto();
}

void AValdorsoCameraFoto::RimettiSfocatura()
{
	if (UCameraComponent* Obiettivo = GetCameraComponent())
	{
		FPostProcessSettings& Effetti = Obiettivo->PostProcessSettings;
		Effetti.bOverride_DepthOfFieldFocalDistance = bSfocatura;
		Effetti.bOverride_DepthOfFieldFstop = bSfocatura;
	}
}

void AValdorsoCameraFoto::MuoviSulPercorso(float DeltaSeconds)
{
	const FValdorsoPercorsoTrailer& Percorso = ValdorsoTrailer::Corrente();
	// il mouse e la rotella non contano mentre va (niente scatto quando si ferma)
	Comandi.Sguardo = FVector2D::ZeroVector;
	Comandi.Zoom = 0.f;
	// caricato, svuotato o rimasto con meno di due tappe da fuori: ci si ferma
	if (GenerazionePercorso != ValdorsoTrailer::Generazione() || Percorso.Tratti() <= 0 || !Tabella.Pronta() || Tabella.Lunghezza() < 1.f)
	{
		FermaPercorso();
		return;
	}

	// quanta strada: la velocità scelta (cambiata con 1-5 ci arriva in un attimo), con la partenza e l'arrivo dolci
	const float Lunghezza = Tabella.Lunghezza();
	bool bFinito = false;
	if (!bPrimoPasso)
	{
		VelocitaUsata = FMath::FInterpTo(VelocitaUsata, Percorso.Velocita, DeltaSeconds, 2.f);
		const float Rimasta = Percorso.bGiro ? -1.f : Lunghezza - DistanzaPercorso;
		DistanzaPercorso += VelocitaUsata * ValdorsoTrailer::FattoreVelocita(TempoPercorso, Rimasta, VelocitaUsata) * DeltaSeconds;
		TempoPercorso += DeltaSeconds;
		if (Percorso.bGiro)
		{
			DistanzaPercorso = FMath::Fmod(DistanzaPercorso, Lunghezza);
		}
		else if (DistanzaPercorso >= Lunghezza - 0.5f)
		{
			DistanzaPercorso = Lunghezza;
			bFinito = true;
		}
	}
	const FValdorsoTappaTrailer PosaAdesso = ValdorsoTrailer::Posa(Percorso, Tabella.Chiave(DistanzaPercorso));

	// dove: con le stesse regole del volo a mano (al massimo 30 m dal colono, mai attraverso muri e rocce);
	// alla prima tappa si salta, se lì la telecamera non tocca niente
	FVector A = PosaAdesso.Posizione;
	if (const APawn* Corpo = Colono.Get())
	{
		A = ValdorsoFoto::DentroIlRaggio(Corpo->GetActorLocation(), A, RaggioMassimo);
	}
	const bool bSalta = bPrimoPasso && LiberoIn(A);
	SetActorLocation(bSalta ? A : PassoSicuro(GetActorLocation(), A));
	bPrimoPasso = false;
	SetActorRotation(PosaAdesso.Rotazione);
	if (UCameraComponent* Obiettivo = GetCameraComponent())
	{
		Obiettivo->SetFieldOfView(FMath::Clamp(PosaAdesso.Campo, 15.f, 100.f));
		if (bFuocoDalPercorso && PosaAdesso.Fuoco > 0.f)
		{
			Obiettivo->PostProcessSettings.DepthOfFieldFocalDistance = FMath::Max(30.f, PosaAdesso.Fuoco);
		}
	}

	// il fotogramma di questo giro (si salva quando il fotogramma è disegnato, senza le scritte)
	if (bFotogrammi)
	{
		const FString File = CartellaFotogrammi / FString::Printf(TEXT("fotogramma_%05d.png"), NumeroFotogramma);
		++NumeroFotogramma;
		FScreenshotRequest::RequestScreenshot(File, /*bInShowUI*/ false, /*bAddFilenameSuffix*/ false);
	}
	if (bFinito)
	{
		FermaPercorso();
	}
}

void AValdorsoCameraFoto::IniziaFotogrammi()
{
	if (bFotogrammi)
	{
		return;
	}
	CartellaFotogrammi = ValdorsoTrailer::Cartella() / ValdorsoTrailer::NomeCorrente();
	if (!IFileManager::Get().MakeDirectory(*CartellaFotogrammi, true))
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Trailer: non riesco a fare la cartella %s, niente fotogrammi"), *CartellaFotogrammi);
		return;
	}
	// il tempo del gioco a passi fissi: ogni fotogramma vale 1/30 di secondo, anche se disegnarlo e salvarlo ci mette di più
	bPassoFissoPrima = FApp::UseFixedTimeStep();
	DeltaFissoPrima = FApp::GetFixedDeltaTime();
	FApp::SetFixedDeltaTime(1.0 / static_cast<double>(ValdorsoTrailer::FotogrammiAlSecondo));
	FApp::SetUseFixedTimeStep(true);
	bFotogrammi = true;
	NumeroFotogramma = 0;
	UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Trailer: fotogrammi a %d al secondo in %s"), ValdorsoTrailer::FotogrammiAlSecondo, *CartellaFotogrammi);
}

void AValdorsoCameraFoto::FinisciFotogrammi()
{
	if (!bFotogrammi)
	{
		return;
	}
	bFotogrammi = false;
	FApp::SetUseFixedTimeStep(bPassoFissoPrima);
	FApp::SetFixedDeltaTime(DeltaFissoPrima);
	UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Trailer: %d fotogrammi in %s"), NumeroFotogramma, *CartellaFotogrammi);
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
			.Visibility_Lambda([this]() { return bAiuto && Lampo <= 0.f && !InPercorso() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
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

				// (08/11) il trailer
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 12.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("TitoloTrailer", "TRAILER"))
					.Font(FontEtichetta)
					.ColorAndOpacity(ValdorsoTema::Oro())
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 6.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Aiuto4", "K metti una tappa · Backspace togli l'ultima · L percorri o ferma"))
					.Font(Caratteri->Testo(19.f))
					.ColorAndOpacity(ValdorsoTema::Pergamena())
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Aiuto5", "1 2 3 4 5 velocità (2, 4, 8, 15, 30 m/s) · O giro · B bande del cinema"))
					.Font(Caratteri->Testo(19.f))
					.ColorAndOpacity(ValdorsoTema::Pergamena())
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Aiuto6", "A modalità chiusa, in console: Valdorso.Trailer.Salva <nome> · .Carica <nome> · .Elenco · .Svuota"))
					.Font(Caratteri->Testo(17.f))
					.ColorAndOpacity(ValdorsoTema::TestoSecondario())
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

				// (08/11) il percorso del trailer
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const FValdorsoPercorsoTrailer& Strada = ValdorsoTrailer::Corrente();
						return FText::Format(LOCTEXT("StatoTrailer", "Percorso: {0} tappe · {1} m/s · giro {2} · bande {3}"),
							FText::AsNumber(Strada.Tappe.Num()),
							FText::AsNumber(FMath::RoundToInt(Strada.Velocita / 100.f)),
							Strada.bGiro ? LOCTEXT("GiroSi", "sì") : LOCTEXT("GiroNo", "no"),
							bBande ? LOCTEXT("BandeSi", "sì") : LOCTEXT("BandeNo", "no"));
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
			.Visibility_Lambda([this]() { return FPlatformTime::Seconds() < FineMessaggio && !InPercorso() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
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
	// (08/11) i tasti del trailer: K, Backspace, L, B, O, 1-5
	if (TastoTrailer(Tasto))
	{
		return FReply::Handled();
	}
	// (08/11) mentre la telecamera percorre la strada lo sguardo, lo zoom e il fuoco li decide il percorso: R e F aspettano
	const bool bPercorsoAdesso = InPercorso();
	if (Tasto == EKeys::Escape || Tasto == EKeys::P || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		OnEsci.ExecuteIfBound();
	}
	else if (Tasto == EKeys::SpaceBar || Tasto == EKeys::Enter || Tasto == EKeys::Gamepad_FaceButton_Bottom)
	{
		Scatta();
	}
	else if (Tasto == EKeys::R && !bPercorsoAdesso)
	{
		C->RimettiDritto();
	}
	else if ((Tasto == EKeys::F || Tasto == EKeys::Gamepad_FaceButton_Left) && !bPercorsoAdesso)
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

bool SValdorsoFoto::InPercorso() const
{
	const AValdorsoCameraFoto* C = Camera.Get();
	return C && C->InPercorso();
}

bool SValdorsoFoto::TastoTrailer(const FKey& Tasto)
{
	AValdorsoCameraFoto* C = Camera.Get();
	if (!C)
	{
		return false;
	}
	FValdorsoPercorsoTrailer& Strada = ValdorsoTrailer::Corrente();
	if (Tasto == EKeys::K)
	{
		// mentre va, le tappe non si toccano
		if (!C->InPercorso())
		{
			int32 Numero = 0;
			if (C->AggiungiTappa(Numero))
			{
				Messaggio(FText::Format(LOCTEXT("TappaAggiunta", "Tappa {0} aggiunta"), FText::AsNumber(Numero)));
			}
			else
			{
				Messaggio(FText::Format(LOCTEXT("TroppeTappe", "Al massimo {0} tappe: togline qualcuna con Backspace."), FText::AsNumber(ValdorsoTrailer::MassimoTappe)));
			}
		}
		return true;
	}
	if (Tasto == EKeys::BackSpace)
	{
		if (!C->InPercorso())
		{
			int32 Rimaste = 0;
			if (C->TogliTappa(Rimaste))
			{
				Messaggio(FText::Format(LOCTEXT("TappaTolta", "Ultima tappa tolta: ne restano {0}"), FText::AsNumber(Rimaste)));
			}
			else
			{
				Messaggio(LOCTEXT("NienteDaTogliere", "Non ci sono tappe da togliere."));
			}
		}
		return true;
	}
	if (Tasto == EKeys::L)
	{
		if (C->InPercorso())
		{
			C->FermaPercorso();
			return true;
		}
		FText Avviso;
		const bool bPartito = C->AvviaPercorso(Avviso);
		if (!Avviso.IsEmpty())
		{
			// se è partito, mentre va i messaggi non si vedono: lo si legge appena si ferma (se non è passato troppo)
			Messaggio(Avviso);
		}
		bEraInPercorso = bPartito;
		return true;
	}
	if (Tasto == EKeys::B)
	{
		bBande = !bBande;
		return true;
	}
	if (Tasto == EKeys::O)
	{
		Strada.bGiro = !Strada.bGiro;
		if (C->InPercorso())
		{
			C->RifaiTabella();
		}
		Messaggio(Strada.bGiro
			? LOCTEXT("GiroAcceso", "Giro: dopo l'ultima tappa si torna alla prima e si ricomincia.")
			: LOCTEXT("GiroSpento", "Niente giro: il percorso si ferma all'ultima tappa."));
		return true;
	}
	int32 Numero = 0;
	if (Tasto == EKeys::One || Tasto == EKeys::NumPadOne) { Numero = 1; }
	else if (Tasto == EKeys::Two || Tasto == EKeys::NumPadTwo) { Numero = 2; }
	else if (Tasto == EKeys::Three || Tasto == EKeys::NumPadThree) { Numero = 3; }
	else if (Tasto == EKeys::Four || Tasto == EKeys::NumPadFour) { Numero = 4; }
	else if (Tasto == EKeys::Five || Tasto == EKeys::NumPadFive) { Numero = 5; }
	if (Numero > 0)
	{
		Strada.Velocita = ValdorsoTrailer::VelocitaTasto(Numero);
		Messaggio(FText::Format(LOCTEXT("VelocitaPercorso", "Velocità del percorso: {0} m/s"), FText::AsNumber(FMath::RoundToInt(Strada.Velocita / 100.f))));
		return true;
	}
	return false;
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
	// (08/11) la telecamera si è fermata (in fondo al percorso, con L, o perché il percorso è cambiato da console)
	const bool bPercorsoAdesso = InPercorso();
	if (bEraInPercorso && !bPercorsoAdesso)
	{
		// un avviso della partenza ancora da leggere resta; se no si dice che è finito
		if (FPlatformTime::Seconds() >= FineMessaggio)
		{
			Messaggio(LOCTEXT("FinePercorso", "Fine del percorso: L lo rifà da capo."));
		}
	}
	bEraInPercorso = bPercorsoAdesso;
	return EActiveTimerReturnType::Continue;
}

int32 SValdorsoFoto::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
	const bool bPercorsoAdesso = InPercorso();
	// (08/11) le bande nere del cinema (2,39 a 1), sotto l'aiuto e i messaggi
	if (bBande)
	{
		const float Banda = ValdorsoTrailer::AltezzaBande(FVector2D(Misura.X, Misura.Y));
		if (Banda >= 1.f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(Misura.X, Banda), FSlateLayoutTransform(FVector2f(0.f, 0.f))), &Pieno, ESlateDrawEffect::None, FLinearColor::Black);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(Misura.X, Banda), FSlateLayoutTransform(FVector2f(0.f, Misura.Y - Banda))), &Pieno, ESlateDrawEffect::None, FLinearColor::Black);
		}
	}
	// la griglia dei terzi: due linee per lato, sottili, d'oro chiaro (non mentre la telecamera percorre la strada)
	if (bGriglia && !bPercorsoAdesso)
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
