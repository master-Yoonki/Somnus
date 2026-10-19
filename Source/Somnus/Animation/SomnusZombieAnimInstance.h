// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/SomnusAnimInstanceBase.h"
#include "SomnusZombieAnimInstance.generated.h"

UCLASS()
class SOMNUS_API USomnusZombieAnimInstance : public USomnusAnimInstanceBase
{
	GENERATED_BODY()

public:
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** Any speed at all. A zombie has no intent to read ahead of its body, unlike the player. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsMoving = false;
};
