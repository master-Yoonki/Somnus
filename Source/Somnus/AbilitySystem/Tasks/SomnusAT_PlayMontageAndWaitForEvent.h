// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "SomnusAT_PlayMontageAndWaitForEvent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSomnusMontageWaitEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

/**
 * Combines PlayMontageAndWait + WaitGameplayEvent into one task.
 * Plays a montage and listens for gameplay events simultaneously.
 * Properly handles edge cases like montage interruption while waiting for events.
 */
UCLASS()
class SOMNUS_API USomnusAT_PlayMontageAndWaitForEvent : public UAbilityTask
{
	GENERATED_BODY()

public:
	USomnusAT_PlayMontageAndWaitForEvent(const FObjectInitializer& ObjectInitializer);

	/**
	 * Play a montage and wait for it to finish or for a gameplay event.
	 * EventTags filters which gameplay events to listen for.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static USomnusAT_PlayMontageAndWaitForEvent* PlayMontageAndWaitForEvent(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		UAnimMontage* MontageToPlay,
		FGameplayTagContainer EventTags,
		float Rate = 1.0f,
		FName StartSection = NAME_None,
		bool bStopWhenAbilityEnds = true,
		float AnimRootMotionTranslationScale = 1.0f);

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	// Montage finished naturally
	UPROPERTY(BlueprintAssignable)
	FSomnusMontageWaitEventDelegate OnCompleted;

	// Montage started to blend out
	UPROPERTY(BlueprintAssignable)
	FSomnusMontageWaitEventDelegate OnBlendOut;

	// Montage was interrupted (e.g., by another ability)
	UPROPERTY(BlueprintAssignable)
	FSomnusMontageWaitEventDelegate OnInterrupted;

	// Montage was cancelled (ability ended)
	UPROPERTY(BlueprintAssignable)
	FSomnusMontageWaitEventDelegate OnCancelled;

	// A gameplay event matching EventTags was received
	UPROPERTY(BlueprintAssignable)
	FSomnusMontageWaitEventDelegate EventReceived;

private:
	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY()
	FGameplayTagContainer EventTags;

	UPROPERTY()
	float Rate;

	UPROPERTY()
	FName StartSection;

	UPROPERTY()
	float AnimRootMotionTranslationScale;

	bool bStopWhenAbilityEnds;

	bool StopPlayingMontage();
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnAbilityCancelled();
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle CancelledHandle;
	FDelegateHandle EventHandle;
};
