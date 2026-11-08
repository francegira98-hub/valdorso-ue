// Valdorso - Gli effetti di gioco scritti in C++ (vedi il .h).

#include "ValdorsoEffetti.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoEtichette.h"
#include "ValdorsoPericoli.h"

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

UValdorsoGE_CostoPugno::UValdorsoGE_CostoPugno()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Costo;
	Costo.Attribute = UValdorsoAttributeSet::GetStaminaAttribute();
	Costo.ModifierOp = EGameplayModOp::Additive;
	Costo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-10.f));
	Modifiers.Add(Costo);
}

UValdorsoGE_DannoPugno::UValdorsoGE_DannoPugno()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Il numero lo mette l'abilità al momento del colpo (già negativo: è salute tolta).
	FSetByCallerFloat DaChiChiama;
	DaChiChiama.DataTag = ValdorsoEtichette::Dato_Danno;

	FGameplayModifierInfo Danno;
	Danno.Attribute = UValdorsoAttributeSet::GetSaluteAttribute();
	Danno.ModifierOp = EGameplayModOp::Additive;
	Danno.ModifierMagnitude = FGameplayEffectModifierMagnitude(DaChiChiama);
	Modifiers.Add(Danno);

	// (08/11) Il colpo fa anche perdere l'equilibrio: 30 a pugno (a zero si barcolla, vedi ValdorsoPericoli.h).
	FGameplayModifierInfo Spinta;
	Spinta.Attribute = UValdorsoAttributeSet::GetEquilibrioAttribute();
	Spinta.ModifierOp = EGameplayModOp::Additive;
	Spinta.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-ValdorsoPericoli::EquilibrioPerPugno));
	Modifiers.Add(Spinta);
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

UValdorsoGE_RecuperoEquilibrio::UValdorsoGE_RecuperoEquilibrio()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.25f);

	FGameplayModifierInfo Recupero;
	Recupero.Attribute = UValdorsoAttributeSet::GetEquilibrioAttribute();
	Recupero.ModifierOp = EGameplayModOp::Additive;
	Recupero.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ValdorsoPericoli::EquilibrioAlSecondo * 0.25f));
	Modifiers.Add(Recupero);
}
