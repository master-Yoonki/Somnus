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

/**
 * How the skeleton is currently driven. Drives, body movement type, gravity and the constraint
 * profile have to agree with each other - a body left simulating while a drive still pulls it back
 * to the animated pose looks frozen rather than physical. Naming the combinations is what keeps
 * an inconsistent one from being reachable.
 */
UENUM(BlueprintType)
enum class ESomnusPhysicsPose : uint8
{
	/** Animation owns the pose; bodies are kinematic. */
	Animated,
	/** Flinch that keeps its feet - world-space drives, feet pinned, no gravity. */
	Braced,
	/** Knocked around, but parent-space drives still pull it back toward the pose. */
	Reeling,
	/** Nothing pulls it back. A corpse. */
	Limp
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

	/** The single way the skeleton's physics state changes. Callers name the pose they want and
	 *  every value that has to agree is written here, so no half-applied combination exists. */
	UFUNCTION(BlueprintCallable, Category = "HitReact")
	void SetPhysicsPose(ESomnusPhysicsPose Pose);

	ESomnusPhysicsPose GetPhysicsPose() const { return CurrentPose; }

protected:
	// Sets the ABP to animate depending on the pose
	void SetAnimation(ESomnusPhysicsPose Pose);
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

	UPROPERTY(VisibleInstanceOnly, Category = "HitReact")
	ESomnusPhysicsPose CurrentPose = ESomnusPhysicsPose::Animated;

	bool IsAnyBoneProcessing = false;
};
