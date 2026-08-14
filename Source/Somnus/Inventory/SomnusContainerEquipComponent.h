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

	/** Puts an already-formed container instance into the slot its data asset declares. Granting
	 *  rather than equipping: the instance has just been minted and is in no grid yet, which is
	 *  why this is not a move like every other way something reaches a slot. Returns false and
	 *  changes nothing when the item is not a container, declares Pockets, or the slot refuses
	 *  it. Server only. */
	bool EquipInstance(const FSomnusItemInstance& Instance);

	// Nothing here equips or unequips any more. A worn slot is a grid, so a drag onto one or off
	// one is the same cross-grid move as any other, and it goes through USomnusLootComponent's
	// Server_MoveItem like the rest - which also means it is validated like the rest, where the
	// pair of RPCs that used to live here took a client's word for the grid it named.


	/** Takes an item away from this character and leaves it in the world in front of them,
	 *  contents and all. Covers both places an item can be - loose in one of the grids, or worn
	 *  in the rig or backpack slot. Returns false and changes nothing when nothing this
	 *  character holds answers to InstanceID, or when the pickup could not be spawned.
	 *  Server only. */
	bool DropItem(FGuid InstanceID);

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

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	class USomnusEquipmentSlotComponent* GetSlot(EContainerSlotType SlotType) const;
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	class ASomnusContainerActor* GetEquippedContainer(EContainerSlotType SlotType) const;
	
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
	
	/** What is worn now lives in these rather than in a replicated FSomnusItemInstance of its own.
	 *  Constructor subobjects rather than anything replicated: both machines build their own at the
	 *  same path, so the pointer is valid everywhere from construction and there is nothing for an
	 *  OnRep to announce - the contents replicate through the grid each one already is. Pocket next
	 *  door is spawned at runtime from a data asset, which is the only reason it needs telling. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<class USomnusEquipmentSlotComponent> RigSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<class USomnusEquipmentSlotComponent> BackpackSlot;

	UFUNCTION()
	void OnRep_Pocket();

	/** Bound to both slots' add and remove delegates, on every machine. Replaces the hand-called
	 *  notify the replicated slots needed: a grid tells its listeners itself, and it tells them on
	 *  the authority too, so the listen server host stops being the one window that never
	 *  refreshed. Teardown for equipment leaving a slot hooks up here. */
	UFUNCTION()
	void HandleSlotContentsChanged(const FSomnusItemInstance& Item);

	// No writable counterpart to GetEquippedInstance any more. What is worn is an entry in a
	// replicated grid now, so it is written the way every other item is - through the slot's own
	// Add/Move/Remove, which mark the list dirty and notify. A raw pointer into the array would
	// skip both.
};
