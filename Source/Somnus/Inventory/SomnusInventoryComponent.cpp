// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/SomnusInventoryComponent.h"

#include "IDetailTreeNode.h"
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

void USomnusInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only grant default items on the server
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority())
	{
		for (const auto& Pair : DefaultItems)
		{
			USomnusItemDataAsset* ItemData = Pair.Key;
			int32 Quantity = Pair.Value;

			if (IsValid(ItemData) && Quantity > 0)
			{
				AddItemAnywhere(ItemData, Quantity);
			}
		}
	}
}

bool USomnusInventoryComponent::IsValidCell(int32 X, int32 Y) const
{
	return X >= 0 && X < GridWidth && Y >= 0 && Y < GridHeight;
}

bool USomnusInventoryComponent::CanFitAt(USomnusItemDataAsset* ItemData, int32 TopLeftX, int32 TopLeftY, bool bRotated, FGuid IgnoreItemID) const
{
	if (!ItemData) return false;
	
	const FIntPoint ItemSize = ItemData->GetEffectiveSize(bRotated);
	
	const FIntPoint TopLeft = FIntPoint(TopLeftX, TopLeftY);
	const FIntPoint BottomRight = TopLeft + ItemSize - FIntPoint(1, 1);
	
	if (!IsValidCell(TopLeft.X, TopLeft.Y)) return false;
	if (!IsValidCell(BottomRight.X, BottomRight.Y)) return false;
	
	for (int32 y = 0; y < ItemSize.Y; ++y)
	{
		for (int32 x = 0; x < ItemSize.X; ++x)
		{
			if (ItemData->IsCellOccupied(x, y, bRotated))
			{
				const FIntPoint CellPos = FIntPoint(TopLeft.X + x, TopLeft.Y + y);
				
				int32 Index = CellPos.Y * GridWidth + CellPos.X;
				if (OccupationGrid[Index])
				{
					bool bIsIgnoringItem = false;
					if (IgnoreItemID.IsValid())
					{
						const FSomnusItemInstance* IgnoredItem = FindItemInstance(IgnoreItemID);
						if (IgnoredItem && IgnoredItem->ItemData)
						{
							int32 LocalX = CellPos.X - IgnoredItem->GridPosition.X;
							int32 LocalY = CellPos.Y - IgnoredItem->GridPosition.Y;
							const FIntPoint IgnoredSize = IgnoredItem->ItemData->GetEffectiveSize(IgnoredItem->bRotated);
							
							if (LocalX >= 0 && LocalX < IgnoredSize.X && LocalY >= 0 && LocalY < IgnoredSize.Y)
							{
								if (IgnoredItem->ItemData->IsCellOccupied(LocalX, LocalY, IgnoredItem->bRotated))
								{
									bIsIgnoringItem = true;
								}
							}
						}
					}
					if (!bIsIgnoringItem)
					{
						return false;
					}
				}
			}
		}
	}
	return true;
}

bool USomnusInventoryComponent::FindFirstFit(USomnusItemDataAsset* ItemData, int32& OutX, int32& OutY, bool& bOutRotated) const
{
	if (!ItemData) return false;
	
	FIntPoint Size = ItemData->GetEffectiveSize(false);
	
	// Rotation = false
	for (int32 y = 0; y <= GridHeight - Size.Y; ++y)
	{
		for (int32 x = 0; x <= GridWidth - Size.X; ++x)
		{
			if (CanFitAt(ItemData, x, y, false))
			{
				OutX = x;
				OutY = y;
				bOutRotated = false;
				return true;
			}
		}
	}
	
	// Rotation = true
	const FIntPoint RotatedSize = ItemData->GetEffectiveSize(true);
	
	for (int32 y = 0; y <= GridHeight - RotatedSize.Y; ++y)
	{
		for (int32 x = 0; x <= GridWidth - RotatedSize.X; ++x)
		{
			if (CanFitAt(ItemData, x, y, true))
			{
				OutX = x;
				OutY = y;
				bOutRotated = true;
				return true;
			}
		}
	}
	
	return false;
}

int32 USomnusInventoryComponent::AddItemAnywhere(USomnusItemDataAsset* ItemData, int32 Quantity)
{
	AActor* OwnerActor = GetOwner();
	if (!ItemData || !OwnerActor || !OwnerActor->HasAuthority()) return Quantity;
	if (Quantity <= 0) return Quantity;
	
	int32 QuantityToAdd = Quantity;
	
	for (FSomnusItemInstance& Item : InventoryList.Items)
	{
		if (Item.ItemData->ItemId == ItemData->ItemId)
		{
			const int32 AvailableSpace = Item.ItemData->MaxStackCount - Item.StackCount;
			const int32 QuantityToMerge = FMath::Min(QuantityToAdd, AvailableSpace);
			
			if (QuantityToMerge > 0)
			{
				Item.StackCount += QuantityToMerge;
				InventoryList.MarkItemDirty(Item);
				OnItemChanged(Item);
				QuantityToAdd -= QuantityToMerge;
			}
		}
		if (QuantityToAdd <= 0) return 0;
	}
	
	int32 AddItemLocationX = 0, AddItemLocationY = 0;
	bool bRotated = false;
	while (QuantityToAdd > 0)
	{
		if (FindFirstFit(ItemData, AddItemLocationX, AddItemLocationY, bRotated))
		{
			int32 QuantityToAssign = FMath::Min(QuantityToAdd, ItemData->MaxStackCount); 
			Internal_AddItem(ItemData, QuantityToAssign, AddItemLocationX, AddItemLocationY, bRotated);
			QuantityToAdd -= QuantityToAssign;
		}
		else
		{
			return QuantityToAdd;
		}
	}
	return 0;
}

int32 USomnusInventoryComponent::AddItemAt(USomnusItemDataAsset* ItemData, int32 Quantity, int32 TopLeftX, int32 TopLeftY, bool bRotated)
{	
	AActor* OwnerActor = GetOwner();
	if (!ItemData || !OwnerActor || !OwnerActor->HasAuthority()) return Quantity;
	
	if (FSomnusItemInstance* OccupiedItem = GetItemAt(TopLeftX, TopLeftY))
	{
		if (OccupiedItem->ItemData->ItemId == ItemData->ItemId)
		{
			const int32 AvailableSpace = OccupiedItem->ItemData->MaxStackCount - OccupiedItem->StackCount;
			
			if (AvailableSpace > 0)
			{
				const int32 QuantityToAssign = FMath::Min(AvailableSpace, Quantity);
				OccupiedItem->StackCount += QuantityToAssign;
				InventoryList.MarkItemDirty(*OccupiedItem);
				OnItemChanged(*OccupiedItem);
				return Quantity - QuantityToAssign;
			}
		}
	}
	
	if (CanFitAt(ItemData, TopLeftX, TopLeftY, bRotated))
	{
		Internal_AddItem(ItemData, FMath::Min(ItemData->MaxStackCount, Quantity), TopLeftX, TopLeftY, bRotated);
		return FMath::Max(Quantity - ItemData->MaxStackCount, 0);
	}
	
	return Quantity;
}

bool USomnusInventoryComponent::RemoveItem(FGuid InstanceID)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return false;

	for (auto It = InventoryList.Items.CreateIterator(); It; ++It)
	{
		if (It->InstanceID == InstanceID)
		{
			// Broadcast before removing so delegates can still read item data
			OnItemRemoved(*It);
			InventoryList.Items.RemoveAt(It.GetIndex());
			InventoryList.MarkArrayDirty();
			RebuildOccupationGrid();
			// PrintDebugGrid();
			return true;
		}
	}
	return false;
}

void USomnusInventoryComponent::Server_AddItemAnywhere_Implementation(USomnusItemDataAsset* ItemData, int32 Quantity)
{
	AddItemAnywhere(ItemData, Quantity);
}

void USomnusInventoryComponent::Server_AddItemAt_Implementation(USomnusItemDataAsset* ItemData, int32 Quantity, int32 TopLeftX, int32 TopLeftY, bool bRotated)
{
	AddItemAt(ItemData, Quantity, TopLeftX, TopLeftY, bRotated);
}

void USomnusInventoryComponent::Server_RemoveItem_Implementation(FGuid InstanceID)
{
	RemoveItem(InstanceID);
}

bool USomnusInventoryComponent::TryMoveItem(FGuid InstanceID, int32 NewTopLeftX, int32 NewTopLeftY, bool bNewRotated)
{
	// 1. Authority Check
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}
	
	// 2. Find the dragged item (Source)
	FSomnusItemInstance* SourceItem = nullptr;
	for (FSomnusItemInstance& ItemInstance : InventoryList.Items)
	{
		if (ItemInstance.InstanceID == InstanceID)
		{
			SourceItem = &ItemInstance;
			break;
		}
	}
	
	if (!SourceItem)
	{
		return false;
	}
	
	// 3. Find what is sitting at the target location (Target)
	FSomnusItemInstance* TargetItem = GetItemAt(NewTopLeftX, NewTopLeftY);

	// ==========================================
	// PATH A: Attempt a Stack Merge
	// ==========================================
	// If there is an item here, and it's not ourself...
	if (TargetItem && (TargetItem->InstanceID != SourceItem->InstanceID))
	{
		// Can they stack together? (Same Data Asset)
		if (TargetItem->ItemData == SourceItem->ItemData)
		{
			const int32 SpaceLeft = TargetItem->ItemData->MaxStackCount - TargetItem->StackCount;
			
			if (SpaceLeft > 0)
			{
				// Calculate how much we can actually transfer
				const int32 AmountToMove = FMath::Min(SpaceLeft, SourceItem->StackCount);
				
				TargetItem->StackCount += AmountToMove;
				SourceItem->StackCount -= AmountToMove;

				// Mark the target as changed
				InventoryList.MarkItemDirty(*TargetItem);
				OnItemChanged(*TargetItem);

				// If we completely drained the dragged item, destroy it safely
				if (SourceItem->StackCount <= 0)
				{
					// RemoveItem already broadcasts OnItemRemoved, rebuilds grid, and marks array dirty
					RemoveItem(SourceItem->InstanceID); 
					return true; 
				}
				
				// Otherwise, the dragged item survived with some leftovers
				InventoryList.MarkItemDirty(*SourceItem);
				OnItemChanged(*SourceItem);
				RebuildOccupationGrid();
				return true;
			}
		}
		
		// If they aren't the same type, or the stack was full, the move fails.
		return false;
	}
	
	// ==========================================
	// PATH B: Attempt a Standard Move
	// ==========================================
	// We get here if the cell was empty, or if we are just rotating in place.
	// We MUST pass SourceItem->InstanceID so it ignores its own old body!
	if (CanFitAt(SourceItem->ItemData, NewTopLeftX, NewTopLeftY, bNewRotated, SourceItem->InstanceID))
	{
		SourceItem->GridPosition = FIntPoint(NewTopLeftX, NewTopLeftY);
		SourceItem->bRotated = bNewRotated;
		
		InventoryList.MarkItemDirty(*SourceItem);
		RebuildOccupationGrid();
		OnItemChanged(*SourceItem);
		return true;
	}
	
	// If it didn't fit, reject the move
	return false;
}

void USomnusInventoryComponent::Server_TryMoveItem_Implementation(FGuid InstanceID, int32 NewTopLeftX, int32 NewTopLeftY, bool bNewRotated)
{
	TryMoveItem(InstanceID, NewTopLeftX, NewTopLeftY, bNewRotated);
}


void USomnusInventoryComponent::Internal_AddItem(USomnusItemDataAsset* ItemData, int32 Quantity, int32 TopLeftX, int32 TopLeftY, bool bRotated)
{
	FSomnusItemInstance& NewInstance = InventoryList.Items.AddDefaulted_GetRef();
	NewInstance.ItemData = ItemData;
	NewInstance.StackCount = Quantity;
	NewInstance.GridPosition = FIntPoint(TopLeftX, TopLeftY);
	NewInstance.bRotated = bRotated;
	NewInstance.InstanceID = FGuid::NewGuid();
	InventoryList.MarkItemDirty(NewInstance);
	RebuildOccupationGrid();
	OnItemAdded(NewInstance);
	// PrintDebugGrid();
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
	OccupationGrid.Init(false, GridWidth * GridHeight);
	
	for (const FSomnusItemInstance& Item : InventoryList.Items)
	{
		if (!Item.ItemData) continue;
		
		const FIntPoint ItemSize = Item.ItemData->GetEffectiveSize(Item.bRotated);
		const FIntPoint Pos = Item.GridPosition;
		
		if (!IsValidCell(Pos.X, Pos.Y) ||
			!IsValidCell(Pos.X + ItemSize.X - 1, Pos.Y + ItemSize.Y - 1))
		{
			UE_LOG(LogSomnusInventory, Error, TEXT("Item %s is out of grid bounds at (%d, %d)"),
			*Item.ItemData->GetName(), Pos.X, Pos.Y);
			continue;
		}
		
		for (int32 y = 0; y < ItemSize.Y; ++y)
		{
			for (int32 x = 0; x < ItemSize.X; ++x)
			{
				FIntPoint CellPos = FIntPoint(Pos.X + x, Pos.Y + y);
				if (Item.ItemData->IsCellOccupied(x, y, Item.bRotated ))
				{
					int32 Index = CellPos.Y * GridWidth + CellPos.X;
					if (OccupationGrid[Index])
					{
						UE_LOG(LogSomnusInventory, Error, TEXT("Cell (%d, %d) is already occupied"), Pos.X, Pos.Y);
						continue;
					}
					OccupationGrid[Index] = true;
				}
			}
		}
	}
}

FSomnusItemInstance* USomnusInventoryComponent::GetItemAt(int32 X, int32 Y)
{
	if (X < 0 || X >= GridWidth || Y < 0 || Y >= GridHeight) return nullptr;
	
	for (FSomnusItemInstance& InventoryItem : InventoryList.Items)
	{
		if (!IsValid(InventoryItem.ItemData)) continue;
		
		// Check if target cell is within item's bounding box
		const FIntPoint TopLeft = InventoryItem.GridPosition;
		const FIntPoint BoundsMax = TopLeft + InventoryItem.ItemData->GetEffectiveSize(InventoryItem.bRotated);
		
		// First, AABB check for efficiency
		if (X >= TopLeft.X && X < BoundsMax.X && Y >= TopLeft.Y && Y < BoundsMax.Y)
		{
			// Check with precise shape mask
			if (InventoryItem.ItemData->IsCellOccupied(X - TopLeft.X, Y - TopLeft.Y, InventoryItem.bRotated))
			{
				return &InventoryItem;
			}
		}
	}
	return nullptr;
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
	RebuildOccupationGrid();
	OnItemAddedDelegate.Broadcast(Item);
}

void USomnusInventoryComponent::OnItemRemoved(const FSomnusItemInstance& Item)
{
	UE_LOG(LogSomnusInventory, Log, TEXT("Item Removed: %s"), Item.ItemData ? *Item.ItemData->DisplayName.ToString() : TEXT("None"));
	RebuildOccupationGrid();
	OnItemRemovedDelegate.Broadcast(Item);
}

void USomnusInventoryComponent::OnItemChanged(const FSomnusItemInstance& Item)
{
	UE_LOG(LogSomnusInventory, Log, TEXT("Item Changed: %s"), Item.ItemData ? *Item.ItemData->DisplayName.ToString() : TEXT("None"));
	OnItemChangedDelegate.Broadcast(Item);
}

void USomnusInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USomnusInventoryComponent, InventoryList);
}
