// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "SomnusAsyncTask_ListenForCooldownChange.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCooldownChanged, FGameplayTag, CooldownTag, float, TimeRemaining, float, Duration);

/**
 * Blueprint async node that listens for cooldown GE application/removal on an ASC.
 * Broadcasts when a cooldown starts (with duration) and when it ends (TimeRemaining = 0).
 * Call EndTask() (e.g., in widget Destruct) to stop listening.
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class SOMNUS_API USomnusAsyncTask_ListenForCooldownChange : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static USomnusAsyncTask_ListenForCooldownChange* ListenForCooldownChange(
		UAbilitySystemComponent* AbilitySystemComponent,
		FGameplayTagContainer CooldownTags,
		bool bUseServerCooldown);

	UFUNCTION(BlueprintCallable)
	void EndTask();

	// Fired when a cooldown GE is applied or removed
	UPROPERTY(BlueprintAssignable)
	FOnCooldownChanged OnCooldownBegin;

	UPROPERTY(BlueprintAssignable)
	FOnCooldownChanged OnCooldownEnd;

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	FGameplayTagContainer CooldownTags;
	bool bUseServerCooldown;

	void OnActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
	void CooldownTagChanged(const FGameplayTag CooldownTag, int32 NewCount);

	bool GetCooldownRemainingForTag(const FGameplayTagContainer& Tags, float& TimeRemaining, float& Duration) const;
};
