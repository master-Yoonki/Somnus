// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
#include "SomnusGA_Jump.generated.h"

/**
 * Jump ability — replaces the native ACharacter::Jump() binding.
 * Supports cooldown, stamina cost, and variable jump height (release early = shorter jump).
 */
UCLASS()
class SOMNUS_API USomnusGA_Jump : public USomnusGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGA_Jump();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);
};
