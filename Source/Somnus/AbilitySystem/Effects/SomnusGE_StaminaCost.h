// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SomnusGE_StaminaCost.generated.h"

/**
 * Instant GE that subtracts Stamina using SetByCaller magnitude (Data.StaminaCost tag).
 * Used as the cost GE for abilities that consume stamina.
 */
UCLASS()
class SOMNUS_API USomnusGE_StaminaCost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USomnusGE_StaminaCost();
};
