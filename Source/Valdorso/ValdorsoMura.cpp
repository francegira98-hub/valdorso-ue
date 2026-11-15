// Valdorso - Le mura: la grata e la leva (vedi il .h). Scritto da Claude il 16/11/2026.

#include "ValdorsoMura.h"
#include "Valdorso.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoRegistroEventi.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ValdorsoMura"

namespace
{
	/** Le sbarre di serie: in piedi e di traverso. */
	constexpr int32 SbarreInPiedi = 9;
	constexpr int32 SbarreTraverso = 6;
	/** Lo spessore di una sbarra (cm). */
	constexpr float SpessoreSbarra = 9.f;

	/** Il colore del ferro (sul materiale semplice dell'Engine) e del legno della leva. */
	void Colora(UStaticMeshComponent* Pezzo, const FLinearColor& Colore)
	{
		if (Pezzo && Pezzo->GetStaticMesh() && Pezzo->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/")))
		{
			if (UMaterialInstanceDynamic* M = Pezzo->CreateDynamicMaterialInstance(0))
			{
				M->SetVectorParameterValue(TEXT("Color"), Colore);
			}
		}
	}

	void SuonaQui(const AActor* Chi, const TCHAR* Nome, const TCHAR* Riserva, float Volume, float Tono)
	{
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		if (!Mondo || Chi->GetNetMode() == NM_DedicatedServer)
		{
			return;
		}
		// il suono suo (S_Grata_*, quando ci sara'), o quello di una porta, piu' basso e lento
		auto Carica = [](const TCHAR* N) -> USoundBase*
		{
			const FString Percorso = FString::Printf(TEXT("/Game/Valle/Suoni/%s.%s"), N, N);
			return LoadObject<USoundBase>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
		};
		USoundBase* Suono = Carica(Nome);
		const bool bRiserva = Suono == nullptr;
		if (bRiserva)
		{
			Suono = Carica(Riserva);
		}
		ValdorsoCombattimento::Suona(Mondo, Suono, Chi->GetActorLocation() + FVector(0.f, 0.f, 200.f), 3000.f, Volume,
			(bRiserva ? Tono * 0.6f : Tono) * FMath::FRandRange(0.95f, 1.05f));
	}
}

// ------------------------------------------------------------------------------------------------
// La grata
// ------------------------------------------------------------------------------------------------

AValdorsoGrata::AValdorsoGrata()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(10.f);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent->SetMobility(EComponentMobility::Static);

	Saracinesca = CreateDefaultSubobject<USceneComponent>(TEXT("Saracinesca"));
	Saracinesca->SetupAttachment(RootComponent);
	Saracinesca->SetMobility(EComponentMobility::Movable);

	Ferro = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ferro"));
	Ferro->SetupAttachment(Saracinesca);
	Ferro->SetMobility(EComponentMobility::Movable);
	Ferro->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ferro->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	for (int32 k = 0; k < SbarreInPiedi + SbarreTraverso; ++k)
	{
		UStaticMeshComponent* S = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Sbarra%d"), k));
		S->SetupAttachment(Saracinesca);
		S->SetMobility(EComponentMobility::Movable);
		S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Cubo.Succeeded())
		{
			S->SetStaticMesh(Cubo.Object);
			if (Semplice.Succeeded())
			{
				S->SetMaterial(0, Semplice.Object);
			}
		}
		Sbarre.Add(S);
	}

	// il blocco che ferma: invisibile, della misura della grata (lo sistema Sistema)
	Blocco = CreateDefaultSubobject<UBoxComponent>(TEXT("Blocco"));
	Blocco->SetupAttachment(Saracinesca);
	Blocco->SetMobility(EComponentMobility::Movable);
	Blocco->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Blocco->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Blocco->SetCanEverAffectNavigation(true);
	Sistema();
}

void AValdorsoGrata::Sistema()
{
	const bool bVero = Modello != nullptr;
	if (Ferro)
	{
		Ferro->SetStaticMesh(Modello);
		Ferro->SetVisibility(bVero);
	}
	for (int32 k = 0; k < Sbarre.Num(); ++k)
	{
		UStaticMeshComponent* S = Sbarre[k];
		if (!S)
		{
			continue;
		}
		S->SetVisibility(!bVero);
		// il cubo dell'Engine e' 100 cm con l'origine al centro
		if (k < SbarreInPiedi)
		{
			const float Y = -Larghezza / 2.f + Larghezza * (k + 0.5f) / SbarreInPiedi;
			S->SetRelativeLocation(FVector(0.f, Y, Altezza / 2.f));
			S->SetRelativeScale3D(FVector(SpessoreSbarra / 100.f, SpessoreSbarra / 100.f, Altezza / 100.f));
		}
		else
		{
			const int32 i = k - SbarreInPiedi;
			const float Z = Altezza * (i + 1.f) / (SbarreTraverso + 1.f);
			S->SetRelativeLocation(FVector(0.f, 0.f, Z));
			S->SetRelativeScale3D(FVector(SpessoreSbarra * 0.8f / 100.f, Larghezza / 100.f, SpessoreSbarra * 0.8f / 100.f));
		}
	}
	if (Blocco)
	{
		Blocco->SetBoxExtent(FVector(15.f, Larghezza / 2.f, Altezza / 2.f));
		Blocco->SetRelativeLocation(FVector(0.f, 0.f, Altezza / 2.f));
	}
}

void AValdorsoGrata::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Corsa = FMath::Min(Corsa, Altezza);
	Sistema();
	// nell'editor si vede come partira'
	if (Saracinesca)
	{
		Saracinesca->SetRelativeLocation(FVector(0.f, 0.f, bAlzataAllInizio ? Corsa : 0.f));
	}
}

void AValdorsoGrata::BeginPlay()
{
	Super::BeginPlay();
	Sistema();
	for (UStaticMeshComponent* S : Sbarre)
	{
		Colora(S, FLinearColor(0.035f, 0.035f, 0.04f));
	}
	if (HasAuthority())
	{
		bSu = bAlzataAllInizio;
	}
	QuotaAttuale = bSu ? Corsa : 0.f;
	Saracinesca->SetRelativeLocation(FVector(0.f, 0.f, QuotaAttuale));
	NataAlle = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void AValdorsoGrata::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoGrata, bSu);
}

void AValdorsoGrata::Muovi(bool bAlzare, AActor* Chi)
{
	if (!HasAuthority() || bAlzare == bSu)
	{
		return;
	}
	bSu = bAlzare;
	ValdorsoRegistroEventi::Annota(bSu ? TEXT("GrataSu") : TEXT("GrataGiu"), Chi, this, GetActorLocation());
	ForceNetUpdate();
	OnRep_Su();
}

void AValdorsoGrata::OnRep_Su()
{
	SetActorTickEnabled(true);
	const UWorld* Mondo = GetWorld();
	// (come le porte) non quando arriva gia' cosi' entrando nella valle
	if (Mondo && HasActorBegunPlay() && Mondo->GetTimeSeconds() - NataAlle > 1.0)
	{
		Suona(bSu ? TEXT("S_Grata_Sale") : TEXT("S_Grata_Scende"), 1.f);
	}
}

void AValdorsoGrata::Suona(const TCHAR* Nome, float Volume) const
{
	SuonaQui(this, Nome, bSu ? TEXT("S_Porta_Apri") : TEXT("S_Porta_Chiudi"), Volume, 1.f);
}

void AValdorsoGrata::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Voluta = bSu ? Corsa : 0.f;
	const float Passo = (bSu ? VelocitaSu : VelocitaGiu) * DeltaSeconds;
	const bool bArrivata = FMath::Abs(Voluta - QuotaAttuale) <= Passo;
	QuotaAttuale = bArrivata ? Voluta : QuotaAttuale + FMath::Sign(Voluta - QuotaAttuale) * Passo;
	Saracinesca->SetRelativeLocation(FVector(0.f, 0.f, QuotaAttuale));
	if (bArrivata)
	{
		SetActorTickEnabled(false);
		// il colpo del ferro che tocca terra (non entrando nella valle, con la grata gia' giu')
		const UWorld* Mondo = GetWorld();
		if (!bSu && Mondo && Mondo->GetTimeSeconds() - NataAlle > 1.0)
		{
			SuonaQui(this, TEXT("S_Grata_Colpo"), TEXT("S_Porta_Chiudi"), 1.f, 0.8f);
		}
	}
}

// ------------------------------------------------------------------------------------------------
// La leva
// ------------------------------------------------------------------------------------------------

AValdorsoLeva::AValdorsoLeva()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.03f;
	bReplicates = true;
	SetReplicatingMovement(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent->SetMobility(EComponentMobility::Static);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// la base (un ceppo di legno 50 x 30 x 60 cm) e l'asta di ferro (110 cm) che gira sul perno in cima
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(RootComponent);
	Perno = CreateDefaultSubobject<USceneComponent>(TEXT("Perno"));
	Perno->SetupAttachment(RootComponent);
	Perno->SetMobility(EComponentMobility::Movable);
	Perno->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Asta = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Asta"));
	Asta->SetupAttachment(Perno);
	Asta->SetMobility(EComponentMobility::Movable);
	if (Cubo.Succeeded())
	{
		Base->SetStaticMesh(Cubo.Object);
		Base->SetRelativeScale3D(FVector(0.5f, 0.3f, 0.6f));
		Base->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
		Asta->SetStaticMesh(Cubo.Object);
		Asta->SetRelativeScale3D(FVector(0.06f, 0.06f, 1.1f));
		Asta->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
		if (Semplice.Succeeded())
		{
			Base->SetMaterial(0, Semplice.Object);
			Asta->SetMaterial(0, Semplice.Object);
		}
	}
	Base->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Asta->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AValdorsoLeva::BeginPlay()
{
	Super::BeginPlay();
	Colora(Base, FLinearColor(0.2f, 0.11f, 0.05f));
	Colora(Asta, FLinearColor(0.04f, 0.04f, 0.045f));
	AngoloAttuale = (Grata && Grata->Su()) ? 35.f : -35.f;
	Perno->SetRelativeRotation(FRotator(AngoloAttuale, 0.f, 0.f));
}

FText AValdorsoLeva::TestoInterazione(const APawn* Chi) const
{
	if (!Grata)
	{
		return FText::GetEmpty();
	}
	if (!Chiave.IsNone() && !ValdorsoChiavi::Ha(Chi, Chiave))
	{
		return LOCTEXT("Lucchetto", "La leva ha un lucchetto");
	}
	return Grata->Su() ? LOCTEXT("Abbassa", "Abbassa la grata") : LOCTEXT("Alza", "Alza la grata");
}

FVector AValdorsoLeva::PuntoInterazione() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 90.f);
}

void AValdorsoLeva::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi || !Grata)
	{
		return;
	}
	if (!Chiave.IsNone() && !ValdorsoChiavi::PuoAprire(Chi, Chiave))
	{
		ValdorsoRegistroEventi::Annota(TEXT("LevaChiusa"), Chi, this, GetActorLocation(), TEXT("ha provato la leva della grata"));
		MulticastLucchetto();
		return;
	}
	Grata->Muovi(!Grata->Su(), Chi);
}

void AValdorsoLeva::MulticastLucchetto_Implementation()
{
	SuonaQui(this, TEXT("S_Porta_Chiave"), TEXT("S_Porta_Chiave"), 0.8f, 0.9f);
}

void AValdorsoLeva::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// l'asta segue la grata: in avanti quando e' alzata, indietro quando e' giu' (sui PC lo stato arriva con la grata)
	const float Voluto = (Grata && Grata->Su()) ? 35.f : -35.f;
	AngoloAttuale = FMath::FInterpConstantTo(AngoloAttuale, Voluto, DeltaSeconds, 140.f);
	Perno->SetRelativeRotation(FRotator(AngoloAttuale, 0.f, 0.f));
}

#undef LOCTEXT_NAMESPACE
