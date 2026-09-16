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
	bIsAiming = ASC && ASC->HasMatchingGameplayTag(SomnusTags::State_Aiming);
	AimStanceYaw = SomnusCharacter ? SomnusCharacter->GetAimStanceYaw() : 0.f;

	UpdateStates();
}

void USomnusCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	bHasOwningActor = static_cast<bool>(TryGetPawnOwner());
	if (bHasOwningActor)
	{
		UpdateEssentialValues(DeltaSeconds);
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
	return bIsMoving;
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
	return bJustLanded && FMath::Abs(LandVelocity.Z) < HeavyLandSpeedThreshold;
}

bool USomnusCharacterAnimInstance::JustLanded_Heavy() const
{
	return bJustLanded && FMath::Abs(LandVelocity.Z) >= HeavyLandSpeedThreshold;
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
	
	// The root follows the mesh, and the mesh is turned by the aim stance on top of the actor.
	// Leaving the turn out would read as a standing yaw error and keep turn-in-place firing.
	OrientationIntent = GetOwningActor()->GetActorRotation() + FRotator(0.f, AimStanceYaw, 0.f);
}

void USomnusCharacterAnimInstance::UpdateStates()
{
	ASomnusCharacter* SomnusCharacter = GetSomnusCharacter();
	MovementMode_LastFrame = MovementMode;
	MovementState_LastFrame = MovementState;
	Gait_LastFrame = Gait;

	bFullBodyMontageActive_LastFrame = bFullBodyMontageActive;
	bFullBodyMontageActive = IsSlotActive(FullBodySlotName);

	if (!SomnusCharacter) return;
	MovementMode = SomnusCharacter->GetMovementMode();
	bIsMoving = SomnusCharacter->IsMoving();
	MovementState = bIsMoving ? ESomnusMovementState::Moving : ESomnusMovementState::Idle;
	Gait = SomnusCharacter->GetGait();

	bJustLanded = SomnusCharacter->JustLanded();
	LandVelocity = SomnusCharacter->GetLandVelocity();
}
