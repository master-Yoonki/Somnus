// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "SomnusMMC_StaminaRegen.generated.h"

/**
 * MMC that reads StaminaRegenRate from the source's attribute set.
 * Used by the stamina regen GE to determine per-tick regen amount.
 */
UCLASS()
class SOMNUS_API USomnusMMC_StaminaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	USomnusMMC_StaminaRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition StaminaRegenRateDef;
};
