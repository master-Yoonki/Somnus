// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_MeleeAttack.h"
#include "AbilitySystem/Tasks/SomnusAT_PlayMontageAndWaitForEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
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

	// Combined task: plays montage AND listens for melee hit events simultaneously
	FGameplayTagContainer EventTagFilter;
	EventTagFilter.AddTag(SomnusTags::Event_Melee_Hit);

	USomnusAT_PlayMontageAndWaitForEvent* Task = USomnusAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this, NAME_None, AttackMontage, EventTagFilter, MontagePlayRate);

	Task->OnCompleted.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCompleted);
	Task->OnBlendOut.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCancelled);
	Task->EventReceived.AddDynamic(this, &USomnusGA_MeleeAttack::OnMeleeHit);

	Task->ReadyForActivation();
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

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SomnusTags::Data_Damage, DamageAmount);
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
