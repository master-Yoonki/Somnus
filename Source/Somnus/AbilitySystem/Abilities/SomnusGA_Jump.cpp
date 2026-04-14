// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_Jump.h"
#include "GameFramework/Character.h"
#include "Core/SomnusGameplayTags.h"

USomnusGA_Jump::USomnusGA_Jump()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(SomnusTags::Ability_Jump);
	SetAssetTags(Tags);

	CooldownTags.AddTag(SomnusTags::Cooldown_Ability_Jump);

	// Block re-activation while already jumping
	ActivationBlockedTags.AddTag(SomnusTags::Ability_Jump);
}

bool USomnusGA_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	return Character && Character->CanJump();
}

void USomnusGA_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->Jump();

	// Listen for landing to end the ability
	Character->LandedDelegate.AddDynamic(this, &USomnusGA_Jump::OnLanded);
}

void USomnusGA_Jump::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// Variable jump height — releasing early cuts the jump short
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		Character->StopJumping();
	}
}

void USomnusGA_Jump::OnLanded(const FHitResult& Hit)
{
	// Unbind before ending to avoid stale delegates
	if (ACharacter* Character = Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		Character->LandedDelegate.RemoveDynamic(this, &USomnusGA_Jump::OnLanded);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
