// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/SomnusMovementTypes.h"
#include "SomnusItemAnimLayers.generated.h"

class USomnusCharacterAnimInstance;

/**
 * Base for the anim layers an equipped item links into the character's animation. A linked layer
 * runs as its own anim instance, so it copies the few values it needs from the main instance
 * instead of deriving from it - deriving would rerun the main instance's movement analysis once
 * per linked layer.
 */
UCLASS(Abstract)
class SOMNUS_API USomnusItemAnimLayers : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "ItemLayers", meta = (BlueprintThreadSafe))
	USomnusCharacterAnimInstance* GetMainAnimInstance() const { return MainAnimInstance; }

	UPROPERTY(Transient)
	TObjectPtr<USomnusCharacterAnimInstance> MainAnimInstance;

	UPROPERTY(BlueprintReadOnly, Category = "ItemLayers")
	ESomnusGait Gait = ESomnusGait::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "ItemLayers")
	ESomnusMovementState MovementState = ESomnusMovementState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "ItemLayers")
	float Speed2D = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "ItemLayers")
	bool bIsAiming = false;

	/** Counter-rotate stance clips by the negative of this so the weapon keeps pointing at the aim. */
	UPROPERTY(BlueprintReadOnly, Category = "ItemLayers")
	float AimStanceYaw = 0.f;
};
