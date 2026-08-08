// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SomnusZombieAnimInstance.generated.h"

class ASomnusZombieCharacter;
class UCharacterMovementComponent;

UCLASS()
class SOMNUS_API USomnusZombieAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<ASomnusZombieCharacter> CachedCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	// Horizontal speed for locomotion blends
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsMoving;

	// True when the character has State.Dead tag — ABP can use to disable locomotion
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead;
};
