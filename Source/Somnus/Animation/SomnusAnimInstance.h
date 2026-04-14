// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Equipment/SomnusWeapon.h"
#include "SomnusAnimInstance.generated.h"

UENUM(BlueprintType)
enum class ESomnusGait : uint8
{
	None        UMETA(DisplayName = "None"),
	Walking     UMETA(DisplayName = "Walking"),
	Jogging     UMETA(DisplayName = "Jogging")
};

UENUM(BlueprintType)
enum class ELocomotionDirection : uint8
{
	Forward		UMETA(DisplayName = "Forward"),
	Backward	UMETA(DisplayName = "Backward"),
	Left		UMETA(DisplayName = "Left"),
	Right		UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class ERootYawOffsetMode : uint8
{
	Hold		UMETA(DisplayName = "Hold"),
	Accumulate	UMETA(DisplayName = "Accumulate"),
	BlendOut	UMETA(DisplayName = "BlendOut"),
};

UCLASS()
class SOMNUS_API USomnusAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	// Game thread only — line trace for distance matching
	void UpdateDistanceToGround();

	// Thread-safe update functions — read cached pointers, write local properties
	void UpdateLocationData();
	void UpdateVelocityData();
	void UpdateAccelerationData();
	void UpdateLocomotionData();
	void UpdateRotationData();
	void UpdateWeaponData();
	void UpdateAimingData();
	void UpdateJumpingData(float DeltaSeconds);
	void CopyFromMainInstance(const USomnusAnimInstance* MainInstance);
	ELocomotionDirection CalculateDirectionWithHysteresis(float Angle, ELocomotionDirection CurrentDir, float Deadzone = 20.0f);

	UPROPERTY(Transient)
	TObjectPtr<ASomnusCharacter> CachedCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UFUNCTION(BlueprintPure, Category = "Setup", meta = (BlueprintThreadSafe))
	UCharacterMovementComponent* GetCharacterMovementComponent() const;
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Location Data")
	FVector Location;
	
	UPROPERTY(BlueprintReadOnly, Category = "Location Data")
	FVector DeltaLocation;
	
	// Velocity of the character
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    FVector Velocity;
	
	// Velocity of the character
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
	FVector Velocity2D;
	
    // Horizontal speed for locomotion blends
    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    float GroundSpeed;
	
	// Acceleration of the character
	UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
	FVector Acceleration;
	
	UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
	FVector Acceleration2D;
	
	UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
	bool bHasAcceleration;
	
	UPROPERTY(BlueprintReadOnly, Category = "Gait Data")
	ESomnusGait Gait;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Data")
	float VelocityLocomotionAngle;
	
	// The currently active movement direction
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Data")
	ELocomotionDirection CurrentDirection;
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	ESomnusWeaponType EquippedWeaponType;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bHasUpperBodyLayer;

	// Additive pose for weapon-specific foot stance correction (set from weapon)
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> StanceCorrectionPose;

	// Alpha for stance correction additive blend (1.0 in idle, 0.0 when moving)
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	float StanceCorrectionAlpha;

	// 1.0 when moving (upper body layer active), 0.0 when idle
	UPROPERTY(BlueprintReadOnly, Category = "Blending")
	float UpperBodyBlendWeight;

	UPROPERTY(BlueprintReadOnly, Category = "Jump")
    float DistanceToGround;
	
	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	bool bIsJumping;
	
	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	bool bIsFalling;
	
	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	bool bIsOnAir;
	
	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	float TimeToJumpApex;
	
	UPROPERTY(BlueprintReadOnly, Category = "Jump")
	float TimeFalling;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsAiming;

	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	float AimYaw;

	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	float AimPitch;
	
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	FRotator WorldRotation;
	
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	float ActorYaw;
	
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	float LastFrameActorYaw;
	
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	float DeltaActorYaw;
	
	// True when the character has State.Dead tag — ABP can use to disable locomotion
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	bool bIsInitialized = false;
};
