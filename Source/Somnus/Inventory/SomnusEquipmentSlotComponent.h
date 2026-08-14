// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/SomnusInventoryComponent.h"
#include "SomnusEquipmentSlotComponent.generated.h"

/**
 * An inventory grid of one cell that admits an item regardless of how large it is.
 *
 * Every path that adds or moves an item in the base class narrows to CanFitAt or FindFirstFit and
 * then settles through RebuildOccupationGrid, so overriding those three is the entire class. A 3x4
 * backpack lands in a slot by the same code that lands it in a pocket, and everything the base
 * already does - stack merging, cross-grid moves, replication, the add and remove delegates that
 * are where a slot's meaning gets attached - is inherited untouched.
 *
 * Item dimensions and bRotated go deliberately unread: what admits an item here is whether the
 * slot is free, never whether it has room. GridWidth and GridHeight are held at 1x1 so the
 * inherited helpers keep answering sensibly, but nothing here decides anything from them.
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class SOMNUS_API USomnusEquipmentSlotComponent : public USomnusInventoryComponent
{
	GENERATED_BODY()
public:
	USomnusEquipmentSlotComponent();

	/** IgnoreItemID carries more weight here than in a grid. A slot holding one item is full by
	 *  definition, so re-placing that same item - which TryMoveItem does on every drop that lands
	 *  where it started - would otherwise be refused by its own presence. */
	virtual bool CanFitAt(class USomnusItemDataAsset* ItemData,
		int32 TopLeftX, int32 TopLeftY, bool bRotated,
		FGuid IgnoreItemID = FGuid()) const override;

	virtual bool FindFirstFit(class USomnusItemDataAsset* ItemData, int32& OutX, int32& OutY, bool& bOutRotated) const override;

protected:
	virtual void RebuildOccupationGrid() override;
};
