// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/SomnusMovementTypes.h"
#include "Animation/AnimNodeReference.h"
#include "SomnusCharacterAnimInstance.generated.h"

class ASomnusCharacter;
/**
 * The player character's animation, named for what it animates rather than how. Which technique
 * picks the pose - motion matching today - is the graph's business; this class only gathers the
 * character's state into something the graph can read on a worker thread.
 */
UCLASS()
class SOMNUS_API USomnusCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
	UFUNCTION(BlueprintPure)
	ASomnusCharacter* GetSomnusCharacter() const;
	
	UFUNCTION(BlueprintPure, Category = "MovementAnalysis", meta = (BlueprintThreadSafe, HideSelfPin))
	virtual bool IsMoving() const;
	UFUNCTION(BlueprintPure, Category = "MovementAnalysis", meta = (BlueprintThreadSafe, HideSelfPin))
	virtual bool IsStarting() const;
	UFUNCTION(BlueprintPure, Category = "MovementAnalysis", meta = (BlueprintThreadSafe, HideSelfPin))
	virtual bool IsPivoting() const;
	UFUNCTION(BlueprintPure, Category = "MovementAnalysis", meta = (BlueprintThreadSafe, HideSelfPin))
	virtual bool IsStopping() const;
	UFUNCTION(BlueprintPure, Category = "MovementAnalysis", meta = (BlueprintThreadSafe, HideSelfPin))
	virtual bool ShouldTurnInPlace() const;
	UFUNCTION(BlueprintPure, Category = "MovementAnalysis", meta = (BlueprintThreadSafe, HideSelfPin))
	virtual bool JustLanded_Light() const;
	UFUNCTION(BlueprintPure, Category = "MovementAnalysis", meta = (BlueprintThreadSafe, HideSelfPin))
	virtual bool JustLanded_Heavy() const;

	ESomnusGait GetGait() const { return Gait; }
	ESomnusMovementState GetMovementState() const { return MovementState; }
	float GetSpeed2D() const { return Speed2D; }
	bool IsAiming() const { return bIsAiming; }
	float GetAimStanceYaw() const { return AimStanceYaw; }
	float GetUpperBodyYawOffset() const { return UpperBodyYawOffset; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "EssentialValues", meta = (BlueprintThreadSafe))
	FAnimNodeReference GetOffsetRootNode();
	
	virtual void UpdateEssentialValues(float DeltaSeconds);

	/** Game thread only - this is where the character is asked anything. */
	virtual void UpdateStates();
	
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	class UCharacterMovementComponent* CharacterMovement;
	
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	bool bHasOwningActor;
	
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FTransform CharacterTransform;
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FTransform CharacterTransform_LastFrame;
	
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FTransform RootTransform;
	
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	float AccelerationAmount;
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	float Speed2D;
	
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	bool bHasAcceleration;
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	bool bHasVelocity;
	
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FVector Acceleration;
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FVector Acceleration_LastFrame;
	
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FVector Velocity;
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FVector Velocity_LastFrame;
	
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FVector VelocityAcceleration;
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	FVector LastNonZeroVelocity;
	
	UPROPERTY(BlueprintReadOnly, Category = "States")
	ESomnusMovementState MovementState;
	UPROPERTY(BlueprintReadOnly, Category = "States")
	ESomnusMovementState MovementState_LastFrame;
	
	UPROPERTY(BlueprintReadOnly, Category = "States")
	ESomnusGait Gait;
	UPROPERTY(BlueprintReadOnly, Category = "States")
	ESomnusGait Gait_LastFrame;
	
	UPROPERTY(BlueprintReadOnly, Category = "States")
	ESomnusMovementMode MovementMode;
	UPROPERTY(BlueprintReadOnly, Category = "States")
	ESomnusMovementMode MovementMode_LastFrame;

	/** Everything below is the character's answer to a question, taken once on the game thread.
	 *  The queries the graph and the choosers call run on a worker thread, where reaching back
	 *  into the actor is neither safe nor possible - in the asset preview there is no actor at
	 *  all, which is what used to take the editor down. */
	UPROPERTY(BlueprintReadOnly, Category = "States")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "InAir")
	bool bJustLanded = false;

	UPROPERTY(BlueprintReadOnly, Category = "InAir")
	FVector LandVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "States")
	bool bIsAiming = false;

	/** How far the character has turned the mesh for the aim stance. */
	UPROPERTY(BlueprintReadOnly, Category = "States")
	float AimStanceYaw = 0.f;

	/** The slot full-body montages play in. The slot is watched rather than any one montage, so
	 *  every clip put in it hands back to locomotion the same way. */
	UPROPERTY(EditDefaultsOnly, Category = "Montage")
	FName FullBodySlotName = TEXT("MeleeHeavyAttack");

	/** True while a full-body montage owns the pose, and false from the frame it starts blending
	 *  out - the moment locomotion has to take over again. */
	UPROPERTY(BlueprintReadOnly, Category = "Montage")
	bool bFullBodyMontageActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Montage")
	bool bFullBodyMontageActive_LastFrame = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "OffsetRoot")
	FRotator OrientationIntent;

	/** How far the offset root trails the mesh, in degrees. The lower body already rides the offset
	 *  root, so a clip posed against the mesh instead - anything blended in mesh space - turns with
	 *  the capsule while the legs do not. Rotating it by this puts both halves back in one frame. */
	UPROPERTY(BlueprintReadOnly, Category = "OffsetRoot")
	float UpperBodyYawOffset = 0.f;
	
	/** Downward speed that makes a landing a heavy one, in cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	float HeavyLandSpeedThreshold = 700.f;
};
