// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_MeleeAttack.h"
#include "AbilitySystem/Tasks/SomnusAT_PlayMontageAndWaitForEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SomnusGameplayEffectContext.h"
#include "AbilitySystem/Tasks/SomnusAT_TrackStrikeVelocity.h"
#include "Core/SomnusGameplayTags.h"

USomnusGA_MeleeAttack::USomnusGA_MeleeAttack()
{
	// Tags are configured per Blueprint subclass:
	// - AbilityTags: identifies this ability (e.g., Ability.Action.Swing or Ability.Action.Aim)
	// - ActivationRequiredTags: what must be present (e.g., Ability.Enable.Swing, State.Aiming)
	// - ActivationBlockedTags: what prevents activation (e.g., State.Aiming for light attack)
}

void USomnusGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Subclasses decide which montage to play (single montage, directional selection, etc.)
	UAnimMontage* MontageToPlay = GetMontageToPlay(TriggerEventData);
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Combined task: plays montage AND listens for melee hit events simultaneously
	FGameplayTagContainer EventTagFilter;
	EventTagFilter.AddTag(SomnusTags::Event_Melee_Hit);

	USomnusAT_PlayMontageAndWaitForEvent* PlayMontageAndWaitForEventTask = USomnusAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this, NAME_None, MontageToPlay, EventTagFilter, MontagePlayRate);

	PlayMontageAndWaitForEventTask->OnCompleted.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCompleted);
	PlayMontageAndWaitForEventTask->OnBlendOut.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCompleted);
	PlayMontageAndWaitForEventTask->OnInterrupted.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCancelled);
	PlayMontageAndWaitForEventTask->OnCancelled.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCancelled);
	PlayMontageAndWaitForEventTask->EventReceived.AddDynamic(this, &USomnusGA_MeleeAttack::OnMeleeHit);
	
	PlayMontageAndWaitForEventTask->ReadyForActivation();
	
	if (ISomnusStrikeSource* StrikeSource = Cast<ISomnusStrikeSource>(GetAvatarActorFromActorInfo()))
	{
		TArray<FSomnusStrikeSourceInfo> SourceInfos = StrikeSource->GetStrikeSources();
		StrikeTracker
			= USomnusAT_TrackStrikeVelocity::TrackStrikeVelocity(this, NAME_None, SourceInfos);
		
		StrikeTracker->ReadyForActivation();
	}
}

void USomnusGA_MeleeAttack::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USomnusGA_MeleeAttack::OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USomnusGA_MeleeAttack::OnMeleeHit(FGameplayTag EventTag, FGameplayEventData EventData)
{
	if (!DamageEffectClass || !EventData.Target)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
		const_cast<AActor*>(EventData.Target.Get()));
	if (!TargetASC)
	{
		return;
	}
	
	FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(EventData.TargetData, 0);

	FGameplayEffectSpecHandle DealDamageSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass);
	
	if (DealDamageSpecHandle.IsValid())
	{	
		if (EventData.TargetData.Num() > 0)
		{
			DealDamageSpecHandle.Data->GetContext().AddHitResult(HitResult);
		}
		DealDamageSpecHandle.Data->SetSetByCallerMagnitude(SomnusTags::Data_Damage, DamageAmount);
		
		if (StrikeTracker)
		{
			FGameplayEffectContextHandle Ctx = DealDamageSpecHandle.Data->GetContext();
		
			if (FSomnusGameplayEffectContext* SomnusCtx = static_cast<FSomnusGameplayEffectContext*>(Ctx.Get()))
			{
				FVector StrikeVelocity;
				float StrikeWeight;
				StrikeTracker->GetVelocityAtContactPoint(HitResult.ImpactPoint, StrikeVelocity, StrikeWeight);

				const FVector Impulse = StrikeVelocity * StrikeWeight * ImpulseScale;
				
				SomnusCtx->SetHitImpulse(Impulse);
				
				const FVector Start = HitResult.ImpactPoint;
				const FVector End = Start + StrikeVelocity * 0.1f;   
				DrawDebugDirectionalArrow(GetWorld(), Start, End, 20.f, FColor::Red, false, 2.f, 0, 1.5f);
			}
		}
		
		TargetASC->ApplyGameplayEffectSpecToSelf(*DealDamageSpecHandle.Data.Get());
	}
	
}
