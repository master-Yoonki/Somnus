// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Tasks/SomnusAT_PlayMontageAndWaitForEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"

USomnusAT_PlayMontageAndWaitForEvent::USomnusAT_PlayMontageAndWaitForEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Rate = 1.0f;
	bStopWhenAbilityEnds = true;
	AnimRootMotionTranslationScale = 1.0f;
}

USomnusAT_PlayMontageAndWaitForEvent* USomnusAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	UAnimMontage* InMontageToPlay,
	FGameplayTagContainer InEventTags,
	float InRate,
	FName InStartSection,
	bool bInStopWhenAbilityEnds,
	float InAnimRootMotionTranslationScale)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(InRate);

	USomnusAT_PlayMontageAndWaitForEvent* Task = NewAbilityTask<USomnusAT_PlayMontageAndWaitForEvent>(OwningAbility, TaskInstanceName);
	Task->MontageToPlay = InMontageToPlay;
	Task->EventTags = InEventTags;
	Task->Rate = InRate;
	Task->StartSection = InStartSection;
	Task->bStopWhenAbilityEnds = bInStopWhenAbilityEnds;
	Task->AnimRootMotionTranslationScale = InAnimRootMotionTranslationScale;

	return Task;
}

void USomnusAT_PlayMontageAndWaitForEvent::Activate()
{
	if (!Ability)
	{
		return;
	}

	bool bPlayedMontage = false;
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();

	if (ASC)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;

		if (AnimInstance != nullptr)
		{
			// Listen for gameplay events matching our filter tags
			EventHandle = ASC->AddGameplayEventTagContainerDelegate(
				EventTags,
				FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(
					this, &USomnusAT_PlayMontageAndWaitForEvent::OnGameplayEvent));

			if (ASC->PlayMontage(Ability, Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection) > 0.0f)
			{
				// Listen for montage cancellation via ability cancel
				if (ShouldBroadcastAbilityTaskDelegates())
				{
					CancelledHandle = Ability->OnGameplayAbilityCancelled.AddUObject(
						this, &USomnusAT_PlayMontageAndWaitForEvent::OnAbilityCancelled);
				}

				// Listen for montage blending out
				BlendingOutDelegate.BindUObject(this, &USomnusAT_PlayMontageAndWaitForEvent::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				// Listen for montage ended
				MontageEndedDelegate.BindUObject(this, &USomnusAT_PlayMontageAndWaitForEvent::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
				if (Character && (Character->GetLocalRole() == ROLE_Authority ||
					(Character->GetLocalRole() == ROLE_AutonomousProxy &&
					 Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
				{
					Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
				}

				bPlayedMontage = true;
			}
		}
	}

	if (!bPlayedMontage)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	SetWaitingOnAvatar();
}

void USomnusAT_PlayMontageAndWaitForEvent::ExternalCancel()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
	}
	Super::ExternalCancel();
}

void USomnusAT_PlayMontageAndWaitForEvent::OnDestroy(bool bInOwnerFinished)
{
	// Remove gameplay event listener
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
	}

	// Unbind ability cancel delegate
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(CancelledHandle);

		// Stop the montage if ability ended and we're configured to do so
		if (bInOwnerFinished && bStopWhenAbilityEnds)
		{
			StopPlayingMontage();
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}

bool USomnusAT_PlayMontageAndWaitForEvent::StopPlayingMontage()
{
	if (!Ability) return false;

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo) return false;

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (!AnimInstance) return false;

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC && Ability)
	{
		if (ASC->GetAnimatingAbility() == Ability && ASC->GetCurrentMontage() == MontageToPlay)
		{
			// Unbind delegates so they don't fire when we manually stop
			if (FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay))
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}

			ASC->CurrentMontageStop();
			return true;
		}
	}

	return false;
}

void USomnusAT_PlayMontageAndWaitForEvent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Ability && Ability->GetCurrentMontage() == MontageToPlay)
	{
		if (Montage == MontageToPlay)
		{
			if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
			{
				ASC->ClearAnimatingAbility(Ability);
			}

			// Reset root motion scale
			ACharacter* Character = Cast<ACharacter>(Ability->GetCurrentActorInfo()->AvatarActor.Get());
			if (Character)
			{
				Character->SetAnimRootMotionTranslationScale(1.0f);
			}
		}
	}

	if (bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
	else
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnBlendOut.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
}

void USomnusAT_PlayMontageAndWaitForEvent::OnAbilityCancelled()
{
	if (StopPlayingMontage())
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
}

void USomnusAT_PlayMontageAndWaitForEvent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	EndTask();
}

void USomnusAT_PlayMontageAndWaitForEvent::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempData = Payload ? *Payload : FGameplayEventData();
		TempData.EventTag = EventTag;
		EventReceived.Broadcast(EventTag, TempData);
	}
}

FString USomnusAT_PlayMontageAndWaitForEvent::GetDebugString() const
{
	const UAnimMontage* PlayingMontage = MontageToPlay;
	if (Ability)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		if (const UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr)
		{
			if (AnimInstance->Montage_IsActive(MontageToPlay))
			{
				PlayingMontage = MontageToPlay;
			}
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWaitForEvent. MontageName: %s"),
		*GetNameSafe(PlayingMontage));
}
