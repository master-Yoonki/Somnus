// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/SomnusCharacterAnimInstance_MM.h"

#include "BlendStack/BlendStackAnimNodeLibrary.h"
#include "Kismet/KismetMathLibrary.h"

bool USomnusCharacterAnimInstance_MM::IsStarting() const
{
	bool bIsFutureVelocityFastEnough = Trj_FutureVelocity.Length() >= Velocity.Length() + 100.f;
	bool bIsPivoting = CurrentDatabaseTags.Contains(FName("Pivots"));
	return IsMoving() && bIsFutureVelocityFastEnough && !bIsPivoting;
}

bool USomnusCharacterAnimInstance_MM::IsPivoting() const
{
	return FMath::Abs(GetTrajectoryTurnAngle()) >= 30.f && IsMoving();
}

bool USomnusCharacterAnimInstance_MM::IsStopping() const
{
	return CurrentDatabaseTags.Contains(FName("Stops"));
}

bool USomnusCharacterAnimInstance_MM::ShouldTurnInPlace() const
{
	float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(OrientationIntent, RootTransform.Rotator()).Yaw;
	return bHasOwningActor && (FMath::Abs(DeltaYaw) > 50.f);
}

EPoseSearchInterruptMode USomnusCharacterAnimInstance_MM::GetMMInterruptMode() const
{
	bool bIsMovementStateChanged = MovementState != MovementState_LastFrame;
	bool bIsMovementModeChanged = MovementMode != MovementMode_LastFrame;
	bool bIsGaitChanged = Gait != Gait_LastFrame;
	
	bool bIsMovementStateMoving = MovementState == ESomnusMovementState::Moving;
	
	bool bRes1 = (bIsMovementStateMoving && bIsGaitChanged) || bIsMovementStateChanged;
	bool bRes2 = bRes1 && (MovementMode == ESomnusMovementMode::OnGround) || bIsMovementModeChanged;
	
	return bRes2 ? EPoseSearchInterruptMode::InterruptOnDatabaseChange : EPoseSearchInterruptMode::DoNotInterrupt;
}

bool USomnusCharacterAnimInstance_MM::ShouldEnableSteering(const FAnimNodeReference& Node) const
{
	return MovementState == ESomnusMovementState::Moving
		&& UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimIsActive(Node);
}

void USomnusCharacterAnimInstance_MM::UpdateEssentialValues(float DeltaSeconds)
{
	Super::UpdateEssentialValues(DeltaSeconds);
	GenerateTrajectory();
}

void USomnusCharacterAnimInstance_MM::GenerateTrajectory()
{
	FPoseSearchTrajectoryData PoseSearchTrajectoryData = 
		Speed2D > 0.f ? TrajectoryGenerationData_Moving : TrajectoryGenerationData_Idle;
	
	FTransformTrajectory GeneratedTrajectory;
	UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectory(
		this, PoseSearchTrajectoryData, GetDeltaSeconds(), Trajectory, PreviousDesiredControllerYaw,
		GeneratedTrajectory, -1.f, 30.f, 0.1f, 15
		);
	
	FTransformTrajectory CollisionHandledTrajectory;
	FPoseSearchTrajectory_WorldCollisionResults WorldCollisionResults;
	
	TArray<AActor*> ActorsToIgnore;
	
	UPoseSearchTrajectoryLibrary::HandleTransformTrajectoryWorldCollisions(
		GetWorld(), this, GeneratedTrajectory, true, 0.01f, 
		CollisionHandledTrajectory, WorldCollisionResults, 
		UEngineTypes::ConvertToTraceType(ECC_Visibility), 
		false, ActorsToIgnore, EDrawDebugTrace::Type::None, true, 150.f);
	
	TrajectoryCollision = WorldCollisionResults;
	Trajectory = CollisionHandledTrajectory;
	
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, -0.3, -0.2, Trj_PastVelocity);
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, 0.0, 0.2, Trj_CurrentVelocity);
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, 0.4, 0.5, Trj_FutureVelocity);
}

float USomnusCharacterAnimInstance_MM::GetTrajectoryTurnAngle() const
{
	FRotator AccRot = UKismetMathLibrary::Conv_VectorToRotator(Acceleration);
	FRotator VelRot = UKismetMathLibrary::Conv_VectorToRotator(Velocity);
	return UKismetMathLibrary::NormalizedDeltaRotator(AccRot, VelRot).Yaw;
}
