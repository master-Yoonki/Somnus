// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_MeleeAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

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

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, MontagePlayRate);

	MontageTask->OnCompleted.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &USomnusGA_MeleeAttack::OnMontageCancelled);

	MontageTask->ReadyForActivation();
}

void USomnusGA_MeleeAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USomnusGA_MeleeAttack::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
