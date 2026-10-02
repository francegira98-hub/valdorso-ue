// Valdorso - La schivata (vedi il .h).

#include "ValdorsoAbilitaSchivata.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoEffetti.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UValdorsoAbilitaSchivata::UValdorsoAbilitaSchivata()
{
	// Una copia dell'abilità per ogni personaggio; il PC la esegue subito, il server la conferma.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Il costo: se non ci sono 20 di stamina, la schivata non parte.
	CostGameplayEffectClass = UValdorsoGE_CostoSchivata::StaticClass();
}

void UValdorsoAbilitaSchivata::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Paga il costo; se non si può, l'abilità finisce subito.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Personaggio = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (Personaggio == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// La direzione: dove il giocatore si sta muovendo; se è fermo, in avanti.
	FVector Direzione = Personaggio->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
	if (Direzione.IsNearlyZero())
	{
		Direzione = Personaggio->GetActorForwardVector().GetSafeNormal2D();
	}

	// L'animazione, se nel Blueprint del personaggio è stata scelta (Montaggio Schivata).
	if (const AValdorsoCharacter* Valdorso = Cast<AValdorsoCharacter>(Personaggio))
	{
		if (Valdorso->MontaggioSchivata)
		{
			UAbilityTask_PlayMontageAndWait* Animazione = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, Valdorso->MontaggioSchivata, 1.f);
			Animazione->ReadyForActivation();
		}
	}

	// Lo scatto: una spinta costante gestita dal motore, prevista dal PC e confermata dal server.
	UAbilityTask_ApplyRootMotionConstantForce* Spinta = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, NAME_None, Direzione, Forza, Durata, false, nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 300.f, true);
	Spinta->OnFinish.AddDynamic(this, &UValdorsoAbilitaSchivata::QuandoFinisce);
	Spinta->ReadyForActivation();
}

void UValdorsoAbilitaSchivata::QuandoFinisce()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}