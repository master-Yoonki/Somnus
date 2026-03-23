
// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/SomnusAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "Equipment/SomnusWeapon.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void USomnusAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner()))
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}
}

void USomnusAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	USkeletalMeshComponent* MeshComp = GetSkelMeshComponent();
	if (!MeshComp) return;

	if (bool bIsMainInstance = (MeshComp->GetAnimInstance() == this))
	{
		UpdateLocationData();
		UpdateVelocityData();
		UpdateAccelerationData();
		if (ASomnusCharacter* Character = Cast<ASomnusCharacter>(MeshComp->GetOwner()))
		{
			Gait = Character->GetCurrentGait();

			VelocityLocomotionAngle  = UKismetAnimationLibrary::CalculateDirection(Velocity, GetOwningActor()->GetActorRotation());

			// Update direction with 20 degrees of deadzone
			CurrentDirection = CalculateDirectionWithHysteresis(VelocityLocomotionAngle, CurrentDirection, 20.0f);

			if (ASomnusWeapon* Weapon = Character->GetEquippedWeapon())
			{
				EquippedWeaponType = Weapon->GetWeaponType();
				bHasUpperBodyLayer = Weapon->HasUpperBodyLayer();
			}
			else
			{
				EquippedWeaponType = ESomnusWeaponType::None;
				bHasUpperBodyLayer = false;
			}

			if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
			{
				bIsAiming = ASC->HasMatchingGameplayTag(SomnusTags::State_Aiming);
			}

			// Calculate aim offset yaw/pitch from controller-to-actor rotation delta
			const FRotator AimRotation = Character->GetBaseAimRotation();
			const FRotator ActorRotation = Character->GetActorRotation();
			const FRotator Delta = (AimRotation - ActorRotation).GetNormalized();
			AimYaw = FMath::Clamp(Delta.Yaw, -180.0f, 180.0f);
			AimPitch = FMath::Clamp(Delta.Pitch, -90.0f, 90.0f);
		}
		UpperBodyBlendWeight = (GroundSpeed > 0.0f) ? 1.0f : 0.0f;
		UpdateJumpingData(DeltaSeconds);
	}
	else
	{
		if (USomnusAnimInstance* MainInstance = Cast<USomnusAnimInstance>(MeshComp->GetAnimInstance()))
		{
			this->Location = MainInstance->Location;
			this->DeltaLocation = MainInstance->DeltaLocation;
			
			this->Velocity = MainInstance->Velocity;
			this->Velocity2D = MainInstance->Velocity2D;
			this->GroundSpeed = MainInstance->GroundSpeed;
			
			this->Acceleration = MainInstance->Acceleration;
			this->Acceleration2D = MainInstance->Acceleration2D;
			this->bHasAcceleration = MainInstance->bHasAcceleration;
			
			this->Gait = MainInstance->Gait;
			this->VelocityLocomotionAngle = MainInstance->VelocityLocomotionAngle;
			this->CurrentDirection = MainInstance->CurrentDirection;
			
			this->EquippedWeaponType = MainInstance->EquippedWeaponType;
			this->bHasUpperBodyLayer = MainInstance->bHasUpperBodyLayer;
			this->UpperBodyBlendWeight = MainInstance->UpperBodyBlendWeight;
			this->DistanceToGround = MainInstance->DistanceToGround;
			
			this->bIsJumping = MainInstance->bIsJumping;
			this->bIsOnAir = MainInstance->bIsOnAir;
			this->bIsFalling = MainInstance->bIsFalling;
			
			this->TimeFalling = MainInstance->TimeFalling;
			this->TimeToJumpApex = MainInstance->TimeToJumpApex;

			this->bIsAiming = MainInstance->bIsAiming;
			this->AimYaw = MainInstance->AimYaw;
			this->AimPitch = MainInstance->AimPitch;
		}
	}
}

UCharacterMovementComponent* USomnusAnimInstance::GetCharacterMovementComponent() const
{
	return MovementComponent;
}

void USomnusAnimInstance::UpdateLocationData()
{
	if (AActor* Actor = GetOwningActor())
	{
		FVector LastFrameLocation = Location;
		Location = Actor->GetActorLocation();
		DeltaLocation = Location - LastFrameLocation;
	}
}

void USomnusAnimInstance::UpdateVelocityData()
{
	if (ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner()))
	{
		Velocity = Character->GetVelocity();
		Velocity2D = FVector(Velocity.X, Velocity.Y, 0.0f);
		GroundSpeed = Velocity2D.Size2D();
	}
}

void USomnusAnimInstance::UpdateAccelerationData()
{
	if (ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner()))
	{
		if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
		{
			Acceleration = CharacterMovement->GetCurrentAcceleration();
			Acceleration2D = FVector(Acceleration.X, Acceleration.Y, 0.0f);
		}
	}
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
		
		if (bIsJumping)
		{
			const float VelocityZ = Velocity.Z;
			const float Gravity = 
				FMath::Min(
					CharacterMovement->GetGravityZ() * 
					CharacterMovement->GravityScale,
					0.1f
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
		
		// Ray cast to ground for distance matching
		if (ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner()))
		{
			if (bIsOnAir)
			{
				// 1. Setup the Line Trace
				FVector StartLocation = Character->GetActorLocation();
				// Cast a ray 20 meters downwards
				FVector EndLocation = StartLocation + FVector(0.0f, 0.0f, -2000.0f); 
            
				FHitResult HitResult;
				FCollisionQueryParams CollisionParams;
				CollisionParams.AddIgnoredActor(Character); // Don't hit ourselves!

				// 2. Fire the raycast (using Visibility channel, or a custom ground channel)
				bool bHit = GetWorld()->LineTraceSingleByChannel(
					HitResult, 
					StartLocation, 
					EndLocation, 
					ECC_Visibility, 
					CollisionParams);

				if (bHit)
				{
					// 3. Calculate exact distance from FEET to ground
					// (Actor's Z minus Capsule's Half Height gives us the Z of the feet)
					float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					float FeetZ = StartLocation.Z - CapsuleHalfHeight;
                
					// The actual distance from feet to the impact point
					DistanceToGround = FMath::Max(0.0f, FeetZ - HitResult.ImpactPoint.Z);
				}
				else
				{
					// If we hit nothing, assume we are high in the sky
					DistanceToGround = -1.0f; // Use -1 or a large number to indicate "no ground"
				}
			}
			else
			{
				DistanceToGround = 0.f;
			}
		}
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

