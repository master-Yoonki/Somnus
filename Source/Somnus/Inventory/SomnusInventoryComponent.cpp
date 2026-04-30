// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/SomnusInventoryComponent.h"
#include "Net/UnrealNetwork.h"

#include "Inventory/SomnusItemDataAsset.h"

DEFINE_LOG_CATEGORY(LogSomnusInventory);

USomnusInventoryComponent::USomnusInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;

	InventoryList.OwnerComponent = this;
}

void USomnusInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Initialize the occupancy grid with false (empty) bits
	OccupationGrid.Init(false, GridWidth * GridHeight);

	UE_LOG(LogSomnusInventory, Log, TEXT("Inventory Component Initialized on %s (Grid: %dx%d)"), *GetOwner()->GetName(), GridWidth, GridHeight);

	// Zero Calibration: Print the empty grid to verify setup
	PrintDebugGrid();
}

bool USomnusInventoryComponent::IsValidCell(int32 X, int32 Y) const
{
	return X >= 0 && X < GridWidth && Y >= 0 && Y < GridHeight;
}

bool USomnusInventoryComponent::CanFitAt(USomnusItemDataAsset* ItemData, int32 TopLeftX, int32 TopLeftY, bool bRotated, FGuid IgnoreItemID) const
{
	// TODO: Implement Step 4 Core Logic
	return false;
}

bool USomnusInventoryComponent::FindFirstFit(USomnusItemDataAsset* ItemData, int32& OutX, int32& OutY, bool& bOutRotated) const
{
	// TODO: Implement Step 4 Core Logic
	return false;
}

void USomnusInventoryComponent::PrintDebugGrid() const
{
	FString GridString = TEXT("\n");
	for (int32 y = 0; y < GridHeight; ++y)
	{
		for (int32 x = 0; x < GridWidth; ++x)
		{
			if (OccupationGrid.IsValidIndex(y * GridWidth + x))
			{
				GridString += OccupationGrid[y * GridWidth + x] ? TEXT("■ ") : TEXT("□ ");
			}
		}
		GridString += TEXT("\n");
	}

	UE_LOG(LogSomnusInventory, Log, TEXT("Current Inventory Grid (%dx%d): %s"), GridWidth, GridHeight, *GridString);
}

void USomnusInventoryComponent::RebuildOccupationGrid()
{
	// TODO: Implement Step 4 Core Logic
}

const FSomnusItemInstance* USomnusInventoryComponent::FindItemInstance(FGuid ID) const
{
	for (const FSomnusItemInstance& Item : InventoryList.Items)
	{
		if (Item.InstanceID == ID)
		{
			return &Item;
		}
	}
	return nullptr;
}

void USomnusInventoryComponent::OnItemAdded(const FSomnusItemInstance& Item)
{
	UE_LOG(LogSomnusInventory, Log, TEXT("Item Added: %s"), Item.ItemData ? *Item.ItemData->DisplayName.ToString() : TEXT("None"));
}

void USomnusInventoryComponent::OnItemRemoved(const FSomnusItemInstance& Item)
{
	UE_LOG(LogSomnusInventory, Log, TEXT("Item Removed: %s"), Item.ItemData ? *Item.ItemData->DisplayName.ToString() : TEXT("None"));
}

void USomnusInventoryComponent::OnItemChanged(const FSomnusItemInstance& Item)
{
	UE_LOG(LogSomnusInventory, Log, TEXT("Item Changed: %s"), Item.ItemData ? *Item.ItemData->DisplayName.ToString() : TEXT("None"));
}

void USomnusInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USomnusInventoryComponent, InventoryList);
}
