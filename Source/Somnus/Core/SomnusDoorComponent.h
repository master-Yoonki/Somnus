// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SomnusDoorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSomnusDoorStateChanged, bool, bOpen);

/**
 * Whether a door is open, held where every machine can agree on it. Interaction is answered by
 * the server alone, so a door that plays its own animation on being interacted with plays it only
 * there; this replaces that flip with replicated state and an event that fires wherever the state
 * lands, which is the same event on the server.
 *
 * Deliberately knows nothing about how a door moves. Sliding, swinging and shuttering are the
 * owner's business - a Blueprint drives its own timeline from OnDoorStateChanged - and a component
 * is used rather than a base class because these doors are bought content whose component
 * hierarchy is not ours to rearrange.
 */
UCLASS(ClassGroup=(Somnus), meta=(BlueprintSpawnableComponent))
class SOMNUS_API USomnusDoorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USomnusDoorComponent();

	/** Server only, and safe to call from anywhere: a client's request is simply ignored, because
	 *  interaction already arrives on the server and nothing else should be opening doors. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void Toggle();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetOpen(bool bNewOpen);

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const { return bOpen; }

	/** Fired on every machine when the door changes, and on join for a door already open. Drive
	 *  the timeline from here rather than from the interaction. */
	UPROPERTY(BlueprintAssignable, Category = "Door")
	FSomnusDoorStateChanged OnDoorStateChanged;

	/** True once the door has been opened or closed at least once. A Blueprint can use it to tell
	 *  a replicated catch-up from a deliberate one and skip the animation for the former. */
	UFUNCTION(BlueprintPure, Category = "Door")
	bool HasEverChanged() const { return bHasEverChanged; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_Open, VisibleInstanceOnly, BlueprintReadOnly, Category = "Door")
	bool bOpen = false;

	UFUNCTION()
	void OnRep_Open();

private:
	bool bHasEverChanged = false;
};
