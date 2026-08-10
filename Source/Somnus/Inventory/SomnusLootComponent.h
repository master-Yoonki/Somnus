// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SomnusLootComponent.generated.h"

/** Broadcast when the body being searched changes, in either direction - opened, closed, or the
 *  body itself going away. Listeners are expected to ask what it changed to rather than assume. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSomnusLootTargetChangedSignature);

UCLASS( ClassGroup=(Somnus), meta=(BlueprintSpawnableComponent) )
class SOMNUS_API USomnusLootComponent : public UActorComponent
{
	GENERATED_BODY()
public:	
	USomnusLootComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server. Opens a body for searching. */
	void OpenLoot(AActor* Body);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loot")
	void Server_CloseLoot();

	UFUNCTION(BlueprintPure, Category = "Loot")
	AActor* GetLootTarget() const { return LootTarget; }

	/** Whether the owner may take from Container right now. */
	bool CanAccessContainer(const class USomnusInventoryComponent* Container) const;

	/** Fires on the owning client, and on a listen server host for its own character. Assignable
	 *  rather than implementable because this component is a C++ subobject: there is no Blueprint
	 *  subclass of it for an event to be implemented in, and the listener that cares is the
	 *  inventory widget, which is somewhere else entirely. */
	UPROPERTY(BlueprintAssignable, Category = "Loot")
	FSomnusLootTargetChangedSignature OnLootTargetChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_LootTarget)
	TObjectPtr<AActor> LootTarget;

	UFUNCTION()
	void OnRep_LootTarget();

	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float LootRange = 400.0f;
};
