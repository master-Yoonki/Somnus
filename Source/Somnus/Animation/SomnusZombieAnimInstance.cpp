// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/SomnusZombieAnimInstance.h"

void USomnusZombieAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	bIsMoving = Speed2D > 3.0f;
}
