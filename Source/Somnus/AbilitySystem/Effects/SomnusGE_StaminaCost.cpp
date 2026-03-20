// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effects/SomnusGE_StaminaCost.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Core/SomnusGameplayTags.h"

USomnusGE_StaminaCost::USomnusGE_StaminaCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo StaminaModifier;
	StaminaModifier.Attribute = USomnusAttributeSet::GetStaminaAttribute();
	StaminaModifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = SomnusTags::Data_StaminaCost;
	StaminaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(StaminaModifier);
}
