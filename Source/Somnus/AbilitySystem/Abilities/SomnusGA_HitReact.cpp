// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_HitReact.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
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
	
	HitComponent->HitReaction(HitBone, HitLocation, HitImpulse);
	
	const float ImpulseSize = static_cast<float>(HitImpulse.Size());
	const float StaggerDuration = EvaluateByImpulse(StaggerDurationByImpulse, ImpulseSize);
	const float KnockbackStrength = EvaluateByImpulse(KnockbackStrengthByImpulse, ImpulseSize);

	// A body whose curves are empty only flinches physically, and that needs nothing kept alive.
	if (StaggerDuration <= 0.f && KnockbackStrength <= 0.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	bool bKnockedBack = false;
	if (KnockbackStrength > 0.f)
	{
		// Along the strike, flat - height in it would lift the body off the floor or drive it in.
		// A swing tracked with no sideways speed, such as a straight chop down, gives no line, so the
		// shove falls back to straight away from whoever struck.
		FVector KnockbackDirection = HitImpulse.GetSafeNormal2D();
		if (KnockbackDirection.IsNearlyZero())
		{
			const AActor* Attacker = TriggerEventData->ContextHandle.GetEffectCauser();
			const AActor* Struck = GetAvatarActorFromActorInfo();
			if (Attacker && Struck)
			{
				KnockbackDirection = (Struck->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D();
			}
		}

		// No line at all only costs the shove. The stagger still runs, so this path must not return.
		if (!KnockbackDirection.IsNearlyZero())
		{
			UAbilityTask_ApplyRootMotionConstantForce* Knockback =
				UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
					this, NAME_None,
					KnockbackDirection, KnockbackStrength, KnockbackDuration,
					/*bIsAdditive*/ false,
					/*StrengthOverTime*/ nullptr,
					ERootMotionFinishVelocityMode::SetVelocity,
					/*SetVelocityOnFinish*/ FVector::ZeroVector,
					/*ClampVelocityOnFinish*/ 0.f,
					/*bEnableGravity*/ true);
			Knockback->ReadyForActivation();
			bKnockedBack = true;
		}
	}

	// One timer decides when this ends, long enough to cover both the stagger and the shove. Ending
	// from each of them separately would let whichever finished first cut the other short, since
	// ending the ability ends every task it is still running.
	const float ReactionDuration = FMath::Max(StaggerDuration, bKnockedBack ? KnockbackDuration : 0.f);
	UAbilityTask_WaitDelay* WaitReaction = UAbilityTask_WaitDelay::WaitDelay(this, ReactionDuration);
	WaitReaction->OnFinish.AddDynamic(this, &USomnusGA_HitReact::OnReactionFinished);
	WaitReaction->ReadyForActivation();
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
	// Zero for an empty curve is the answer wanted here, not a fallback: no curve means this body
	// does not stagger or get shoved, which is how a body type opts out.
	const FRichCurve* RichCurve = Curve.GetRichCurveConst();
	if (!RichCurve || RichCurve->GetNumKeys() == 0) return 0.f;

	return RichCurve->Eval(ImpulseSize);
}

void USomnusGA_HitReact::OnReactionFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
