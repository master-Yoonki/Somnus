// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SomnusHitTraceTestCharacter.generated.h"

class UCameraComponent;

// Minimal test-only first-person character: WASD to move, mouse to look, left-click
// to fire a trace from the camera forward. Skips damage/GAS entirely and calls
// USomnusHitReactComponent::HitReaction() directly on whatever bone it hits - this
// is for iterating on hit-react physics only, not the damage pipeline.
UCLASS()
class SOMNUS_API ASomnusHitTraceTestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASomnusHitTraceTestCharacter();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "Hit Trace Test")
	float ImpulseStrength = 50000.f;

	UPROPERTY(EditAnywhere, Category = "Hit Trace Test")
	float TraceRange = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Hit Trace Test")
	float TraceRadius = 5.f;

	UPROPERTY(EditAnywhere, Category = "Hit Trace Test")
	float FireCooldown = 0.25f;

	void MoveForwardPositive(float Value);
	void MoveForwardNegative(float Value);
	void MoveRightPositive(float Value);
	void MoveRightNegative(float Value);
	void AddLookPitchInput(float Value);
	void Fire();
	void ResetFireGate();

	bool bCanFire = true;
	FTimerHandle FireCooldownTimer;
};
