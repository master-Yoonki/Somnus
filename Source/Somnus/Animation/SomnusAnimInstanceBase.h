// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SomnusAnimInstanceBase.generated.h"

class ASomnusCharacterBase;
class UCharacterMovementComponent;

/**
 * What every character's animation reads, whatever else it does with it. Kept to the values that
 * mean the same thing on every body - a player and a zombie agree on how fast they are going and on
 * whether they are dead, and on nothing much else.
 *
 * Whether a character is moving is deliberately not here. For the player it is the character's
 * intent, which motion matching needs before the body has picked up any speed; for a zombie it is
 * simply having some. One name for both would be right for one of them.
 */
UCLASS(Abstract)
class SOMNUS_API USomnusAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	float GetSpeed2D() const { return Speed2D; }

protected:
	/** Null in an asset preview, where there is no character to animate. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ASomnusCharacterBase> OwningCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	/** Horizontal speed, taken on the worker update from the movement component. */
	UPROPERTY(BlueprintReadOnly, Category = "EssentialValues")
	float Speed2D = 0.f;

	/** Read from the character rather than its ability system: the character's flag replicates,
	 *  and a player corpse has no ability system left to ask. */
	UPROPERTY(BlueprintReadOnly, Category = "States")
	bool bIsDead = false;
};
