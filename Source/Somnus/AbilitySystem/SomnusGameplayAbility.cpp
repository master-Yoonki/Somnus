// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SomnusGameplayAbility.h"

USomnusGameplayAbility::USomnusGameplayAbility()
{
	// By default, abilities are activated when triggered and end on their own
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}
