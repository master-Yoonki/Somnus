// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusEquipmentSlotComponent.h"

USomnusEquipmentSlotComponent::USomnusEquipmentSlotComponent()
{
	GridWidth = 1;
	GridHeight = 1;
}

bool USomnusEquipmentSlotComponent::CanFitAt(class USomnusItemDataAsset* ItemData, int32 TopLeftX, int32 TopLeftY,
	bool bRotated, FGuid IgnoreItemID) const
{
	if (!ItemData) return false;
	if (!(TopLeftX == 0 && TopLeftY == 0)) return false;
	for (const FSomnusItemInstance& ItemInstance : InventoryList.Items)
	{
		if (IgnoreItemID.IsValid() && ItemInstance.InstanceID == IgnoreItemID)
		{
			continue;
		}
		return false;
	}
	return true;
}

bool USomnusEquipmentSlotComponent::FindFirstFit(class USomnusItemDataAsset* ItemData, int32& OutX, int32& OutY,
	bool& bOutRotated) const
{
	if (!ItemData) return false;
	if (CanFitAt(ItemData, 0, 0, false))
	{
		OutX = 0;
		OutY = 0;
		bOutRotated = false;
		return true;
	}
	OutX = -1;
	OutY = -1;
	bOutRotated = false;
	return false;
}

void USomnusEquipmentSlotComponent::RebuildOccupationGrid()
{
	OccupationGrid.Init(!(InventoryList.Items.IsEmpty()), GridWidth * GridHeight);
}
