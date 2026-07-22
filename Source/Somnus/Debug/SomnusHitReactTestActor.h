// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SomnusHitReactTestActor.generated.h"

class UBillboardComponent;

// Level-placeable debug actor for iterating on USomnusHitReactComponent without
// needing a full combat swing to land. Select the placed instance in the
// Outliner (works during PIE too) and click "Trigger Hit" in the Details panel.
UCLASS()
class SOMNUS_API ASomnusHitReactTestActor : public AActor
{
	GENERATED_BODY()

public:
	ASomnusHitReactTestActor();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> Billboard;

	// Drag the target character (player or zombie placed in the level) in here.
	UPROPERTY(EditInstanceOnly, Category = "Hit React Test")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, Category = "Hit React Test")
	FName BoneName = "spine_02";

	UPROPERTY(EditAnywhere, Category = "Hit React Test")
	FVector ImpulseDirection = FVector(1.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Hit React Test")
	float ImpulseStrength = 50000.f;

	UFUNCTION(CallInEditor, Category = "Hit React Test")
	void TriggerHit();
};
