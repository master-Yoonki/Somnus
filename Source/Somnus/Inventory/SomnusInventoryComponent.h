// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/SomnusItemInstance.h"
#include "SomnusInventoryComponent.generated.h"

class ASomnusContainerActor;

DECLARE_LOG_CATEGORY_EXTERN(LogSomnusInventory, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemChanged, const FSomnusItemInstance&, Item);

/**
 * Manages a grid-based inventory for the owner actor.
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class SOMNUS_API USomnusInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryItemChanged OnItemAddedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryItemChanged OnItemRemovedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryItemChanged OnItemChangedDelegate;
	
public:	
	USomnusInventoryComponent();

	virtual void InitializeComponent() override;
	
	/** Returns all items currently in the inventory (useful for UI initialization) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FSomnusItemInstance> GetAllItems() const
	{
		return InventoryList.Items;                                               
	}
	
	/** Copies out the item carrying this ID. Returns false and leaves OutItem alone when this
	 *  grid does not hold it.
	 *  A copy rather than a pointer, deliberately: this is the door other classes come in
	 *  through, and InventoryList.Items reallocates on every add and remove - including the
	 *  ones a delegate listener sets off - so a pointer held across any of those calls is
	 *  already suspect. Callers reaching in from outside want a snapshot regardless, since
	 *  they all add before they remove, which needs the old values to outlive the add. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool FindItemByID(FGuid InstanceID, FSomnusItemInstance& OutItem) const;

	/** True when Grid is one of MovingContainer's own compartments, or sits somewhere inside
	 *  a container nested within it. This is the same self-containment rule AddExistingItemAt/
	 *  AddExistingItemAnywhere refuse on their own - exposed here read-only so UI can preview
	 *  the refusal (e.g. hover feedback while dragging) before ever calling either of them. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static bool IsInsideContainer(const USomnusInventoryComponent* Grid, const ASomnusContainerActor* MovingContainer);

	/** The actor ultimately holding this grid - a character, a pickup. Not GetOwner(): containers
	 *  nest, so a grid inside a backpack answers with whoever is carrying the backpack. Ask per
	 *  use rather than caching, because that answer changes the moment the backpack changes hands. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	AActor* GetHoldingActor() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	void InitializeGridSize(int32 NewX, int32 NewY);
	
	/** Checks if a coordinate is within the grid bounds */
	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	bool IsValidCell(int32 X, int32 Y) const;

	/** Checks if an item can fit at the specified location (ShapeMask aware) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	bool CanFitAt(class USomnusItemDataAsset* ItemData, int32 TopLeftX, int32 TopLeftY, bool bRotated, FGuid IgnoreItemID = FGuid()) const;

	/** Finds the first available empty space for an item */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	bool FindFirstFit(class USomnusItemDataAsset* ItemData, int32& OutX, int32& OutY, bool& bOutRotated) const;
	
	/** Tops up stacks of this item that already exist here, without ever opening a new cell.
	 *  Returns the quantity that found no room. Split out from AddItemAnywhere so a caller
	 *  spanning several containers can merge across all of them before any of them starts
	 *  consuming free space. Server-only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	int32 MergeItemIntoStacks(class USomnusItemDataAsset* ItemData, int32 Quantity);

	/** Existing-instance counterpart of MergeItemIntoStacks. Consumes from the incoming
	 *  instance in place and returns what is left of it. Server-only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	int32 MergeExistingItemIntoStacks(FSomnusItemInstance& IncomingItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	int32 AddExistingItemAnywhere(FSomnusItemInstance& IncomingItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	int32 AddExistingItemAt(FSomnusItemInstance& IncomingItemInstance, int32 TopLeftX, int32 TopLeftY, bool bRotated);
	
	/** Attempts to add an item by finding the first available slot. 
	 * Returns the leftover quantity that failed to add (0 if fully successful). Server-only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	int32 AddItemAnywhere(class USomnusItemDataAsset* ItemData, int32 Quantity = 1);

	/** Attempts to add an item at a specific location. Returns the leftover quantity that failed to add (0 if fully successful). Server-only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	int32 AddItemAt(class USomnusItemDataAsset* ItemData, int32 Quantity, int32 TopLeftX, int32 TopLeftY, bool bRotated);

	/** Removes an item by its unique instance ID. Returns true if successful. Server-only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool RemoveItem(FGuid InstanceID);

	/** Attempts to move an existing item to a new location. Server-only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool TryMoveItem(FGuid InstanceID, int32 NewTopLeftX, int32 NewTopLeftY, bool bNewRotated);

	/** Pulls an item out of another grid and drops it into this one at a specific cell,
	 *  preserving its identity and contents. Handles partial stack merges by leaving the
	 *  remainder behind. Passing this component as Source is allowed and routes to
	 *  TryMoveItem. Returns true when anything moved at all. Server-only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool MoveItemFrom(USomnusInventoryComponent* Source, FGuid InstanceID, int32 TopLeftX, int32 TopLeftY, bool bRotated);

	// No client entry points live here on purpose. A grid is reached through whoever is holding
	// it - USomnusLootComponent - because a body on the floor has no connection for an RPC to
	// arrive over, and because a grid cannot tell whether the client naming it is entitled to.

	/** Helper to find an item instance by its unique ID. Public, unlike its mutable counterpart:
	 *  handing out a read-only view costs nothing, while writing through the other one without
	 *  going on to mark the list dirty leaves every listener looking at the previous state. */
	const FSomnusItemInstance* FindItemInstance(FGuid ID) const;

	/** Internal logic to add an item after validation. */
	void Internal_AddItem(class USomnusItemDataAsset* ItemData, int32 Quantity, int32 TopLeftX, int32 TopLeftY, bool bRotated);

	/** Prints the current grid state to the log (■ for occupied, □ for empty) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void PrintDebugGrid() const;

	/** Called by ItemInstances when they are synchronized over the network */
	void OnItemAdded(const FSomnusItemInstance& Item);
	void OnItemRemoved(const FSomnusItemInstance& Item);
	void OnItemChanged(const FSomnusItemInstance& Item);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Rebuilds the OccupationGrid bit array from scratch based on current items */
	void RebuildOccupationGrid();

	/** Helper to find an item instance that covers a specific grid cell */
	FSomnusItemInstance* GetItemAt(int32 X, int32 Y);

	/** Writable counterpart of FindItemInstance, for the paths that adjust an item in place. */
	FSomnusItemInstance* FindItemInstanceMutable(FGuid ID);
	
	UFUNCTION(BlueprintPure, Category = "Inventory|Size")
	int32 GetGridWidth() const { return GridWidth; };
	
	UFUNCTION(BlueprintPure, Category = "Inventory|Size")
	int32 GetGridHeight() const { return GridHeight; }
	
	/** Total columns in the grid */
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Inventory|Grid")
	int32 GridWidth = 10;

	/** Total rows in the grid */
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Inventory|Grid")
	int32 GridHeight = 6;

	/** The replicated list of items */
	UPROPERTY(Replicated)
	FSomnusInventoryList InventoryList;

	/** Bit array representing the occupancy of each grid cell. Rebuilt locally. */
	TBitArray<> OccupationGrid;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Debug")
	TMap<USomnusItemDataAsset*, int32> DefaultItems;
};
