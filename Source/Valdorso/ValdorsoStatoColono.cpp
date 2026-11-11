// Valdorso - Lo stato del colono che vedono tutti (vedi ValdorsoStatoColono.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoStatoColono.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

AValdorsoPlayerState::AValdorsoPlayerState()
{
}

void AValdorsoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoPlayerState, Forza);
	DOREPLIFETIME(AValdorsoPlayerState, bForzaNota);
}

void AValdorsoPlayerState::ImpostaForza(float Nuova)
{
	if (!HasAuthority())
	{
		return;
	}
	const uint8 Arrotondata = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt32(Nuova), 0, 100));
	if (Arrotondata != Forza || !bForzaNota)
	{
		Forza = Arrotondata;
		bForzaNota = true;
		ForceNetUpdate();
	}
}

bool AValdorsoPlayerState::ForzaDi(const AActor* Corpo, float& OutForza)
{
	const APawn* Colono = Cast<APawn>(Corpo);
	const AValdorsoPlayerState* Stato = Colono ? Colono->GetPlayerState<AValdorsoPlayerState>() : nullptr;
	if (!Stato || !Stato->bForzaNota)
	{
		return false;
	}
	OutForza = Stato->GetForza();
	return true;
}
