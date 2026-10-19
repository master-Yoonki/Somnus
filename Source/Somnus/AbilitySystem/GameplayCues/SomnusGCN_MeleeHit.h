// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "SomnusGCN_MeleeHit.generated.h"

class UCameraShakeBase;
class UFXSystemAsset;
class USoundBase;

/**
 * Everything a melee strike looks, sounds and feels like, fired once per hit on every machine by
 * the damage effect's GameplayCue.Hit.Melee. The strike itself travels on the effect context: the
 * hit result for where and which way, the instigator for who swung.
 *
 * Blood and sound play for everyone. The hit stop freezes both bodies' animation for a moment on
 * each machine that shows them - animation only, since world time is shared in multiplayer. The
 * camera shake is the swinger's alone.
 *
 * Assets and the cue tag are set in a blueprint subclass, which is also what makes the cue manager
 * find it. Name it after its tag (GC_Hit_Melee for GameplayCue.Hit.Melee) and the editor fills the
 * tag in from the name on its own.
 */
UCLASS()
class SOMNUS_API USomnusGCN_MeleeHit : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	USomnusGCN_MeleeHit();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	/** Spawned at the impact point, facing out along the hit surface. Either a Niagara system or a
	 *  Cascade particle system - the common base takes both, so swapping one for the other is a
	 *  blueprint change rather than a code one. */
	UPROPERTY(EditDefaultsOnly, Category = "Blood")
	TObjectPtr<UFXSystemAsset> BloodEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> HitSound;

	/** Played on the swinger's camera only. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraShakeScale = 1.f;

	/** How long both bodies' animation is held. */
	UPROPERTY(EditDefaultsOnly, Category = "HitStop", meta = (ClampMin = "0.0", Units = "Seconds"))
	float HitStopDuration = 0.08f;

	/** Animation rate while held. Near zero reads as a freeze, higher as a stutter. */
	UPROPERTY(EditDefaultsOnly, Category = "HitStop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitStopRateScale = 0.05f;
};
