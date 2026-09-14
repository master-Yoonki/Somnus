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
	
protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "EssentialValues", meta = (BlueprintThreadSafe))
	FAnimNodeReference GetOffsetRootNode();
	
	virtual void UpdateEssentialValues(float DeltaSeconds);
	virtual void UpdateStates();
	
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	class UCharacterMovementComponent* CharacterMovement;
	
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	bool bHasOwningActor;
	
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	FTransform CharacterTransform;
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	FTransform CharacterTransform_LastFrame;
	
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	FTransform RootTransform;
	
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	float AccelerationAmount;
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	float Speed2D;
	
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	bool bHasAcceleration;
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	bool bHasVelocity;
	
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	FVector Acceleration;
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	FVector Acceleration_LastFrame;
	
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	FVector Velocity;
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	FVector Velocity_LastFrame;
	
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
	FVector VelocityAcceleration;
	UPROPERTY(BlueprintReadOnly, Category = "EsseitnalValues")
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
	
	UPROPERTY(BlueprintReadOnly, Category = "OffsetRoot")
	FRotator OrientationIntent;
	
	UPROPERTY(BlueprintReadOnly, Category = "InAir")
	float HeavyLandSpeedThreshold;
};
