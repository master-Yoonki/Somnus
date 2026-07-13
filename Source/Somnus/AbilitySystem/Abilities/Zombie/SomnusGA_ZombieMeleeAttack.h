// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
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
class SOMNUS_API USomnusGA_ZombieMeleeAttack : public USomnusGameplayAbility
{
	GENERATED_BODY()
public:
	USomnusGA_ZombieMeleeAttack();
	
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;
	
	UFUNCTION(BlueprintCallable)
	UAnimMontage* SelectAttackMontage(AActor* TargetActor);
protected:
	// The montage to play when attacking (set in the Blueprint subclass)
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<FSomnusZombieAttackMontage> AttackMontages;
	
	// Playback rate for the montage
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	float MontagePlayRate = 1.0f;
	
	//
	// // TODO: Animation Controlled by montage for now, just simple delay to simulate
	// UFUNCTION()
	// void OnDelayFinished();
	//
	// Damage GE to apply on hit (should use SetByCaller with Data.Damage tag)
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	// Damage amount passed to the GE via SetByCaller
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageAmount = 20.0f;
	
private:
	static FSomnusZombieAttackMontage* PickByPriority(const TArray<FSomnusZombieAttackMontage*>& MontagesArray);
	
	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMeleeHit(FGameplayTag EventTag, FGameplayEventData EventData);
};
