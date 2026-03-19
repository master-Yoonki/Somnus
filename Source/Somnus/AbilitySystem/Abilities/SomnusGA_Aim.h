// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
#include "SomnusGA_Aim.generated.h"

/**
 * Aim ability — active while the player holds the aim input.
 * Adds State.Aiming to the ASC while active, enabling light attacks
 * and blocking heavy attacks.
 */
UCLASS()
class SOMNUS_API USomnusGA_Aim : public USomnusGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGA_Aim();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
