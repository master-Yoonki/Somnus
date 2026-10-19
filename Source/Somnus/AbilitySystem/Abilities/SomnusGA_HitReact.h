// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SomnusGameplayAbility.h"
#include "Curves/CurveFloat.h"
#include "SomnusGA_HitReact.generated.h"

class UAnimMontage;

/** Where on the body a strike landed, as far as choosing a reaction cares. */
UENUM(BlueprintType)
enum class ESomnusHitRegion : uint8
{
	Torso,
	Head,
	LeftArm,
	RightArm,
	LeftLeg,
	RightLeg
};

/** The authored reactions for one region. A TMap value cannot itself be an array, hence the wrapper. */
USTRUCT(BlueprintType)
struct FSomnusHitReactMontages
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "HitReact")
	TArray<TObjectPtr<UAnimMontage>> Montages;
};

/**
 * How a body answers a strike. The attacker only says how hard it landed - the impulse on the
 * effect context - and everything about the answer is decided here, by the body that was struck:
 * how long it staggers, how far it is shoved, and whether a strong enough blow from the front to a
 * limb or the head plays an authored reaction instead of the physical one.
 *
 * Each body type tunes this in its own blueprint subclass. The player's leaves the curves and the
 * montage table empty, so it only ever reacts physically and never loses control.
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
	UPROPERTY(EditDefaultsOnly)
	float TestImpulseAmount = 4500.f;

	/** Seconds of stagger for an impulse of a given size. Empty means no stagger. */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Stagger")
	FRuntimeFloatCurve StaggerDurationByImpulse;

	/** Shove strength for an impulse of a given size. Empty means no shove. */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Knockback")
	FRuntimeFloatCurve KnockbackStrengthByImpulse;

	/** Seconds the shove is applied over. */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Knockback", meta = (ClampMin = "0.0", Units = "Seconds"))
	float KnockbackDuration = 0.2f;

	/** Impulse size at or above which a blow from the front to a limb or the head plays the
	 *  region's authored reaction instead of the physical one. */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Montage", meta = (ClampMin = "0.0"))
	float MontageImpulseThreshold = 0.f;

	/** Half-width of the arc in front of the body that counts as a blow from the front. */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Montage", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
	float FrontConeHalfAngle = 60.f;

	/** The bone each region starts at. A hit bone belongs to the first of these found walking up
	 *  its parents, so a hand or a foot lands in its limb; one that reaches the root finds none and
	 *  counts as the torso. */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Montage")
	TMap<ESomnusHitRegion, FName> RegionRootBones;

	/** Authored reactions per region; one is picked at random. A region left out reacts physically. */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Montage")
	TMap<ESomnusHitRegion, FSomnusHitReactMontages> RegionMontages;

	/** Which region a hit bone belongs to. See RegionRootBones. */
	ESomnusHitRegion ResolveHitRegion(const USkeletalMeshComponent* Mesh, FName HitBone) const;

	/** Whether the blow came in through the front arc of the struck body: the strike travels from
	 *  the front toward the back, so it points against the body's facing. Judged by the direction
	 *  of the force rather than where the attacker stood, so the authored reaction - recoiling
	 *  backward - and the shove agree even for a sideways swing thrown from in front.
	 *  StrikeDirection is flat and unit length, the same one the knockback pushes along. */
	bool IsHitFromFront(const AActor* StruckActor, const FVector& StrikeDirection) const;

	/** One of the region's reactions at random, or null when the region has none. */
	UAnimMontage* PickMontage(ESomnusHitRegion Region) const;

	/** Reads a curve at an impulse size, 0 when the curve has no keys. */
	static float EvaluateByImpulse(const FRuntimeFloatCurve& Curve, float ImpulseSize);

	/** Where the stagger ends, whichever way it was played - montage done, delay elapsed, or
	 *  the montage cut short. */
	UFUNCTION()
	void OnReactionFinished();
};
