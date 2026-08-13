// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SomnusInteractorComponent.generated.h"

/**
 * The half of interaction that does the reaching - counterpart to ISomnusInteractable, which is
 * the half that can be reached. Owns the one definition of what its owner is pointing at, so the
 * server's interaction and the local highlight cannot come to disagree about range.
 */
UCLASS( ClassGroup=(Somnus), meta=(BlueprintSpawnableComponent) )
class SOMNUS_API USomnusInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USomnusInteractorComponent();

	/** What the owner is pointing at, if anything. Callable on any machine - the trace itself is
	 *  cheap and side-effect free; only the focus tracking below is a local concern. */
	bool TraceForInteractable(FHitResult& OutHit) const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float TraceLength = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float TraceRadius = 20.f;

private:
	/** The only place focus changes hands, so the two actors involved are always told in the
	 *  right order and exactly once. Passing nullptr means looking at nothing. */
	void SetFocus(AActor* NewFocus);

	// Weak, because loot can be taken by someone else while it sits under the crosshair. A
	// destroyed actor needs no un-highlighting, only to be forgotten.
	TWeakObjectPtr<AActor> FocusedActor;
};
