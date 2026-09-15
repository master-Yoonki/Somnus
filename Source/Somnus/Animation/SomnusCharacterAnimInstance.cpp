// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/SomnusCharacterAnimInstance.h"

#include "AnimationWarpingLibrary.h"
#include "BoneControllers/AnimNode_OffsetRootBone.h"
#include "Kismet/KismetMathLibrary.h"

#include "AbilitySystemComponent.h"
#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"


void USomnusCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (ASomnusCharacter* SomnusCharacter = GetSomnusCharacter())
	{
		CharacterMovement = SomnusCharacter->GetCharacterMovement();
	}
}

void USomnusCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// The ability system lives on the player state, so it is missing until one is assigned.
	const ASomnusCharacter* SomnusCharacter = GetSomnusCharacter();
	const UAbilitySystemComponent* ASC = SomnusCharacter ? SomnusCharacter->GetAbilitySystemComponent() : nullptr;
	bIsAiming = ASC && ASC->HasMatchingGameplayTag(SomnusTags::State_Aiming);}

void USomnusCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	bHasOwningActor = static_cast<bool>(TryGetPawnOwner());
	if (bHasOwningActor)
	{
		UpdateEssentialValues(DeltaSeconds);
		UpdateStates();
	}
}

ASomnusCharacter* USomnusCharacterAnimInstance::GetSomnusCharacter() const
{
	APawn* Pawn = TryGetPawnOwner();
	if (!Pawn) return nullptr;
	return Cast<ASomnusCharacter>(Pawn);
}

bool USomnusCharacterAnimInstance::IsMoving() const
{
	if (!GetSomnusCharacter()) return false;
	return GetSomnusCharacter()->IsMoving();
}

bool USomnusCharacterAnimInstance::IsStarting() const
{
	return Speed2D >= 0.f && AccelerationAmount >= 0.f;
}

bool USomnusCharacterAnimInstance::IsPivoting() const
{
	return false;
}

bool USomnusCharacterAnimInstance::IsStopping() const
{
	return false;
}

bool USomnusCharacterAnimInstance::ShouldTurnInPlace() const
{
	return false;
}

bool USomnusCharacterAnimInstance::JustLanded_Light() const
{
	if (!GetSomnusCharacter()) return false;
	return GetSomnusCharacter()->JustLanded() && 
		(FMath::Abs(GetSomnusCharacter()->GetLandVelocity().Z) < HeavyLandSpeedThreshold);
}

bool USomnusCharacterAnimInstance::JustLanded_Heavy() const
{	
	if (!GetSomnusCharacter()) return false;
	return GetSomnusCharacter()->JustLanded() && 
		(FMath::Abs(GetSomnusCharacter()->GetLandVelocity().Z) >= HeavyLandSpeedThreshold);
}

void USomnusCharacterAnimInstance::UpdateEssentialValues(float DeltaSeconds)
{
	CharacterTransform_LastFrame = CharacterTransform;
	CharacterTransform = TryGetPawnOwner()->GetTransform();
	if (FAnimNode_OffsetRootBone* OffsetRootNode = GetOffsetRootNode().GetAnimNodePtr<FAnimNode_OffsetRootBone>())
	{
		FTransform OffsetRootTransform;
		OffsetRootNode->GetOffsetRootTransform(OffsetRootTransform);
		RootTransform.SetTranslation(OffsetRootTransform.GetTranslation());
		FRotator OffsetRootRotator = OffsetRootTransform.Rotator();
		OffsetRootRotator.Add(0.f, 90.f, 0.f);
		RootTransform.SetRotation(OffsetRootRotator.Quaternion());
	}
	
	Acceleration_LastFrame = Acceleration;
	Acceleration = CharacterMovement->GetCurrentAcceleration();
	AccelerationAmount = UKismetMathLibrary::SafeDivide(Acceleration.Length(), CharacterMovement->GetMaxAcceleration());
	
	Velocity_LastFrame = Velocity;
	Velocity = CharacterMovement->Velocity;
	Speed2D = Velocity.Size2D();
	bHasVelocity = Speed2D > 5.0;
	VelocityAcceleration = (Velocity - Velocity_LastFrame) / FMath::Max(DeltaSeconds, 0.001);
	if (bHasVelocity)
	{
		LastNonZeroVelocity = Velocity;
	}
	
	OrientationIntent = GetOwningActor()->GetActorRotation();
}

void USomnusCharacterAnimInstance::UpdateStates()
{
	ASomnusCharacter* SomnusCharacter = GetSomnusCharacter();
	MovementMode_LastFrame = MovementMode;
	MovementState_LastFrame = MovementState;
	Gait_LastFrame = Gait;
	
	if (!SomnusCharacter) return;
	MovementMode = SomnusCharacter->GetMovementMode(); 
	MovementState = SomnusCharacter->IsMoving() ? ESomnusMovementState::Moving : ESomnusMovementState::Idle;
	Gait = SomnusCharacter->GetGait();
}
