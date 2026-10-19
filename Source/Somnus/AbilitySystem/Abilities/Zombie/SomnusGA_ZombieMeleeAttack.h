// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SomnusGA_MeleeAttack.h"
#include "SomnusGA_ZombieMeleeAttack.generated.h"

USTRUCT(BlueprintType)
struct FSomnusZombieAttackMontage
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> Montage;

	// Weight for random selection if multiple montages match, low number = higher priorty
	UPROPERTY(EditDefaultsOnly, Category = "Selection")
	int32 Priority = 0;

	// Angle relative to zombie's forward vector (-180 to 180)
	UPROPERTY(EditDefaultsOnly, Category = "Conditions")
	float MinAngle = -180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Conditions")
	float MaxAngle = 180.0f;

	// Distance to target (for picking in-place vs lunge attacks)
	UPROPERTY(EditDefaultsOnly, Category = "Conditions")
	float MinDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Conditions")
	float MaxDistance = 150.0f;

	/** For a montage with a motion warping window: how far short of the target's centre the body
	 *  should stand when the strike lands, so the hand meets the target instead of the body walking
	 *  into it. Measured per montage (Content/Python/measure_attack_reach.py). */
	UPROPERTY(EditDefaultsOnly, Category = "Warping", meta = (ClampMin = "0", Units = "cm"))
	float WarpContactDistance = 0.0f;
};

UCLASS()
class SOMNUS_API USomnusGA_ZombieMeleeAttack : public USomnusGA_MeleeAttack
{
	GENERATED_BODY()
public:
	USomnusGA_ZombieMeleeAttack();

	UFUNCTION(BlueprintCallable)
	UAnimMontage* SelectAttackMontage(const AActor* TargetActor);

	/** Whether any montage in the table could be swung at Target from where Source stands now, by the
	 *  same angle and distance rules SelectAttackMontage picks with. Reads nothing from an activation,
	 *  so it can be asked of the class default. */
	bool HasReachableMontage(const AActor* Source, const AActor* Target) const;

protected:
	virtual bool GetWarpTarget(const AActor*& OutTarget, float& OutContactDistance) const override;
	
	// Candidate montages; the one matching the target's angle/distance is picked at activation.
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<FSomnusZombieAttackMontage> AttackMontages;

	/** Blackboard key holding who to attack, read when the tree activates this with no event payload. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FName TargetActorKeyName = FName("TargetActor");

	// Resolves the AI target, then selects a directional montage. Called by the base ActivateAbility.
	virtual UAnimMontage* GetMontageToPlay(const FGameplayEventData* TriggerEventData) override;

private:
	static FSomnusZombieAttackMontage* PickByPriority(TArray<FSomnusZombieAttackMontage>* MontagesArray);

	/** Horizontal distance from Source to Target, and the target's yaw off Source's facing (-180 to 180). */
	static void MeasureTarget(const AActor* Source, const AActor* Target, float& OutDistance, float& OutAngle);

	/** The one rule for whether a montage can be swung at a target this far away at this angle,
	 *  shared by picking a montage and by asking whether any would do. */
	static bool CanSwing(const FSomnusZombieAttackMontage& MontageConfig, float Distance, float Angle);
	
	TWeakObjectPtr<const AActor> WarpTarget;
	float WarpContactDistance = 0.f;
};
