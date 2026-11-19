// Valdorso - Il registro della ronda. Vedi ValdorsoRegistroRonda.h. Scritto da Claude il 19/11/2026.

#include "ValdorsoRegistroRonda.h"
#include "ValdorsoMilizia.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "ValdorsoRegistroRonda"

AValdorsoRegistroRonda::AValdorsoRegistroRonda()
{
	PrimaryActorTick.bCanEverTick = false;

	Modello = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Modello"));
	RootComponent = Modello;
	Modello->SetMobility(EComponentMobility::Static);
	Modello->SetCollisionProfileName(TEXT("BlockAll"));
}

FText AValdorsoRegistroRonda::TestoInterazione(const APawn* Chi) const
{
	const APlayerController* Giocatore = Chi ? Cast<APlayerController>(Chi->GetController()) : nullptr;
	return ValdorsoMilizia::GradoDi(Giocatore) > 0
		? LOCTEXT("Firma", "Firma il registro della ronda")
		: LOCTEXT("Leggi", "Leggi il registro della ronda");
}

void AValdorsoRegistroRonda::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	if (APlayerController* Giocatore = Cast<APlayerController>(Chi->GetController()))
	{
		ValdorsoMilizia::FirmaRegistro(Giocatore, this);
	}
}

#undef LOCTEXT_NAMESPACE
