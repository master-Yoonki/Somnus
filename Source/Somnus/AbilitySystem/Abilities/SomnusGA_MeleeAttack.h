// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "SomnusGA_MeleeAttack.generated.h"

class UGameplayEffect;

/**
 * Melee attack ability that plays an attack montage.
 * Hit detection is handled by USomnusAnimNotifyState_MeleeTrace on the montage.
 * Uses the combined PlayMontageAndWaitForEvent task for clean montage + event handling.
 */
UCLASS()
class SOMNUS_API USomnusGA_MeleeAttack : public USomnusGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGA_MeleeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Who this strike's root motion should be bent toward, and how far short of their centre the
	 *  body should stand when it lands. False leaves the montage to play as authored. */
	virtual bool GetWarpTarget(const AActor*& OutTarget, float& OutContactDistance) const { return false; }
	
	// The montage to play when attacking (set in the Blueprint subclass)
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// Playback rate for the montage
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	float MontagePlayRate = 1.0f;

	/** Each swing plays at MontagePlayRate scaled by a random factor within this much either side
	 *  (0.1 = 0.9x to 1.1x), so bodies sharing a montage drift out of step instead of moving as one.
	 *  The strike's timing shifts with it, so keep it small. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (ClampMin = "0", ClampMax = "0.5"))
	float MontagePlayRateVariance = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "MotionWarping")
	FName WarpTargetName = FName("AttackTarget");
	
	// Damage GE to apply on hit (should use SetByCaller with Data.Damage tag)
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	// Damage amount passed to the GE via SetByCaller
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageAmount = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float ImpulseScale = 5.0f;

	UPROPERTY()
	TObjectPtr<class USomnusAT_TrackStrikeVelocity> StrikeTracker;
	
	virtual UAnimMontage* GetMontageToPlay(const FGameplayEventData* TriggerEventData)
	{
		return AttackMontage;
	}
private:
	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMeleeHit(FGameplayTag EventTag, FGameplayEventData EventData);
	

};
