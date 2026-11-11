// Valdorso - Il pugno (vedi il .h).

#include "ValdorsoAbilitaPugno.h"
#include "ValdorsoArmi.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoEffetti.h"
#include "ValdorsoEtichette.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoScherma.h"
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

	// (09/11) con l'arma in mano: il suo danno, la sua portata, il suo equilibrio e il suo suono; e piu' fatica
	const FValdorsoTipoOggetto* Arma = nullptr;
	const FValdorsoTipoOggetto* Scudo = nullptr;
	if (const UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(Personaggio))
	{
		Arma = Armi->Arma();
		Scudo = Armi->Scudo();
	}
	// solo lo scudo (niente nella destra): si colpisce con lo scudo
	const FValdorsoTipoOggetto* ConCosa = Arma ? Arma : Scudo;
	const float DannoColpo = ConCosa ? ConCosa->DannoArma : Danno;
	// (11/11, v0.1.5) la guardia: la lunga arriva piu' lontano, l'alta stanca di piu' (ValdorsoScherma.h)
	float PerGuardiaPortata = 1.f;
	float PerGuardiaFatica = 1.f;
	if (const UValdorsoSchermaComponent* Scherma = UValdorsoSchermaComponent::Di(Personaggio); Scherma && !Scherma->InCambio())
	{
		PerGuardiaPortata = ValdorsoScherma::PortataGuardia(Scherma->GetGuardia());
		PerGuardiaFatica = ValdorsoScherma::StaminaGuardia(Scherma->GetGuardia());
	}
	const float PortataColpo = (ConCosa ? ConCosa->PortataArma : Portata) * PerGuardiaPortata;
	const float EquilibrioColpo = ConCosa ? ConCosa->EquilibrioArma : ValdorsoPericoli::EquilibrioPerPugno;
	const EValdorsoTipoColpo TipoColpo = ConCosa ? static_cast<EValdorsoTipoColpo>(ConCosa->TipoColpo) : EValdorsoTipoColpo::Pugno;
	if (ConCosa && ConCosa->StaminaArma > 0.f)
	{
		FGameplayEffectSpecHandle Fatica = MioContenitore->MakeOutgoingSpec(UValdorsoGE_Fatica::StaticClass(), 1.f, MioContenitore->MakeEffectContext());
		if (Fatica.IsValid())
		{
			Fatica.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Stamina, -ConCosa->StaminaArma * PerGuardiaFatica);
			MioContenitore->ApplyGameplayEffectSpecToSelf(*Fatica.Data.Get());
		}
	}

	// Una sfera che parte dalla mano destra e va avanti per la portata del pugno (o dell'arma).
	USkeletalMeshComponent* Corpo = Personaggio->GetMesh();
	const FVector Inizio = (Corpo && Corpo->DoesSocketExist(OssoMano)) ? Corpo->GetSocketLocation(OssoMano) : Personaggio->GetActorLocation();
	const FVector Avanti = Personaggio->GetActorForwardVector();
	const FVector Fine = Inizio + Avanti * PortataColpo;

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

		// (09/11) il colpo passa da ValdorsoCombattimento: danno ed equilibrio, la pace del battito, la spinta, il suono,
		// l'arresto e la telecamera che trema, il registro eventi
		if (!ValdorsoCombattimento::Colpisci(Personaggio, Bersaglio, DannoColpo, EquilibrioColpo, Colpo.ImpactPoint, TipoColpo))
		{
			continue;
		}
		bColpitoQualcuno = true;
	}

	if (!bColpitoQualcuno)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Pugno: a vuoto"));
	}
}
