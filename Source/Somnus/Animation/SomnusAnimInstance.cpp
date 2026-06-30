// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/SomnusAnimInstance.h"

#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "Equipment/SomnusWeapon.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void USomnusAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CachedCharacter = Cast<ASomnusCharacter>(TryGetPawnOwner());
	if (CachedCharacter)
	{
		MovementComponent = CachedCharacter->GetCharacterMovement();
	}
	bIsInitialized = false;
}

void USomnusAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Line trace must run on game thread
	UpdateDistanceToGround();
}

void USomnusAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	USkeletalMeshComponent* MeshComp = GetSkelMeshComponent();
	if (!MeshComp) return;

	if (bool bIsMainInstance = (MeshComp->GetAnimInstance() == this))
	{
		UpdateLocationData();
		UpdateVelocityData();
		UpdateAccelerationData();
		UpdateLocomotionData();
		UpdateRotationData();
		UpdateWeaponData();
		UpdateAimingData();
		UpdateJumpingData(DeltaSeconds);
	}
	else
	{
		if (const USomnusAnimInstance* MainInstance = Cast<USomnusAnimInstance>(MeshComp->GetAnimInstance()))
		{
			CopyFromMainInstance(MainInstance);
		}
	}

	UpperBodyBlendWeight = (GroundSpeed > 0.0f) ? 1.0f : 0.0f;

	// Stance correction: full weight in idle, blend out when moving
	float TargetStanceAlpha = (StanceCorrectionPose && GroundSpeed < 1.0f) ? 1.0f : 0.0f;
	StanceCorrectionAlpha = FMath::FInterpTo(StanceCorrectionAlpha, TargetStanceAlpha, DeltaSeconds, 8.0f);

	bIsInitialized = true;
}

UCharacterMovementComponent* USomnusAnimInstance::GetCharacterMovementComponent() const
{
	return MovementComponent;
}

void USomnusAnimInstance::UpdateLocomotionData()
{
	if (!CachedCharacter) return;
	Gait = CachedCharacter->GetCurrentGait();
	VelocityLocomotionAngle = UKismetAnimationLibrary::CalculateDirection(Velocity, CachedCharacter->GetActorRotation());
	CurrentDirection = CalculateDirectionWithHysteresis(VelocityLocomotionAngle, CurrentDirection, 20.0f);
}

void USomnusAnimInstance::UpdateRotationData()
{
	if (!CachedCharacter) return;
	WorldRotation = CachedCharacter->GetActorRotation();
	LastFrameActorYaw = ActorYaw;
	ActorYaw = WorldRotation.Yaw;
	
	if (bIsInitialized)
	{
		// Use NormalizeAxis to handle the -180 to 180 degree wrap correctly
		DeltaActorYaw = UKismetMathLibrary::NormalizeAxis(ActorYaw - LastFrameActorYaw);
	}
}

void USomnusAnimInstance::UpdateWeaponData()
{
	if (!CachedCharacter) return;
	if (ASomnusWeapon* Weapon = CachedCharacter->GetEquippedWeapon())
	{
		EquippedWeaponType = Weapon->GetWeaponType();
		bHasUpperBodyLayer = Weapon->HasUpperBodyLayer();
		StanceCorrectionPose = Weapon->GetStanceCorrectionPose();
	}
	else
	{
		EquippedWeaponType = ESomnusWeaponType::None;
		bHasUpperBodyLayer = false;
		StanceCorrectionPose = nullptr;
	}
}

void USomnusAnimInstance::UpdateAimingData()
{
	if (!CachedCharacter) return;
	if (UAbilitySystemComponent* ASC = CachedCharacter->GetAbilitySystemComponent())
	{
		bIsAiming = ASC->HasMatchingGameplayTag(SomnusTags::State_Aiming);
		bIsDead = ASC->HasMatchingGameplayTag(SomnusTags::State_Dead);
	}

	const FRotator AimRotation = CachedCharacter->GetBaseAimRotation();
	const FRotator ActorRotation = CachedCharacter->GetActorRotation();
	const FRotator Delta = (AimRotation - ActorRotation).GetNormalized();
	AimYaw = FMath::Clamp(Delta.Yaw, -180.0f, 180.0f);
	AimPitch = FMath::Clamp(Delta.Pitch, -90.0f, 90.0f);
}

void USomnusAnimInstance::CopyFromMainInstance(const USomnusAnimInstance* MainInstance)
{
	// Location
	Location = MainInstance->Location;
	DeltaLocation = MainInstance->DeltaLocation;

	// Velocity
	Velocity = MainInstance->Velocity;
	Velocity2D = MainInstance->Velocity2D;
	GroundSpeed = MainInstance->GroundSpeed;

	// Acceleration
	Acceleration = MainInstance->Acceleration;
	Acceleration2D = MainInstance->Acceleration2D;
	bHasAcceleration = MainInstance->bHasAcceleration;

	// Locomotion
	Gait = MainInstance->Gait;
	VelocityLocomotionAngle = MainInstance->VelocityLocomotionAngle;
	CurrentDirection = MainInstance->CurrentDirection;

	// Rotation
	WorldRotation = MainInstance->WorldRotation;
	ActorYaw = MainInstance->ActorYaw;
	DeltaActorYaw = MainInstance->DeltaActorYaw;

	// Weapon
	EquippedWeaponType = MainInstance->EquippedWeaponType;
	bHasUpperBodyLayer = MainInstance->bHasUpperBodyLayer;
	UpperBodyBlendWeight = MainInstance->UpperBodyBlendWeight;
	StanceCorrectionPose = MainInstance->StanceCorrectionPose;
	StanceCorrectionAlpha = MainInstance->StanceCorrectionAlpha;

	// Jump
	DistanceToGround = MainInstance->DistanceToGround;
	bIsJumping = MainInstance->bIsJumping;
	bIsOnAir = MainInstance->bIsOnAir;
	bIsFalling = MainInstance->bIsFalling;
	TimeFalling = MainInstance->TimeFalling;
	TimeToJumpApex = MainInstance->TimeToJumpApex;

	// Aiming
	bIsAiming = MainInstance->bIsAiming;
	AimYaw = MainInstance->AimYaw;
	AimPitch = MainInstance->AimPitch;

	// State
	bIsDead = MainInstance->bIsDead;
}

void USomnusAnimInstance::UpdateLocationData()
{
	if (!CachedCharacter) return;
	FVector LastFrameLocation = Location;
	Location = CachedCharacter->GetActorLocation();
	DeltaLocation = Location - LastFrameLocation;
}

void USomnusAnimInstance::UpdateVelocityData()
{
	if (!CachedCharacter) return;
	Velocity = CachedCharacter->GetVelocity();
	Velocity2D = FVector(Velocity.X, Velocity.Y, 0.0f);
	GroundSpeed = Velocity2D.Size2D();
}

void USomnusAnimInstance::UpdateAccelerationData()
{
	if (!MovementComponent) return;
	Acceleration = MovementComponent->GetCurrentAcceleration();
	Acceleration2D = FVector(Acceleration.X, Acceleration.Y, 0.0f);
	bHasAcceleration = Acceleration.Size2D() > 0.0f;
}

void USomnusAnimInstance::UpdateJumpingData(float DeltaSeconds)
{
	if (UCharacterMovementComponent* CharacterMovement = GetCharacterMovementComponent())
	{
		bIsOnAir = (CharacterMovement->MovementMode == EMovementMode::MOVE_Falling);

		if (bIsOnAir)
		{
			bIsJumping = (Velocity.Z > 0.f);
			bIsFalling = (Velocity.Z < 0.f);
		}
		else
		{
			bIsJumping = false;
			bIsFalling = false;
		}

		if (bIsJumping)
		{
			const float VelocityZ = Velocity.Z;
			const float Gravity =
				FMath::Min(
					CharacterMovement->GetGravityZ() *
					CharacterMovement->GravityScale,
					-0.1f
					);
			TimeToJumpApex = VelocityZ / Gravity;
		}
		else
		{
			TimeToJumpApex = 0.f;
		}

		if (bIsFalling)
		{
			TimeFalling += DeltaSeconds;
		}
		else
		{
			TimeFalling = 0.f;
		}
	}
}

void USomnusAnimInstance::UpdateDistanceToGround()
{
	if (!CachedCharacter || !MovementComponent) return;

	if (MovementComponent->MovementMode != EMovementMode::MOVE_Falling)
	{
		DistanceToGround = 0.f;
		return;
	}

	FVector StartLocation = CachedCharacter->GetActorLocation();
	FVector EndLocation = StartLocation + FVector(0.0f, 0.0f, -2000.0f);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(CachedCharacter);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		CollisionParams);

	if (bHit)
	{
		float CapsuleHalfHeight = CachedCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		float FeetZ = StartLocation.Z - CapsuleHalfHeight;
		DistanceToGround = FMath::Max(0.0f, FeetZ - HitResult.ImpactPoint.Z);
	}
	else
	{
		DistanceToGround = -1.0f;
	}
}

ELocomotionDirection USomnusAnimInstance::CalculateDirectionWithHysteresis(float Angle, ELocomotionDirection CurrentDir,
                                                                           float Deadzone)
{
	// Define the strict boundary angles (45 degrees and 135 degrees)
	const float ForwardThreshold = 45.0f;
	const float BackwardThreshold = 135.0f;
	

	// 1. First, check if the angle is still within the 'sticky' expanded bounds of our CURRENT direction
	switch (CurrentDir)
	{
	case ELocomotionDirection::Forward:
		// Sticky bounds: -65 to +65
		if (Angle >= -(ForwardThreshold + Deadzone) && Angle <= (ForwardThreshold + Deadzone))
			return ELocomotionDirection::Forward;
		break;

	case ELocomotionDirection::Right:
		// Sticky bounds: +25 to +155
		if (Angle >= (ForwardThreshold - Deadzone) && Angle <= (BackwardThreshold + Deadzone))
			return ELocomotionDirection::Right;
		break;

	case ELocomotionDirection::Left:
		// Sticky bounds: -155 to -25
		if (Angle >= -(BackwardThreshold + Deadzone) && Angle <= -(ForwardThreshold - Deadzone))
			return ELocomotionDirection::Left;
		break;

	case ELocomotionDirection::Backward:
		// Sticky bounds: >= 115 OR <= -115
		if (Angle >= (BackwardThreshold - Deadzone) || Angle <= -(BackwardThreshold - Deadzone))
			return ELocomotionDirection::Backward;
		break;
	}

	// 2. If the angle broke out of the sticky bounds, strictly calculate the NEW direction
	float AbsAngle = FMath::Abs(Angle);

	if (AbsAngle <= ForwardThreshold)
	{
		return ELocomotionDirection::Forward;
	}
	else if (AbsAngle >= BackwardThreshold)
	{
		return ELocomotionDirection::Backward;
	}
	else if (Angle > 0.0f)
	{
		return ELocomotionDirection::Right;
	}
	else
	{
		return ELocomotionDirection::Left;
	}
}

