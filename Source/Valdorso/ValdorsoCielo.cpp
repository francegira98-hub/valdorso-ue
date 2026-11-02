// Valdorso - Il giorno e la notte (vedi il .h). Scritto da Claude il 03/11/2026.

#include "ValdorsoCielo.h"
#include "Valdorso.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UObjectIterator.h"

namespace
{
	/** Valdorso.Ora 21: mette l'ora in tutti i mondi con il cielo (il server la manda poi a tutti). */
	FAutoConsoleCommand ComandoOra(
		TEXT("Valdorso.Ora"),
		TEXT("Mette l'ora della valle (0-24), per esempio Valdorso.Ora 21. Senza numero dice l'ora di adesso."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti)
		{
			for (TObjectIterator<AValdorsoCielo> Cielo; Cielo; ++Cielo)
			{
				if (!IsValid(*Cielo) || !Cielo->GetWorld() || !Cielo->GetWorld()->IsGameWorld())
				{
					continue;
				}
				if (Argomenti.Num() > 0 && Cielo->HasAuthority())
				{
					Cielo->ImpostaOra(FCString::Atof(*Argomenti[0]));
				}
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Cielo (%s): sono le %.2f"),
					Cielo->HasAuthority() ? TEXT("server") : TEXT("PC"), Cielo->Ora());
			}
		}));

	/** Valdorso.Giorno.Minuti 2: un giorno della valle in 2 minuti veri (per vedere alba e tramonto in fretta). */
	FAutoConsoleCommand ComandoMinuti(
		TEXT("Valdorso.Giorno.Minuti"),
		TEXT("Quanti minuti veri dura un giorno della valle, per esempio Valdorso.Giorno.Minuti 2 (di partenza 120)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti)
		{
			if (Argomenti.Num() == 0)
			{
				return;
			}
			for (TObjectIterator<AValdorsoCielo> Cielo; Cielo; ++Cielo)
			{
				if (IsValid(*Cielo) && Cielo->HasAuthority() && Cielo->GetWorld() && Cielo->GetWorld()->IsGameWorld())
				{
					Cielo->ImpostaMinutiPerGiorno(FCString::Atof(*Argomenti[0]));
				}
			}
		}));
}

AValdorsoCielo::AValdorsoCielo()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));

	// La luna: una seconda luce del cielo, fredda e debole, accesa solo di notte.
	Luna = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Luna"));
	Luna->SetupAttachment(RootComponent);
	Luna->SetMobility(EComponentMobility::Movable);
	Luna->SetIntensity(0.f);
	Luna->SetLightColor(FLinearColor(0.62f, 0.72f, 1.f));
	Luna->SetAtmosphereSunLight(true);
	Luna->SetAtmosphereSunLightIndex(1);
	Luna->SetCastShadows(true);
}

void AValdorsoCielo::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		OraAllInizio = FMath::Fmod(OraIniziale, 24.f);
		MinutiReplicati = FMath::Max(1.f, MinutiPerGiorno);
		SecondiAllInizio = TempoServer();
	}
	// Il server dedicato non disegna niente: non serve che giri.
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Cielo: un giorno dura %.0f minuti, sono le %.2f"), MinutiReplicati, Ora());
}

void AValdorsoCielo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoCielo, OraAllInizio);
	DOREPLIFETIME(AValdorsoCielo, SecondiAllInizio);
	DOREPLIFETIME(AValdorsoCielo, MinutiReplicati);
}

double AValdorsoCielo::TempoServer() const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return 0.0;
	}
	if (const AGameStateBase* Stato = Mondo->GetGameState())
	{
		return Stato->GetServerWorldTimeSeconds();
	}
	return Mondo->GetTimeSeconds();
}

float AValdorsoCielo::Ora() const
{
	const double Giorni = (TempoServer() - SecondiAllInizio) / (FMath::Max(1.f, MinutiReplicati) * 60.0);
	const double Valore = FMath::Fmod(static_cast<double>(OraAllInizio) + Giorni * 24.0, 24.0);
	return static_cast<float>(Valore < 0.0 ? Valore + 24.0 : Valore);
}

void AValdorsoCielo::ImpostaOra(float NuovaOra)
{
	if (!HasAuthority())
	{
		return;
	}
	OraAllInizio = FMath::Fmod(FMath::Max(0.f, NuovaOra), 24.f);
	SecondiAllInizio = TempoServer();
	ForceNetUpdate();
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Cielo: ora messa alle %.2f"), OraAllInizio);
}

void AValdorsoCielo::ImpostaMinutiPerGiorno(float Minuti)
{
	if (!HasAuthority())
	{
		return;
	}
	const float Adesso = Ora();
	MinutiReplicati = FMath::Max(1.f, Minuti);
	OraAllInizio = Adesso;
	SecondiAllInizio = TempoServer();
	ForceNetUpdate();
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Cielo: un giorno dura ora %.1f minuti"), MinutiReplicati);
}

void AValdorsoCielo::TrovaLuci()
{
	bLuciCercate = true;
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	// Il sole: la Directional Light con etichetta "Sole"; se nessuna ce l'ha, la prima del livello.
	ADirectionalLight* Scelta = nullptr;
	for (TActorIterator<ADirectionalLight> It(Mondo); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Sole")))
		{
			Scelta = *It;
			break;
		}
		if (!Scelta)
		{
			Scelta = *It;
		}
	}
	if (Scelta)
	{
		if (UDirectionalLightComponent* Luce = Cast<UDirectionalLightComponent>(Scelta->GetLightComponent()))
		{
			if (Luce->Mobility != EComponentMobility::Movable)
			{
				// Una luce fissa non si può girare: diventa mobile (meglio metterla "Mobile" anche nel livello).
				UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Cielo: %s non era Mobile, la rendo Mobile (mettila Mobile nel livello)."), *Scelta->GetName());
				Luce->SetMobility(EComponentMobility::Movable);
			}
			Sole = Luce;
			IntensitaSole = Luce->Intensity;
			Luce->SetAtmosphereSunLightIndex(0);
		}
	}
	else
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Cielo: nel livello non c'è una Directional Light, il sole non gira."));
	}
	for (TActorIterator<ASkyLight> It(Mondo); It; ++It)
	{
		LuceCielo = It->GetLightComponent();
		break;
	}
}

void AValdorsoCielo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLuciCercate)
	{
		TrovaLuci();
	}

	const float Adesso = Ora();
	// Dalle 6 alle 18 il sole fa mezzo giro da est a ovest; di notte è sotto l'orizzonte e la luna è dall'altra parte.
	const float Giro = (Adesso - 6.f) / 12.f;                                 // 0 all'alba, 1 al tramonto
	const float Altezza = FMath::Sin(Giro * PI) * AltezzaMassimaSole;           // gradi sopra l'orizzonte (negativi di notte)
	const float Direzione = DirezioneAlba + Giro * 180.f;
	// Il sole sfuma tra 4 gradi sotto e 4 gradi sopra l'orizzonte (niente luce da sotto la terra).
	const float Giorno = FMath::Clamp((Altezza + 4.f) / 8.f, 0.f, 1.f);

	if (UDirectionalLightComponent* Luce = Sole.Get())
	{
		// La luce va dal sole verso il basso: pitch negativo dell'altezza, girata di 180 gradi rispetto a dov'è il sole.
		Luce->SetWorldRotation(FRotator(-Altezza, Direzione + 180.f, 0.f));
		Luce->SetIntensity(IntensitaSole * Giorno);
	}
	if (Luna)
	{
		// La luna sta dalla parte opposta del sole e non scende sotto i 15 gradi (la notte resta leggibile).
		const float AltezzaLuna = FMath::Max(15.f, -Altezza);
		Luna->SetWorldRotation(FRotator(-AltezzaLuna, Direzione, 0.f));
		Luna->SetIntensity(LuceLuna * (1.f - Giorno));
	}

	// La luce del cielo (riflessi e ombre morbide) va ricatturata se non lo fa da sola.
	if (USkyLightComponent* LuceDelCielo = LuceCielo.Get())
	{
		const double Secondi = GetWorld()->GetTimeSeconds();
		if (!LuceDelCielo->bRealTimeCapture && Secondi >= ProssimaCattura)
		{
			ProssimaCattura = Secondi + 5.0;
			LuceDelCielo->RecaptureSky();
		}
	}
}
