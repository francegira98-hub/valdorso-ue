// Valdorso - Componente di movimento del personaggio (vedi il .h).

#include "ValdorsoMovementComponent.h"
#include "ValdorsoAcque.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PhysicsVolume.h"

// Una "mossa salvata": ogni passo del giocatore viene ricordato insieme allo stato della corsa,
// così se il server corregge la posizione il PC rifà i passi alla velocità giusta, senza scatti.
class FSavedMove_Valdorso : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	uint8 bSalvatoCorrere : 1;

	FSavedMove_Valdorso()
	{
		bSalvatoCorrere = false;
	}

	virtual void Clear() override
	{
		Super::Clear();
		bSalvatoCorrere = false;
	}

	// La corsa viaggia verso il server come un segnale acceso o spento dentro ogni passo.
	virtual uint8 GetCompressedFlags() const override
	{
		uint8 Risultato = Super::GetCompressedFlags();
		if (bSalvatoCorrere)
		{
			Risultato |= FLAG_Custom_0;
		}
		return Risultato;
	}

	// Due passi si possono unire solo se in tutti e due si correva (o non si correva).
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override
	{
		const FSavedMove_Valdorso* Nuova = static_cast<const FSavedMove_Valdorso*>(NewMove.Get());
		if (bSalvatoCorrere != Nuova->bSalvatoCorrere)
		{
			return false;
		}
		return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
	}

	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override
	{
		Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
		if (const UValdorsoMovementComponent* Movimento = Cast<UValdorsoMovementComponent>(C->GetCharacterMovement()))
		{
			bSalvatoCorrere = Movimento->bVuoleCorrere;
		}
	}

	virtual void PrepMoveFor(ACharacter* C) override
	{
		Super::PrepMoveFor(C);
		if (UValdorsoMovementComponent* Movimento = Cast<UValdorsoMovementComponent>(C->GetCharacterMovement()))
		{
			Movimento->bVuoleCorrere = bSalvatoCorrere;
		}
	}
};

// Dice a Unreal di usare le nostre "mosse salvate" al posto di quelle di serie.
class FNetworkPredictionData_Client_Valdorso : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	explicit FNetworkPredictionData_Client_Valdorso(const UCharacterMovementComponent& ClientMovement)
		: Super(ClientMovement)
	{
	}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return FSavedMovePtr(new FSavedMove_Valdorso());
	}
};

UValdorsoMovementComponent::UValdorsoMovementComponent()
{
	bVuoleCorrere = false;
	MaxWalkSpeed = VelocitaCamminata;
	// (06/11) Il nuoto: a galla con la testa fuori (con 1 starebbe tutto sotto), non troppo veloce.
	Buoyancy = 1.12f;
	MaxSwimSpeed = 220.f;
	NavAgentProps.bCanSwim = true;
}

float UValdorsoMovementComponent::ImmersioneNellaValle() const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo || !UpdatedComponent)
	{
		return 0.f;
	}
	if (!Acque.IsValid())
	{
		const double Ora = Mondo->GetTimeSeconds();
		if (Ora < ProssimaRicercaAcque)
		{
			return 0.f;
		}
		ProssimaRicercaAcque = Ora + 2.0;
		Acque = AValdorsoAcque::Di(Mondo);
		if (!Acque.IsValid())
		{
			return 0.f;
		}
	}
	const FVector Dove = UpdatedComponent->GetComponentLocation();
	float Pelo = 0.f;
	if (!Acque->PeloQui(FVector2D(Dove), Pelo))
	{
		return 0.f;
	}
	const float Mezza = CharacterOwner && CharacterOwner->GetCapsuleComponent()
		? CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	return ValdorsoAcqua::Immersione(Pelo, static_cast<float>(Dove.Z), Mezza);
}

float UValdorsoMovementComponent::AcquaSopraLaTesta() const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo || !UpdatedComponent || ImmersioneNellaValle() <= 0.f || !Acque.IsValid())
	{
		return -10000.f;
	}
	const FVector Dove = UpdatedComponent->GetComponentLocation();
	float Pelo = 0.f;
	if (!Acque->PeloQui(FVector2D(Dove), Pelo))
	{
		return -10000.f;
	}
	const float Mezza = CharacterOwner && CharacterOwner->GetCapsuleComponent()
		? CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	// la bocca: poco sotto la cima della capsula
	return Pelo - static_cast<float>(Dove.Z + Mezza * 0.75f);
}

bool UValdorsoMovementComponent::IsInWater() const
{
	if (Super::IsInWater())
	{
		return true;
	}
	// Chi nuota smette quando l'acqua scende sotto AcquaPerUscire; chi cammina comincia sopra AcquaPerNuotare.
	const float Immersione = ImmersioneNellaValle();
	return MovementMode == MOVE_Swimming ? Immersione >= AcquaPerUscire : Immersione >= AcquaPerNuotare;
}

float UValdorsoMovementComponent::ImmersionDepth() const
{
	const APhysicsVolume* Volume = GetPhysicsVolume();
	if (Volume && Volume->bWaterVolume)
	{
		return Super::ImmersionDepth();
	}
	return ImmersioneNellaValle();
}

void UValdorsoMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
	if (!CanEverSwim() || !UpdatedComponent)
	{
		return;
	}
	// Solo tra camminare, cadere e nuotare: sulla scala (volo), seduti o a letto (fermi) non si tocca niente.
	const bool bATerra = MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking || MovementMode == MOVE_Falling;
	if (bATerra && IsInWater())
	{
		SetMovementMode(MOVE_Swimming);
	}
	else if (MovementMode == MOVE_Swimming && !IsInWater())
	{
		SetMovementMode(MOVE_Falling);
	}
}

void UValdorsoMovementComponent::ImpostaCorsa(bool bCorrere)
{
	bVuoleCorrere = bCorrere;
}

float UValdorsoMovementComponent::GetMaxSpeed() const
{
	// A piedi (e non accucciati) la velocità la decide la corsa; negli altri casi (salto, nuoto...) quella di serie.
	if ((MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking) && !IsCrouching())
	{
		// (08/11) con lo zaino troppo pesante non si corre, e oltre il sovraccarico si cammina piano
		return (bVuoleCorrere && bCaricoPermetteCorsa ? VelocitaCorsa : VelocitaCamminata) * FattoreCarico;
	}
	return Super::GetMaxSpeed();
}

void UValdorsoMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	// Sul server: legge dal passo ricevuto se il giocatore stava correndo.
	Super::UpdateFromCompressedFlags(Flags);
	bVuoleCorrere = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

FNetworkPredictionData_Client* UValdorsoMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UValdorsoMovementComponent* MutableThis = const_cast<UValdorsoMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Valdorso(*this);
	}
	return ClientPredictionData;
}

void UValdorsoMovementComponent::PhysSwimming(float deltaTime, int32 Iterations)
{
	Super::PhysSwimming(deltaTime, Iterations);
	// (10/11, v0.1.4) la corrente dei fiumi porta chi nuota (uguale sul server e sul PC: dipende solo da dove si e')
	if (MovementMode == MOVE_Swimming && Acque.IsValid() && UpdatedComponent && deltaTime > 0.f)
	{
		const FVector2D Corrente = Acque->CorrenteQui(FVector2D(UpdatedComponent->GetComponentLocation()));
		if (!Corrente.IsNearlyZero())
		{
			FHitResult Urto;
			SafeMoveUpdatedComponent(FVector(Corrente * deltaTime, 0.0), UpdatedComponent->GetComponentQuat(), true, Urto);
			if (Urto.IsValidBlockingHit())
			{
				SlideAlongSurface(FVector(Corrente * deltaTime, 0.0), 1.f - Urto.Time, Urto.Normal, Urto, false);
			}
		}
	}
	// Il motore, alla fine del passo a nuoto, mette "cadere" se il Physics Volume non è d'acqua (e qui non lo è mai:
	// l'acqua la dice AValdorsoAcque). Se il corpo è ancora abbastanza sott'acqua, si continua a nuotare: così ogni
	// passo finisce a nuoto per le animazioni, la rete e la soglia per uscire.
	const APhysicsVolume* Volume = GetPhysicsVolume();
	if (MovementMode == MOVE_Falling && !(Volume && Volume->bWaterVolume) && ImmersioneNellaValle() >= AcquaPerUscire)
	{
		SetMovementMode(MOVE_Swimming);
	}
}
