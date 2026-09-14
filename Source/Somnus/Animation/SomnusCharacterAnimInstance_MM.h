// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/SomnusCharacterAnimInstance.h"
#include "Animation/AnimNodeReference.h"
#include "Animation/TrajectoryTypes.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "PoseSearch/PoseSearchLibrary.h"

#include "SomnusCharacterAnimInstance_MM.generated.h"

/**
 * 
 */
UCLASS()
class SOMNUS_API USomnusCharacterAnimInstance_MM : public USomnusCharacterAnimInstance
{
	GENERATED_BODY()
public:
	virtual bool IsStarting() const override;
	virtual bool IsPivoting() const override;
	virtual bool IsStopping() const override;
	virtual bool ShouldTurnInPlace() const override;
protected:
	UFUNCTION(BlueprintPure, Category = "MotionMatching", meta = (BlueprintThreadSafe, HideSelfPin))
	EPoseSearchInterruptMode GetMMInterruptMode() const;

	/** Steering corrects the gap between where the selected animation is heading and where the
	 *  trajectory wants to go, so it only has something to correct while the character is moving
	 *  and the blend stack is playing a clip it chose. Node must be a blend stack node - the motion
	 *  matching node is one. */
	UFUNCTION(BlueprintPure, Category = "MotionMatching", meta = (BlueprintThreadSafe, HideSelfPin))
	bool ShouldEnableSteering(const FAnimNodeReference& Node) const;

	virtual void UpdateEssentialValues(float DeltaSeconds) override;
	void GenerateTrajectory();
	UFUNCTION(BlueprintPure, Category = "Trajectory")
	float GetTrajectoryTurnAngle() const;
	
	UPROPERTY(BlueprintReadWrite, Category = "MotionMatching")
	UObject* CurrentSelectedAnim;
	UPROPERTY(BlueprintReadWrite, Category = "MotionMatching")
	UPoseSearchDatabase* CurrentSelectedDatabase;
	UPROPERTY(BlueprintReadWrite, Category = "MotionMatching")
	TArray<FName> CurrentDatabaseTags;
	
	UPROPERTY(BlueprintReadOnly, Category = "Trajectory")
	FTransformTrajectory Trajectory;
	UPROPERTY(BlueprintReadOnly, Category = "Trajectory")
	float PreviousDesiredControllerYaw;
	UPROPERTY(BlueprintReadOnly, Category = "Trajectory")
	FVector Trj_PastVelocity;
	UPROPERTY(BlueprintReadOnly, Category = "Trajectory")
	FVector Trj_CurrentVelocity;
	UPROPERTY(BlueprintReadOnly, Category = "Trajectory")
	FVector Trj_FutureVelocity;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory")
	FPoseSearchTrajectoryData TrajectoryGenerationData_Idle;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory")
	FPoseSearchTrajectoryData TrajectoryGenerationData_Moving;
	UPROPERTY(BlueprintReadOnly, Category = "Trajectory")
	FPoseSearchTrajectory_WorldCollisionResults TrajectoryCollision;
};
