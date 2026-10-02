// Valdorso - Gli effetti di gioco scritti in C++ (costi, recuperi...).
// Ogni effetto cambia una statistica: una volta sola, oppure a intervalli regolari.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ValdorsoEffetti.generated.h"

/** Costo della schivata: toglie 20 di stamina, una volta. */
UCLASS()
class VALDORSO_API UValdorsoGE_CostoSchivata : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UValdorsoGE_CostoSchivata();
};

/** Costo dello scavalcare e del salire sopra: toglie 10 di stamina, una volta. */
UCLASS()
class VALDORSO_API UValdorsoGE_CostoScavalca : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UValdorsoGE_CostoScavalca();
};

/** Costo del pugno: toglie 10 di stamina, una volta. */
UCLASS()
class VALDORSO_API UValdorsoGE_CostoPugno : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UValdorsoGE_CostoPugno();
};

/** Danno del pugno: toglie salute, una volta. Quanta lo dice l'abilità al momento del colpo (Valdorso.Dato.Danno). */
UCLASS()
class VALDORSO_API UValdorsoGE_DannoPugno : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UValdorsoGE_DannoPugno();
};

/** Recupero della stamina: per sempre, ogni 0,25 secondi ridà 2,5 di stamina (10 al secondo). */
UCLASS()
class VALDORSO_API UValdorsoGE_RecuperoStamina : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UValdorsoGE_RecuperoStamina();
};