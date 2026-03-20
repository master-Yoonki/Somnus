// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SomnusGE_StaminaRegen.generated.h"

/**
 * Infinite-duration periodic GE that regenerates Stamina over time.
 * Ticks every 0.1s, adding 1.0 Stamina per tick (10/sec by default).
 * Add to the character's DefaultGameplayEffects array.
 */
UCLASS()
class SOMNUS_API USomnusGE_StaminaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USomnusGE_StaminaRegen();
};
