// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SomnusGE_Cooldown.generated.h"

/**
 * Shared cooldown GE for all Somnus abilities.
 * Duration is set via SetByCaller (Data.CooldownDuration).
 * Cooldown tags are applied dynamically per-ability in USomnusGameplayAbility::ApplyCooldown().
 */
UCLASS()
class SOMNUS_API USomnusGE_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USomnusGE_Cooldown();
};
