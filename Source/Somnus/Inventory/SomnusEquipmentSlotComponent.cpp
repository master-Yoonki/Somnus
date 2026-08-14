// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusEquipmentSlotComponent.h"

#include "GameFramework/Actor.h"

USomnusEquipmentSlotComponent::USomnusEquipmentSlotComponent()
{
	GridWidth = 1;
	GridHeight = 1;
}

void USomnusEquipmentSlotComponent::InitializeSlot(FGameplayTag InSlotTag,
	const FGameplayTagContainer& InAcceptedTags, const FText& InLabel, bool bInLocked)
{
	SlotTag = InSlotTag;
	SlotLabel = InLabel;
	bLocked = bInLocked;
	InitializeAcceptedItemTags(InAcceptedTags);
}

bool USomnusEquipmentSlotComponent::AcceptsItem(USomnusItemDataAsset* ItemData) const
{
	if (AcceptedTags.IsEmpty())
	{
		return false;
	}

	return Super::AcceptsItem(ItemData);
}

USomnusEquipmentSlotComponent* USomnusEquipmentSlotComponent::FindSlot(const AActor* Owner, FGameplayTag InSlotTag)
{
	if (!Owner || !InSlotTag.IsValid())
	{
		return nullptr;
	}

	TArray<USomnusEquipmentSlotComponent*> Slots;
	Owner->GetComponents<USomnusEquipmentSlotComponent>(Slots);

	for (USomnusEquipmentSlotComponent* Slot : Slots)
	{
		if (Slot && Slot->SlotTag == InSlotTag)
		{
			return Slot;
		}
	}

	return nullptr;
}

bool USomnusEquipmentSlotComponent::CanFitAt(class USomnusItemDataAsset* ItemData, int32 TopLeftX, int32 TopLeftY,
	bool bRotated, FGuid IgnoreItemID) const
{
	if (!ItemData) return false;

	// Called rather than inherited: this override replaces the base CanFitAt outright, so the
	// admission question would go unasked here - the one place it matters most, since a slot's
	// whole job is to hold one particular kind of thing.
	if (!AcceptsItem(ItemData)) return false;

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
