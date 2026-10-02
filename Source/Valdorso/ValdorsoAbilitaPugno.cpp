// Valdorso - Il pugno (vedi il .h).

#include "ValdorsoAbilitaPugno.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoEffetti.h"
#include "ValdorsoEtichette.h"
#include "ValdorsoRegistroEventi.h"
#include "CombatDamageable.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Valdorso.h"

UValdorsoAbilitaPugno::UValdorsoAbilitaPugno()
{
	// Una copia dell'abilità per ogni personaggio; il PC la esegue subito, il server la conferma.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Il costo: se non ci sono 10 di stamina, il pugno non parte.
	CostGameplayEffectClass = UValdorsoGE_CostoPugno::StaticClass();

	// Mentre dura, il personaggio porta l'etichetta "sta tirando un pugno"; durante la schivata il pugno non parte.
	ActivationOwnedTags.AddTag(ValdorsoEtichette::Stato_Pugno);
	ActivationBlockedTags.AddTag(ValdorsoEtichette::Stato_Schivata);
}

bool UValdorsoAbilitaPugno::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// Solo con i piedi a terra: niente pugni in salto, in caduta o mentre si scavalca (lì il movimento è "in volo").
	const ACharacter* Personaggio = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Personaggio && Personaggio->GetCharacterMovement() && Personaggio->GetCharacterMovement()->IsMovingOnGround();
}

void UValdorsoAbilitaPugno::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Paga il costo; se non si può, l'abilità finisce subito.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bColpoFatto = false;

	const AValdorsoCharacter* Valdorso = Cast<AValdorsoCharacter>(ActorInfo->AvatarActor.Get());
	if (Valdorso == nullptr || Valdorso->MontaggioPugno == nullptr)
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Pugno: manca il Montaggio Pugno nel Blueprint del personaggio"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Aspetta la notifica "Valdorso: Colpo" del montaggio (una volta sola).
	UAbilityTask_WaitGameplayEvent* Attesa = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ValdorsoEtichette::Evento_Colpo, nullptr, true, true);
	Attesa->EventReceived.AddDynamic(this, &UValdorsoAbilitaPugno::QuandoColpisce);
	Attesa->ReadyForActivation();

	// L'animazione: AM_Pugno, sulla corsia della parte alta del corpo.
	UAbilityTask_PlayMontageAndWait* Animazione = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Valdorso->MontaggioPugno, 1.f);
	Animazione->OnBlendOut.AddDynamic(this, &UValdorsoAbilitaPugno::QuandoFinisce);
	Animazione->OnCompleted.AddDynamic(this, &UValdorsoAbilitaPugno::QuandoFinisce);
	Animazione->OnInterrupted.AddDynamic(this, &UValdorsoAbilitaPugno::QuandoFinisce);
	Animazione->OnCancelled.AddDynamic(this, &UValdorsoAbilitaPugno::QuandoFinisce);
	Animazione->ReadyForActivation();
}

void UValdorsoAbilitaPugno::QuandoFinisce()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UValdorsoAbilitaPugno::QuandoColpisce(FGameplayEventData Dati)
{
	if (bColpoFatto)
	{
		return;
	}
	bColpoFatto = true;

	// Chi è stato colpito lo decide solo il server.
	if (CurrentActorInfo && CurrentActorInfo->IsNetAuthority())
	{
		EseguiColpo();
	}
}

void UValdorsoAbilitaPugno::EseguiColpo()
{
	ACharacter* Personaggio = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UWorld* Mondo = Personaggio ? Personaggio->GetWorld() : nullptr;
	UAbilitySystemComponent* MioContenitore = GetAbilitySystemComponentFromActorInfo();
	if (Mondo == nullptr || MioContenitore == nullptr)
	{
		return;
	}

	// Una sfera che parte dalla mano destra e va avanti per la portata del pugno.
	USkeletalMeshComponent* Corpo = Personaggio->GetMesh();
	const FVector Inizio = (Corpo && Corpo->DoesSocketExist(OssoMano)) ? Corpo->GetSocketLocation(OssoMano) : Personaggio->GetActorLocation();
	const FVector Avanti = Personaggio->GetActorForwardVector();
	const FVector Fine = Inizio + Avanti * Portata;

	FCollisionObjectQueryParams Oggetti;
	Oggetti.AddObjectTypesToQuery(ECC_Pawn);
	Oggetti.AddObjectTypesToQuery(ECC_WorldDynamic);
	Oggetti.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoPugno), false, Personaggio);

	TArray<FHitResult> Colpi;
	Mondo->SweepMultiByObjectType(Colpi, Inizio, Fine, FQuat::Identity, Oggetti, FCollisionShape::MakeSphere(Raggio), Parametri);

	TSet<AActor*> GiaColpiti;
	bool bColpitoQualcuno = false;

	for (const FHitResult& Colpo : Colpi)
	{
		AActor* Bersaglio = Colpo.GetActor();
		if (Bersaglio == nullptr || Bersaglio == Personaggio || GiaColpiti.Contains(Bersaglio))
		{
			continue;
		}
		GiaColpiti.Add(Bersaglio);

		if (UAbilitySystemComponent* SuoContenitore = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Bersaglio))
		{
			// Chi ha le statistiche (giocatori, più avanti PNG e mostri): il danno è un effetto GAS deciso dal server.
			FGameplayEffectSpecHandle Effetto = MakeOutgoingGameplayEffectSpec(UValdorsoGE_DannoPugno::StaticClass(), GetAbilityLevel());
			if (!Effetto.IsValid())
			{
				continue;
			}
			Effetto.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Danno, -Danno);
			MioContenitore->ApplyGameplayEffectSpecToTarget(*Effetto.Data.Get(), SuoContenitore);
		}
		else if (ICombatDamageable* Colpibile = Cast<ICombatDamageable>(Bersaglio))
		{
			// I manichini e le casse di Variant_Combat: si colpiscono con la loro interfaccia (utili per le prove).
			const FVector Spinta = Avanti * 250.f + FVector::UpVector * 100.f;
			Colpibile->ApplyDamage(Danno, Personaggio, Colpo.ImpactPoint, Spinta);
		}
		else
		{
			continue;
		}

		// Colpire qualcuno un giorno può essere un crimine: passa dal registro eventi.
		ValdorsoRegistroEventi::Annota(TEXT("Colpo"), Personaggio, Bersaglio, Colpo.ImpactPoint,
			FString::Printf(TEXT("pugno, danno %.0f"), Danno));
		bColpitoQualcuno = true;
	}

	if (!bColpitoQualcuno)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Pugno: a vuoto"));
	}
}
