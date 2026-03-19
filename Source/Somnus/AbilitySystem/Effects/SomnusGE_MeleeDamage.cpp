// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effects/SomnusGE_MeleeDamage.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Core/SomnusGameplayTags.h"

USomnusGE_MeleeDamage::USomnusGE_MeleeDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = USomnusAttributeSet::GetIncomingDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = SomnusTags::Data_Damage;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(DamageModifier);
}
