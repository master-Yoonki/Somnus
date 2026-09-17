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

// Starting, pivoting, stopping and turning in place are all read from the trajectory, which this
// class does not build. A graph driven some other way answers no rather than inventing one.
bool USomnusCharacterAnimInstance::IsStarting() const
{
	return false;
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
	if (FAnimNode_OffsetRootBone* OffsetRootNode = GetOffsetRootNode().GetAnimNodePtr<FAnimNode_OffsetRootBone>())
	{
		FTransform OffsetRootTransform;
		OffsetRootNode->GetOffsetRootTransform(OffsetRootTransform);
		RootTransform.SetTranslation(OffsetRootTransform.GetTranslation());
		FRotator OffsetRootRotator = OffsetRootTransform.Rotator();
		OffsetRootRotator.Add(0.f, 90.f, 0.f);
		RootTransform.SetRotation(OffsetRootRotator.Quaternion());

		// The 90 degrees above put the root back into the actor's frame, and the mesh is the actor
		// turned by the aim stance, so the two yaws are comparable. The root is read from the last
		// evaluation, which leaves the offset one frame behind the capsule.
		UpperBodyYawOffset = FRotator::NormalizeAxis(
			RootTransform.Rotator().Yaw - (CharacterTransform.Rotator().Yaw + AimStanceYaw));
	}
	else
	{
		UpperBodyYawOffset = 0.f;
	}
	
	Acceleration_LastFrame = Acceleration;
	Acceleration = CharacterMovement->GetCurrentAcceleration();
	AccelerationAmount = UKismetMathLibrary::SafeDivide(Acceleration.Length(), CharacterMovement->GetMaxAcceleration());
	bHasAcceleration = AccelerationAmount > 0.f;
	
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
	OrientationIntent = CharacterTransform.Rotator() + FRotator(0.f, AimStanceYaw, 0.f);
}

void USomnusCharacterAnimInstance::UpdateStates()
{
	ASomnusCharacter* SomnusCharacter = GetSomnusCharacter();
	MovementMode_LastFrame = MovementMode;
	MovementState_LastFrame = MovementState;
	Gait_LastFrame = Gait;

	bFullBodyMontageActive_LastFrame = bFullBodyMontageActive;
	bFullBodyMontageActive = IsSlotActive(FullBodySlotName);

	CharacterTransform_LastFrame = CharacterTransform;

	if (!SomnusCharacter) return;
	CharacterTransform = SomnusCharacter->GetTransform();
	MovementMode = SomnusCharacter->GetMovementMode();
	bIsMoving = SomnusCharacter->IsMoving();
	MovementState = bIsMoving ? ESomnusMovementState::Moving : ESomnusMovementState::Idle;
	Gait = SomnusCharacter->GetGait();

	bJustLanded = SomnusCharacter->JustLanded();
	LandVelocity = SomnusCharacter->GetLandVelocity();
}
