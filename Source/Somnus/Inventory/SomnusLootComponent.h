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

	/** The question every reach across bodies actually asks: its owner's own storage always, and
	 *  the one body it has open for as long as it stays beside it. Everything else - grids, worn
	 *  storage, whatever comes later - reduces to this, so the rule is written down once here. */
	bool CanAccessHolder(const AActor* Holder) const;

	/** Whether the owner may take from Container right now. */
	bool CanAccessContainer(const class USomnusInventoryComponent* Container) const;

	/** Fires on the owning client, and on a listen server host for its own character. Assignable
	 *  rather than implementable because this component is a C++ subobject: there is no Blueprint
	 *  subclass of it for an event to be implemented in, and the listener that cares is the
	 *  inventory widget, which is somewhere else entirely. */
	UPROPERTY(BlueprintAssignable, Category = "Loot")
	FSomnusLootTargetChangedSignature OnLootTargetChanged;
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loot")
	void Server_MoveItem(
		USomnusInventoryComponent* Source, 
		USomnusInventoryComponent* Dest, 
		FGuid InstanceID, 
		int32 TopLeftX, 
		int32 TopLeftY, 
		bool bRotated);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loot")
	void Server_DropFrom(class USomnusContainerEquipComponent* Source, FGuid InstanceID);
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_LootTarget)
	TObjectPtr<AActor> LootTarget;

	UFUNCTION()
	void OnRep_LootTarget();

	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float LootRange = 400.0f;

private:
	/** Measured again on every request rather than trusted from the moment the body was opened:
	 *  a panel left up while its owner walks away must stop being a way in. */
	bool IsWithinReach(const AActor* Body) const;

	/** The only writer. Announces a change and nothing else, so the authority - which never gets
	 *  an OnRep of its own - stays in step with what every other machine was told. */
	void SetLootTarget(AActor* Body);
};
