// Valdorso - Le statistiche del personaggio per il Gameplay Ability System:
// salute, stamina e mana, ognuna con il suo massimo. Le decide il server e viaggiano in rete ai giocatori.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ValdorsoAttributeSet.generated.h"

// Crea da solo le funzioni per leggere e scrivere una statistica (GetStamina, SetStamina, InitStamina...).
#define VALDORSO_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class VALDORSO_API UValdorsoAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UValdorsoAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Statistiche", ReplicatedUsing = OnRep_Salute)
	FGameplayAttributeData Salute;
	VALDORSO_ATTRIBUTE_ACCESSORS(UValdorsoAttributeSet, Salute)

	UPROPERTY(BlueprintReadOnly, Category = "Statistiche", ReplicatedUsing = OnRep_SaluteMax)
	FGameplayAttributeData SaluteMax;
	VALDORSO_ATTRIBUTE_ACCESSORS(UValdorsoAttributeSet, SaluteMax)

	UPROPERTY(BlueprintReadOnly, Category = "Statistiche", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	VALDORSO_ATTRIBUTE_ACCESSORS(UValdorsoAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Statistiche", ReplicatedUsing = OnRep_StaminaMax)
	FGameplayAttributeData StaminaMax;
	VALDORSO_ATTRIBUTE_ACCESSORS(UValdorsoAttributeSet, StaminaMax)

	UPROPERTY(BlueprintReadOnly, Category = "Statistiche", ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	VALDORSO_ATTRIBUTE_ACCESSORS(UValdorsoAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, Category = "Statistiche", ReplicatedUsing = OnRep_ManaMax)
	FGameplayAttributeData ManaMax;
	VALDORSO_ATTRIBUTE_ACCESSORS(UValdorsoAttributeSet, ManaMax)

protected:
	UFUNCTION()
	void OnRep_Salute(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SaluteMax(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StaminaMax(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ManaMax(const FGameplayAttributeData& OldValue);
};