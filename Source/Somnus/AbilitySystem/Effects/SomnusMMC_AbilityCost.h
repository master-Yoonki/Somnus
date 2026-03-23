// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "SomnusMMC_AbilityCost.generated.h"

/**
 * MMC that reads the StaminaCost from the owning GameplayAbility instance.
 * Returns a negative value (subtracts from Stamina via Additive modifier).
 * Reusable across all abilities — each ability defines its own StaminaCost.
 */
UCLASS()
class SOMNUS_API USomnusMMC_AbilityCost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
