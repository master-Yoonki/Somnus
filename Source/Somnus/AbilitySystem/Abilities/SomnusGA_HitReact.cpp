// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_HitReact.h"

#include "Character/SomnusHitReactComponent.h"
#include "Core/SomnusGameplayTags.h"

USomnusGA_HitReact::USomnusGA_HitReact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// Getting hit again mid-reaction must start a new reaction, not be swallowed.
	bRetriggerInstancedAbility = true;

	SetAssetTags(FGameplayTagContainer(SomnusTags::Ability_HitReact));

	// Server-only ability, and Die() adds State.Dead on the server before the corpse ragdolls.
	// Blocking here keeps the flinch from fighting the death ragdoll for the same bodies,
	// without the component needing to know about death at all.
	ActivationBlockedTags.AddTag(SomnusTags::State_Dead);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = SomnusTags::Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

FVector USomnusGA_HitReact::ResolveImpulseDirection(const FHitResult& Hit, const AActor* Instigator)
{
	const FVector FromNormal = -Hit.ImpactNormal;
	if (!Hit.bStartPenetrating && !FromNormal.IsNearlyZero())
	{
		return FromNormal;
	}

	if (Instigator)
	{
		const FVector FromInstigator = (Hit.ImpactPoint - Instigator->GetActorLocation()).GetSafeNormal();
		if (!FromInstigator.IsNearlyZero())
		{
			return FromInstigator;
		}
	}

	return FVector::ZeroVector;
}

void USomnusGA_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = GetAvatarActorFromActorInfo();
	const FHitResult* Hit = TriggerEventData ? TriggerEventData->ContextHandle.GetHitResult() : nullptr;

	// The attribute set only raises this event for damage that carried a HitResult, so a miss
	// here means the event was sent from somewhere that has not been wired up.
	if (!ensure(Avatar && Hit))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	USomnusHitReactComponent* HitReact = Avatar->FindComponentByClass<USomnusHitReactComponent>();
	const FVector Direction = ResolveImpulseDirection(*Hit, TriggerEventData->Instigator.Get());

	if (HitReact && !Direction.IsNearlyZero())
	{
		const float Magnitude = FMath::Clamp(
			TriggerEventData->EventMagnitude * ImpulsePerDamage, MinImpulse, MaxImpulse);

		HitReact->TriggerHitReact(*Hit, Direction * Magnitude);
	}

	// Nothing latent here — the component owns the reaction's lifetime from this point.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
