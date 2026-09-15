// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/SomnusItemAnimLayers.h"

#include "Animation/SomnusCharacterAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

void USomnusItemAnimLayers::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (const USkeletalMeshComponent* Mesh = GetOwningComponent())
	{
		MainAnimInstance = Cast<USomnusCharacterAnimInstance>(Mesh->GetAnimInstance());
	}
}

void USomnusItemAnimLayers::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// Null while the layer is previewed on its own, with no main instance of ours underneath it.
	if (!MainAnimInstance)
	{
		return;
	}

	Gait = MainAnimInstance->GetGait();
	MovementState = MainAnimInstance->GetMovementState();
	Speed2D = MainAnimInstance->GetSpeed2D();
	bIsAiming = MainAnimInstance->IsAiming();}
