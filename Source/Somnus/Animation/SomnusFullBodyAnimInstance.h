// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "SomnusAnimInstance.h"
#include "SomnusFullBodyAnimInstance.generated.h"

UCLASS()
class SOMNUS_API USomnusFullBodyAnimInstance : public USomnusAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	void UpdateRootYawOffset(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Idle", meta = (BlueprintThreadSafe))
	void OnIdleStateUpdate();

	UPROPERTY(BlueprintReadWrite, Category = "RootYawOffset")
	ERootYawOffsetMode RootYawOffsetMode;

	UPROPERTY(BlueprintReadWrite, Category = "RootYawOffset")
	float RootYawOffset;
	
	UPROPERTY(BlueprintReadWrite, Category = "RootYawOffset")
	float AccumulatedRootYawOffset;

	UPROPERTY(BlueprintReadOnly, Category = "RootYawOffset")
	FFloatSpringState FloatSpringState;

	UPROPERTY(BlueprintReadOnly, Category = "RootYawOffset")
	float LastFrameTurnYawCurveValue;

	UPROPERTY(BlueprintReadOnly, Category = "RootYawOffset")
	float TurnYawCurveValue;

	UPROPERTY(BlueprintReadWrite, Category = "TurnInPlace")
	float TurnInPlaceTime;
};
