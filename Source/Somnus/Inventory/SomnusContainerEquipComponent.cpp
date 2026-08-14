// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusContainerEquipComponent.h"

#include "SomnusContainerActor.h"
#include "SomnusContainerDataAsset.h"
#include "SomnusInventoryComponent.h"
#include "Core/SomnusGameplayTags.h"
#include "Inventory/SomnusEquipmentSlotComponent.h"
#include "Inventory/SomnusItemDataAsset.h"
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
	RigSlot = CreateDefaultSubobject<USomnusEquipmentSlotComponent>("RigSlot");
	BackpackSlot = CreateDefaultSubobject<USomnusEquipmentSlotComponent>("BackpackSlot");
	// ...
}

void USomnusContainerEquipComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Only Pocket. The two slots are constructor subobjects that exist on every machine already,
	// and what is in them replicates as the grid contents it is.
	DOREPLIFETIME(USomnusContainerEquipComponent, Pocket);
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

	// The slots themselves are deliberately not listed here. This answers "what storage can hold a
	// loose item", which is what TryAddItemAnywhere walks - and a rig slot that appeared in it
	// would swallow the first bandage that did not fit a pocket. The UI needs the slots too, but
	// that is a different question and gets its own answer when the panels are built.
	const FSomnusItemInstance WornBackpack = GetEquippedInstance(EContainerSlotType::Backpack);
	if (WornBackpack.ContainerActor)
	{
		TArray<USomnusInventoryComponent*> Compartments = WornBackpack.ContainerActor->GetCompartments();
		AddCompartmentsInfo(EContainerSlotType::Backpack, WornBackpack.ItemData, Compartments);
	}

	const FSomnusItemInstance WornRig = GetEquippedInstance(EContainerSlotType::Rig);
	if (WornRig.ContainerActor)
	{
		TArray<USomnusInventoryComponent*> Compartments = WornRig.ContainerActor->GetCompartments();
		AddCompartmentsInfo(EContainerSlotType::Rig, WornRig.ItemData, Compartments);
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

class ASomnusContainerActor* USomnusContainerEquipComponent::GetEquippedContainer(EContainerSlotType SlotType) const
{
	if (SlotType == EContainerSlotType::Pockets)
	{
		return Pocket;
	}
	else if (USomnusEquipmentSlotComponent* EquipmentSlot = GetSlot(SlotType))
	{
		if (!EquipmentSlot->GetAllItems().IsEmpty())
		{
			FSomnusItemInstance EquippedItemInstance = EquipmentSlot->GetAllItems()[0];
			return EquippedItemInstance.ContainerActor;
		}
	}
	return nullptr;
}

// Called when the game starts
void USomnusContainerEquipComponent::BeginPlay()
{
	Super::BeginPlay();

	// Before the authority gate on purpose: a client's panels have to redraw when what is worn
	// changes, and a client never reaches anything past it.
	for (USomnusEquipmentSlotComponent* Slot : { RigSlot.Get(), BackpackSlot.Get() })
	{
		if (!Slot)
		{
			continue;
		}
		Slot->OnItemAddedDelegate.AddDynamic(this, &USomnusContainerEquipComponent::HandleSlotContentsChanged);
		Slot->OnItemRemovedDelegate.AddDynamic(this, &USomnusContainerEquipComponent::HandleSlotContentsChanged);
	}
	
	{
		FGameplayTagContainer RigSlotAcceptedTags;
		RigSlotAcceptedTags.AddTag(SomnusTags::Item_Equipment_Container_Rig);
		RigSlot->InitializeAcceptedItemTags(RigSlotAcceptedTags);
	}
	{
		FGameplayTagContainer BackpackSlotAcceptedTags;
		BackpackSlotAcceptedTags.AddTag(SomnusTags::Item_Equipment_Container_Backpack);
		BackpackSlot->InitializeAcceptedItemTags(BackpackSlotAcceptedTags);
	}


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

	USomnusEquipmentSlotComponent* TargetSlot = GetSlot(ContainerDataAsset->SlotType);
	if (!TargetSlot)
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("EquipInstance: %s declares a slot that cannot be worn."),
			*ContainerDataAsset->GetName());
		return false;
	}

	// Occupancy is not checked here any more - the slot refuses on its own, because a slot holding
	// one item is full by definition and CanFitAt says so. A copy, because AddExistingItemAt
	// consumes what it is given and Instance is const for the caller's sake.
	FSomnusItemInstance ItemToEquip = Instance;
	if (TargetSlot->AddExistingItemAt(ItemToEquip, 0, 0, false) != 0)
	{
		UE_LOG(LogSomnusInventory, Log,
			TEXT("EquipInstance: the slot for %s refused it - most likely already occupied."),
			*ContainerDataAsset->GetName());
		return false;
	}

	// After the placement, never before: storage follows its holder so relevancy keeps resolving
	// and permission keeps answering, and a refused equip must leave both pointing where they were.
	Instance.ContainerActor->SetOwner(GetOwner());
	return true;
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
	const USomnusEquipmentSlotComponent* Slot = GetSlot(SlotType);
	if (!Slot)
	{
		return FSomnusItemInstance();
	}

	// A slot holds one item or none, so the first entry is the whole answer.
	const TArray<FSomnusItemInstance> Worn = Slot->GetAllItems();
	return Worn.Num() > 0 ? Worn[0] : FSomnusItemInstance();
}

class USomnusEquipmentSlotComponent* USomnusContainerEquipComponent::GetSlot(EContainerSlotType SlotType) const
{
	if (SlotType == EContainerSlotType::Backpack)
	{
		return BackpackSlot;
	}
	else if (SlotType == EContainerSlotType::Rig)
	{
		return RigSlot;
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

void USomnusContainerEquipComponent::HandleSlotContentsChanged(const FSomnusItemInstance& Item)
{
	// Whatever a slot now holds decides which compartments exist, so the active set just changed.
	// Reached by delegate on every machine, which is what makes the old pairing of an OnRep for
	// clients and a hand call for the authority unnecessary.
	OnActiveContainersChangedDelegate.Broadcast();
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

	// Worn equipment is in a grid too now, but not one GetActiveContainers lists - the slots stay
	// out of that answer so loose items never land in them - so the search above still cannot see
	// it and this second look is still needed.
	USomnusEquipmentSlotComponent* WornSlot = nullptr;
	if (!HoldingContainer)
	{
		for (const EContainerSlotType SlotType : { EContainerSlotType::Rig, EContainerSlotType::Backpack })
		{
			USomnusEquipmentSlotComponent* Slot = GetSlot(SlotType);
			FSomnusItemInstance Worn;
			if (Slot && Slot->FindItemByID(InstanceID, Worn))
			{
				Dropping = Worn;
				WornSlot = Slot;
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

	// Spawn first, release second - the rule MoveItemFrom follows for the same
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

	// Both places an item can be release it the same way now, so there is one path rather than two.
	// Loud rather than clever: by this point the pickup already holds the item, so a miss here
	// means it exists twice and no undo can help.
	USomnusInventoryComponent* Releasing = WornSlot ? static_cast<USomnusInventoryComponent*>(WornSlot) : HoldingContainer;
	const bool bReleased = Releasing->RemoveItem(InstanceID);
	ensureMsgf(bReleased,
		TEXT("DropItem: %s was handed to a pickup but %s never released it - the item may now exist twice."),
		*GetNameSafe(Dropping.ItemData), *GetNameSafe(Releasing));

	return true;
}
