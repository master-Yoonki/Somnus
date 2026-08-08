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
};

UCLASS()
class SOMNUS_API USomnusGA_ZombieMeleeAttack : public USomnusGA_MeleeAttack
{
	GENERATED_BODY()
public:
	USomnusGA_ZombieMeleeAttack();

	UFUNCTION(BlueprintCallable)
	UAnimMontage* SelectAttackMontage(const AActor* TargetActor);

protected:
	// Candidate montages; the one matching the target's angle/distance is picked at activation.
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<FSomnusZombieAttackMontage> AttackMontages;

	// Resolves the AI target, then selects a directional montage. Called by the base ActivateAbility.
	virtual UAnimMontage* GetMontageToPlay(const FGameplayEventData* TriggerEventData) override;

private:
	static FSomnusZombieAttackMontage* PickByPriority(TArray<FSomnusZombieAttackMontage>* MontagesArray);
};
