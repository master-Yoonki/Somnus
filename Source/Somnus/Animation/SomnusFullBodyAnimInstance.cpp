// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/SomnusFullBodyAnimInstance.h"

void USomnusFullBodyAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	UpdateRootYawOffset(DeltaSeconds);
}

void USomnusFullBodyAnimInstance::OnIdleStateUpdate()
{
	RootYawOffsetMode = ERootYawOffsetMode::Accumulate;
	// Process Turn Yaw Table
	LastFrameTurnYawCurveValue = TurnYawCurveValue;

	float TurnYawWeight = GetCurveValue(FName("turnyawweight"));
	if (TurnYawWeight <= 0.01f)
	{
		TurnYawCurveValue = 0.f;
		LastFrameTurnYawCurveValue = 0.f;
	}
	else
	{
		float RootRotationZ = GetCurveValue(FName("remainingturnyaw"));
	
		TurnYawCurveValue = UKismetMathLibrary::SafeDivide(RootRotationZ, TurnYawWeight);
	
		if (LastFrameTurnYawCurveValue != 0.f)
		{
			RootYawOffset =
				UKismetMathLibrary::NormalizeAxis(RootYawOffset - (TurnYawCurveValue - LastFrameTurnYawCurveValue));
		}
	}
}

void USomnusFullBodyAnimInstance::UpdateRootYawOffset(float DeltaSeconds)
{
	if (RootYawOffsetMode == ERootYawOffsetMode::Accumulate)
	{
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - DeltaActorYaw);
	}
	else if (RootYawOffsetMode == ERootYawOffsetMode::BlendOut)
	{
		const float SprintInterpResult = UKismetMathLibrary::FloatSpringInterp(
			RootYawOffset,
			0.f,
			FloatSpringState,
			80.f,
			1.f,
			DeltaSeconds,
			1.f,
			0.5f
			);
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(SprintInterpResult);
	}
	RootYawOffsetMode = ERootYawOffsetMode::BlendOut;
}
