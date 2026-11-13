// Valdorso - Il pozzo del villaggio (vedi ValdorsoPozzo.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoPozzo.h"
#include "Valdorso.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "ValdorsoPozzo"

AValdorsoPozzo::AValdorsoPozzo()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	SetRootComponent(Presa);
	Presa->InitSphereRadius(160.f);
	Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Presa->SetCollisionObjectType(ECC_WorldDynamic);
	Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
	Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// (13/11) l'acqua dentro: il cilindro di serie (100 cm) schiacciato in un disco di 138 cm, appena meno della canna
	// del pozzo (raggio 70 cm). L'attore sta 60 cm sopra la base del pozzo e il bordo è a 90 cm: l'acqua 130 cm più giù.
	Specchio = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Specchio"));
	Specchio->SetupAttachment(Presa);
	Specchio->SetMobility(EComponentMobility::Movable);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Disco(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Disco.Succeeded())
	{
		Specchio->SetStaticMesh(Disco.Object);
	}
	Specchio->SetRelativeLocation(FVector(0.f, 0.f, -100.f));
	Specchio->SetRelativeScale3D(FVector(1.38f, 1.38f, 0.01f));
	Specchio->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Specchio->SetCastShadow(false);
	Specchio->SetCanEverAffectNavigation(false);
}

void AValdorsoPozzo::BeginPlay()
{
	Super::BeginPlay();
	if (!Specchio || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// il materiale dell'acqua del villaggio (lo stesso dei ruscelli); se manca, un blu scuro che luccica un po'
	if (UMaterialInterface* Acqua = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/Valle/Villaggio/Materiali/MI_Villaggio_Acqua.MI_Villaggio_Acqua"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		Specchio->SetMaterial(0, Acqua);
	}
	else if (UMaterialInterface* Semplice = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		if (UMaterialInstanceDynamic* Blu = UMaterialInstanceDynamic::Create(Semplice, this))
		{
			Blu->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.06f, 0.08f));
			Specchio->SetMaterial(0, Blu);
		}
	}
}

FText AValdorsoPozzo::TestoInterazione(const APawn* Chi) const
{
	return LOCTEXT("Tira", "Tira su l'acqua dal pozzo");
}

void AValdorsoPozzo::Interagisci(APawn* Chi)
{
	AValdorsoPlayerController* Giocatore = Chi ? Cast<AValdorsoPlayerController>(Chi->GetController()) : nullptr;
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Giocatore);
	if (!HasAuthority() || !Zaino || FVector::Dist(Chi->GetActorLocation(), GetActorLocation()) > 400.f)
	{
		return;
	}
	const FName Vuoto(TEXT("Secchio"));
	const FName Pieno(TEXT("Secchio_Acqua_Chiara"));
	const FValdorsoTipoOggetto* TipoPieno = ValdorsoOggetti::Trova(Pieno);
	bool bSecchio = false;
	bool bBorraccia = false;
	Zaino->Cambia([&](FValdorsoZaino& Dentro)
	{
		for (FValdorsoPostoZaino& Posto : Dentro.Posti)
		{
			if (!Posto.Vuoto() && Posto.Id == Vuoto && TipoPieno)
			{
				Posto.Id = Pieno;
				Posto.Cariche = TipoPieno->Cariche;
				for (FName& Azione : Dentro.Azioni)
				{
					Azione = Azione == Vuoto ? Pieno : Azione;
				}
				bSecchio = true;
				break;
			}
		}
		bBorraccia = ValdorsoZaino::RiempiBorracce(Dentro);
		return bSecchio || bBorraccia;
	});
	Giocatore->ClientAvviso(bSecchio ? LOCTEXT("Secchio", "Cali il secchio e lo tiri su pieno: acqua chiara, fresca di pietra.")
		: (bBorraccia ? LOCTEXT("Borraccia", "Riempi la borraccia al pozzo. (Con un secchio, del falegname, ne porti via di piu'.)")
			: LOCTEXT("Mani", "Bevi un sorso con le mani. Per portarla via serve un secchio: il falegname ne ha.")), 3.5f, 0.f);
}

#undef LOCTEXT_NAMESPACE
