// Valdorso - Gli effetti di gioco scritti in C++ (vedi il .h).

#include "ValdorsoEffetti.h"
#include "ValdorsoAttributeSet.h"

UValdorsoGE_CostoSchivata::UValdorsoGE_CostoSchivata()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Costo;
	Costo.Attribute = UValdorsoAttributeSet::GetStaminaAttribute();
	Costo.ModifierOp = EGameplayModOp::Additive;
	Costo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-20.f));
	Modifiers.Add(Costo);
}

UValdorsoGE_CostoScavalca::UValdorsoGE_CostoScavalca()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Costo;
	Costo.Attribute = UValdorsoAttributeSet::GetStaminaAttribute();
	Costo.ModifierOp = EGameplayModOp::Additive;
	Costo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-10.f));
	Modifiers.Add(Costo);
}

UValdorsoGE_RecuperoStamina::UValdorsoGE_RecuperoStamina()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.25f);

	FGameplayModifierInfo Recupero;
	Recupero.Attribute = UValdorsoAttributeSet::GetStaminaAttribute();
	Recupero.ModifierOp = EGameplayModOp::Additive;
	Recupero.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(2.5f));
	Modifiers.Add(Recupero);
}