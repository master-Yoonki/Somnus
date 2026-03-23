// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effects/SomnusGE_StaminaCost.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "AbilitySystem/Effects/SomnusMMC_AbilityCost.h"

USomnusGE_StaminaCost::USomnusGE_StaminaCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo StaminaModifier;
	StaminaModifier.Attribute = USomnusAttributeSet::GetStaminaAttribute();
	StaminaModifier.ModifierOp = EGameplayModOp::Additive;

	FCustomCalculationBasedFloat CustomCalc;
	CustomCalc.CalculationClassMagnitude = USomnusMMC_AbilityCost::StaticClass();
	StaminaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);

	Modifiers.Add(StaminaModifier);
}
