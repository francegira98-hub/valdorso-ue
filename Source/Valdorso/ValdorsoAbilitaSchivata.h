// Valdorso - La schivata, prima abilità del Gameplay Ability System.
// Costa stamina, sposta il personaggio di circa 4 metri nella direzione in cui si muove (o in avanti),
// la prevede il PC di chi gioca e la conferma il server.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ValdorsoAbilitaSchivata.generated.h"

UCLASS()
class VALDORSO_API UValdorsoAbilitaSchivata : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UValdorsoAbilitaSchivata();

	/** Velocità dello scatto, in centimetri al secondo. */
	UPROPERTY(EditDefaultsOnly, Category = "Schivata")
	float Forza = 650.f;

	/** Durata dello scatto, in secondi (Forza x Durata = distanza, circa 4 metri). */
	UPROPERTY(EditDefaultsOnly, Category = "Schivata")
	float Durata = 0.6f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void QuandoFinisce();
};