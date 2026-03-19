// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_Aim.h"
#include "Core/SomnusGameplayTags.h"

USomnusGA_Aim::USomnusGA_Aim()
{
	AbilityTags.AddTag(SomnusTags::Ability_Aim);
	ActivationRequiredTags.AddTag(SomnusTags::Weapon_Enable_Aim);

	// State.Aiming is automatically added to the ASC while this ability is active
	// and removed when the ability ends
	ActivationOwnedTags.AddTag(SomnusTags::State_Aiming);
}

void USomnusGA_Aim::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Ability stays active until externally cancelled (character's HoldRelease binding
	// calls CancelAbilities on input release)
}
