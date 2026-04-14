// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SomnusGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/SomnusGE_StaminaCost.h"
#include "AbilitySystem/Effects/SomnusGE_Cooldown.h"
#include "Core/SomnusGameplayTags.h"

USomnusGameplayAbility::USomnusGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Block all abilities while dead
	ActivationBlockedTags.AddTag(SomnusTags::State_Dead);

	// Shared cost GE — MMC inside reads StaminaCost from the ability instance.
	// Abilities with StaminaCost == 0 will pass CheckCost automatically (modifier evaluates to 0).
	CostGameplayEffectClass = USomnusGE_StaminaCost::StaticClass();

	// Cooldown GE is NOT assigned to CooldownGameplayEffectClass — the engine validates
	// that the class has statically-granted tags, but we apply tags dynamically per-ability
	// in ApplyCooldown(). Leaving this null avoids the editor validation warning.
}

void USomnusGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (bActivateOnGranted && ActorInfo && !Spec.IsActive())
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

const FGameplayTagContainer* USomnusGameplayAbility::GetCooldownTags() const
{
	TempCooldownTags.Reset();
	TempCooldownTags.AppendTags(CooldownTags);
	const FGameplayTagContainer* ParentTags = Super::GetCooldownTags();
	if (ParentTags)
	{
		TempCooldownTags.AppendTags(*ParentTags);
	}
	return &TempCooldownTags;
}

void USomnusGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownDuration <= 0.0f || CooldownTags.IsEmpty())
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(USomnusGE_Cooldown::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SomnusTags::Data_CooldownDuration, CooldownDuration);
		SpecHandle.Data->DynamicGrantedTags.AppendTags(CooldownTags);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}
