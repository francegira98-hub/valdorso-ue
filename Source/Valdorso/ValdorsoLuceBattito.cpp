// Valdorso - La lucina e lo spirito del Battito (vedi il .h). Scritto da Claude il 14/11/2026.

#include "ValdorsoLuceBattito.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** A che altezza sta sul terreno (cm), quanto va veloce la lucina (cm/s), quanto si allontana da dove e' nata. */
	constexpr float LuceAltezza = 130.f;
	constexpr float LuceVelocita = 140.f;
	constexpr float LuceGiro = 2500.f;
}

AValdorsoLuceBattito::AValdorsoLuceBattito()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
	// la vede solo il colono della storia (il proprietario)
	bOnlyRelevantToOwner = true;
	SetNetUpdateFrequency(10.f);

	Sfera = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sfera"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Palla(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Palla.Succeeded())
	{
		Sfera->SetStaticMesh(Palla.Object);
	}
	Sfera->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sfera->SetCastShadow(false);
	Sfera->SetRelativeScale3D(FVector(0.1f));
	RootComponent = Sfera;

	Luce = CreateDefaultSubobject<UPointLightComponent>(TEXT("Luce"));
	Luce->SetupAttachment(Sfera);
	Luce->SetCastShadows(false);
	Luce->SetAttenuationRadius(900.f);
	Luce->SetIntensity(6000.f);
	Luce->SetLightColor(FLinearColor(0.45f, 0.75f, 1.f));
}

void AValdorsoLuceBattito::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoLuceBattito, bSpirito);
}

AValdorsoLuceBattito* AValdorsoLuceBattito::Accendi(APlayerController* Per, const FVector& Dove, bool bInSpirito)
{
	UWorld* Mondo = Per ? Per->GetWorld() : nullptr;
	if (!Mondo || !Per->HasAuthority())
	{
		return nullptr;
	}
	FVector Punto = Dove;
	FHitResult Terra;
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoLuceBattito), false, Per->GetPawn());
	if (Mondo->LineTraceSingleByChannel(Terra, Dove + FVector(0.f, 0.f, 400.f), Dove - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Parametri))
	{
		Punto.Z = Terra.ImpactPoint.Z + LuceAltezza;
	}
	FActorSpawnParameters Come;
	Come.Owner = Per;
	Come.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AValdorsoLuceBattito* Nuova = Mondo->SpawnActor<AValdorsoLuceBattito>(AValdorsoLuceBattito::StaticClass(), Punto, FRotator::ZeroRotator, Come);
	if (Nuova)
	{
		Nuova->bSpirito = bInSpirito;
		Nuova->Partenza = Punto;
		Nuova->Rotta = FMath::FRandRange(0.f, 360.f);
		Nuova->SetLifeSpan(300.f);
		Nuova->Colora();
	}
	return Nuova;
}

void AValdorsoLuceBattito::BeginPlay()
{
	Super::BeginPlay();
	Colora();
}

void AValdorsoLuceBattito::OnRep_Spirito()
{
	Colora();
}

void AValdorsoLuceBattito::Colora()
{
	if (!Sfera || !Luce)
	{
		return;
	}
	// lo spirito: una luce pallida e alta come una persona; la lucina: piccola e azzurra
	Sfera->SetRelativeScale3D(bSpirito ? FVector(0.3f, 0.3f, 0.9f) : FVector(0.1f));
	Luce->SetLightColor(bSpirito ? FLinearColor(0.75f, 0.85f, 0.95f) : FLinearColor(0.45f, 0.75f, 1.f));
	Luce->SetAttenuationRadius(bSpirito ? 1200.f : 900.f);
}

void AValdorsoLuceBattito::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Tempo += DeltaSeconds;
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	// su ogni PC: la luce che respira
	if (Luce && GetNetMode() != NM_DedicatedServer)
	{
		const float Respiro = bSpirito ? 0.7f + 0.3f * FMath::Sin(Tempo * 1.5f) : 0.8f + 0.2f * FMath::Sin(Tempo * 7.f);
		Luce->SetIntensity((bSpirito ? 4000.f : 6000.f) * Respiro);
	}
	if (!HasAuthority())
	{
		return;
	}
	FVector Qui = GetActorLocation();
	if (!bSpirito)
	{
		// la lucina vaga piano, e torna indietro se si allontana troppo da dove e' nata
		Rotta += FMath::FRandRange(-50.f, 50.f) * DeltaSeconds;
		const FVector VersoCasa = Partenza - Qui;
		if (VersoCasa.SizeSquared2D() > FMath::Square(LuceGiro))
		{
			Rotta = FMath::RadiansToDegrees(FMath::Atan2(VersoCasa.Y, VersoCasa.X));
		}
		const float Rad = FMath::DegreesToRadians(Rotta);
		Qui += FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.f) * LuceVelocita * DeltaSeconds;
		FHitResult Terra;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoLuceBattitoVola), false, this);
		if (Mondo->LineTraceSingleByChannel(Terra, Qui + FVector(0.f, 0.f, 300.f), Qui - FVector(0.f, 0.f, 800.f), ECC_Visibility, Parametri))
		{
			Qui.Z = FMath::FInterpTo(static_cast<float>(Qui.Z), static_cast<float>(Terra.ImpactPoint.Z) + LuceAltezza + 20.f * FMath::Sin(Tempo * 2.f), DeltaSeconds, 3.f);
		}
	}
	else
	{
		// lo spirito resta dov'e', e ondeggia
		Qui.Z = Partenza.Z + 25.f * FMath::Sin(Tempo * 1.2f);
	}
	SetActorLocation(Qui);
}
