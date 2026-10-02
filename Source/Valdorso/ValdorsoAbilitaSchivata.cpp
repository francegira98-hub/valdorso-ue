// Valdorso - La schivata (vedi il .h).
// Passo 2 (02/10): con la capriola di Mixamo, usa solo il pezzo scelto nel Blueprint e ne adatta la spinta.
// Passo 2.3k: se il montaggio ha il suo spostamento (root motion), è l'animazione a muovere il personaggio,
// così corpo e capsula restano sempre insieme (niente corpo dentro i muri, niente passo indietro alla fine).
// Passo 2.3n: prima di partire guarda davanti; se un muro è troppo vicino, al posto della capriola fa lo scatto corto.

#include "ValdorsoAbilitaSchivata.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoEffetti.h"
#include "ValdorsoEtichette.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

UValdorsoAbilitaSchivata::UValdorsoAbilitaSchivata()
{
	// Una copia dell'abilità per ogni personaggio; il PC la esegue subito, il server la conferma.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Il costo: se non ci sono 20 di stamina, la schivata non parte.
	CostGameplayEffectClass = UValdorsoGE_CostoSchivata::StaticClass();

	// Mentre dura, il personaggio porta l'etichetta "sta schivando" (il pugno non parte).
	ActivationOwnedTags.AddTag(ValdorsoEtichette::Stato_Schivata);
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

	// Di serie: spinta e durata dell'abilità, montaggio dall'inizio a velocità normale.
	float ForzaUsata = Forza;
	float DurataUsata = Durata;
	float InizioMontaggio = 0.f;
	float VelocitaMontaggio = 1.f;

	// L'animazione, se nel Blueprint del personaggio è stata scelta (Montaggio Schivata).
	if (const AValdorsoCharacter* Valdorso = Cast<AValdorsoCharacter>(Personaggio))
	{
		// C'è spazio per la capriola? Si fa scorrere la capsula in avanti per lo spazio minimo.
		bool bSpazioLibero = true;
		if (UWorld* Mondo = Personaggio->GetWorld())
		{
			const UCapsuleComponent* Capsula = Personaggio->GetCapsuleComponent();
			const FVector Inizio = Personaggio->GetActorLocation();
			const FVector Fine = Inizio + Direzione * Valdorso->CapriolaSpazioMinimo;
			FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoSpazioCapriola), false, Personaggio);
			FHitResult Urto;
			bSpazioLibero = !Mondo->SweepSingleByChannel(Urto, Inizio, Fine, FQuat::Identity, ECC_Pawn,
				FCollisionShape::MakeCapsule(Capsula->GetScaledCapsuleRadius(), Capsula->GetScaledCapsuleHalfHeight() * 0.8f), Parametri);
		}

		UAnimMontage* Montaggio = Valdorso->MontaggioSchivata;
		const bool bCapriola = Montaggio && bSpazioLibero;
		if (!bCapriola && Valdorso->MontaggioScattoCorto)
		{
			// Muro vicino: lo scatto corto, con la spinta di serie (la capsula si ferma contro il muro).
			Montaggio = Valdorso->MontaggioScattoCorto;
		}

		if (Montaggio)
		{
			if (bCapriola)
			{
				// La capriola: si usa solo il pezzo tra il primo e l'ultimo fotogramma scelti nel Blueprint.
				const int32 Primo = Valdorso->CapriolaPrimoFotogramma;
				const int32 Ultimo = Valdorso->CapriolaUltimoFotogramma;
				if (Ultimo > Primo && Valdorso->CapriolaVelocita > 0.f)
				{
					VelocitaMontaggio = Valdorso->CapriolaVelocita;
					InizioMontaggio = Primo / 30.f;
					DurataUsata = (Ultimo - Primo) / 30.f / VelocitaMontaggio;
					ForzaUsata = Valdorso->CapriolaDistanza / FMath::Max(DurataUsata, 0.1f);
				}
			}

			// Quando l'abilità finisce il montaggio sfuma da solo.
			UAbilityTask_PlayMontageAndWait* Animazione = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, Montaggio, VelocitaMontaggio, NAME_None, true, 1.f, InizioMontaggio);
			Animazione->ReadyForActivation();

			// Capriola con il suo spostamento: si gira il personaggio verso la direzione scelta,
			// l'animazione lo porta avanti da sola, e l'abilità finisce all'ultimo fotogramma scelto.
			if (bCapriola && Montaggio->HasRootMotion())
			{
				Personaggio->SetActorRotation(Direzione.Rotation());

				UAbilityTask_WaitDelay* Attesa = UAbilityTask_WaitDelay::WaitDelay(this, DurataUsata);
				Attesa->OnFinish.AddDynamic(this, &UValdorsoAbilitaSchivata::QuandoFinisce);
				Attesa->ReadyForActivation();
				return;
			}
		}
	}

	// Lo scatto: una spinta costante gestita dal motore, prevista dal PC e confermata dal server.
	UAbilityTask_ApplyRootMotionConstantForce* Spinta = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, NAME_None, Direzione, ForzaUsata, DurataUsata, false, nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 300.f, true);
	Spinta->OnFinish.AddDynamic(this, &UValdorsoAbilitaSchivata::QuandoFinisce);
	Spinta->ReadyForActivation();
}

void UValdorsoAbilitaSchivata::QuandoFinisce()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}