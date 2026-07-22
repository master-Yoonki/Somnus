// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsControlComponent.h"
#include "SomnusHitReactComponent.generated.h"

USTRUCT()
struct FBoneHitReactProcessingData
{
	GENERATED_BODY()
	
	float ProcessingTime = 0.f;
	FTimerHandle DelayTimer;
};

UCLASS( ClassGroup=(Somnus), meta=(BlueprintSpawnableComponent) )
class SOMNUS_API USomnusHitReactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USomnusHitReactComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Applies a physics impulse at the given bone/location. Impulse carries both
	// direction and magnitude (composed upstream from the strike velocity).
	void HitReaction(FName BoneName, const FVector& Location, const FVector& Impulse);
protected:
	// Hands the skeleton back to pure animation once every bone has recovered.
	void StopPhysicalAnimProcessing();
	
	// Sets up physics as we transition into a new hit. This depends on whether we will ragdoll or not.
	void SetupPhysicsForHit(bool bIsRagdoll);
	// Sets the ABP to animate depending on whether we are ragdolling
	void SetAnimation(bool bIsRagdoll);
	// Fired when a bone's delay timer elapses; payload (bone + impulse) is captured at bind time.
	void ApplyDelayedImpulse(FName BoneName, FVector Location, FVector Impulse);
	
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitReaction(FName BoneName, const FVector& Location, const FVector& Impulse);
	
	//User Setup
	UPROPERTY(EditAnywhere)
	float ImpuseApplyDelay = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> SkeletalKinematicCurveFloat;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> AnimToAnimCurveFloat;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FPhysicsControlLimbSetupData> LimbSetupData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPhysicsControlData WorldSpaceControlData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPhysicsControlData ParentSpaceControlData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPhysicsControlModifierData BodyModifierData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ConstraintProfile;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RecoveryTime;
	// Multiple hits can land on the same bone before any of them finish recovering
	// (e.g. two attacks landing on a bone within RecoveryTime of each other), so each
	// bone tracks an array of independent in-flight hits rather than a single record.
	// Not a UPROPERTY: UHT doesn't support a container-of-containers as a reflected
	// member (TMap value can't itself be a TArray), and nothing here needs GC tracking
	// (FBoneHitReactProcessingData holds no UObject pointers) or Blueprint exposure.
	TMap<FName, TArray<FBoneHitReactProcessingData>> BoneHitReactProcessingData;
	
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	UPROPERTY()
	TObjectPtr<UPhysicsControlComponent> PhysicsControlComponent;
	
	UPROPERTY()
	TMap<FName, FPhysicsControlNames> LimbWorldSpaceControls;
	UPROPERTY()
	TMap<FName, FPhysicsControlNames> LimbParentSpaceControls;
	UPROPERTY()
	TMap<FName, FPhysicsControlNames> LimbBodyModifiers;
	
	FPhysicsControlNames AllBodyModifiers;
	FPhysicsControlNames AllParentSpaceControls;
	FPhysicsControlNames AllWorldSpaceControls;
	
	bool IsAnyBoneProcessing = false;
};
