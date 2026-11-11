// Valdorso - Il pozzo del villaggio (vedi ValdorsoPozzo.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoPozzo.h"
#include "Valdorso.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Components/SphereComponent.h"
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
