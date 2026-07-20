// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
#include "SomnusGA_HitReact.generated.h"

UCLASS()
class SOMNUS_API USomnusGA_HitReact : public USomnusGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGA_HitReact();
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;
	
protected:
	UPROPERTY(EditDefaultsOnly)
	float TestImpulseAmount = 4500.f;
};
