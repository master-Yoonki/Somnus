// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "SomnusAbilitySystemGlobals.generated.h"

/**
 * Overrides the effect-context allocation so every effect context in the project is a
 * FSomnusGameplayEffectContext. Registered via AbilitySystemGlobalsClassName in DefaultGame.ini.
 */
UCLASS()
class SOMNUS_API USomnusAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
