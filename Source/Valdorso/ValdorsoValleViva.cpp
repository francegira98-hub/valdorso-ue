// Valdorso - La valle che reagisce (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoValleViva.h"
#include "ValdorsoVitaValle.h"
#include "Valdorso.h"
#include "ValdorsoAcque.h"
#include "ValdorsoAmbiente.h"
#include "ValdorsoCielo.h"
#include "ValdorsoTempo.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

namespace
{
	TAutoConsoleVariable<int32> CVarValleViva(
		TEXT("Valdorso.ValleViva"),
		1,
		TEXT("La valle che reagisce (stormi che si alzano, pesci che saltano): 1 accesa, 0 spenta."),
		ECVF_Default);

	const TCHAR* CartellaValleViva = TEXT("/Game/Valle/Viva/");
	constexpr int32 NumeroPesci = 3;
	constexpr int32 NumeroVociTuffo = 2;

	template <typename T>
	T* CaricaViva(const TCHAR* Nome)
	{
		const FString Percorso = FString::Printf(TEXT("%s%s.%s"), CartellaValleViva, Nome, Nome);
		return LoadObject<T>(nullptr, *Percorso, nullptr, LOAD_NoWarn);
	}
}

namespace ValdorsoValleViva
{
	float DistanzaSpavento(float Velocita)
	{
		if (Velocita > 350.f)
		{
			return 1100.f;          // corre
		}
		if (Velocita > 40.f)
		{
			return 600.f;           // cammina
		}
		return 300.f;               // fermo: solo se proprio ci si mette in mezzo
	}

	bool OraDeiPesci(float Ora)
	{
		return (Ora >= 5.5f && Ora <= 7.5f) || (Ora >= 17.5f && Ora <= 20.5f);
	}

	FVector Salto(const FVector& Inizio, const FVector& Direzione, float Distanza, float Altezza, float S)
	{
		const float T = FMath::Clamp(S, 0.f, 1.f);
		return Inizio + Direzione.GetSafeNormal2D() * Distanza * T + FVector(0.0, 0.0, 4.0 * Altezza * T * (1.0 - T));
	}
}

AValdorsoValleViva::AValdorsoValleViva()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	bReplicates = true;                 // nasce sul server (modalità di gioco) e arriva a ogni PC
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));

	Stormo = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stormo"));
	Stormo->SetupAttachment(RootComponent);
	Stormo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stormo->SetGenerateOverlapEvents(false);
	Stormo->SetCastShadow(false);
	Stormo->SetNumCustomDataFloats(1);           // 0 = posato (ali chiuse), 1 = in volo (ali che battono)

	for (int32 i = 0; i < NumeroPesci; ++i)
	{
		UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Pesce%d"), i));
		P->SetupAttachment(RootComponent);
		P->SetUsingAbsoluteLocation(true);
		P->SetUsingAbsoluteRotation(true);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetGenerateOverlapEvents(false);
		P->SetVisibility(false);
		Pesci.Add(P);
	}
	VoceAli = CreateDefaultSubobject<UAudioComponent>(TEXT("VoceAli"));
	VoceAli->SetupAttachment(RootComponent);
	VoceAli->SetUsingAbsoluteLocation(true);
	VoceAli->bAutoActivate = false;
	for (int32 i = 0; i < NumeroVociTuffo; ++i)
	{
		UAudioComponent* A = CreateDefaultSubobject<UAudioComponent>(*FString::Printf(TEXT("VoceTuffo%d"), i));
		A->SetupAttachment(RootComponent);
		A->SetUsingAbsoluteLocation(true);
		A->bAutoActivate = false;
		VociTuffo.Add(A);
	}
}

void AValdorsoValleViva::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}
	// (10/11) il cielo e i boschi che vivono (ValdorsoVitaValle.h): uno per PC, solo qui
	if (UWorld* Mondo = GetWorld(); Mondo && !TActorIterator<AValdorsoVitaValle>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoVitaValle>(AValdorsoVitaValle::StaticClass(), FTransform::Identity, Parametri);
	}
	UStaticMesh* MeshUccello = CaricaViva<UStaticMesh>(TEXT("SM_Uccello"));
	UStaticMesh* MeshPesce = CaricaViva<UStaticMesh>(TEXT("SM_Pesce"));
	if (!MeshUccello || !MeshPesce)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Valle viva: mancano SM_Uccello o SM_Pesce in %s (lanciare Content/Python/nuovi/crea_valle_viva.py)"),
			CartellaValleViva);
	}
	if (MeshUccello)
	{
		Stormo->SetStaticMesh(MeshUccello);
	}
	for (UStaticMeshComponent* P : Pesci)
	{
		if (MeshPesce)
		{
			P->SetStaticMesh(MeshPesce);
		}
	}
	AValdorsoAmbiente::PreparaAudio(VoceAli, CaricaViva<USoundBase>(TEXT("S_Ali")), 400.f, 3500.f);
	USoundBase* SuonoTuffo = CaricaViva<USoundBase>(TEXT("S_Tuffo"));
	for (UAudioComponent* A : VociTuffo)
	{
		AValdorsoAmbiente::PreparaAudio(A, SuonoTuffo, 300.f, 2500.f);
	}
}

bool AValdorsoValleViva::Suolo(const FVector& Punto, FVector& OutSuolo) const
{
	FHitResult Colpo;
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoValleVivaSuolo), false);
	const APlayerController* Giocatore = GetWorld()->GetFirstPlayerController();
	if (Giocatore && Giocatore->GetPawn())
	{
		Parametri.AddIgnoredActor(Giocatore->GetPawn());
	}
	if (!GetWorld()->LineTraceSingleByChannel(Colpo, Punto + FVector(0.0, 0.0, 3000.0), Punto - FVector(0.0, 0.0, 3000.0), ECC_Visibility, Parametri))
	{
		return false;
	}
	if (Colpo.ImpactNormal.Z < 0.8)
	{
		return false;                       // troppo ripido (o un muro)
	}
	float Pelo = 0.f;
	if (Acque.IsValid() && Acque->PeloQui(FVector2D(Colpo.ImpactPoint.X, Colpo.ImpactPoint.Y), Pelo) && Pelo > Colpo.ImpactPoint.Z - 10.f)
	{
		return false;                       // è acqua
	}
	OutSuolo = Colpo.ImpactPoint;
	return true;
}

bool AValdorsoValleViva::PosaStormo(const FVector& Colono, const FRotator& Sguardo)
{
	if (!Stormo->GetStaticMesh())
	{
		return false;
	}
	for (int32 Tentativo = 0; Tentativo < 6; ++Tentativo)
	{
		const float Yaw = Sguardo.Yaw + FMath::FRandRange(-70.f, 70.f);
		const float Dist = FMath::FRandRange(1800.f, 3200.f);
		const FVector Prova = Colono + FRotator(0.f, Yaw, 0.f).Vector() * Dist;
		FVector Centro;
		if (!Suolo(Prova, Centro))
		{
			continue;
		}
		const int32 Quanti = FMath::RandRange(6, 11);
		Uccelli.Reset();
		Stormo->ClearInstances();
		for (int32 i = 0; i < Quanti; ++i)
		{
			const FVector2D Disco = FMath::RandPointInCircle(250.f);
			FVector Dove;
			if (!Suolo(Centro + FVector(Disco.X, Disco.Y, 0.0), Dove))
			{
				continue;
			}
			FUccello U;
			U.Posto = Dove + FVector(0.0, 0.0, 3.0);
			U.Yaw = FMath::FRandRange(0.f, 360.f);
			U.Fase = FMath::FRandRange(0.f, 6.28f);
			U.ProssimoGesto = GetWorld()->GetTimeSeconds() + FMath::FRandRange(0.3f, 2.0f);
			Uccelli.Add(U);
			const int32 Indice = Stormo->AddInstance(FTransform(FRotator(0.f, U.Yaw, 0.f), U.Posto, FVector(1.0)), true);
			Stormo->SetCustomDataValue(Indice, 0, 0.f, false);
		}
		if (Uccelli.Num() < 3)
		{
			Uccelli.Reset();
			Stormo->ClearInstances();
			continue;
		}
		Stormo->MarkRenderStateDirty();
		CentroStormo = Centro;
		bStormoVola = false;
		return true;
	}
	return false;
}

void AValdorsoValleViva::TogliStormo(double Adesso)
{
	Uccelli.Reset();
	Stormo->ClearInstances();
	bStormoVola = false;
	ProssimoStormo = Adesso + FMath::FRandRange(15.f, 45.f);
}

void AValdorsoValleViva::AlzaStormo(const FVector& DaDove, double Adesso)
{
	bStormoVola = true;
	StormoPartito = Adesso;
	const FVector Via = (CentroStormo - DaDove).GetSafeNormal2D();
	for (int32 i = 0; i < Uccelli.Num(); ++i)
	{
		FUccello& U = Uccelli[i];
		const FVector Laterale = FVector(-Via.Y, Via.X, 0.0) * FMath::FRandRange(-0.5f, 0.5f);
		U.Velocita = (Via + Laterale).GetSafeNormal() * FMath::FRandRange(500.f, 800.f) + FVector(0.0, 0.0, FMath::FRandRange(350.f, 600.f));
		Stormo->SetCustomDataValue(i, 0, 1.f, false);
	}
	if (VoceAli->Sound)
	{
		VoceAli->SetWorldLocation(CentroStormo);
		VoceAli->Play();
	}
}

void AValdorsoValleViva::AggiornaStormo(float DeltaSeconds, double Adesso)
{
	for (int32 i = 0; i < Uccelli.Num(); ++i)
	{
		FUccello& U = Uccelli[i];
		FTransform Dove;
		if (bStormoVola)
		{
			// sale sempre di più e si allontana
			U.Velocita.Z += 120.f * DeltaSeconds;
			U.Posto += U.Velocita * DeltaSeconds;
			const FRotator Verso = U.Velocita.Rotation();
			Dove = FTransform(FRotator(FMath::Clamp(Verso.Pitch, -20.f, 35.f), Verso.Yaw, 0.f), U.Posto);
		}
		else
		{
			// becchetta: ogni tanto si gira o fa un saltello
			if (Adesso >= U.ProssimoGesto)
			{
				U.Yaw += FMath::FRandRange(-90.f, 90.f);
				U.ProssimoGesto = Adesso + FMath::FRandRange(0.6f, 2.5f);
				U.Fase = static_cast<float>(Adesso);
			}
			const float Dal = static_cast<float>(Adesso) - U.Fase;
			const float Saltello = Dal < 0.25f ? FMath::Sin(Dal / 0.25f * PI) * 4.f : 0.f;
			const float Becco = FMath::Sin(static_cast<float>(Adesso) * 9.f + i) > 0.6f ? 18.f : 0.f;
			Dove = FTransform(FRotator(-Becco, U.Yaw, 0.f), U.Posto + FVector(0.0, 0.0, Saltello));
		}
		Stormo->UpdateInstanceTransform(i, Dove, true, false, true);
	}
	Stormo->MarkRenderStateDirty();
}

void AValdorsoValleViva::Tuffo(const FVector& Dove, float Volume)
{
	if (VociTuffo.Num() == 0)
	{
		return;
	}
	UAudioComponent* A = VociTuffo[ProssimaVoceTuffo % VociTuffo.Num()];
	ProssimaVoceTuffo = (ProssimaVoceTuffo + 1) % VociTuffo.Num();
	if (A && A->Sound)
	{
		A->SetWorldLocation(Dove);
		A->SetVolumeMultiplier(Volume);
		A->Play();
	}
}

void AValdorsoValleViva::FaiSaltarePesce(const FVector& Colono, double Adesso)
{
	if (!Acque.IsValid() || Pesci.Num() == 0 || !Pesci[0]->GetStaticMesh())
	{
		return;
	}
	int32 Libero = INDEX_NONE;
	for (int32 i = 0; i < Salti.Num(); ++i)
	{
		if (Salti[i].Partito < 0.0)
		{
			Libero = i;
			break;
		}
	}
	if (Libero == INDEX_NONE)
	{
		return;
	}
	for (int32 Tentativo = 0; Tentativo < 10; ++Tentativo)
	{
		const FVector2D Disco = FMath::RandPointInCircle(3500.f);
		if (Disco.Size() < 800.f)
		{
			continue;
		}
		const FVector2D Punto(Colono.X + Disco.X, Colono.Y + Disco.Y);
		float Pelo = 0.f;
		if (!Acque->PeloQui(Punto, Pelo))
		{
			continue;
		}
		// l'acqua deve essere profonda almeno 60 cm (niente pesci che saltano sulla riva); la linea parte appena sotto
		// il pelo, così non tocca la superficie dell'acqua se questa ha una collisione
		FHitResult Colpo;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoValleVivaFondo), false);
		const FVector Su(Punto.X, Punto.Y, Pelo - 5.f);
		if (GetWorld()->LineTraceSingleByChannel(Colpo, Su, Su - FVector(0.0, 0.0, 2000.0), ECC_Visibility, Parametri)
			&& Colpo.ImpactPoint.Z > Pelo - 60.f)
		{
			continue;
		}
		FPesce& P = Salti[Libero];
		P.Inizio = FVector(Punto.X, Punto.Y, Pelo - 15.f);
		P.Direzione = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector();
		P.Distanza = FMath::FRandRange(80.f, 160.f);
		P.Altezza = FMath::FRandRange(50.f, 95.f);
		P.Durata = FMath::FRandRange(0.65f, 0.95f);
		P.Partito = Adesso;
		Pesci[Libero]->SetVisibility(true);
		Tuffo(P.Inizio, 0.35f);
		return;
	}
}

void AValdorsoValleViva::AggiornaPesci(double Adesso)
{
	for (int32 i = 0; i < Salti.Num() && i < Pesci.Num(); ++i)
	{
		FPesce& P = Salti[i];
		if (P.Partito < 0.0)
		{
			continue;
		}
		const float S = static_cast<float>((Adesso - P.Partito) / P.Durata);
		if (S >= 1.f)
		{
			Tuffo(ValdorsoValleViva::Salto(P.Inizio, P.Direzione, P.Distanza, P.Altezza, 1.f), 1.f);
			Pesci[i]->SetVisibility(false);
			P.Partito = -1.0;
			continue;
		}
		const FVector Dove = ValdorsoValleViva::Salto(P.Inizio, P.Direzione, P.Distanza, P.Altezza, S);
		const FVector Dopo = ValdorsoValleViva::Salto(P.Inizio, P.Direzione, P.Distanza, P.Altezza, FMath::Min(1.f, S + 0.05f));
		const FRotator Verso = (Dopo - Dove).Rotation();
		Pesci[i]->SetWorldLocationAndRotation(Dove, FRotator(Verso.Pitch, Verso.Yaw, 0.f));
	}
}

void AValdorsoValleViva::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* Mondo = GetWorld();
	const double Adesso = Mondo->GetTimeSeconds();
	if (Salti.Num() != Pesci.Num())
	{
		Salti.SetNum(Pesci.Num());
	}
	if ((!Cielo.IsValid() || !Tempo.IsValid() || !Acque.IsValid()) && Adesso >= ProssimaRicerca)
	{
		ProssimaRicerca = Adesso + 2.0;
		for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
		{
			Cielo = *It;
			break;
		}
		Tempo = AValdorsoTempo::Di(Mondo);
		Acque = AValdorsoAcque::Di(Mondo);
	}
	const APlayerController* Giocatore = Mondo->GetFirstPlayerController();
	const APawn* Colono = Giocatore ? Giocatore->GetPawn() : nullptr;
	if (!Colono || !Giocatore->PlayerCameraManager)
	{
		return;
	}
	const bool bAccesa = CVarValleViva.GetValueOnGameThread() != 0;
	const FVector Dove = Colono->GetActorLocation();
	const float Velocita = static_cast<float>(Colono->GetVelocity().Size2D());
	const float Ora = Cielo.IsValid() ? Cielo->Ora() : 12.f;
	const float Giorno = Cielo.IsValid() ? Cielo->LuceDelGiorno() : 1.f;
	const float Pioggia = Tempo.IsValid() ? Tempo->AriaAdesso().Pioggia : 0.f;

	// ---- gli uccellini
	if (Uccelli.Num() == 0)
	{
		if (bAccesa && Adesso >= ProssimoStormo && Giorno > 0.6f && Pioggia < 0.3f)
		{
			if (!PosaStormo(Dove, Giocatore->PlayerCameraManager->GetCameraRotation()))
			{
				ProssimoStormo = Adesso + 5.0;       // riprova tra poco, da un'altra parte
			}
		}
	}
	else if (!bStormoVola)
	{
		const float Distanza = static_cast<float>(FVector::Dist2D(Dove, CentroStormo));
		if (!bAccesa || Distanza > 6000.f)
		{
			TogliStormo(Adesso);                   // troppo lontano: se ne va senza farsi vedere
		}
		else if (Distanza < ValdorsoValleViva::DistanzaSpavento(Velocita) || Giorno < 0.4f || Pioggia > 0.5f)
		{
			AlzaStormo(Dove, Adesso);
		}
	}
	else if (Adesso - StormoPartito > 7.0)
	{
		TogliStormo(Adesso);
	}
	if (Uccelli.Num() > 0)
	{
		AggiornaStormo(DeltaSeconds, Adesso);
	}

	// ---- i pesci
	if (bAccesa && ValdorsoValleViva::OraDeiPesci(Ora) && Pioggia < 0.6f && Adesso >= ProssimoPesce)
	{
		ProssimoPesce = Adesso + FMath::FRandRange(4.f, 12.f);
		FaiSaltarePesce(Dove, Adesso);
	}
	AggiornaPesci(Adesso);
}
