// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SomnusGameplayAbility.generated.h"

/**
 * Base GameplayAbility for Project Somnus.
 * All project abilities should inherit from this class.
 * Provides SetByCaller stamina cost support — set StaminaCost > 0 to consume stamina on commit.
 */
UCLASS(Abstract)
class SOMNUS_API USomnusGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGameplayAbility();

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	// Stamina consumed when CommitAbility() is called. 0 = no cost.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	float StaminaCost = 0.0f;
};
