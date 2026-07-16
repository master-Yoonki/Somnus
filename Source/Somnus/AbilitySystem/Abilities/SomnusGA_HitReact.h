// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
#include "SomnusGA_HitReact.generated.h"

/**
 * Triggered by Event.HitReact on any non-lethal damage that carried a HitResult.
 * Resolves the impulse (direction and strength) and hands it to the avatar's
 * USomnusHitReactComponent, which owns the presentation on every machine.
 *
 * Granted to both the player and zombies; nothing here is character-specific.
 */
UCLASS()
class SOMNUS_API USomnusGA_HitReact : public USomnusGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGA_HitReact();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// Impulse magnitude per point of damage.
	UPROPERTY(EditDefaultsOnly, Category = "HitReact", meta = (ClampMin = "0.0"))
	float ImpulsePerDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "HitReact", meta = (ClampMin = "0.0"))
	float MinImpulse = 200.0f;

	// Keeps a single large hit from launching the character. For reference, the whole-body
	// death ragdoll impulse is 1500.
	UPROPERTY(EditDefaultsOnly, Category = "HitReact", meta = (ClampMin = "0.0"))
	float MaxImpulse = 800.0f;

private:
	// Direction the weapon was traveling. Prefers the surface normal, but the zombie melee
	// trace sweeps zero length, so an initial overlap can hand back a depenetration normal
	// instead — fall back to instigator-to-impact in that case.
	static FVector ResolveImpulseDirection(const FHitResult& Hit, const AActor* Instigator);
};
