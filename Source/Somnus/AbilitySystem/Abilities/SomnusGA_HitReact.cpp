// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_HitReact.h"

#include "AbilitySystem/SomnusGameplayEffectContext.h"
#include "Character/SomnusCharacterBase.h"
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

	// One instance per body, restarted by a fresh hit so a strike landing mid-stagger extends it
	// instead of being dropped.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;

	// Held for as long as the ability runs, which is as long as the stagger lasts. A zombie's own
	// swing is cut off by it; the player has no ability with that tag, so nothing of theirs is.
	ActivationOwnedTags.AddTag(SomnusTags::State_Staggered);
	CancelAbilitiesWithTag.AddTag(SomnusTags::Ability_Zombie_Melee);

	RegionRootBones.Add(ESomnusHitRegion::Head, TEXT("head"));
	RegionRootBones.Add(ESomnusHitRegion::LeftArm, TEXT("upperarm_l"));
	RegionRootBones.Add(ESomnusHitRegion::RightArm, TEXT("upperarm_r"));
	RegionRootBones.Add(ESomnusHitRegion::LeftLeg, TEXT("thigh_l"));
	RegionRootBones.Add(ESomnusHitRegion::RightLeg, TEXT("thigh_r"));
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
	
	// Debug
	// The struck body is the one running this ability. The striking body is the context's effect
	// causer, not its instigator: the instigator is whoever owns the attacker's ability system,
	// which for a player is the PlayerState.
	// ASomnusCharacterBase* SourceCharacter = Cast<ASomnusCharacterBase>(TriggerEventData->ContextHandle.GetEffectCauser());
	// ASomnusCharacterBase* HitCharacter = Cast<ASomnusCharacterBase>(GetAvatarActorFromActorInfo());
	//
	// SourceCharacter->ApplyHitStop(0.5f, 0.05f);
	// HitCharacter->ApplyHitStop(0.5f, 0.05f);
	
	HitComponent->HitReaction(HitBone, HitLocation, HitImpulse);
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

ESomnusHitRegion USomnusGA_HitReact::ResolveHitRegion(const USkeletalMeshComponent* Mesh, FName HitBone) const
{
	return ESomnusHitRegion::Torso;
}

bool USomnusGA_HitReact::IsHitFromFront(const AActor* StruckActor, const AActor* Instigator) const
{
	return false;
}

UAnimMontage* USomnusGA_HitReact::PickMontage(ESomnusHitRegion Region) const
{
	return nullptr;
}

float USomnusGA_HitReact::EvaluateByImpulse(const FRuntimeFloatCurve& Curve, float ImpulseSize)
{
	return 0.f;
}

void USomnusGA_HitReact::OnReactionFinished()
{
}
