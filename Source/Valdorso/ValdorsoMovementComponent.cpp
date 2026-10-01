// Valdorso - Componente di movimento del personaggio (vedi il .h).

#include "ValdorsoMovementComponent.h"
#include "GameFramework/Character.h"

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
		return bVuoleCorrere ? VelocitaCorsa : VelocitaCamminata;
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