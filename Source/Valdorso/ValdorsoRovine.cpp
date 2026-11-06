// Valdorso - Le Rovine degli Antichi (vedi il .h). Scritto da Claude il 06/11/2026; (06/11, sera) le rune "sempre" della grotta est.

#include "ValdorsoRovine.h"
#include "Valdorso.h"
#include "ValdorsoCielo.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "ValdorsoRovine"

namespace
{
	TAutoConsoleVariable<int32> CVarRuneAccese(
		TEXT("Valdorso.Rune.Accese"),
		0,
		TEXT("1: le rune delle rovine si accendono anche di giorno (per le prove). 0: solo di notte."),
		ECVF_Default);

	const FName EtichettaRune(TEXT("ValdorsoRune"));
	const FName EtichettaRuneSempre(TEXT("ValdorsoRuneSempre"));
}

namespace ValdorsoRune
{
	float Battito(float Secondi)
	{
		const float T = FMath::Fmod(FMath::Max(0.f, Secondi), 1.6f);
		auto Colpo = [](float X, float Centro)
		{
			const float D = (X - Centro) / 0.07f;
			return FMath::Exp(-D * D);
		};
		return FMath::Clamp(Colpo(T, 0.1f) + 0.75f * Colpo(T, 0.38f), 0.f, 1.f);
	}

	float Luce(float Notte, float BattitoAdesso)
	{
		return FMath::Clamp(Notte, 0.f, 1.f) * (6.f + 18.f * FMath::Clamp(BattitoAdesso, 0.f, 1.f));
	}
}

// --------------------------------------------------------------------------------------------- le rune che si accendono

AValdorsoRuneNotte::AValdorsoRuneNotte()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.03f;
	bReplicates = true;                 // nasce sul server (modalità di gioco) e arriva a ogni PC
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
}

void AValdorsoRuneNotte::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
	}
}

void AValdorsoRuneNotte::Cerca()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Attore = *It;
		const bool bSempre = Attore->ActorHasTag(EtichettaRuneSempre);
		if ((!bSempre && !Attore->ActorHasTag(EtichettaRune)) || Trovati.Contains(Attore))
		{
			continue;
		}
		Trovati.Add(Attore);
		TArray<UStaticMeshComponent*> Mesh;
		Attore->GetComponents(Mesh);
		for (UStaticMeshComponent* Pezzo : Mesh)
		{
			for (int32 i = 0; i < Pezzo->GetNumMaterials(); ++i)
			{
				const UMaterialInterface* Materiale = Pezzo->GetMaterial(i);
				if (Materiale && Materiale->GetName().Contains(TEXT("Runa")))
				{
					if (UMaterialInstanceDynamic* Dinamico = Pezzo->CreateDynamicMaterialInstance(i))
					{
						FMaterialeRuna R;
						R.Materiale = Dinamico;
						R.bSempre = bSempre;
						Materiali.Add(R);
					}
				}
			}
		}
		TArray<ULightComponent*> Lampade;
		Attore->GetComponents(Lampade);
		for (ULightComponent* Lampada : Lampade)
		{
			FLuceRuna L;
			L.Luce = Lampada;
			L.Intensita = Lampada->Intensity;
			L.bSempre = bSempre;
			Lampada->SetIntensity(0.f);
			Luci.Add(L);
		}
	}
}

void AValdorsoRuneNotte::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* Mondo = GetWorld();
	const double Adesso = Mondo->GetTimeSeconds();
	if (Adesso >= ProssimaRicerca)
	{
		ProssimaRicerca = Adesso + 3.0;
		Cerca();
		if (!Cielo.IsValid())
		{
			for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
			{
				Cielo = *It;
				break;
			}
		}
	}
	if (Materiali.Num() == 0 && Luci.Num() == 0)
	{
		return;
	}
	const float Giorno = Cielo.IsValid() ? Cielo->LuceDelGiorno() : 1.f;
	const float Voluta = CVarRuneAccese.GetValueOnGameThread() != 0 ? 1.f : FMath::Clamp(1.f - Giorno, 0.f, 1.f);
	Notte = FMath::FInterpTo(Notte, Voluta, DeltaSeconds, 1.f);
	const float B = ValdorsoRune::Battito(static_cast<float>(Adesso));
	const float Valore = ValdorsoRune::Luce(Notte, B);
	const float ValoreSempre = ValdorsoRune::Luce(1.f, B);
	for (const FMaterialeRuna& M : Materiali)
	{
		if (UMaterialInstanceDynamic* Dinamico = M.Materiale.Get())
		{
			Dinamico->SetScalarParameterValue(TEXT("Luce"), M.bSempre ? ValoreSempre : Valore);
		}
	}
	for (const FLuceRuna& L : Luci)
	{
		if (ULightComponent* Lampada = L.Luce.Get())
		{
			Lampada->SetIntensity(L.Intensita * (L.bSempre ? 1.f : Notte) * (0.45f + 0.55f * B));
		}
	}
}

// --------------------------------------------------------------------------------------------- l'iscrizione

AValdorsoIscrizione::AValdorsoIscrizione()
{
	Titolo = LOCTEXT("TitoloIscrizione", "Iscrizione degli Antichi");
	Avvisi.Reset();
	// Niente legno: si vede la pietra delle rovine. La tavola resta (invisibile) perché la E la trovi; i pali no.
	if (Tavola)
	{
		Tavola->SetVisibility(false);
		Tavola->SetCastShadow(false);
	}
	for (UStaticMeshComponent* Palo : { PaloSinistro.Get(), PaloDestro.Get() })
	{
		if (Palo)
		{
			Palo->SetVisibility(false);
			Palo->SetCastShadow(false);
			Palo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

FText AValdorsoIscrizione::TestoInterazione(const APawn* Chi) const
{
	// come la bacheca: si legge stando davanti
	return Super::TestoInterazione(Chi).IsEmpty() ? FText::GetEmpty() : LOCTEXT("LeggiIscrizione", "Leggi l'iscrizione");
}

#undef LOCTEXT_NAMESPACE
