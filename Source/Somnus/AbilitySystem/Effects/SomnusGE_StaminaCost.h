// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SomnusGE_StaminaCost.generated.h"

/**
 * Instant GE that subtracts Stamina via MMC (reads StaminaCost from the ability instance).
 * Shared across all abilities — each ability's StaminaCost property determines the cost.
 */
UCLASS()
class SOMNUS_API USomnusGE_StaminaCost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USomnusGE_StaminaCost();
};
