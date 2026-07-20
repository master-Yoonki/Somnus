// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SomnusAbilitySystemGlobals.h"

#include "AbilitySystem/SomnusGameplayEffectContext.h"

FGameplayEffectContext* USomnusAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FSomnusGameplayEffectContext();
}
