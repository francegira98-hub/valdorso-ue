// Valdorso - Il baule del colono (vedi ValdorsoBaule.h). Scritto da Claude il 12/11/2026.

#include "ValdorsoBaule.h"
#include "Valdorso.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "SValdorsoBaule.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "ValdorsoBaule"

AValdorsoBaule::AValdorsoBaule()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	SetRootComponent(Presa);
	Presa->InitSphereRadius(140.f);
	Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Presa->SetCollisionObjectType(ECC_WorldDynamic);
	Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
	Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

bool AValdorsoBaule::Vicino(const APawn* Colono)
{
	UWorld* Mondo = Colono ? Colono->GetWorld() : nullptr;
	if (!Mondo)
	{
		return false;
	}
	for (TActorIterator<AValdorsoBaule> It(Mondo); It; ++It)
	{
		// vicino, e dalla stessa parte del muro (il baule sta contro il muro della locanda)
		if (FVector::Dist(Colono->GetActorLocation(), It->GetActorLocation()) <= Raggio && ValdorsoInterazione::SiVede(Colono, *It))
		{
			return true;
		}
	}
	return false;
}

FText AValdorsoBaule::TestoInterazione(const APawn* Chi) const
{
	return LOCTEXT("Apri", "Apri il tuo baule");
}

void AValdorsoBaule::Interagisci(APawn* Chi)
{
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi ? Cast<APlayerController>(Chi->GetController()) : nullptr);
	if (!HasAuthority() || !Zaino || FVector::Dist(Chi->GetActorLocation(), GetActorLocation()) > Raggio)
	{
		return;
	}
	Zaino->ClientApriBaule();
}

// --- Lo zaino davanti al baule (UValdorsoZainoComponent, le richieste del PC) -------------------------------------

void UValdorsoZainoComponent::ClientApriBaule_Implementation()
{
	ValdorsoBauleSchermo::Apri(Cast<APlayerController>(GetOwner()));
}

namespace
{
	/** Il colono puo' usare il baule adesso? (c'e', e' in piedi, e' davanti al baule) */
	bool BaulePuoUsare(const UValdorsoZainoComponent* Zaino)
	{
		const APlayerController* Controllore = Zaino ? Cast<APlayerController>(Zaino->GetOwner()) : nullptr;
		const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
		if (!Corpo)
		{
			return false;
		}
		const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
		return !(Pericoli && Pericoli->Caduto()) && AValdorsoBaule::Vicino(Corpo);
	}
}

void UValdorsoZainoComponent::ServerDeposita_Implementation(int32 Posto)
{
	if (TroppeRichieste() || !Zaino.Posti.IsValidIndex(Posto) || Zaino.Posti[Posto].Vuoto())
	{
		return;
	}
	if (!BaulePuoUsare(this))
	{
		Avvisa(LOCTEXT("Lontano", "Sei troppo lontano dal baule."), 2.f);
		return;
	}
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Zaino.Posti[Posto].Id);
	if (!ValdorsoZaino::SiPuoDepositare(Zaino, Posto))
	{
		Avvisa(FText::Format(LOCTEXT("NonNelBaule", "{0}: questo lo tieni sempre con te."), FText::FromString(Tipo ? Tipo->Nome : TEXT("?"))), 2.5f);
		return;
	}
	if (!Cambia([Posto](FValdorsoZaino& Dentro) { return ValdorsoZaino::Deposita(Dentro, Posto); }))
	{
		Avvisa(LOCTEXT("BaulePieno", "Il baule e' pieno."), 2.f);
	}
}

void UValdorsoZainoComponent::ServerRitira_Implementation(int32 PostoBaule)
{
	if (TroppeRichieste() || !Zaino.Baule.IsValidIndex(PostoBaule) || Zaino.Baule[PostoBaule].Vuoto())
	{
		return;
	}
	if (!BaulePuoUsare(this))
	{
		Avvisa(LOCTEXT("LontanoRitira", "Sei troppo lontano dal baule."), 2.f);
		return;
	}
	if (!Cambia([PostoBaule](FValdorsoZaino& Dentro) { return ValdorsoZaino::Ritira(Dentro, PostoBaule); }))
	{
		Avvisa(LOCTEXT("ZainoPieno", "Nello zaino non c'e' piu' posto."), 2.f);
	}
}

#undef LOCTEXT_NAMESPACE
