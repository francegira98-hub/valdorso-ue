// Valdorso - Le statistiche del personaggio (vedi il .h).

#include "ValdorsoAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UValdorsoAttributeSet::UValdorsoAttributeSet()
{
	InitSalute(100.f);
	InitSaluteMax(100.f);
	InitStamina(100.f);
	InitStaminaMax(100.f);
	InitMana(100.f);
	InitManaMax(100.f);
}

void UValdorsoAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UValdorsoAttributeSet, Salute, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdorsoAttributeSet, SaluteMax, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdorsoAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdorsoAttributeSet, StaminaMax, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdorsoAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdorsoAttributeSet, ManaMax, COND_None, REPNOTIFY_Always);
}

void UValdorsoAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Nessuna statistica scende sotto zero o supera il suo massimo.
	if (Attribute == GetSaluteAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetSaluteMax());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetStaminaMax());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetManaMax());
	}
}

void UValdorsoAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Dopo un costo o un recupero, il valore resta tra zero e il massimo.
	if (Data.EvaluatedData.Attribute == GetSaluteAttribute())
	{
		SetSalute(FMath::Clamp(GetSalute(), 0.f, GetSaluteMax()));
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetStaminaMax()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetManaMax()));
	}
}

void UValdorsoAttributeSet::OnRep_Salute(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UValdorsoAttributeSet, Salute, OldValue);
}

void UValdorsoAttributeSet::OnRep_SaluteMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UValdorsoAttributeSet, SaluteMax, OldValue);
}

void UValdorsoAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UValdorsoAttributeSet, Stamina, OldValue);
}

void UValdorsoAttributeSet::OnRep_StaminaMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UValdorsoAttributeSet, StaminaMax, OldValue);
}

void UValdorsoAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UValdorsoAttributeSet, Mana, OldValue);
}

void UValdorsoAttributeSet::OnRep_ManaMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UValdorsoAttributeSet, ManaMax, OldValue);
}