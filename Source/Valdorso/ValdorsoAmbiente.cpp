// Valdorso - I suoni della valle, l'ambiente (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoAmbiente.h"
#include "Valdorso.h"
#include "ValdorsoAcque.h"
#include "ValdorsoCielo.h"
#include "ValdorsoTempo.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

namespace
{
	TAutoConsoleVariable<float> CVarAmbienteVolume(
		TEXT("Valdorso.Ambiente.Volume"),
		1.f,
		TEXT("Il volume di tutti i suoni della valle (vento, uccelli, acqua, fucina...): 0 = spenti, 1 = normale."),
		ECVF_Default);

	const TCHAR* CartellaSuoniValle = TEXT("/Game/Valle/Suoni/");

	USoundBase* SuonoValle(const TCHAR* Nome)
	{
		const FString Percorso = FString::Printf(TEXT("%s%s.%s"), CartellaSuoniValle, Nome, Nome);
		return LoadObject<USoundBase>(nullptr, *Percorso, nullptr, LOAD_NoWarn);
	}

	/** 0 prima di A, 1 dopo B, morbido in mezzo. */
	float Rampa(float X, float A, float B)
	{
		return FMath::SmoothStep(A, B, X);
	}

	UAudioComponent* NuovoAudio(AActor* Proprietario, const TCHAR* Nome, USceneComponent* Radice)
	{
		UAudioComponent* A = Proprietario->CreateDefaultSubobject<UAudioComponent>(Nome);
		A->SetupAttachment(Radice);
		A->bAutoActivate = false;
		A->bIsUISound = false;
		return A;
	}

	template <typename T>
	T* PrimoDel(UWorld* Mondo)
	{
		for (TActorIterator<T> It(Mondo); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}
}

// --------------------------------------------------------------------------------------------- le regole

namespace ValdorsoAmbiente
{
	float Volume(EValdorsoQuando Quando, float Ora, float Giorno, float Pioggia)
	{
		switch (Quando)
		{
		case EValdorsoQuando::Giorno:
			return FMath::Clamp(Giorno, 0.f, 1.f);
		case EValdorsoQuando::Notte:
			return FMath::Clamp(1.f - Giorno, 0.f, 1.f);
		case EValdorsoQuando::Lavoro:
			// dalle 7 alle 19, sfumando mezz'ora
			return Rampa(Ora, 6.5f, 7.f) * (1.f - Rampa(Ora, 19.f, 19.5f));
		case EValdorsoQuando::Sera:
			// sempre un poco; pieno dalle 18 a mezzanotte (la gente in locanda)
			return 0.35f + 0.65f * FMath::Max(Rampa(Ora, 17.f, 18.f) * (1.f - Rampa(Ora, 23.5f, 24.f)), 1.f - Rampa(Ora, 0.f, 1.f));
		default:
			return 1.f;
		}
	}

	float Vento(float Nuvole, float Pioggia)
	{
		return FMath::Clamp(0.25f + 0.35f * Nuvole + 0.6f * Pioggia, 0.f, 1.f);
	}

	float Uccelli(float Ora, float Giorno, float Pioggia)
	{
		// all'alba cantano di più (il coro del mattino)
		const float Alba = Rampa(Ora, 4.5f, 5.5f) * (1.f - Rampa(Ora, 8.f, 9.f));
		return FMath::Clamp(Giorno, 0.f, 1.f) * (1.f - FMath::Clamp(Pioggia * 1.4f, 0.f, 1.f)) * (0.7f + 0.3f * Alba);
	}

	float Grilli(float Giorno, float Pioggia)
	{
		return FMath::Clamp(1.f - Giorno, 0.f, 1.f) * (1.f - FMath::Clamp(Pioggia * 1.4f, 0.f, 1.f)) * 0.8f;
	}

	float Rane(float Ora, float Giorno, float Pioggia)
	{
		// qualcuna anche di giorno, il coro di sera e di notte; la pioggia non le zittisce del tutto
		return (0.15f + 0.85f * FMath::Clamp(1.f - Giorno, 0.f, 1.f)) * (1.f - 0.4f * FMath::Clamp(Pioggia, 0.f, 1.f));
	}

	bool RaneSpaventate(float Distanza, float Velocita, float Raggio)
	{
		return Distanza < Raggio && Velocita > 120.f;
	}

	bool PiuVicinoSuLinea(const TArray<FVector>& Linea, const FVector& P, FVector& OutPunto)
	{
		if (Linea.Num() == 0)
		{
			return false;
		}
		if (Linea.Num() == 1)
		{
			OutPunto = Linea[0];
			return true;
		}
		double Migliore = TNumericLimits<double>::Max();
		const FVector2D P2(P.X, P.Y);
		for (int32 i = 0; i + 1 < Linea.Num(); ++i)
		{
			const FVector2D A(Linea[i].X, Linea[i].Y);
			const FVector2D B(Linea[i + 1].X, Linea[i + 1].Y);
			const FVector2D AB = B - A;
			const double Lungo = AB.SizeSquared();
			const double T = Lungo > 1e-6 ? FMath::Clamp(FVector2D::DotProduct(P2 - A, AB) / Lungo, 0.0, 1.0) : 0.0;
			const FVector2D Q = A + AB * T;
			const double D = FVector2D::DistSquared(P2, Q);
			if (D < Migliore)
			{
				Migliore = D;
				OutPunto = FVector(Q.X, Q.Y, FMath::Lerp(Linea[i].Z, Linea[i + 1].Z, T));
			}
		}
		return true;
	}

	bool PiuVicinoSuRiva(const TArray<FVector2D>& Riva, float Pelo, const FVector& P, FVector& OutPunto)
	{
		if (Riva.Num() == 0)
		{
			return false;
		}
		TArray<FVector> Linea;
		Linea.Reserve(Riva.Num() + 1);
		for (const FVector2D& R : Riva)
		{
			Linea.Add(FVector(R.X, R.Y, Pelo));
		}
		const FVector Primo = Linea[0];      // (una copia: Add non vuole un pezzo dello stesso array)
		Linea.Add(Primo);                    // la riva è chiusa
		return PiuVicinoSuLinea(Linea, P, OutPunto);
	}
}

// --------------------------------------------------------------------------------------------- la sorgente ferma

AValdorsoSorgenteSuono::AValdorsoSorgenteSuono()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	Audio = NuovoAudio(this, TEXT("Audio"), RootComponent);
}

void AValdorsoSorgenteSuono::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer || !Suono)
	{
		SetActorTickEnabled(false);
		return;
	}
	AValdorsoAmbiente::PreparaAudio(Audio, Suono, Vicino, Lontano);
	Audio->SetVolumeMultiplier(0.001f);
	Audio->Play();
}

void AValdorsoSorgenteSuono::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* Mondo = GetWorld();
	if (!Cielo.IsValid())
	{
		Cielo = PrimoDel<AValdorsoCielo>(Mondo);
	}
	if (!Tempo.IsValid())
	{
		Tempo = AValdorsoTempo::Di(Mondo);
	}
	const float Ora = Cielo.IsValid() ? Cielo->Ora() : 12.f;
	const float Giorno = Cielo.IsValid() ? Cielo->LuceDelGiorno() : 1.f;
	const float Pioggia = Tempo.IsValid() ? Tempo->AriaAdesso().Pioggia : 0.f;
	float Voluto = Volume * ValdorsoAmbiente::Volume(Quando, Ora, Giorno, Pioggia) * FMath::Max(0.f, CVarAmbienteVolume.GetValueOnGameThread());
	// (06/11) le rane della palude tacciono quando il colono passa vicino, e riprendono piano
	float Sfuma = 1.f;
	if (bTaceVicino)
	{
		const APlayerController* Giocatore = Mondo->GetFirstPlayerController();
		const APawn* Colono = Giocatore ? Giocatore->GetPawn() : nullptr;
		if (Colono && ValdorsoAmbiente::RaneSpaventate(static_cast<float>(FVector::Dist(Colono->GetActorLocation(), GetActorLocation())),
			static_cast<float>(Colono->GetVelocity().Size2D()), Vicino + 500.f))
		{
			TaceFino = Mondo->GetTimeSeconds() + 10.0;
		}
		if (Mondo->GetTimeSeconds() < TaceFino)
		{
			Voluto = 0.f;
			Sfuma = 4.f;                       // smettono di colpo, riprendono piano
		}
	}
	VolumeAdesso = FMath::FInterpTo(VolumeAdesso, Voluto, DeltaSeconds, Sfuma);
	// (mai proprio 0: un componente a volume 0 si ferma e poi riparte dall'inizio)
	Audio->SetVolumeMultiplier(FMath::Max(0.001f, VolumeAdesso));
}

// --------------------------------------------------------------------------------------------- l'ambiente intorno al colono

AValdorsoAmbiente::AValdorsoAmbiente()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	bReplicates = true;                 // nasce sul server (modalità di gioco) e arriva a ogni PC
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	Vento = NuovoAudio(this, TEXT("Vento"), RootComponent);
	Uccelli = NuovoAudio(this, TEXT("Uccelli"), RootComponent);
	Grilli = NuovoAudio(this, TEXT("Grilli"), RootComponent);
	Fiume = NuovoAudio(this, TEXT("Fiume"), RootComponent);
	Riva = NuovoAudio(this, TEXT("Riva"), RootComponent);
	RaneRiva = NuovoAudio(this, TEXT("RaneRiva"), RootComponent);
	// i suoni dell'acqua stanno nel mondo, non seguono l'attore
	Fiume->SetUsingAbsoluteLocation(true);
	Riva->SetUsingAbsoluteLocation(true);
	RaneRiva->SetUsingAbsoluteLocation(true);
}

UAudioComponent* AValdorsoAmbiente::PreparaAudio(UAudioComponent* Audio, USoundBase* Suono, float Vicino, float Lontano)
{
	if (!Audio)
	{
		return nullptr;
	}
	Audio->SetSound(Suono);
	if (Lontano <= 0.f)
	{
		// intorno: senza posto
		Audio->bAllowSpatialization = false;
		Audio->bOverrideAttenuation = false;
		return Audio;
	}
	Audio->bAllowSpatialization = true;
	Audio->bOverrideAttenuation = true;
	FSoundAttenuationSettings& A = Audio->AttenuationOverrides;
	A.bAttenuate = true;
	A.bSpatialize = true;
	A.AttenuationShape = EAttenuationShape::Sphere;
	A.AttenuationShapeExtents = FVector(Vicino, 0.0, 0.0);
	A.FalloffDistance = Lontano;
	A.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	return Audio;
}

void AValdorsoAmbiente::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}
	struct FPezzo { UAudioComponent* Audio; const TCHAR* Nome; float Vicino; float Lontano; };
	const FPezzo Pezzi[] = {
		{ Vento, TEXT("S_Vento"), 0.f, 0.f },
		{ Uccelli, TEXT("S_Uccelli"), 0.f, 0.f },
		{ Grilli, TEXT("S_Grilli"), 0.f, 0.f },
		{ Fiume, TEXT("S_Fiume"), 600.f, 5000.f },
		{ Riva, TEXT("S_Lago"), 300.f, 3000.f },
		{ RaneRiva, TEXT("S_Rane"), 800.f, 5000.f },
	};
	int32 Mancano = 0;
	for (const FPezzo& P : Pezzi)
	{
		USoundBase* Suono = SuonoValle(P.Nome);
		if (!Suono)
		{
			++Mancano;
			continue;
		}
		PreparaAudio(P.Audio, Suono, P.Vicino, P.Lontano);
		P.Audio->SetVolumeMultiplier(0.001f);
		P.Audio->Play();
	}
	if (Mancano > 0)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Ambiente: mancano %d suoni in %s (lanciare Content/Python/nuovi/crea_suoni_valle.py)"),
			Mancano, CartellaSuoniValle);
	}
}

void AValdorsoAmbiente::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* Mondo = GetWorld();
	const double Adesso = Mondo->GetTimeSeconds();
	if ((!Cielo.IsValid() || !Tempo.IsValid() || !Acque.IsValid()) && Adesso >= ProssimaRicerca)
	{
		ProssimaRicerca = Adesso + 2.0;
		Cielo = PrimoDel<AValdorsoCielo>(Mondo);
		Tempo = AValdorsoTempo::Di(Mondo);
		Acque = AValdorsoAcque::Di(Mondo);
	}
	const APlayerController* Giocatore = Mondo->GetFirstPlayerController();
	const APlayerCameraManager* Camera = Giocatore ? Giocatore->PlayerCameraManager : nullptr;
	if (!Camera)
	{
		return;
	}
	const FVector Orecchio = Camera->GetCameraLocation();
	const float Ora = Cielo.IsValid() ? Cielo->Ora() : 12.f;
	const float Giorno = Cielo.IsValid() ? Cielo->LuceDelGiorno() : 1.f;
	const FValdorsoAria Aria = Tempo.IsValid() ? Tempo->AriaAdesso() : FValdorsoAria();
	const float Tutto = FMath::Max(0.f, CVarAmbienteVolume.GetValueOnGameThread());

	// L'acqua più vicina, quattro volte al secondo: il fiume sulla linea del corso, la riva sul giro del lago.
	if (Acque.IsValid() && Adesso >= ProssimoPunto)
	{
		ProssimoPunto = Adesso + 0.25;
		double MigliorFiume = TNumericLimits<double>::Max();
		for (const FValdorsoCorsoAcqua& Corso : Acque->Corsi)
		{
			FVector Q;
			if (ValdorsoAmbiente::PiuVicinoSuLinea(Corso.Punti, Orecchio, Q) && FVector::DistSquared(Q, Orecchio) < MigliorFiume)
			{
				MigliorFiume = FVector::DistSquared(Q, Orecchio);
				PuntoFiume = Q;
				bFiume = true;
			}
		}
		double MigliorRiva = TNumericLimits<double>::Max();
		for (const FValdorsoSpecchioAcqua& Specchio : Acque->Specchi)
		{
			FVector Q;
			if (ValdorsoAmbiente::PiuVicinoSuRiva(Specchio.Riva, Specchio.Pelo, Orecchio, Q) && FVector::DistSquared(Q, Orecchio) < MigliorRiva)
			{
				MigliorRiva = FVector::DistSquared(Q, Orecchio);
				PuntoRiva = Q;
				bRiva = true;
			}
		}
	}
	// si spostano morbidi (niente salti quando cambia il tratto più vicino)
	if (bFiume)
	{
		Fiume->SetWorldLocation(FMath::VInterpTo(Fiume->GetComponentLocation(), PuntoFiume, DeltaSeconds, 3.f));
	}
	if (bRiva)
	{
		const FVector Riv = FMath::VInterpTo(Riva->GetComponentLocation(), PuntoRiva, DeltaSeconds, 3.f);
		Riva->SetWorldLocation(Riv);
		RaneRiva->SetWorldLocation(Riv);
	}

	auto Metti = [](UAudioComponent* A, float V)
	{
		if (A && A->Sound)
		{
			A->SetVolumeMultiplier(FMath::Max(0.001f, V));
		}
	};
	Metti(Vento, Tutto * 0.8f * ValdorsoAmbiente::Vento(Aria.Nuvole, Aria.Pioggia));
	Metti(Uccelli, Tutto * ValdorsoAmbiente::Uccelli(Ora, Giorno, Aria.Pioggia));
	Metti(Grilli, Tutto * ValdorsoAmbiente::Grilli(Giorno, Aria.Pioggia));
	Metti(Fiume, Tutto * (bFiume ? 1.f : 0.f));
	Metti(Riva, Tutto * (bRiva ? 0.7f : 0.f));
	// (06/11) le rane della riva tacciono quando il colono passa vicino muovendosi, e riprendono piano dopo 10 s
	if (bRiva && Giocatore->GetPawn() && ValdorsoAmbiente::RaneSpaventate(static_cast<float>(FVector::Dist(Orecchio, PuntoRiva)),
		static_cast<float>(Giocatore->GetPawn()->GetVelocity().Size2D())))
	{
		RaneTaccionoFino = Adesso + 10.0;
	}
	const bool bTacciono = Adesso < RaneTaccionoFino;
	RaneAdesso = FMath::FInterpTo(RaneAdesso, bTacciono ? 0.f : 1.f, DeltaSeconds, bTacciono ? 4.f : 0.3f);
	Metti(RaneRiva, Tutto * RaneAdesso * (bRiva ? ValdorsoAmbiente::Rane(Ora, Giorno, Aria.Pioggia) : 0.f));
}
