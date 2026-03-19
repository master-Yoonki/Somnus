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

UCLASS()
class SOMNUS_API USomnusAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	void UpdateLocationData();
	void UpdateVelocityData();
	void UpdateAccelerationData();
	void UpdateJumpingData(float DeltaSeconds);
	// Calculates the new direction with a sticky deadzone to prevent flickering
	ELocomotionDirection CalculateDirectionWithHysteresis(float Angle, ELocomotionDirection CurrentDir, float Deadzone = 20.0f);

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
};
