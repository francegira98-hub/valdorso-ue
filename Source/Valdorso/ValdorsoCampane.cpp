// Valdorso - Le campane del Tempio del Cuore (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoCampane.h"
#include "Valdorso.h"
#include "ValdorsoAmbiente.h"
#include "ValdorsoCielo.h"
#include "ValdorsoRegistroEventi.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectIterator.h"

namespace
{
	FAutoConsoleCommand ComandoCampaneAllarme(
		TEXT("Valdorso.Campane.Allarme"),
		TEXT("Le campane del tempio suonano l'allarme: Valdorso.Campane.Allarme 1 (0 lo ferma). Solo sul server."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti)
		{
			for (TObjectIterator<AValdorsoCampane> It; It; ++It)
			{
				if (!IsValid(*It) || !It->GetWorld() || !It->GetWorld()->IsGameWorld() || !It->HasAuthority())
				{
					continue;
				}
				It->ImpostaAllarme(Argomenti.Num() == 0 || FCString::Atoi(*Argomenti[0]) != 0);
			}
		}));

	FAutoConsoleCommand ComandoCampaneProva(
		TEXT("Valdorso.Campane.Prova"),
		TEXT("Tre rintocchi adesso, per sentire la campana (su questo PC)."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			for (TObjectIterator<AValdorsoCampane> It; It; ++It)
			{
				if (IsValid(*It) && It->GetWorld() && It->GetWorld()->IsGameWorld() && It->GetNetMode() != NM_DedicatedServer)
				{
					It->Rintocca(3);
				}
			}
		}));
}

namespace ValdorsoCampane
{
	int32 Rintocchi(int32 Ora)
	{
		switch (Ora)
		{
		case 6: case 9: case 12: case 15: case 18: case 21:
		{
			const int32 SuDodici = Ora % 12;
			return SuDodici == 0 ? 12 : SuDodici;
		}
		default:
			return 0;
		}
	}
}

AValdorsoCampane::AValdorsoCampane()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	bReplicates = true;
	bAlwaysRelevant = true;              // l'allarme si sente in tutta la valle
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(2.f);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	for (int32 i = 0; i < 3; ++i)
	{
		UAudioComponent* Voce = CreateDefaultSubobject<UAudioComponent>(*FString::Printf(TEXT("Voce%d"), i));
		Voce->SetupAttachment(RootComponent);
		Voce->bAutoActivate = false;
		Voci.Add(Voce);
	}
}

void AValdorsoCampane::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoCampane, bAllarme);
}

void AValdorsoCampane::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}
	if (!Suono)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Campane: manca il suono (lanciare Content/Python/nuovi/crea_suoni_valle.py)"));
		SetActorTickEnabled(false);
		return;
	}
	for (UAudioComponent* Voce : Voci)
	{
		AValdorsoAmbiente::PreparaAudio(Voce, Suono, Vicino, Lontano);
	}
}

void AValdorsoCampane::ImpostaAllarme(bool bSuona)
{
	if (!HasAuthority() || bAllarme == bSuona)
	{
		return;
	}
	bAllarme = bSuona;
	ForceNetUpdate();
	OnRep_Allarme();
	ValdorsoRegistroEventi::Annota(TEXT("Allarme"), nullptr, this, GetActorLocation(), bSuona ? TEXT("le campane suonano") : TEXT("le campane tacciono"));
}

void AValdorsoCampane::OnRep_Allarme()
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Campane: allarme %s"), bAllarme ? TEXT("acceso") : TEXT("spento"));
	ProssimoColpo = 0.0;
	DaSuonare = 0;
}

void AValdorsoCampane::Rintocca(int32 Quanti)
{
	DaSuonare += FMath::Max(0, Quanti);
}

void AValdorsoCampane::Colpo(float Tono)
{
	if (Voci.Num() == 0)
	{
		return;
	}
	UAudioComponent* Voce = Voci[ProssimaVoce % Voci.Num()];
	ProssimaVoce = (ProssimaVoce + 1) % Voci.Num();
	if (Voce && Voce->Sound)
	{
		Voce->SetPitchMultiplier(Tono);
		Voce->Play();
	}
}

void AValdorsoCampane::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* Mondo = GetWorld();
	const double Adesso = Mondo->GetTimeSeconds();
	if (!Cielo.IsValid())
	{
		for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
		{
			Cielo = *It;
			break;
		}
	}
	// Le ore: quando l'ora intera cambia (anche con Valdorso.Ora) si suonano i rintocchi di quella nuova.
	if (Cielo.IsValid())
	{
		const int32 OraIntera = FMath::FloorToInt(Cielo->Ora()) % 24;
		if (UltimaOra < 0)
		{
			UltimaOra = OraIntera;           // all'ingresso non si suona l'ora già passata
		}
		else if (OraIntera != UltimaOra)
		{
			UltimaOra = OraIntera;
			DaSuonare += ValdorsoCampane::Rintocchi(OraIntera);
		}
	}
	if (Adesso < ProssimoColpo)
	{
		return;
	}
	if (bAllarme)
	{
		// fitto, due toni che si alternano
		bTonoAlto = !bTonoAlto;
		Colpo(bTonoAlto ? 1.06f : 1.f);
		ProssimoColpo = Adesso + SecondiAllarme;
	}
	else if (DaSuonare > 0)
	{
		--DaSuonare;
		Colpo(1.f);
		ProssimoColpo = Adesso + SecondiTraRintocchi;
	}
}
