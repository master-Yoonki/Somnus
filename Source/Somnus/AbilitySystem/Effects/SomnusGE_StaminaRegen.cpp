// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effects/SomnusGE_StaminaRegen.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "AbilitySystem/Effects/SomnusMMC_StaminaRegen.h"

USomnusGE_StaminaRegen::USomnusGE_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period.SetValue(0.1f);

	FGameplayModifierInfo StaminaModifier;
	StaminaModifier.Attribute = USomnusAttributeSet::GetStaminaAttribute();
	StaminaModifier.ModifierOp = EGameplayModOp::Additive;

	// Use MMC to read StaminaRegenRate from the source each tick
	FCustomCalculationBasedFloat CustomCalc;
	CustomCalc.CalculationClassMagnitude = USomnusMMC_StaminaRegen::StaticClass();
	StaminaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);

	Modifiers.Add(StaminaModifier);
}
