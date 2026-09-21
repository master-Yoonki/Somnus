// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/SomnusAnimInstanceBase.h"

#include "Character/SomnusCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

void USomnusAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCharacter = Cast<ASomnusCharacterBase>(TryGetPawnOwner());
	CharacterMovement = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
}

void USomnusAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	bIsDead = OwningCharacter && OwningCharacter->IsDead();
}

void USomnusAnimInstanceBase::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (CharacterMovement)
	{
		Speed2D = CharacterMovement->Velocity.Size2D();
	}
}
