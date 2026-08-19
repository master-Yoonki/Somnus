// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/SomnusInventoryComponent.h"

#include "IDetailTreeNode.h"
#include "SomnusContainerActor.h"
#include "Net/UnrealNetwork.h"

#include "Inventory/SomnusContainerDataAsset.h"
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

	// Again, because the constructor's assignment does not survive on every path here. A component
	// made with CreateDefaultSubobject is built from the class default object's copy: the
	// constructor runs and points this at the new instance, and then the archetype's properties
	// are copied over the top, carrying a back-pointer to the CDO's component with them. It is a
	// plain object property, so nothing remaps it. What follows is a grid whose replication
	// callbacks fire on the CDO instead - no crash, no warning, just delegates broadcast where
	// nobody is listening, and only on the machines that learn by replication.
	//
	// Grids made with NewObject never had the problem, which is why every compartment behaved and
	// only the equipment slots went quiet.
	InventoryList.OwnerComponent = this;

	// Initialize the occupancy grid with false (empty) bits
	OccupationGrid.Init(false, GridWidth * GridHeight);

	// UE_LOG(LogSomnusInventory, Log, TEXT("Inventory Component Initialized on %s (Grid: %dx%d)"), *GetOwner()->GetName(), GridWidth, GridHeight);

	// Zero Calibration: Print the empty grid to verify setup
	// PrintDebugGrid();
}

void USomnusInventoryComponent::InitializeAcceptedItemTags(const FGameplayTagContainer& InTags)
{
	AcceptedTags = InTags;
}

bool USomnusInventoryComponent::AcceptsItem(USomnusItemDataAsset* ItemData) const
{
	if (!ItemData)
	{
		return false;
	}

	if (AcceptedTags.IsEmpty())
	{
		return true;
	}

	// The item's tag is the leaf and the grid's is the parent, so the match has to run this way
	// round: "Item.Equipment.Weapon.Melee".MatchesAny({"Item.Equipment.Weapon"}) is true, while
	// the container asking the same question of the tag is not. That is what lets one weapon slot
	// accept every weapon kind without hearing about a new one.
	return ItemData->ItemTag.MatchesAny(AcceptedTags);
}

void USomnusInventoryComponent::InitializeGridSize(int32 NewX, int32 NewY)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return;
	if (NewX <= 0 || NewY <= 0) return;
	GridWidth = NewX;
	GridHeight = NewY;
}

bool USomnusInventoryComponent::IsValidCell(int32 X, int32 Y) const
{
	return X >= 0 && X < GridWidth && Y >= 0 && Y < GridHeight;
}

bool USomnusInventoryComponent::CanFitAt(USomnusItemDataAsset* ItemData, int32 TopLeftX, int32 TopLeftY, bool bRotated, FGuid IgnoreItemID) const
{
	if (!ItemData) return false;
	if (!AcceptsItem(ItemData)) return false;

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

int32 USomnusInventoryComponent::MergeExistingItemIntoStacks(FSomnusItemInstance& IncomingItemInstance)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return IncomingItemInstance.StackCount;
	if (!IncomingItemInstance.ItemData || IncomingItemInstance.StackCount <= 0) return IncomingItemInstance.StackCount;

	for (FSomnusItemInstance& ItemInstance : InventoryList.Items)
	{
		if (IncomingItemInstance.StackCount <= 0) break;
		if (!ItemInstance.ItemData || ItemInstance.ItemData->ItemId != IncomingItemInstance.ItemData->ItemId) continue;

		const int32 AvailableSpace = ItemInstance.ItemData->MaxStackCount - ItemInstance.StackCount;
		const int32 QuantityToMerge = FMath::Min(IncomingItemInstance.StackCount, AvailableSpace);
		if (QuantityToMerge <= 0) continue;

		ItemInstance.StackCount += QuantityToMerge;
		IncomingItemInstance.StackCount -= QuantityToMerge;
		// No RebuildOccupationGrid - a merge changes stack counts, never the footprint.
		InventoryList.MarkItemDirty(ItemInstance);
		OnItemChanged(ItemInstance);
	}
	return IncomingItemInstance.StackCount;
}

/** True when Grid is one of MovingContainer's own compartments, or sits somewhere inside a
 *  container nested within it. Descends through the contents rather than walking up an owner
 *  chain, because a container held as an item in a grid has no owner to walk. */
static bool SomnusInventory_IsInsideContainer(const USomnusInventoryComponent* Grid, const ASomnusContainerActor* MovingContainer)
{
	if (!Grid || !MovingContainer)
	{
		return false;
	}

	for (USomnusInventoryComponent* Compartment : MovingContainer->GetCompartments())
	{
		if (!Compartment)
		{
			continue;
		}
		if (Compartment == Grid)
		{
			return true;
		}
		for (const FSomnusItemInstance& Item : Compartment->GetAllItems())
		{
			if (Item.ContainerActor && SomnusInventory_IsInsideContainer(Grid, Item.ContainerActor))
			{
				return true;
			}
		}
	}
	return false;
}

bool USomnusInventoryComponent::IsInsideContainer(const USomnusInventoryComponent* Grid, const ASomnusContainerActor* MovingContainer)
{
	return SomnusInventory_IsInsideContainer(Grid, MovingContainer);
}

AActor* USomnusInventoryComponent::GetHoldingActor() const
{
	return ASomnusContainerActor::ResolveRootHolder(GetOwner());
}

int32 USomnusInventoryComponent::AddExistingItemAnywhere(FSomnusItemInstance& IncomingItemInstance)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return IncomingItemInstance.StackCount;
	if (!IncomingItemInstance.ItemData) return IncomingItemInstance.StackCount;

	// Same refusal AddExistingItemAt makes, but taken before the loop rather than inside it.
	// A rejection down there leaves StackCount untouched, which trips the progress ensure and
	// blames CanFitAt/FindFirstFit for a disagreement they never had.
	if (IncomingItemInstance.ContainerActor
		&& SomnusInventory_IsInsideContainer(this, IncomingItemInstance.ContainerActor))
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("AddExistingItemAnywhere: refusing to put %s inside its own storage - it and everything in it would be unreachable."),
			*GetNameSafe(IncomingItemInstance.ItemData));
		return IncomingItemInstance.StackCount;
	}

	MergeExistingItemIntoStacks(IncomingItemInstance);

	// What is left needs its own cells. One stack per pass, because AddExistingItemAt caps
	// each placement at MaxStackCount and hands the remainder back.
	int32 GridX = 0;
	int32 GridY = 0;
	bool bIsRotated = false;
	while (IncomingItemInstance.StackCount > 0)
	{
		if (!FindFirstFit(IncomingItemInstance.ItemData, GridX, GridY, bIsRotated)) break;

		const int32 QuantityBefore = IncomingItemInstance.StackCount;
		AddExistingItemAt(IncomingItemInstance, GridX, GridY, bIsRotated);

		// The invariant is progress, not a zero remainder - a split legitimately leaves one.
		// Bailing on no progress also keeps a CanFitAt/FindFirstFit disagreement from
		// spinning this loop forever.
		if (!ensureMsgf(IncomingItemInstance.StackCount < QuantityBefore,
			TEXT("FindFirstFit reported free space at (%d, %d) but AddExistingItemAt placed nothing - CanFitAt/FindFirstFit disagree."),
			GridX, GridY))
		{
			break;
		}
	}
	return IncomingItemInstance.StackCount;
}

int32 USomnusInventoryComponent::AddExistingItemAt(FSomnusItemInstance& IncomingItemInstance, int32 TopLeftX, int32 TopLeftY,
                                                   bool bRotated)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return IncomingItemInstance.StackCount;
	if (IncomingItemInstance.StackCount <= 0) return IncomingItemInstance.StackCount;

	// This and AddExistingItemAnywhere are the only two doors an existing instance enters a
	// grid through, so refusing here covers every caller - cross-grid drag, unequip, pickup -
	// without any of them having to remember. Nesting a container inside its own storage would
	// make it and everything in it unreachable, with no path left to get any of it back.
	if (IncomingItemInstance.ContainerActor
		&& SomnusInventory_IsInsideContainer(this, IncomingItemInstance.ContainerActor))
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("AddExistingItemAt: refusing to put %s inside its own storage - it and everything in it would be unreachable."),
			*GetNameSafe(IncomingItemInstance.ItemData));
		return IncomingItemInstance.StackCount;
	}

	if (FSomnusItemInstance* ExistingItemInstance = GetItemAt(TopLeftX, TopLeftY))
	{
		if (ExistingItemInstance->ItemData->ItemId != IncomingItemInstance.ItemData->ItemId)
		{
			return IncomingItemInstance.StackCount;
		}
		const int32 AvailableSpace = 
			ExistingItemInstance->ItemData->MaxStackCount - ExistingItemInstance->StackCount; 
			
		const int32 QuantityToAdd = FMath::Min(IncomingItemInstance.StackCount, AvailableSpace);
			
		ExistingItemInstance->StackCount += QuantityToAdd;
		IncomingItemInstance.StackCount -= QuantityToAdd;
		InventoryList.MarkItemDirty(*ExistingItemInstance);	
		RebuildOccupationGrid();
		OnItemChanged(*ExistingItemInstance);
		return IncomingItemInstance.StackCount;
	}
	else
	{
		if (CanFitAt(IncomingItemInstance.ItemData, TopLeftX, TopLeftY, bRotated))
		{
			const int32 QuantityToPlace = FMath::Min(IncomingItemInstance.StackCount, IncomingItemInstance.ItemData->MaxStackCount);
			// A stack too large for one cell leaves a remainder behind, so this placement is
			// only a spin-off. The incoming identity - GUID, durability, and the container
			// actor holding its contents - stays with the remainder and lands on the final
			// stack; the spin-offs mint their own, the same way Internal_AddItem does on the
			// new-item path. Two grid entries sharing one GUID would make the second one
			// invisible to RemoveItem/TryMoveItem, which resolve by first match.
			const bool bIsSpinOff = QuantityToPlace < IncomingItemInstance.StackCount;

			FSomnusItemInstance& NewInstance = InventoryList.Items.AddDefaulted_GetRef();
			NewInstance.ItemData = IncomingItemInstance.ItemData;
			NewInstance.StackCount = QuantityToPlace;
			NewInstance.GridPosition = FIntPoint(TopLeftX, TopLeftY);
			NewInstance.bRotated = bRotated;
			NewInstance.InstanceID = bIsSpinOff ? FGuid::NewGuid() : IncomingItemInstance.InstanceID;
			NewInstance.CurrentDurability = IncomingItemInstance.CurrentDurability;
			// Only ever non-null on the final stack. Container items are MaxStackCount == 1
			// by construction, so they never take the spin-off branch - but duplicating the
			// pointer if one ever did would give two grid entries the same storage.
			NewInstance.ContainerActor = bIsSpinOff ? nullptr : IncomingItemInstance.ContainerActor;

			IncomingItemInstance.StackCount -= QuantityToPlace;

			InventoryList.MarkItemDirty(NewInstance);
			RebuildOccupationGrid();
			OnItemAdded(NewInstance);

			return IncomingItemInstance.StackCount;
		}
		return IncomingItemInstance.StackCount;
	}
}

int32 USomnusInventoryComponent::MergeItemIntoStacks(USomnusItemDataAsset* ItemData, int32 Quantity)
{
	AActor* OwnerActor = GetOwner();
	if (!ItemData || !OwnerActor || !OwnerActor->HasAuthority()) return Quantity;
	if (Quantity <= 0) return Quantity;

	for (FSomnusItemInstance& Item : InventoryList.Items)
	{
		if (Quantity <= 0) break;
		if (!Item.ItemData || Item.ItemData->ItemId != ItemData->ItemId) continue;

		const int32 AvailableSpace = Item.ItemData->MaxStackCount - Item.StackCount;
		const int32 QuantityToMerge = FMath::Min(Quantity, AvailableSpace);
		if (QuantityToMerge <= 0) continue;

		Item.StackCount += QuantityToMerge;
		Quantity -= QuantityToMerge;
		InventoryList.MarkItemDirty(Item);
		OnItemChanged(Item);
	}
	return Quantity;
}

int32 USomnusInventoryComponent::AddItemAnywhere(USomnusItemDataAsset* ItemData, int32 Quantity)
{
	AActor* OwnerActor = GetOwner();
	if (!ItemData || !OwnerActor || !OwnerActor->HasAuthority()) return Quantity;
	if (Quantity <= 0) return Quantity;

	int32 QuantityToAdd = MergeItemIntoStacks(ItemData, Quantity);

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
			// A copy, then the removal, then the notice - in that order, and the copy is what makes
			// the order possible. Announcing the removal while the entry was still in the array let
			// a listener be told an item had left and then find it by re-reading the grid, which is
			// how a weapon dragged out of its slot kept the actor that stood for it: the reconcile
			// pass saw the slot still holding what it had just been told was gone, decided nothing
			// had changed, and never retired anything.
			const FSomnusItemInstance Removed = *It;

			InventoryList.Items.RemoveAt(It.GetIndex());
			InventoryList.MarkArrayDirty();

			// Rebuilds the occupation grid before it broadcasts, so listeners read a grid that
			// agrees with the list.
			OnItemRemoved(Removed);
			return true;
		}
	}
	return false;
}

bool USomnusInventoryComponent::ConsumeOneFromStack(FGuid InstanceID)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return false;

	FSomnusItemInstance* Mutable = FindItemInstanceMutable(InstanceID);
	if (!Mutable) return false;

	Mutable->StackCount -= 1;
	if (Mutable->StackCount <= 0)
	{
		RemoveItem(InstanceID);
	}
	else
	{
		InventoryList.MarkItemDirty(*Mutable);
		OnItemChanged(*Mutable);
	}
	return true;
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

bool USomnusInventoryComponent::MoveItemFrom(USomnusInventoryComponent* Source, FGuid InstanceID, int32 TopLeftX, int32 TopLeftY, bool bRotated)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Source)
	{
		return false;
	}

	// Same grid: the item still occupies its old cells, so a plain placement would collide
	// with its own body. TryMoveItem is the path that knows to ignore it via CanFitAt's
	// IgnoreItemID. Decided here rather than in the widget, so a client that gets it wrong
	// still cannot corrupt anything.
	if (Source == this)
	{
		return TryMoveItem(InstanceID, TopLeftX, TopLeftY, bRotated);
	}

	// Copy the instance out rather than holding a pointer into Source's array: the add below
	// fires OnItemAdded, and a listener is free to touch either inventory.
	FSomnusItemInstance Moving;
	{
		const FSomnusItemInstance* SourceItem = Source->FindItemInstance(InstanceID);
		if (!SourceItem || !SourceItem->ItemData)
		{
			// The client asked to move something that is not there. Its view will be corrected
			// by the next replication of Source, so there is nothing to undo here.
			return false;
		}
		Moving = *SourceItem;
	}

	// Add first, remove second. The other order destroys the item whenever the placement is
	// rejected - same rule EquipInstance follows. A container dropped into its own storage is
	// one of the rejections AddExistingItemAt makes on its own, and it arrives here as a full
	// leftover, so it falls out at the check below with the source untouched.
	const int32 QuantityBefore = Moving.StackCount;
	const int32 Leftover = AddExistingItemAt(Moving, TopLeftX, TopLeftY, bRotated);

	if (Leftover >= QuantityBefore)
	{
		return false;   // nothing was accepted, so the source keeps everything
	}

	if (Leftover <= 0)
	{
		Source->RemoveItem(InstanceID);
	}
	else if (FSomnusItemInstance* Remainder = Source->FindItemInstanceMutable(InstanceID))
	{
		// A partial stack merge: the destination took what it had room for and the rest stays
		// where it was.
		Remainder->StackCount = Leftover;
		Source->InventoryList.MarkItemDirty(*Remainder);
		Source->OnItemChanged(*Remainder);
	}

	// Storage follows its holder so relevancy keeps resolving and permission keeps answering:
	// the owner chain is what both read. Source and destination sit on different bodies as soon
	// as anything is looted, which is exactly when this line stops being a formality.
	if (Moving.ContainerActor)
	{
		Moving.ContainerActor->SetOwner(ASomnusContainerActor::ResolveRootHolder(OwnerActor));
	}
	return true;
}


void USomnusInventoryComponent::Internal_AddItem(USomnusItemDataAsset* ItemData, int32 Quantity, int32 TopLeftX, int32 TopLeftY, bool bRotated)
{
	// Minting the instance (ID, container actor) lives on the struct itself; this function
	// only decides where it goes in the grid.
	FSomnusItemInstance NewInstance = FSomnusItemInstance::MakeItemInstance(GetWorld(), ItemData, Quantity);
	if (!NewInstance.ItemData)
	{
		return;
	}

	NewInstance.GridPosition = FIntPoint(TopLeftX, TopLeftY);
	NewInstance.bRotated = bRotated;

	FSomnusItemInstance& AddedInstance = InventoryList.Items.Add_GetRef(NewInstance);

	InventoryList.MarkItemDirty(AddedInstance);
	RebuildOccupationGrid();
	OnItemAdded(AddedInstance);
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

FSomnusItemInstance* USomnusInventoryComponent::FindItemInstanceMutable(FGuid ID)
{
	for (FSomnusItemInstance& Item : InventoryList.Items)
	{
		if (Item.InstanceID == ID)
		{
			return &Item;
		}
	}
	return nullptr;
}

bool USomnusInventoryComponent::FindItemByID(FGuid InstanceID, FSomnusItemInstance& OutItem) const
{
	if (const FSomnusItemInstance* FoundItem = FindItemInstance(InstanceID))
	{
		OutItem = *FoundItem;
		return true;
	}
	return false;
}

/** Which grid, on which machine. These three fire from two unrelated places - the authority's own
 *  edits and the FastArray callbacks on everyone else - so a line that names only the item leaves
 *  the reader counting log lines to guess where each came from. */
FString USomnusInventoryComponent::DescribeForLog() const
{
	const AActor* OwnerActor = GetOwner();
	return FString::Printf(TEXT("[%s] %s on %s"),
		OwnerActor && OwnerActor->HasAuthority() ? TEXT("Server") : TEXT("Client"),
		*GetName(),
		*GetNameSafe(GetHoldingActor()));
}

void USomnusInventoryComponent::OnItemAdded(const FSomnusItemInstance& Item)
{
	UE_LOG(LogSomnusInventory, Log, TEXT("%s  Item Added: %s"), *DescribeForLog(),
		Item.ItemData ? *Item.ItemData->DisplayName.ToString() : TEXT("None"));
	RebuildOccupationGrid();
	OnItemAddedDelegate.Broadcast(Item);
}

void USomnusInventoryComponent::OnItemRemoved(const FSomnusItemInstance& Item)
{
	UE_LOG(LogSomnusInventory, Log, TEXT("%s  Item Removed: %s"), *DescribeForLog(),
		Item.ItemData ? *Item.ItemData->DisplayName.ToString() : TEXT("None"));
	RebuildOccupationGrid();
	OnItemRemovedDelegate.Broadcast(Item);
}

void USomnusInventoryComponent::OnItemChanged(const FSomnusItemInstance& Item)
{
	UE_LOG(LogSomnusInventory, Log, TEXT("%s  Item Changed: %s"), *DescribeForLog(),
		Item.ItemData ? *Item.ItemData->DisplayName.ToString() : TEXT("None"));
	RebuildOccupationGrid();
	OnItemChangedDelegate.Broadcast(Item);
}

void USomnusInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USomnusInventoryComponent, InventoryList);
	DOREPLIFETIME(USomnusInventoryComponent, GridWidth);
	DOREPLIFETIME(USomnusInventoryComponent, GridHeight);
	DOREPLIFETIME(USomnusInventoryComponent, AcceptedTags);
}
