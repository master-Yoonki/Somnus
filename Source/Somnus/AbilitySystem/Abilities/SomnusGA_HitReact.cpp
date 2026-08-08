// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_HitReact.h"

#include "AbilitySystem/SomnusGameplayEffectContext.h"
#include "Character/SomnusHitReactComponent.h"
#include "Core/SomnusGameplayTags.h"

USomnusGA_HitReact::USomnusGA_HitReact()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = SomnusTags::Event_HitReact;
	AbilityTriggers.Add(TriggerData);
	
	SetAssetTags(FGameplayTagContainer(SomnusTags::Ability_HitReact));
	SourceBlockedTags.AddTag(SomnusTags::State_Dead);
	
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	
}

void USomnusGA_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	USomnusHitReactComponent* HitComponent = ActorInfo->AvatarActor->GetComponentByClass<USomnusHitReactComponent>();
	if (!HitComponent || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	const FHitResult* HitResult = TriggerEventData->ContextHandle.GetHitResult();
	if (!HitResult)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	const FName HitBone = HitResult->BoneName;
	
	const FVector HitLocation = HitResult->ImpactPoint;
	
	const FSomnusGameplayEffectContext* SomnusCtx =
		static_cast<const FSomnusGameplayEffectContext*>(TriggerEventData->ContextHandle.Get());
	const FVector HitImpulse = SomnusCtx->GetHitImpulse();
	
	HitComponent->HitReaction(HitBone, HitLocation, HitImpulse);
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
