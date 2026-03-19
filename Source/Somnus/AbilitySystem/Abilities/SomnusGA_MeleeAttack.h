// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
#include "SomnusGA_MeleeAttack.generated.h"

/**
 * Melee attack ability that plays an attack montage.
 * Hit detection is handled by USomnusAnimNotifyState_MeleeTrace on the montage.
 */
UCLASS()
class SOMNUS_API USomnusGA_MeleeAttack : public USomnusGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGA_MeleeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// The montage to play when attacking (set in the Blueprint subclass)
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// Playback rate for the montage
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	float MontagePlayRate = 1.0f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();
};
