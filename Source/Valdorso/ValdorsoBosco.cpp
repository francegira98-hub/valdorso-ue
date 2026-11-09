// Valdorso - Il bosco (vedi il .h). Scritto da Claude il 09/11/2026.

#include "ValdorsoBosco.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Valdorso.Bosco.Lontano: i metri oltre i quali spariscono le cose piccole (0 = come deciso dal generatore). */
	float GBoscoLontanoMetri = 0.f;

	FAutoConsoleVariableRef CVarBoscoLontano(
		TEXT("Valdorso.Bosco.Lontano"),
		GBoscoLontanoMetri,
		TEXT("Da quanti metri spariscono cespugli, ceppi e tronchi a terra (0 = come deciso dal generatore). Vale per i boschi ricostruiti dopo."),
		ECVF_Default);

}

namespace ValdorsoBosco
{
	FTransform Tronco(const FTransform& Albero, float Raggio, float Altezza, bool bSdraiato)
	{
		const float Largo = FMath::Max(1.f, Raggio * 2.f) / 100.f;
		const float Lungo = FMath::Max(1.f, Altezza) / 100.f;   // (sdraiato)
		FTransform Forma;
		if (bSdraiato)
		{
			// sdraiato lungo X (il tronco caduto e' fatto cosi'), appoggiato: il centro a un raggio da terra
			Forma = FTransform(FRotator(90.f, 0.f, 0.f), FVector(0.f, 0.f, Raggio * 0.7f), FVector(Largo, Largo, Lungo));
		}
		else
		{
			// in piedi, da 30 cm sotto il terreno (sui pendii non resta un buco) fino a Altezza
			const float Tutto = (FMath::Max(1.f, Altezza) + 30.f) / 100.f;
			Forma = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, (Altezza - 30.f) * 0.5f), FVector(Largo, Largo, Tutto));
		}
		// la forma nel sistema dell'albero (con la sua scala e la sua rotazione), poi l'albero
		return Forma * Albero;
	}
}

AValdorsoBosco::AValdorsoBosco()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent->SetMobility(EComponentMobility::Static);
	// il bosco e' uguale per tutti e non cambia: ogni macchina lo ha gia' nel livello, niente rete
	bReplicates = false;
	// il cilindro di Unreal (alto 100, largo 100, centro a meta'): il tronco invisibile. Preso qui, cosi' finisce
	// sempre nel gioco impacchettato e nel server
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CercaCilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Cilindro = CercaCilindro.Object;
	// (con World Partition crea_bosco.py gli spegne "Is Spatially Loaded": il bosco si vede da tutta la valle)
}

void AValdorsoBosco::Svuota()
{
	Modify();
	Specie.Reset();
	Togli();
}

void AValdorsoBosco::AggiungiSpecie(UStaticMesh* Modello, const TArray<FTransform>& Dove, float RaggioTronco, float AltezzaTronco,
	bool bSdraiato, float Sparisce, bool bOmbra)
{
	if (!Modello || Dove.Num() == 0)
	{
		return;
	}
	Modify();
	FValdorsoSpecieBosco Nuova;
	Nuova.Modello = Modello;
	Nuova.Dove = Dove;
	Nuova.RaggioTronco = RaggioTronco;
	Nuova.AltezzaTronco = AltezzaTronco;
	Nuova.bSdraiato = bSdraiato;
	Nuova.Sparisce = Sparisce;
	Nuova.bOmbra = bOmbra;
	Specie.Add(MoveTemp(Nuova));
}

int32 AValdorsoBosco::QuantiAlberi() const
{
	int32 Quanti = 0;
	for (const FValdorsoSpecieBosco& Una : Specie)
	{
		Quanti += Una.Dove.Num();
	}
	return Quanti;
}

void AValdorsoBosco::Togli()
{
	for (UHierarchicalInstancedStaticMeshComponent* Fatto : Fatti)
	{
		if (Fatto)
		{
			Fatto->DestroyComponent();
		}
	}
	Fatti.Reset();
}

void AValdorsoBosco::Ricostruisci()
{
	Togli();
	UWorld* Mondo = GetWorld();
	if (!Mondo || !RootComponent)
	{
		return;
	}
	const bool bDisegna = Mondo->GetNetMode() != NM_DedicatedServer;

	auto NuovoComponente = [this](UStaticMesh* Modello, const TCHAR* Prefisso, int32 Indice) -> UHierarchicalInstancedStaticMeshComponent*
	{
		UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this,
			MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *FString::Printf(TEXT("%s%d"), Prefisso, Indice)),
			RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
		C->SetMobility(EComponentMobility::Static);
		C->SetupAttachment(RootComponent);
		C->SetStaticMesh(Modello);
		Fatti.Add(C);
		return C;
	};

	for (int32 i = 0; i < Specie.Num(); ++i)
	{
		const FValdorsoSpecieBosco& Una = Specie[i];
		if (!Una.Modello || Una.Dove.Num() == 0)
		{
			continue;
		}
		if (bDisegna)
		{
			UHierarchicalInstancedStaticMeshComponent* Alberi = NuovoComponente(Una.Modello, TEXT("Alberi"), i);
			Alberi->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Alberi->SetCanEverAffectNavigation(false);
			Alberi->SetGenerateOverlapEvents(false);
			Alberi->SetCastShadow(Una.bOmbra);
			float Sparisce = Una.Sparisce;
			if (GBoscoLontanoMetri > 0.f && Sparisce > 0.f)
			{
				Sparisce = GBoscoLontanoMetri * 100.f;
			}
			if (Sparisce > 0.f)
			{
				Alberi->SetCullDistances(FMath::RoundToInt(Sparisce * 0.8f), FMath::RoundToInt(Sparisce));
			}
			// le istanze prima di registrare (un componente fermo non si cambia dopo, nel gioco)
			Alberi->AddInstances(Una.Dove, false, false);
			Alberi->RegisterComponent();
		}
		if (Cilindro && Una.RaggioTronco > 0.f)
		{
			TArray<FTransform> Tronchi;
			Tronchi.Reserve(Una.Dove.Num());
			for (const FTransform& Albero : Una.Dove)
			{
				Tronchi.Add(ValdorsoBosco::Tronco(Albero, Una.RaggioTronco, Una.AltezzaTronco, Una.bSdraiato));
			}
			UHierarchicalInstancedStaticMeshComponent* Fermi = NuovoComponente(Cilindro, TEXT("Tronchi"), i);
			Fermi->SetVisibility(false);
			Fermi->SetHiddenInGame(true);
			Fermi->SetCastShadow(false);
			Fermi->SetCollisionProfileName(TEXT("BlockAll"));
			// la telecamera non sbatte contro i tronchi invisibili
			Fermi->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Fermi->SetGenerateOverlapEvents(false);
			Fermi->AddInstances(Tronchi, false, false);
			Fermi->RegisterComponent();
		}
	}
}

void AValdorsoBosco::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Ricostruisci();
}

void AValdorsoBosco::BeginPlay()
{
	Super::BeginPlay();
	// nel gioco impacchettato OnConstruction non gira per gli attori del livello: qui si fanno le istanze
	if (Fatti.Num() == 0)
	{
		Ricostruisci();
	}
}

void AValdorsoBosco::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Togli();
	Super::EndPlay(EndPlayReason);
}
