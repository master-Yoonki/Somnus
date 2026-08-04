// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SomnusItemTypes.h"
#include "Components/ActorComponent.h"
#include "Inventory/SomnusItemInstance.h"
#include "SomnusContainerEquipComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActiveContainersChanged);

class USomnusContainerDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SOMNUS_API USomnusContainerEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPROPERTY(BlueprintAssignable, Category = "Container|Events")
	FOnActiveContainersChanged OnActiveContainersChangedDelegate;
	
	// Sets default values for this component's properties
	USomnusContainerEquipComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FSomnusActiveContainerInfo> GetActiveContainers() const;

	/** Finds which container this character can reach holds InstanceID, and copies the instance
	 *  out. Null when nothing reachable holds it - including whatever is worn, which lives in a
	 *  slot rather than in a grid, so a caller that means to cover equipment has to ask
	 *  GetEquippedInstance separately. The copy is deliberate: callers that go on to remove or
	 *  move the item would be left holding a pointer into an array they just resized. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	USomnusInventoryComponent* FindContainerHolding(FGuid InstanceID, FSomnusItemInstance& OutInstance) const;

	/** Puts an already-formed container instance into the slot its data asset declares.
	 *  Returns false and changes nothing when the item is not a container, declares
	 *  Pockets, or the target slot is already occupied. Server only. */
	bool EquipInstance(const FSomnusItemInstance& Instance);
	
	/** Pulls a container item out of a grid and wears it, contents and all. TargetSlot is the
	 *  slot the player dropped on, and a mismatch with the item's own declared slot is a
	 *  refusal - EquipInstance would otherwise route a backpack dropped on the rig panel into
	 *  the backpack slot. Returns false and changes nothing on any refusal. Server only. */
	bool EquipFrom(USomnusInventoryComponent* Source, FGuid InstanceID, EContainerSlotType TargetSlot);

	/** Server RPC for a drag that started on a grid and ended on an equipped slot. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment|RPC")
	void Server_EquipFrom(USomnusInventoryComponent* Source, FGuid InstanceID, EContainerSlotType TargetSlot);

	/** Takes what is worn in SlotType and drops it into a grid at a specific cell, contents
	 *  and all. Returns false and changes nothing when the slot is empty, cannot be worn, or
	 *  the destination refuses the placement - including the destination being storage the
	 *  item itself provides. Public because server-side callers other than the drag RPC need
	 *  it: dropping equipment on death is the next one. Server only. */
	bool UnequipTo(EContainerSlotType SlotType, USomnusInventoryComponent* Destination,
				   int32 TopLeftX, int32 TopLeftY, bool bRotated);

	/** Server RPC for a drag that started on an equipped slot and ended on a grid cell. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment|RPC")
	void Server_UnequipTo(EContainerSlotType SlotType, USomnusInventoryComponent* Destination,
						  int32 TopLeftX, int32 TopLeftY, bool bRotated);
	
	/** Takes an item away from this character and leaves it in the world in front of them,
	 *  contents and all. Covers both places an item can be - loose in one of the grids, or worn
	 *  in the rig or backpack slot. Returns false and changes nothing when nothing this
	 *  character holds answers to InstanceID, or when the pickup could not be spawned.
	 *  Server only. */
	bool DropItem(FGuid InstanceID);

	/** Server RPC for a drop the player asked for from the UI. Discards the result, like every
	 *  other inventory RPC: a client learns the outcome from the item leaving its grid, which
	 *  replication delivers on its own. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment|RPC")
	void Server_DropItem(FGuid InstanceID);

	/** Mints a new item and spreads it across every container this character can reach, in
	 *  GetActiveContainers() order, merging into existing stacks everywhere before opening
	 *  any new cell. Returns the quantity that found no room. Server only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	int32 TryAddItemAnywhere(USomnusItemDataAsset* ItemDataAsset, int32 Quantity);

	/** Existing-instance counterpart. Consumes from ItemInstance in place and returns what is
	 *  left of it - on a non-zero return the caller still owns the remainder, and its
	 *  ContainerActor, and has to decide what happens to it. Server only. */
	int32 TryAddExistingItemAnywhere(FSomnusItemInstance& ItemInstance);
	
	/** What is worn in SlotType, or a default-constructed instance when nothing is - test
	 *  InstanceID.IsValid() to tell the two apart, never StackCount, which defaults to 1.
	 *  Pockets are not a worn slot and always read as empty. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FSomnusItemInstance GetEquippedInstance(EContainerSlotType SlotType) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<class ASomnusPickupActor> PickupActorClass;

	/** How far in front of the character a dropped item appears. Far enough that it does not
	 *  spawn inside the capsule and get shoved out sideways; near enough to still be in reach. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment", meta = (ClampMin = "0.0"))
	float DropDistance = 120.f;

	/** Radius of the circle a drop is scattered within. Emptying a container otherwise stacks
	 *  every item on one spot, where the bodies interpenetrate and shove each other apart. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment", meta = (ClampMin = "0.0"))
	float DropScatterRadius = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TObjectPtr<USomnusContainerDataAsset> PocketData;

	/** Containers worn from the start. Routed by each asset's own SlotType.
	 *  Leave empty to spawn with nothing worn. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<TObjectPtr<USomnusContainerDataAsset>> DefaultEquipment;

	/** Loose items granted at spawn, placed wherever they fit across everything the character
	 *  can reach by then - pockets plus whatever DefaultEquipment granted. A quantity above
	 *  the item's MaxStackCount splits into as many stacks as it needs. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TMap<TObjectPtr<USomnusItemDataAsset>, int32> DefaultItems;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Pocket)
	TObjectPtr<ASomnusContainerActor> Pocket;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedRig)
	FSomnusItemInstance EquippedRig;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedBackpack)
	FSomnusItemInstance EquippedBackpack;
	
	UFUNCTION()
	void OnRep_Pocket();
	
	UFUNCTION()
	void OnRep_EquippedRig(FSomnusItemInstance Old);

	UFUNCTION()
	void OnRep_EquippedBackpack(FSomnusItemInstance Old);
	
	/** Reaction to an equipped slot changing. Driven by the OnReps on clients, and called
	 *  directly on the server, which never receives them. */
	void HandleEquippedChanged(EContainerSlotType SlotType, const FSomnusItemInstance& OldInstance);

	/** Writable counterpart of GetEquippedInstance, for the paths that change what is worn.
	 *  Null for a slot that cannot be worn. Protected on purpose: it hands out a pointer
	 *  straight into a replicated member, and writing through it without going on to call
	 *  HandleEquippedChanged leaves every listener looking at the previous state. */
	FSomnusItemInstance* GetEquippedInstanceMutable(EContainerSlotType SlotType);
};
