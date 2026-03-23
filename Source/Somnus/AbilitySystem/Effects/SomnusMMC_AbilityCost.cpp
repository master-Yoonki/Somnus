// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effects/SomnusMMC_AbilityCost.h"
#include "AbilitySystem/SomnusGameplayAbility.h"

float USomnusMMC_AbilityCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const USomnusGameplayAbility* Ability = Cast<USomnusGameplayAbility>(Spec.GetContext().GetAbilityInstance_NotReplicated());
	if (!Ability)
	{
		return 0.0f;
	}

	return -Ability->GetStaminaCost();
}
