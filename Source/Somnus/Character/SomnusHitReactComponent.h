// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "Engine/NetSerialization.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "SomnusHitReactComponent.generated.h"

/**
 * Physics-driven flinch shared by the player and zombies.
 *
 * The simulated set is fixed (SimulationRootBone and below) rather than chosen per hit, so
 * overlapping reactions never fight over a body's blend weight — repeat hits just add another
 * impulse and refresh the timer. Physical animation motors pull the simulated bodies back to
 * the animated pose, which is what lets the blend-out be short and unnoticeable.
 */
UCLASS( ClassGroup=(Somnus), meta=(BlueprintSpawnableComponent) )
class SOMNUS_API USomnusHitReactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USomnusHitReactComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Impulse arrives fully resolved: the ability owns both direction (it holds the fallback for
	// degenerate sweep normals) and strength (damage, weapon type).
	UFUNCTION(BlueprintCallable, Category = "HitReact")
	void TriggerHitReact(const FHitResult& HitResult, const FVector& Impulse);

	// Hands the mesh back to the animation pose immediately. Death takes the whole mesh for its
	// ragdoll, so an in-flight reaction must release it or the recovery blend will partially
	// un-ragdoll the corpse. Safe to call on every machine and when no reaction is active.
	UFUNCTION(BlueprintCallable, Category = "HitReact")
	void StopHitReact();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitReact(FName HitBone, FVector_NetQuantize ImpactPoint, FVector_NetQuantize Impulse);

	void BeginReaction();

	// This bone and its skeletal descendants simulate for a reaction's duration — specifically,
	// whichever of them the physics asset gives a body to. The bone itself does not need one.
	// Rooting this at the pelvis drags the legs in, and simulated legs fight CharacterMovement.
	// A hit outside the subtree is ignored entirely, reaction and all.
	UPROPERTY(EditDefaultsOnly, Category = "HitReact")
	FName SimulationRootBone = TEXT("spine_01");

	// Motor strengths that pull the simulated bodies back toward the animated pose. Authored here
	// rather than as a physics asset profile so both skeletons can share one component default.
	UPROPERTY(EditDefaultsOnly, Category = "HitReact")
	FPhysicalAnimationData PhysicalAnimationSettings;

	UPROPERTY(EditDefaultsOnly, Category = "HitReact", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxBlendWeight = 1.0f;

	// How long the body stays simulated after the last hit.
	UPROPERTY(EditDefaultsOnly, Category = "HitReact", meta = (ClampMin = "0.0"))
	float RecoveryTime = 0.5f;

	// Tail of RecoveryTime spent blending back to pure animation. Short is fine: the motors have
	// already converged the simulated pose onto the animated one by this point.
	UPROPERTY(EditDefaultsOnly, Category = "HitReact", meta = (ClampMin = "0.0"))
	float BlendOutTime = 0.15f;

	// --- Runtime state ---
	// Owned rather than read back from the tick function: Activate/Deactivate are BlueprintCallable
	// and drive the tick flag, so borrowing it would let a details-panel click strand a reaction
	// with the mesh left simulating at full blend weight.
	bool bIsReacting = false;

	float RecoveryElapsed = 0.0f;

	UPROPERTY()
	TObjectPtr<class USkeletalMeshComponent> CachedSkeletalMeshComponent;

	UPROPERTY()
	TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimation;
};
