// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/SomnusItemInstance.h"
#include "SomnusInventoryComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSomnusInventory, Log, All);

/**
 * Manages a grid-based inventory for the owner actor.
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class SOMNUS_API USomnusInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USomnusInventoryComponent();

	virtual void InitializeComponent() override;

	/** Checks if a coordinate is within the grid bounds */
	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	bool IsValidCell(int32 X, int32 Y) const;

	/** Checks if an item can fit at the specified location (ShapeMask aware) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	bool CanFitAt(class USomnusItemDataAsset* ItemData, int32 TopLeftX, int32 TopLeftY, bool bRotated, FGuid IgnoreItemID = FGuid()) const;

	/** Finds the first available empty space for an item */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	bool FindFirstFit(class USomnusItemDataAsset* ItemData, int32& OutX, int32& OutY, bool& bOutRotated) const;

	/** Prints the current grid state to the log (■ for occupied, □ for empty) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void PrintDebugGrid() const;

	/** Called by ItemInstances when they are synchronized over the network */
	void OnItemAdded(const FSomnusItemInstance& Item);
	void OnItemRemoved(const FSomnusItemInstance& Item);
	void OnItemChanged(const FSomnusItemInstance& Item);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Rebuilds the OccupationGrid bit array from scratch based on current items */
	void RebuildOccupationGrid();

	/** Helper to find an item instance by its unique ID */
	const FSomnusItemInstance* FindItemInstance(FGuid ID) const;

	/** Total columns in the grid */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Grid")
	int32 GridWidth = 10;

	/** Total rows in the grid */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Grid")
	int32 GridHeight = 6;

	/** The replicated list of items */
	UPROPERTY(Replicated)
	FSomnusInventoryList InventoryList;

	/** Bit array representing the occupancy of each grid cell. Rebuilt locally. */
	TBitArray<> OccupationGrid;
};
