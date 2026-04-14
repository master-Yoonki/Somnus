// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SomnusGameplayAbility.generated.h"

/**
 * Base GameplayAbility for Project Somnus.
 * All project abilities should inherit from this class.
 * Cost is handled via a shared Cost GE + MMC that reads StaminaCost from the ability instance.
 * Cooldown is handled via a shared Cooldown GE with SetByCaller duration + dynamic tags.
 */
UCLASS(Abstract)
class SOMNUS_API USomnusGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGameplayAbility();

	float GetStaminaCost() const { return StaminaCost; }
	float GetCooldownDuration() const { return CooldownDuration; }

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// Called when the ability is granted and the avatar actor is set.
	// Used to auto-activate passive abilities.
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	// Stamina consumed when CommitAbility() is called. 0 = no cost.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	float StaminaCost = 0.0f;

	// Cooldown duration in seconds. 0 = no cooldown.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldown")
	float CooldownDuration = 0.0f;

	// Tag applied while this ability is on cooldown (e.g., Cooldown.Ability.Jump).
	// Must be unique per ability so cooldowns don't interfere with each other.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldown")
	FGameplayTagContainer CooldownTags;

	// If true, this ability will try to activate immediately when granted (for passives).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	bool bActivateOnGranted = false;

	// Scratch container returned by GetCooldownTags() — rebuilt each call to avoid
	// the accumulation bug where AppendTags grows CooldownTags indefinitely.
	UPROPERTY(Transient)
	mutable FGameplayTagContainer TempCooldownTags;
};
