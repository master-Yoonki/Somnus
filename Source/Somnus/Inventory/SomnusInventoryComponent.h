// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/SomnusItemInstance.h"
#include "SomnusInventoryComponent.generated.h"

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

	/** Server RPC for adding an item anywhere. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|RPC")
	void Server_AddItemAnywhere(class USomnusItemDataAsset* ItemData, int32 Quantity = 1);

	/** Server RPC for adding an item at a specific location. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|RPC")
	void Server_AddItemAt(class USomnusItemDataAsset* ItemData, int32 Quantity, int32 TopLeftX, int32 TopLeftY, bool bRotated);

	/** Server RPC for removing an item. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|RPC")
	void Server_RemoveItem(FGuid InstanceID);

	/** Server RPC for moving an item. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|RPC")
	void Server_TryMoveItem(FGuid InstanceID, int32 NewTopLeftX, int32 NewTopLeftY, bool bNewRotated);

	/** Server RPC for a drag that crossed grids. Call it on the grid that was dropped on;
	 *  Source is the grid the drag started in. Safe to call when they are the same. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|RPC")
	void Server_MoveItemFrom(USomnusInventoryComponent* Source, FGuid InstanceID, int32 TopLeftX, int32 TopLeftY, bool bRotated);

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

	/** Helper to find an item instance by its unique ID */
	const FSomnusItemInstance* FindItemInstance(FGuid ID) const;

	/** Writable counterpart, for the paths that adjust an item in place. */
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
