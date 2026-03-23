// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SomnusGameplayAbility.generated.h"

/**
 * Base GameplayAbility for Project Somnus.
 * All project abilities should inherit from this class.
 * Cost is handled via a shared Cost GE + MMC that reads StaminaCost from the ability instance.
 */
UCLASS(Abstract)
class SOMNUS_API USomnusGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGameplayAbility();

	float GetStaminaCost() const { return StaminaCost; }

protected:
	// Stamina consumed when CommitAbility() is called. 0 = no cost.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	float StaminaCost = 0.0f;
};
