// Valdorso - Le interazioni con il mondo (vedi il .h). Scritto da Claude il 03/11/2026.

#include "ValdorsoInterazione.h"
#include "Valdorso.h"
#include "ValdorsoRegistroEventi.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ValdorsoInterazione"

namespace ValdorsoInterazione
{
	AActor* PiuVicina(const APawn* Chi)
	{
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		if (!Mondo)
		{
			return nullptr;
		}
		const FVector Centro = Chi->GetActorLocation();
		const FVector Avanti = Chi->GetActorForwardVector();
		TArray<FOverlapResult> Trovati;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoInterazione), false, Chi);
		Mondo->OverlapMultiByObjectType(Trovati, Centro, FQuat::Identity,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), FCollisionShape::MakeSphere(Raggio), Parametri);

		AActor* Migliore = nullptr;
		float Punteggio = TNumericLimits<float>::Max();
		for (const FOverlapResult& Trovato : Trovati)
		{
			AActor* Attore = Trovato.GetActor();
			const IValdorsoInterattivo* Cosa = Cast<IValdorsoInterattivo>(Attore);
			if (!Cosa || Cosa->TestoInterazione(Chi).IsEmpty())
			{
				continue;
			}
			const FVector Verso = Cosa->PuntoInterazione() - Centro;
			const float Distanza = Verso.Size2D();
			if (Distanza > Raggio)
			{
				continue;
			}
			// Conta di più quello che il colono ha davanti: chi sta alle spalle "costa" il doppio.
			const float Davanti = FVector::DotProduct(Avanti, Verso.GetSafeNormal2D());
			const float Costo = Distanza * (Davanti > 0.f ? 1.f : 2.f);
			if (Costo < Punteggio)
			{
				Punteggio = Costo;
				Migliore = Attore;
			}
		}
		return Migliore;
	}
}

AValdorsoPorta::AValdorsoPorta()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(10.f);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent->SetMobility(EComponentMobility::Static);

	// La cerniera sta sul bordo sinistro dell'anta; l'origine dell'attore è al centro della soglia, a terra.
	Cerniera = CreateDefaultSubobject<USceneComponent>(TEXT("Cerniera"));
	Cerniera->SetupAttachment(RootComponent);
	Cerniera->SetMobility(EComponentMobility::Movable);
	Cerniera->SetRelativeLocation(FVector(0.f, -50.f, 0.f));

	Anta = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Anta"));
	Anta->SetupAttachment(Cerniera);
	Anta->SetMobility(EComponentMobility::Movable);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	// (03/11) Il materiale semplice con il parametro "Color": senza, il cubo usava la griglia a scacchi.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Cubo.Succeeded())
	{
		Anta->SetStaticMesh(Cubo.Object);
		// Il cubo di serie è 100 cm con l'origine al centro: anta 100 × 6 × 210 cm, che parte dalla cerniera.
		Anta->SetRelativeScale3D(FVector(0.06f, 1.f, 2.1f));
		Anta->SetRelativeLocation(FVector(0.f, 50.f, 105.f));
		if (Semplice.Succeeded())
		{
			Anta->SetMaterial(0, Semplice.Object);
		}
	}
	Anta->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// La telecamera non deve "saltare" quando passa vicino all'anta.
	Anta->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void AValdorsoPorta::BeginPlay()
{
	Super::BeginPlay();
	// Il colore del legno sul cubo di serie (solo se nessuno ha messo un modello vero).
	if (Anta && Anta->GetStaticMesh() && Anta->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/")))
	{
		if (UMaterialInstanceDynamic* Legno = Anta->CreateDynamicMaterialInstance(0))
		{
			Legno->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.23f, 0.13f, 0.06f));
		}
	}
	AngoloAttuale = Verso * AngoloApertura;
	Cerniera->SetRelativeRotation(FRotator(0.f, AngoloAttuale, 0.f));
}

void AValdorsoPorta::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoPorta, Verso);
	DOREPLIFETIME(AValdorsoPorta, bChiusaAChiave);
}

FText AValdorsoPorta::TestoInterazione(const APawn* Chi) const
{
	if (Verso != 0)
	{
		return LOCTEXT("Chiudi", "Chiudi la porta");
	}
	return bChiusaAChiave ? LOCTEXT("AChiave", "La porta è chiusa a chiave") : LOCTEXT("Apri", "Apri la porta");
}

FVector AValdorsoPorta::PuntoInterazione() const
{
	// Il centro della soglia, a mezza altezza.
	return GetActorLocation() + FVector(0.f, 0.f, 100.f);
}

void AValdorsoPorta::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	if (Verso == 0 && bChiusaAChiave)
	{
		ValdorsoRegistroEventi::Annota(TEXT("PortaChiusa"), Chi, this, GetActorLocation(), TEXT("ha provato ad aprire"));
		return;
	}
	if (Verso != 0)
	{
		Verso = 0;
	}
	else
	{
		// Si apre dalla parte opposta a chi spinge. Con Verso +1 l'anta gira verso il lato X negativo della porta:
		// quindi chi sta sul lato X positivo la spinge con +1. (03/11: prima era al contrario, si apriva addosso.)
		const FVector DaChi = Chi->GetActorLocation() - GetActorLocation();
		Verso = FVector::DotProduct(DaChi, GetActorForwardVector()) >= 0.f ? 1 : -1;
	}
	ValdorsoRegistroEventi::Annota(Verso != 0 ? TEXT("PortaAperta") : TEXT("PortaChiusa"), Chi, this, GetActorLocation());
	ForceNetUpdate();
	OnRep_Verso();
}

void AValdorsoPorta::OnRep_Verso()
{
	// Si muove nel Tick finché arriva all'angolo voluto.
	SetActorTickEnabled(true);
}

void AValdorsoPorta::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Voluto = Verso * AngoloApertura;
	const float Passo = AngoloApertura / FMath::Max(0.1f, Durata) * DeltaSeconds;
	AngoloAttuale = FMath::Abs(Voluto - AngoloAttuale) <= Passo ? Voluto : AngoloAttuale + FMath::Sign(Voluto - AngoloAttuale) * Passo;
	Cerniera->SetRelativeRotation(FRotator(0.f, AngoloAttuale, 0.f));
	if (AngoloAttuale == Voluto)
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
