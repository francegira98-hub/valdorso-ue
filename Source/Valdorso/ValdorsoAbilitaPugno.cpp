// Valdorso - Il pugno (vedi il .h).

#include "ValdorsoAbilitaPugno.h"
#include "ValdorsoArmi.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoEffetti.h"
#include "ValdorsoEtichette.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoQualita.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "ValdorsoFreccia.h"
#include "ValdorsoPlayerController.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
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
	// (14/11, v0.2.3) l'arco: senza frecce nello zaino non si tende (lo zaino lo sanno il server e il PC di chi gioca)
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const UValdorsoArmiComponent* ArmiInMano = UValdorsoArmiComponent::Di(Avatar);
	const FValdorsoTipoOggetto* ArmaInMano = ArmiInMano ? ArmiInMano->Arma() : nullptr;
	const bool bArco = ArmaInMano && ArmaInMano->bDueMani;
	if (bArco)
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::DelCorpo(Avatar);
		if (Zaino && ValdorsoZaino::Conta(Zaino->Contenuto(), ValdorsoOggetti::IdFrecce) <= 0)
		{
			if (ActorInfo->IsNetAuthority())
			{
				if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Zaino->GetOwner()))
				{
					Giocatore->ClientAvviso(FText::FromString(TEXT("Non hai frecce: il falegname le vende a dieci.")), 2.5f, 0.f);
				}
			}
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

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

	// (14/11, v0.2.3) l'arco: si tende, poi parte la freccia (niente animazione del pugno)
	if (bArco)
	{
		UAbilityTask_WaitDelay* Tendi = UAbilityTask_WaitDelay::WaitDelay(this, ValdorsoArmi::SecondiTendere);
		Tendi->OnFinish.AddDynamic(this, &UValdorsoAbilitaPugno::QuandoTira);
		Tendi->ReadyForActivation();
		return;
	}

	// (14/11, v0.2.3) con un'arma in mano: l'animazione della sua famiglia, sulla stessa corsia del pugno
	UAnimMontage* Montaggio = Valdorso->MontaggioPugno;
	float Velocita = 1.f;
	float VelocitaGioco = 1.f;
	float AttesaColpo = -1.f;
	const FValdorsoTipoOggetto* ConCosa = ArmaInMano ? ArmaInMano : (ArmiInMano ? ArmiInMano->Scudo() : nullptr);
	MontaggioArma = nullptr;
	if (ConCosa)
	{
		float Istante = 0.5f;
		if (UAnimSequenceBase* Colpo = ValdorsoArmi::AnimazioneColpo(ConCosa->Id, NumeroColpo++, Istante, Velocita))
		{
			const FName Corsia = Valdorso->MontaggioPugno->SlotAnimTracks.Num() > 0
				? Valdorso->MontaggioPugno->SlotAnimTracks[0].SlotName : FName(TEXT("DefaultSlot"));
			MontaggioArma = UAnimMontage::CreateSlotAnimationAsDynamicMontage(Colpo, Corsia, 0.1f, 0.2f, 1.f);
			if (MontaggioArma)
			{
				Montaggio = MontaggioArma;
				VelocitaGioco = FMath::Max(0.1f, Velocita);
				AttesaColpo = Colpo->GetPlayLength() * Istante / FMath::Max(0.1f, Velocita);
			}
		}
	}

	if (AttesaColpo >= 0.f)
	{
		// l'animazione dell'arma non ha la notifica del colpo: il colpo arriva dopo un tempo fisso
		UAbilityTask_WaitDelay* Attesa = UAbilityTask_WaitDelay::WaitDelay(this, AttesaColpo);
		Attesa->OnFinish.AddDynamic(this, &UValdorsoAbilitaPugno::QuandoArrivaIlColpo);
		Attesa->ReadyForActivation();
	}
	else
	{
		// Aspetta la notifica "Valdorso: Colpo" del montaggio (una volta sola).
		UAbilityTask_WaitGameplayEvent* Attesa = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, ValdorsoEtichette::Evento_Colpo, nullptr, true, true);
		Attesa->EventReceived.AddDynamic(this, &UValdorsoAbilitaPugno::QuandoColpisce);
		Attesa->ReadyForActivation();
	}

	// L'animazione: AM_Pugno (o quella dell'arma), sulla corsia della parte alta del corpo.
	UAbilityTask_PlayMontageAndWait* Animazione = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montaggio, VelocitaGioco);
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

void UValdorsoAbilitaPugno::QuandoArrivaIlColpo()
{
	QuandoColpisce(FGameplayEventData());
}

void UValdorsoAbilitaPugno::QuandoTira()
{
	APawn* Personaggio = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (Personaggio && CurrentActorInfo && CurrentActorInfo->IsNetAuthority())
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::DelCorpo(Personaggio);
		const UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(Personaggio);
		const FValdorsoTipoOggetto* Arco = Armi ? Armi->Arma() : nullptr;
		if (Zaino && Arco && Arco->bDueMani && ValdorsoZaino::Conta(Zaino->Contenuto(), ValdorsoOggetti::IdFrecce) > 0)
		{
			// dove guarda la telecamera; parte davanti al petto, un poco a sinistra (la mano dell'arco)
			const FRotator Mira = Personaggio->GetControlRotation();
			const FVector Avanti = Mira.Vector();
			const FVector Da = Personaggio->GetActorLocation() + FVector(0.f, 0.f, 55.f) + Personaggio->GetActorForwardVector() * 45.f
				- Personaggio->GetActorRightVector() * 12.f;
			const float DannoFreccia = Arco->DannoArma * Zaino->ForzaInMano(false, false);
			if (AValdorsoFreccia::Scocca(Personaggio, Da, Avanti, DannoFreccia, Arco->EquilibrioArma))
			{
				// la freccia esce dallo zaino solo se e' partita davvero
				Zaino->Cambia([](FValdorsoZaino& Z) { return ValdorsoZaino::TogliOggetto(Z, ValdorsoOggetti::IdFrecce, 1); });
				// l'arco si consuma a ogni tiro, e stanca un poco
				Zaino->ConsumaInMano(false, 1);
				if (UAbilitySystemComponent* MioContenitore = GetAbilitySystemComponentFromActorInfo(); MioContenitore && Arco->StaminaArma > 0.f)
				{
					FGameplayEffectSpecHandle Fatica = MioContenitore->MakeOutgoingSpec(UValdorsoGE_Fatica::StaticClass(), 1.f, MioContenitore->MakeEffectContext());
					if (Fatica.IsValid())
					{
						Fatica.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Stamina, -Arco->StaminaArma);
						MioContenitore->ApplyGameplayEffectSpecToSelf(*Fatica.Data.Get());
					}
				}
				ValdorsoCombattimento::Suona(Personaggio->GetWorld(), ValdorsoCombattimento::SuonoPericoli(TEXT("S_Fodero_Sfodera")),
					Da, 1500.f, 0.6f, 1.6f);
			}
		}
	}
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
	// (14/11, v0.2.3) il grado dell'arma (e la meta' se e' rovinata)
	UValdorsoZainoComponent* MioZaino = UValdorsoZainoComponent::DelCorpo(Personaggio);
	const bool bConLoScudo = !Arma && Scudo;
	const float PerGrado = (ConCosa && MioZaino) ? MioZaino->ForzaInMano(bConLoScudo, false) : 1.f;
	const float DannoColpo = ConCosa ? ConCosa->DannoArma * PerGrado : Danno;
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
	else if (ConCosa && MioZaino)
	{
		// (14/11, v0.2.3) un colpo andato a segno consuma l'arma (una volta per colpo, anche se ne prende due)
		MioZaino->ConsumaInMano(bConLoScudo, 1);
	}
}
