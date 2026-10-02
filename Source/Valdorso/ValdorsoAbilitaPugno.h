// Valdorso - Il pugno, abilità del Gameplay Ability System.
// Costa stamina, suona il montaggio del pugno sulla corsia della parte alta del corpo (le gambe continuano
// a camminare), e nell'istante del colpo (notifica "Valdorso: Colpo") il server guarda davanti alla mano destra:
// chi ha le statistiche perde salute con un effetto GAS; i manichini di Variant_Combat ricevono il colpo
// con la loro interfaccia. Ogni colpo passa dal registro eventi.
// Non parte in aria, durante uno scavalcamento (il personaggio non è a terra) o durante la schivata.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ValdorsoAbilitaPugno.generated.h"

UCLASS()
class VALDORSO_API UValdorsoAbilitaPugno : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UValdorsoAbilitaPugno();

	/** Quanta salute toglie un pugno. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Pugno")
	float Danno = 10.f;

	/** Quanto avanti dalla mano si cerca il bersaglio, in centimetri. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Pugno")
	float Portata = 70.f;

	/** Quanto è "grosso" il pugno nella ricerca, in centimetri (raggio della sfera). */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Pugno")
	float Raggio = 30.f;

	/** L'osso da cui parte il colpo. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Pugno")
	FName OssoMano = TEXT("hand_r");

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Il montaggio è finito, sfuma o è stato interrotto (per esempio da una schivata). */
	UFUNCTION()
	void QuandoFinisce();

	/** È arrivata la notifica del colpo. */
	UFUNCTION()
	void QuandoColpisce(FGameplayEventData Dati);

	/** Solo sul server: cerca chi è davanti alla mano e gli dà il colpo. */
	void EseguiColpo();

	/** Un pugno colpisce una volta sola. */
	bool bColpoFatto = false;
};
