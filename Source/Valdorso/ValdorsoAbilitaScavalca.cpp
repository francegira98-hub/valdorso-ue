// Valdorso - Scavalcare e salire sugli ostacoli bassi (vedi il .h). Versione 3b del 02/10.

#include "ValdorsoAbilitaScavalca.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoEffetti.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier.h"
#include "AnimNotifyState_MotionWarping.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace ValdorsoScavalca
{
	const float AltezzaMin = 40.f;       // sotto: basta camminare o saltare
	const float AltezzaMax = 130.f;      // sopra: troppo alto (arrampicata più avanti)
	const float DistanzaFermo = 100.f;   // quanto lontano si guarda da fermi...
	const float DistanzaCorsa = 250.f;   // ...e correndo (in mezzo: in proporzione alla velocità)
	const float VelocitaCorsa = 500.f;
	const float SpessoreScavalca = 70.f; // fino a qui si scavalca, oltre si sale sopra
	const float SpessoreMax = 120.f;     // oltre si considera un piano su cui salire
	const float Passo = 10.f;
}

UValdorsoAbilitaScavalca::UValdorsoAbilitaScavalca()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CostGameplayEffectClass = UValdorsoGE_CostoScavalca::StaticClass();
}

bool UValdorsoAbilitaScavalca::TrovaOstacolo(const ACharacter* Personaggio, FValdorsoOstacolo& Risultato)
{
	using namespace ValdorsoScavalca;
	Risultato = FValdorsoOstacolo();

	if (Personaggio == nullptr || Personaggio->GetWorld() == nullptr)
	{
		return false;
	}

	UWorld* Mondo = Personaggio->GetWorld();
	const UCapsuleComponent* Capsula = Personaggio->GetCapsuleComponent();
	const float Raggio = Capsula->GetScaledCapsuleRadius();
	const float MezzaAltezza = Capsula->GetScaledCapsuleHalfHeight();
	const FVector Piedi = Personaggio->GetActorLocation() - FVector(0.f, 0.f, MezzaAltezza);
	const FVector Avanti = Personaggio->GetActorForwardVector().GetSafeNormal2D();

	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoScavalca), false, Personaggio);

	// Più si va veloci, più lontano si guarda: le animazioni di corsa hanno bisogno di rincorsa.
	const float DistanzaAvanti = FMath::GetMappedRangeValueClamped(FVector2D(0.f, VelocitaCorsa),
		FVector2D(DistanzaFermo, DistanzaCorsa), Personaggio->GetVelocity().Size2D());

	// 1) La faccia dell'ostacolo: una capsula che copre le altezze da 30 a 130 cm, spinta in avanti.
	const FVector Centro = Piedi + FVector(0.f, 0.f, 80.f);
	FHitResult Muro;
	if (!Mondo->SweepSingleByChannel(Muro, Centro, Centro + Avanti * DistanzaAvanti, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeCapsule(20.f, 50.f), Parametri) || Muro.bStartPenetrating)
	{
		return false;
	}

	if (Muro.GetActor() && Muro.GetActor()->IsA<APawn>())
	{
		return false; // le persone non si scavalcano
	}

	const FVector Normale = Muro.ImpactNormal.GetSafeNormal2D();
	if (Normale.IsNearlyZero() || FVector::DotProduct(Normale, -Avanti) < 0.5f)
	{
		return false; // l'ostacolo deve essere davanti, non di lato
	}

	// 2) La cima: dall'alto verso il basso, appena dentro la faccia dell'ostacolo.
	const FVector PuntoFaccia(Muro.ImpactPoint.X, Muro.ImpactPoint.Y, Piedi.Z);
	const FVector InizioCima = PuntoFaccia - Normale * 5.f + FVector(0.f, 0.f, AltezzaMax + 20.f);
	FHitResult Cima;
	if (!Mondo->LineTraceSingleByChannel(Cima, InizioCima, InizioCima - FVector(0.f, 0.f, AltezzaMax + 20.f),
		ECC_Visibility, Parametri) || Cima.bStartPenetrating || Cima.ImpactNormal.Z < 0.7f)
	{
		return false;
	}

	const float Altezza = Cima.ImpactPoint.Z - Piedi.Z;
	if (Altezza < AltezzaMin || Altezza > AltezzaMax)
	{
		return false;
	}

	const FVector BordoDavanti(PuntoFaccia.X, PuntoFaccia.Y, Cima.ImpactPoint.Z);

	// 3) Lo spessore: si cammina sopra la cima a passi di 10 cm finché la cima c'è.
	float Spessore = 0.f;
	bool bFineTrovata = false;
	for (float D = Passo; D <= SpessoreMax; D += Passo)
	{
		const FVector Sopra = BordoDavanti - Normale * D + FVector(0.f, 0.f, 15.f);
		FHitResult Tocco;
		const bool bColpito = Mondo->LineTraceSingleByChannel(Tocco, Sopra, Sopra - FVector(0.f, 0.f, 30.f), ECC_Visibility, Parametri);
		if (!bColpito || FMath::Abs(Tocco.ImpactPoint.Z - BordoDavanti.Z) > 10.f)
		{
			bFineTrovata = true;
			break;
		}
		Spessore = D;
	}

	const FVector BordoDietro = BordoDavanti - Normale * Spessore;
	const FCollisionShape Corpo = FCollisionShape::MakeCapsule(Raggio, MezzaAltezza);

	Risultato.Altezza = Altezza;
	Risultato.BordoDavanti = BordoDavanti;
	Risultato.BordoDietro = BordoDietro;
	Risultato.Normale = Normale;
	Risultato.Componente = Muro.GetComponent();

	// 4) Ostacolo sottile: si scavalca se dietro c'è terra e il corpo ci sta.
	if (bFineTrovata && Spessore <= SpessoreScavalca)
	{
		const FVector Oltre = BordoDietro - Normale * 60.f + FVector(0.f, 0.f, 15.f);
		FHitResult Terra;
		if (Mondo->LineTraceSingleByChannel(Terra, Oltre, Oltre - FVector(0.f, 0.f, Altezza + 150.f), ECC_Visibility, Parametri)
			&& Terra.ImpactNormal.Z > 0.7f)
		{
			const FVector Arrivo = Terra.ImpactPoint + FVector(0.f, 0.f, MezzaAltezza + 2.f);
			if (!Mondo->OverlapBlockingTestByChannel(Arrivo, FQuat::Identity, ECC_Pawn, Corpo, Parametri))
			{
				Risultato.TerraDietro = Terra.ImpactPoint;
				Risultato.bSaliSopra = false;
				Risultato.bTrovato = true;
				return true;
			}
		}
	}

	// 5) Altrimenti si sale sopra, se c'è abbastanza piano e spazio per stare in piedi.
	if (Spessore >= Raggio + 5.f)
	{
		const FVector Arrivo = BordoDavanti - Normale * (Raggio + 5.f) + FVector(0.f, 0.f, MezzaAltezza + 2.f);
		if (!Mondo->OverlapBlockingTestByChannel(Arrivo, FQuat::Identity, ECC_Pawn, Corpo, Parametri))
		{
			Risultato.TerraDietro = BordoDavanti - Normale * (Raggio + 5.f);
			Risultato.bSaliSopra = true;
			Risultato.bTrovato = true;
			return true;
		}
	}

	return false;
}

float UValdorsoAbilitaScavalca::FineUltimoWarping(const UAnimMontage* Montaggio)
{
	float Fine = 0.f;
	if (Montaggio)
	{
		TArray<FMotionWarpingWindowData> Finestre;
		UMotionWarpingUtilities::GetMotionWarpingWindowsFromAnimation(Montaggio, Finestre);
		for (const FMotionWarpingWindowData& Finestra : Finestre)
		{
			Fine = FMath::Max(Fine, Finestra.EndTime);
		}
	}
	return Fine;
}

float UValdorsoAbilitaScavalca::CalcolaInizio(const UAnimMontage* Montaggio, FName BersaglioDavanti, float DistanzaVera, float& Pista)
{
	Pista = 0.f;
	if (Montaggio == nullptr)
	{
		return 0.f;
	}

	// Il primo rettangolo "bordo davanti" con il punto fisso: dice dov'è il bordo nell'animazione.
	TArray<FMotionWarpingWindowData> Finestre;
	UMotionWarpingUtilities::GetMotionWarpingWindowsFromAnimation(Montaggio, Finestre);
	const FMotionWarpingWindowData* Prima = nullptr;
	const URootMotionModifier_Warp* Modifica = nullptr;
	for (const FMotionWarpingWindowData& Finestra : Finestre)
	{
		const URootMotionModifier_Warp* Warp = Finestra.AnimNotify ? Cast<URootMotionModifier_Warp>(Finestra.AnimNotify->RootMotionModifier) : nullptr;
		if (Warp && Warp->WarpTargetName == BersaglioDavanti && Warp->WarpPointAnimProvider == EWarpPointAnimProvider::Static
			&& (Prima == nullptr || Finestra.StartTime < Prima->StartTime))
		{
			Prima = &Finestra;
			Modifica = Warp;
		}
	}
	if (Prima == nullptr)
	{
		return 0.f;
	}

	// Il punto fisso è misurato rispetto ai piedi alla FINE del rettangolo: per sapere quanto è lontano
	// il bordo dalla partenza, si aggiunge il cammino fatto dall'animazione fino a quel momento.
	const FTransform CamminoFinoAllaFine = UMotionWarpingUtilities::ExtractRootMotionFromAnimation(Montaggio, 0.f, Prima->EndTime);
	Pista = (Modifica->WarpPointAnimTransform * CamminoFinoAllaFine).GetLocation().Size2D();
	if (DistanzaVera >= Pista)
	{
		return 0.f; // c'è abbastanza spazio: si parte dall'inizio
	}

	// Troppo vicini: si salta la parte di rincorsa che manca, ma mai oltre l'inizio del primo rettangolo.
	float Inizio = 0.f;
	for (float T = 0.f; T <= Prima->StartTime; T += 0.02f)
	{
		Inizio = T;
		const float Fatta = UMotionWarpingUtilities::ExtractRootMotionFromAnimation(Montaggio, 0.f, T).GetTranslation().Size2D();
		if (Pista - Fatta <= DistanzaVera)
		{
			break;
		}
	}
	return Inizio;
}

void UValdorsoAbilitaScavalca::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AValdorsoCharacter* Personaggio = Cast<AValdorsoCharacter>(ActorInfo->AvatarActor.Get());
	FValdorsoOstacolo Ostacolo;
	if (Personaggio == nullptr || !TrovaOstacolo(Personaggio, Ostacolo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* Montaggio = Personaggio->ScegliMontaggioScavalca(Ostacolo.bSaliSopra);
	if (Montaggio == nullptr || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bSaliSopraAttivo = Ostacolo.bSaliSopra;
	bFermoAllInizio = Personaggio->GetVelocity().Size2D() < 50.f;
	bOltreIlMuretto = false;
	BordoDietroAttivo = Ostacolo.BordoDietro;
	NormaleAttiva = Ostacolo.Normale;

	// Il personaggio si gira verso l'ostacolo.
	const FRotator Verso = (-Ostacolo.Normale).Rotation();
	Personaggio->SetActorRotation(Verso);

	// I tre bersagli del Motion Warping: bordo davanti, bordo dietro, terra (o piano) d'arrivo.
	if (UMotionWarpingComponent* Warping = Personaggio->GetMotionWarping())
	{
		Warping->AddOrUpdateWarpTargetFromLocationAndRotation(Personaggio->BersaglioBordoDavanti, Ostacolo.BordoDavanti, Verso);
		Warping->AddOrUpdateWarpTargetFromLocationAndRotation(Personaggio->BersaglioBordoDietro, Ostacolo.BordoDietro, Verso);
		Warping->AddOrUpdateWarpTargetFromLocationAndRotation(Personaggio->BersaglioTerraDietro, Ostacolo.TerraDietro, Verso);
	}

	// Durante il passaggio: niente urti con l'ostacolo e niente gravità (muove l'animazione).
	if (UPrimitiveComponent* Componente = Ostacolo.Componente.Get())
	{
		Personaggio->GetCapsuleComponent()->IgnoreComponentWhenMoving(Componente, true);
		ComponenteIgnorato = Componente;
	}
	Personaggio->GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	// La telecamera non si accorcia contro l'ostacolo mentre ci si passa sopra.
	if (USpringArmComponent* Asta = Personaggio->FindComponentByClass<USpringArmComponent>())
	{
		AstaTelecamera = Asta;
		bAstaUrtiPrima = Asta->bDoCollisionTest;
		Asta->bDoCollisionTest = false;
	}

	// Da dove far partire il montaggio: la distanza vera fra il corpo e la faccia dell'ostacolo.
	const FVector Posizione = Personaggio->GetActorLocation();
	const float DistanzaVera = FVector::DotProduct(
		FVector(Ostacolo.BordoDavanti.X - Posizione.X, Ostacolo.BordoDavanti.Y - Posizione.Y, 0.f), -Ostacolo.Normale);
	float Pista = 0.f;
	InizioMontaggio = CalcolaInizio(Montaggio, Personaggio->BersaglioBordoDavanti, DistanzaVera, Pista);
	UE_LOG(LogTemp, Log, TEXT("[Valdorso] Scavalca: %s, distanza vera %.0f cm, rincorsa dell'animazione %.0f cm, parte da %.2f s"),
		*Montaggio->GetName(), DistanzaVera, Pista, InizioMontaggio);

	UAbilityTask_PlayMontageAndWait* Animazione = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montaggio, 1.f, NAME_None, true, 1.f, InizioMontaggio);
	Animazione->OnCompleted.AddDynamic(this, &UValdorsoAbilitaScavalca::QuandoFinisce);
	Animazione->OnBlendOut.AddDynamic(this, &UValdorsoAbilitaScavalca::QuandoFinisce);
	Animazione->OnInterrupted.AddDynamic(this, &UValdorsoAbilitaScavalca::QuandoFinisce);
	Animazione->OnCancelled.AddDynamic(this, &UValdorsoAbilitaScavalca::QuandoFinisce);
	Animazione->ReadyForActivation();

	if (bSaliSopraAttivo)
	{
		// Mantle: alla fine dell'ultimo rettangolo MotionWarping il corpo è sopra il piano.
		const float FineWarping = FineUltimoWarping(Montaggio);
		if (FineWarping > 0.f)
		{
			UAbilityTask_WaitDelay* Attesa = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(FineWarping - InizioMontaggio, 0.01f));
			Attesa->OnFinish.AddDynamic(this, &UValdorsoAbilitaScavalca::QuandoFinisceIlWarping);
			Attesa->ReadyForActivation();
		}
	}
	else if (UWorld* Mondo = GetWorld())
	{
		// Vault: si controlla spesso se il corpo ha già superato il muretto.
		Mondo->GetTimerManager().SetTimer(TimerPassaggio, this, &UValdorsoAbilitaScavalca::ControllaPassaggio, 0.02f, true);
	}
}

void UValdorsoAbilitaScavalca::RiaccendiUrti()
{
	ACharacter* Personaggio = CurrentActorInfo ? Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	if (Personaggio)
	{
		if (UPrimitiveComponent* Componente = ComponenteIgnorato.Get())
		{
			Personaggio->GetCapsuleComponent()->IgnoreComponentWhenMoving(Componente, false);
		}
	}
	ComponenteIgnorato.Reset();
}

void UValdorsoAbilitaScavalca::QuandoFinisceIlWarping()
{
	ACharacter* Personaggio = CurrentActorInfo ? Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	if (!IsActive() || Personaggio == nullptr)
	{
		return;
	}

	// Mantle: il corpo è sopra il piano. Prima gli urti (così il piano lo regge e non ci cade dentro),
	// poi la camminata.
	RiaccendiUrti();
	Personaggio->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (!bFermoAllInizio)
	{
		// Camminando o correndo il resto dell'animazione non serve: si torna alla camminata normale.
		MontageStop(SfumaturaUscita);
	}
}

void UValdorsoAbilitaScavalca::ControllaPassaggio()
{
	ACharacter* Personaggio = CurrentActorInfo ? Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	if (!IsActive() || Personaggio == nullptr || bOltreIlMuretto)
	{
		return;
	}

	// Quanto il centro del corpo è andato oltre il bordo dietro, nella direzione del passaggio.
	const float Raggio = Personaggio->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Direzione = -NormaleAttiva;
	const FVector Posizione = Personaggio->GetActorLocation();
	const float Oltre = FVector::DotProduct(FVector(Posizione.X, Posizione.Y, 0.f) - FVector(BordoDietroAttivo.X, BordoDietroAttivo.Y, 0.f), Direzione);
	if (Oltre < Raggio + 5.f)
	{
		return; // ancora sopra o davanti al muretto
	}

	// Il corpo ha superato il muretto: urti accesi, caduta fino a terra.
	bOltreIlMuretto = true;
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerPassaggio);
	}
	RiaccendiUrti();

	UAbilityTask_WaitMovementModeChange* Atterraggio = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	Atterraggio->OnChange.AddDynamic(this, &UValdorsoAbilitaScavalca::QuandoAtterra);
	Atterraggio->ReadyForActivation();

	Personaggio->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}

void UValdorsoAbilitaScavalca::QuandoAtterra(EMovementMode NuovaModalita)
{
	if (IsActive())
	{
		// Fa sfumare il montaggio verso la camminata; la fine del montaggio chiude l'abilità.
		MontageStop(SfumaturaUscita);
	}
}

void UValdorsoAbilitaScavalca::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Rimette urti, gravità e telecamera come prima.
	if (ActorInfo)
	{
		if (ACharacter* Personaggio = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			if (UPrimitiveComponent* Componente = ComponenteIgnorato.Get())
			{
				Personaggio->GetCapsuleComponent()->IgnoreComponentWhenMoving(Componente, false);
			}

			UCharacterMovementComponent* Movimento = Personaggio->GetCharacterMovement();
			if (Movimento->MovementMode == MOVE_Flying)
			{
				// Camminata: se sotto non c'è pavimento, Unreal passa da solo alla caduta.
				Movimento->SetMovementMode(MOVE_Walking);
			}

			// Niente scivolata: si tiene al massimo la velocità di camminata/corsa normale.
			const float Massima = Movimento->GetMaxSpeed();
			const FVector Orizzontale(Movimento->Velocity.X, Movimento->Velocity.Y, 0.f);
			if (Orizzontale.Size() > Massima)
			{
				const FVector Limitata = Orizzontale.GetSafeNormal() * Massima;
				Movimento->Velocity = FVector(Limitata.X, Limitata.Y, Movimento->Velocity.Z);
			}
		}
	}
	ComponenteIgnorato.Reset();

	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerPassaggio);
	}

	if (USpringArmComponent* Asta = AstaTelecamera.Get())
	{
		Asta->bDoCollisionTest = bAstaUrtiPrima;
	}
	AstaTelecamera.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UValdorsoAbilitaScavalca::QuandoFinisce()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}