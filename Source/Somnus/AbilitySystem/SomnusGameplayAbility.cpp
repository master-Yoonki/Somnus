// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SomnusGameplayAbility.h"
#include "AbilitySystem/Effects/SomnusGE_StaminaCost.h"

USomnusGameplayAbility::USomnusGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Shared cost GE — MMC inside reads StaminaCost from the ability instance.
	// Abilities with StaminaCost == 0 will pass CheckCost automatically (modifier evaluates to 0).
	CostGameplayEffectClass = USomnusGE_StaminaCost::StaticClass();
}
