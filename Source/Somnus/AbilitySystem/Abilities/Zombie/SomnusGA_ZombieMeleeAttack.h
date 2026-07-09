// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
#include "SomnusGA_ZombieMeleeAttack.generated.h"

/**
 * 
 */
UCLASS()
class SOMNUS_API USomnusGA_ZombieMeleeAttack : public USomnusGameplayAbility
{
	GENERATED_BODY()
public:
	USomnusGA_ZombieMeleeAttack();
	
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;
	
protected:
	// TODO: Animation Controlled by montage for now, just simple delay to simulate
	UFUNCTION()
	void OnDelayFinished();
	
	// Damage GE to apply on hit (should use SetByCaller with Data.Damage tag)
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	// Damage amount passed to the GE via SetByCaller
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageAmount = 20.0f;
};
