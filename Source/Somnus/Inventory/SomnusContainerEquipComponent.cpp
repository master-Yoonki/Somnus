// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusContainerEquipComponent.h"

#include "SomnusContainerActor.h"
#include "SomnusContainerDataAsset.h"
#include "SomnusInventoryComponent.h"
#include "SomnusItemDataAsset.h"
#include "Inventory/SomnusPickupActor.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
USomnusContainerEquipComponent::USomnusContainerEquipComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	PickupActorClass = ASomnusPickupActor::StaticClass();
	// ...
}

void USomnusContainerEquipComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(USomnusContainerEquipComponent, Pocket);
	DOREPLIFETIME(USomnusContainerEquipComponent, EquippedBackpack);
	DOREPLIFETIME(USomnusContainerEquipComponent, EquippedRig);
}

TArray<FSomnusActiveContainerInfo> USomnusContainerEquipComponent::GetActiveContainers() const
{
	TArray<FSomnusActiveContainerInfo> ContainerInfos;
	
	// Entries are appended one slot type at a time, so each container's compartments come out
	// as a contiguous run in SlotIndex order. The UI relies on that - do not interleave.
	auto AddCompartmentsInfo = [this, &ContainerInfos](EContainerSlotType SlotType, USomnusItemDataAsset* SourceItemData, TArray<USomnusInventoryComponent*>& Compartments)
	{
		const int32 NumCompartments = Compartments.Num();
		FSomnusActiveContainerInfo CompartmentInfo;
		CompartmentInfo.SlotType = SlotType;
		CompartmentInfo.SourceItemData = SourceItemData;
		for (int32 i = 0; i < NumCompartments; i++)
		{
			if (Compartments[i])
			{
				CompartmentInfo.SlotIndex = i;
				CompartmentInfo.Container = Compartments[i];
				ContainerInfos.Add(CompartmentInfo);
			}
		}
	};

	if (Pocket)
	{
		TArray<USomnusInventoryComponent*> Compartments = Pocket->GetCompartments();
		AddCompartmentsInfo(EContainerSlotType::Pockets, PocketData, Compartments);
	}

	if (EquippedBackpack.InstanceID.IsValid())
	{
		if (TObjectPtr<ASomnusContainerActor> Container = EquippedBackpack.ContainerActor)
		{
			TArray<USomnusInventoryComponent*> Compartments = Container->GetCompartments();
			AddCompartmentsInfo(EContainerSlotType::Backpack, EquippedBackpack.ItemData, Compartments);
		}
	}

	if (EquippedRig.InstanceID.IsValid())
	{
		if (TObjectPtr<ASomnusContainerActor> Container = EquippedRig.ContainerActor)
		{
			TArray<USomnusInventoryComponent*> Compartments = Container->GetCompartments();
			AddCompartmentsInfo(EContainerSlotType::Rig, EquippedRig.ItemData, Compartments);
		}
	}
	
	return ContainerInfos;
}

USomnusInventoryComponent* USomnusContainerEquipComponent::FindContainerHolding(FGuid InstanceID, FSomnusItemInstance& OutInstance) const
{
	if (!InstanceID.IsValid())
	{
		return nullptr;
	}

	for (const FSomnusActiveContainerInfo& Info : GetActiveContainers())
	{
		if (!Info.Container)
		{
			continue;
		}

		if (const FSomnusItemInstance* Found = Info.Container->FindItemInstance(InstanceID))
		{
			OutInstance = *Found;
			return Info.Container;
		}
	}

	return nullptr;
}

// Called when the game starts
void USomnusContainerEquipComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (!PocketData)
	{
		UE_LOG(LogSomnusInventory, Error,
			TEXT("%s has no PocketData assigned - this character will have no pockets."),
			*GetOwner()->GetName());
		return;
	}

	Pocket = GetWorld()->SpawnActor<ASomnusContainerActor>();
	if (!Pocket)
	{
		UE_LOG(LogSomnusInventory, Error,
			TEXT("Failed to spawn the pocket container for %s."), *GetOwner()->GetName());
		return;
	}
	
	Pocket->SetOwner(GetOwner());
	Pocket->Initialize(PocketData);
	// When server pocket ready, server can't re
	OnActiveContainersChangedDelegate.Broadcast();

	for (const TObjectPtr<USomnusContainerDataAsset>& EquipmentData : DefaultEquipment)
	{
		if (!EquipmentData)
		{
			continue;
		}

		const FSomnusItemInstance Instance = FSomnusItemInstance::MakeItemInstance(GetWorld(), EquipmentData);
		if (EquipInstance(Instance))
		{
			continue;
		}

		UE_LOG(LogSomnusInventory, Warning, TEXT("%s could not be granted to %s as default equipment."),
			*EquipmentData->GetName(), *GetOwner()->GetName());

		// The instance was already minted, so its container actor exists and is referenced
		// by nothing. Leaving it behind would keep an always-relevant actor alive forever.
		if (Instance.ContainerActor)
		{
			GetWorld()->DestroyActor(Instance.ContainerActor);
		}
	}

	// Loose items go in last, so they can land in the storage the loop above just granted.
	for (const TPair<TObjectPtr<USomnusItemDataAsset>, int32>& Pair : DefaultItems)
	{
		if (!Pair.Key || Pair.Value <= 0)
		{
			continue;
		}

		const int32 Leftover = TryAddItemAnywhere(Pair.Key, Pair.Value);
		if (Leftover > 0)
		{
			UE_LOG(LogSomnusInventory, Warning,
				TEXT("%s: only %d of %d %s fit as default items - the rest was discarded."),
				*GetOwner()->GetName(), Pair.Value - Leftover, Pair.Value, *Pair.Key->GetName());
		}
	}
}

bool USomnusContainerEquipComponent::EquipInstance(const FSomnusItemInstance& Instance)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	USomnusContainerDataAsset* ContainerDataAsset = Cast<USomnusContainerDataAsset>(Instance.ItemData);
	if (!ContainerDataAsset)
	{
		UE_LOG(LogSomnusInventory, Warning, TEXT("EquipInstance: %s is not a container item."),
			*GetNameSafe(Instance.ItemData));
		return false;
	}

	if (!Instance.ContainerActor)
	{
		UE_LOG(LogSomnusInventory, Error,
			TEXT("EquipInstance: %s has no container actor - refusing to equip storage that does not exist."),
			*ContainerDataAsset->GetName());
		return false;
	}

	FSomnusItemInstance* TargetSlot = nullptr;
	switch (ContainerDataAsset->SlotType)
	{
	case EContainerSlotType::Backpack: TargetSlot = &EquippedBackpack; break;
	case EContainerSlotType::Rig:      TargetSlot = &EquippedRig;      break;
	default:
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("EquipInstance: %s declares a slot that cannot be worn."),
			*ContainerDataAsset->GetName());
		return false;
	}

	if (TargetSlot->InstanceID.IsValid())
	{
		UE_LOG(LogSomnusInventory, Log,
			TEXT("EquipInstance: the slot for %s is already occupied - unequip it first."),
			*ContainerDataAsset->GetName());
		return false;
	}

	const FSomnusItemInstance OldInstance = *TargetSlot;
	Instance.ContainerActor->SetOwner(GetOwner());
	*TargetSlot = Instance;

	HandleEquippedChanged(ContainerDataAsset->SlotType, OldInstance);
	return true;
}

bool USomnusContainerEquipComponent::EquipFrom(USomnusInventoryComponent* Source, FGuid InstanceID, EContainerSlotType TargetSlot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false; 
	if (!Source) return false;
	if (!InstanceID.IsValid()) return false;
	
	FSomnusItemInstance InstanceToEquip;
	if (!Source->FindItemByID(InstanceID, InstanceToEquip))
	{
		return false;
	}

	// The drop landed on one particular panel, so confirm the player asked for the slot this
	// item actually belongs in. EquipInstance routes off the asset alone and would happily
	// wear a backpack dropped on the rig panel - that is the equipment model's rule, and this
	// is the UI's intent. Two different layers, checked in two different places.
	const USomnusContainerDataAsset* ContainerDataAsset = Cast<USomnusContainerDataAsset>(InstanceToEquip.ItemData);
	if (!ContainerDataAsset || ContainerDataAsset->SlotType != TargetSlot)
	{
		return false;
	}

	if (!EquipInstance(InstanceToEquip))
	{
		return false;
	}

	// Add first, remove second, so a refused equip leaves the item exactly where it was.
	// RemoveItem can only miss when the id is no longer in Source at all, which means a
	// listener woken by the equip moved it somewhere else - and undoing the equip at that
	// point would delete the item outright rather than undo anything, since Source no longer
	// holds a copy to fall back on. So this stays loud instead of clever.
	const bool bRemovedFromSource = Source->RemoveItem(InstanceID);
	ensureMsgf(bRemovedFromSource,
		TEXT("EquipFrom: %s was equipped but %s never released it - the item may now exist twice."),
		*ContainerDataAsset->GetName(), *GetNameSafe(Source));

	return true;
}

void USomnusContainerEquipComponent::Server_EquipFrom_Implementation(USomnusInventoryComponent* Source,
	FGuid InstanceID, EContainerSlotType TargetSlot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	EquipFrom(Source, InstanceID, TargetSlot);
}

int32 USomnusContainerEquipComponent::TryAddItemAnywhere(USomnusItemDataAsset* ItemDataAsset, int32 Quantity)
{
	if (!ItemDataAsset || Quantity <= 0) return Quantity;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return Quantity;

	const TArray<FSomnusActiveContainerInfo> ActiveContainerInfos = GetActiveContainers();

	// Merge before placing, across every container rather than within each one. Picking up
	// two bandages tops off the half-empty stack in the backpack instead of opening a fresh
	// pocket cell just because pockets come first in the priority order.
	for (const FSomnusActiveContainerInfo& ContainerInfo : ActiveContainerInfos)
	{
		if (Quantity <= 0) return 0;
		Quantity = ContainerInfo.Container->MergeItemIntoStacks(ItemDataAsset, Quantity);
	}

	// Whatever is left needs new cells, in GetActiveContainers() priority order. The merge
	// inside AddItemAnywhere finds nothing to do by now.
	for (const FSomnusActiveContainerInfo& ContainerInfo : ActiveContainerInfos)
	{
		if (Quantity <= 0) return 0;
		Quantity = ContainerInfo.Container->AddItemAnywhere(ItemDataAsset, Quantity);
	}
	return Quantity;
}

int32 USomnusContainerEquipComponent::TryAddExistingItemAnywhere(FSomnusItemInstance& ItemInstance)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return ItemInstance.StackCount;
	if (!ItemInstance.ItemData || !ItemInstance.InstanceID.IsValid()) return ItemInstance.StackCount;
	if (ItemInstance.StackCount <= 0) return ItemInstance.StackCount;

	// A container can never be a destination for itself - it would end up inside its own
	// grid, taking everything nested in it out of reach. Filtered once, up front, so neither
	// pass below can be the one that forgets the check.
	TArray<USomnusInventoryComponent*> Destinations;
	for (const FSomnusActiveContainerInfo& ContainerInfo : GetActiveContainers())
	{
		if (ItemInstance.ContainerActor && ContainerInfo.Container->GetOwner() == ItemInstance.ContainerActor.Get())
		{
			continue;
		}
		Destinations.Add(ContainerInfo.Container);
	}

	// Same two passes as TryAddItemAnywhere; see the reasoning there. Both Add paths consume
	// from ItemInstance in place, so its StackCount is the running remainder.
	for (USomnusInventoryComponent* Destination : Destinations)
	{
		if (ItemInstance.StackCount <= 0) return 0;
		Destination->MergeExistingItemIntoStacks(ItemInstance);
	}

	for (USomnusInventoryComponent* Destination : Destinations)
	{
		if (ItemInstance.StackCount <= 0) return 0;
		Destination->AddExistingItemAnywhere(ItemInstance);
	}
	return ItemInstance.StackCount;
}

FSomnusItemInstance USomnusContainerEquipComponent::GetEquippedInstance(EContainerSlotType SlotType) const
{
	if (SlotType == EContainerSlotType::Backpack)
	{
		return EquippedBackpack;
	}
	else if (SlotType == EContainerSlotType::Rig)
	{
		return EquippedRig;
	}
	return FSomnusItemInstance();
}

FSomnusItemInstance* USomnusContainerEquipComponent::GetEquippedInstanceMutable(EContainerSlotType SlotType)
{
	if (SlotType == EContainerSlotType::Backpack)
	{
		return &EquippedBackpack;
	}
	else if (SlotType == EContainerSlotType::Rig)
	{
		return &EquippedRig;
	}
	return nullptr;
}

void USomnusContainerEquipComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (EndPlayReason == EEndPlayReason::Type::Destroyed || EndPlayReason == EEndPlayReason::Type::RemovedFromWorld)
	{
		if (Pocket && GetOwner()->HasAuthority())
		{
			GetWorld()->DestroyActor(Pocket);
		}
	}
}

void USomnusContainerEquipComponent::OnRep_Pocket()
{
	OnActiveContainersChangedDelegate.Broadcast();
}

void USomnusContainerEquipComponent::OnRep_EquippedRig(FSomnusItemInstance Old)
{
	HandleEquippedChanged(EContainerSlotType::Rig, Old);
}

void USomnusContainerEquipComponent::OnRep_EquippedBackpack(FSomnusItemInstance Old)
{
	HandleEquippedChanged(EContainerSlotType::Backpack, Old);
}

void USomnusContainerEquipComponent::HandleEquippedChanged(EContainerSlotType SlotType, const FSomnusItemInstance& OldInstance)
{
	// The set of active containers just changed. Teardown for OldInstance hooks up here.

	// Broadcast last, so listeners see the finished state rather than a half-updated one, and
	// from here rather than from the OnReps - a server never receives those, so on a listen
	// server the host's own UI would be the one window that never refreshed.
	OnActiveContainersChangedDelegate.Broadcast();
}

bool USomnusContainerEquipComponent::UnequipTo(EContainerSlotType SlotType, USomnusInventoryComponent* Destination,
	int32 TopLeftX, int32 TopLeftY, bool bRotated)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!Destination) return false;

	FSomnusItemInstance* Slot = GetEquippedInstanceMutable(SlotType);
	if (!Slot) return false;
	if (!Slot->InstanceID.IsValid()) return false;
	if (!Slot->ItemData) return false;

	// Hand the placement a copy, never the slot itself. AddExistingItemAt consumes what it is
	// given in place, so aliasing the slot would spend a replicated member down before we know
	// whether the move is even allowed - and no OnRep would ever correct it, because the server
	// would be the one holding the wrong value. Same reason MoveItemFrom copies out of Source.
	FSomnusItemInstance ItemToMove = *Slot;

	// Containers are MaxStackCount == 1, so the placement takes the whole thing or none of it -
	// there is no partial merge that could legitimately leave a remainder. Anything non-zero
	// means the destination refused (no room, or it is storage this very item provides), and
	// the slot keeps what it had. Give containers a stack count above 1 and this stops holding.
	const int32 LeftOver = Destination->AddExistingItemAt(ItemToMove, TopLeftX, TopLeftY, bRotated);
	if (LeftOver != 0)
	{
		return false;
	}

	// Read the slot, not ItemToMove: the placement above spent that copy down to zero.
	const FSomnusItemInstance OldInstance = *Slot;
	*Slot = FSomnusItemInstance();

	// No SetOwner here, and that is a decision rather than an omission. The destination grid
	// belongs to this character or to one of its container actors, so the storage's root holder
	// comes out as the actor it already had. The day equipment can be unequipped straight into
	// a corpse or a world container, this is the line that has to start following it.

	HandleEquippedChanged(SlotType, OldInstance);
	return true;
}

bool USomnusContainerEquipComponent::DropItem(FGuid InstanceID)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return false;
	if (!InstanceID.IsValid()) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	if (!PickupActorClass)
	{
		UE_LOG(LogSomnusInventory, Error,
			TEXT("DropItem: %s has no PickupActorClass assigned - nothing this character carries can be dropped."),
			*OwnerActor->GetName());
		return false;
	}

	// An item is in one of two places, and only one of them is a grid.
	FSomnusItemInstance Dropping;
	USomnusInventoryComponent* HoldingContainer = FindContainerHolding(InstanceID, Dropping);

	// Worn equipment lives in a slot rather than in any grid, so the search above cannot see it.
	// An empty slot holds an invalid id, which the valid-id guard at the top already excluded
	// from matching.
	FSomnusItemInstance* WornSlot = nullptr;
	EContainerSlotType WornSlotType = EContainerSlotType::Pockets;
	if (!HoldingContainer)
	{
		for (const EContainerSlotType SlotType : { EContainerSlotType::Rig, EContainerSlotType::Backpack })
		{
			FSomnusItemInstance* Slot = GetEquippedInstanceMutable(SlotType);
			if (Slot && Slot->InstanceID == InstanceID)
			{
				Dropping = *Slot;
				WornSlot = Slot;
				WornSlotType = SlotType;
				break;
			}
		}
	}

	if (!HoldingContainer && !WornSlot)
	{
		return false;
	}

	// Scattered, and turned to face anywhere. Emptying a container drops item after item onto
	// one spot otherwise, where identical orientations read as a single object and the bodies
	// spend their first second shoving each other out of the pile. Server-only randomness: the
	// resulting transform is what replicates, so no client ever rolls its own.
	const FVector2D Scatter = FMath::RandPointInCircle(DropScatterRadius);
	const FVector DropLocation = OwnerActor->GetActorLocation()
		+ OwnerActor->GetActorForwardVector() * DropDistance
		+ FVector(Scatter.X, Scatter.Y, 0.f);

	const FRotator DropRotation(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
	const FTransform SpawnTransform(DropRotation, DropLocation);

	// Spawn first, release second - the rule EquipFrom and MoveItemFrom follow for the same
	// reason. Taking the item away before its replacement exists destroys it outright whenever
	// the spawn is the thing that fails.
	//
	// Deferred, because a plain SpawnActor runs BeginPlay before returning: the pickup would
	// resolve its mesh against an empty PickupItem, land on the fallback cube, and start
	// simulating that body before ever hearing what it is actually carrying.
	ASomnusPickupActor* Pickup = World->SpawnActorDeferred<ASomnusPickupActor>(
		PickupActorClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Pickup)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("DropItem: failed to spawn %s - %s keeps the item."),
			*GetNameSafe(PickupActorClass), *OwnerActor->GetName());
		return false;
	}

	// Hands over the instance and, with it, ownership of any storage it carries.
	Pickup->InitializeFromInstance(&Dropping);
	Pickup->FinishSpawning(SpawnTransform);

	if (WornSlot)
	{
		// Same teardown UnequipTo does, minus a destination grid: clear the slot, then let the
		// listeners see the finished state.
		const FSomnusItemInstance OldInstance = *WornSlot;
		*WornSlot = FSomnusItemInstance();
		HandleEquippedChanged(WornSlotType, OldInstance);
	}
	else
	{
		// Loud rather than clever, for the reason EquipFrom spells out: by this point the pickup
		// already holds the item, so a miss here means it exists twice and no undo can help.
		const bool bRemoved = HoldingContainer->RemoveItem(InstanceID);
		ensureMsgf(bRemoved,
			TEXT("DropItem: %s was handed to a pickup but %s never released it - the item may now exist twice."),
			*GetNameSafe(Dropping.ItemData), *GetNameSafe(HoldingContainer));
	}

	return true;
}

void USomnusContainerEquipComponent::Server_UnequipTo_Implementation(EContainerSlotType SlotType,
                                                                     USomnusInventoryComponent* Destination, int32 TopLeftX, int32 TopLeftY, bool bRotated)
{
	UnequipTo(SlotType, Destination, TopLeftX, TopLeftY, bRotated);
}
