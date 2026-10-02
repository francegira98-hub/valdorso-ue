// Valdorso - La notifica "Valdorso: Colpo" (vedi il .h).

#include "ValdorsoNotificaColpo.h"
#include "ValdorsoEtichette.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"

void UValdorsoNotificaColpo::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Proprietario = MeshComp ? MeshComp->GetOwner() : nullptr;

	// Nell'anteprima dell'editor il manichino non ha abilità: lì non si fa niente.
	if (Proprietario == nullptr || UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Proprietario) == nullptr)
	{
		return;
	}

	FGameplayEventData Dati;
	Dati.EventTag = ValdorsoEtichette::Evento_Colpo;
	Dati.Instigator = Proprietario;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Proprietario, ValdorsoEtichette::Evento_Colpo, Dati);
}

FString UValdorsoNotificaColpo::GetNotifyName_Implementation() const
{
	return TEXT("Valdorso: Colpo");
}
