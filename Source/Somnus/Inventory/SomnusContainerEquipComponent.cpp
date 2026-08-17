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
	bWantsInitializeComponent = true;
	PickupActorClass = ASomnusPickupActor::StaticClass();

	PocketSlot = CreateDefaultSubobject<USomnusEquipmentSlotComponent>("PocketSlot");
	BackpackSlot = CreateDefaultSubobject<USomnusEquipmentSlotComponent>("BackpackSlot");
	RigSlot = CreateDefaultSubobject<USomnusEquipmentSlotComponent>("RigSlot");
}

void USomnusContainerEquipComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Nothing. The slots are constructor subobjects that exist on every machine already, and what
	// is in them replicates as the grid contents it is.
}

TArray<USomnusEquipmentSlotComponent*> USomnusContainerEquipComponent::GetStorageSlots() const
{
	return { PocketSlot, BackpackSlot, RigSlot };
}

TArray<FSomnusActiveContainerInfo> USomnusContainerEquipComponent::GetActiveContainers() const
{
	TArray<FSomnusActiveContainerInfo> ContainerInfos;

	// One slot at a time in GetStorageSlots order, so each container's compartments come out as a
	// contiguous run in SlotIndex order and pockets stay ahead of the backpack. The UI relies on
	// the first, auto-placement on the second - do not interleave, do not sort.
	//
	// The slots themselves are deliberately not entries. This answers "what storage can hold a
	// loose item", which is what TryAddItemAnywhere walks, and a slot listed here would swallow
	// the first bandage that did not fit a pocket. The panels ask for slots by tag instead.
	for (const USomnusEquipmentSlotComponent* Slot : GetStorageSlots())
	{
		if (!Slot)
		{
			continue;
		}

		const FSomnusItemInstance Worn = GetEquippedInstance(Slot->GetSlotTag());
		if (!Worn.ContainerActor)
		{
			continue;
		}

		FSomnusActiveContainerInfo Info;
		Info.SlotTag = Slot->GetSlotTag();
		Info.SourceItemData = Worn.ItemData;

		const TArray<USomnusInventoryComponent*> Compartments = Worn.ContainerActor->GetCompartments();
		for (int32 i = 0; i < Compartments.Num(); ++i)
		{
			if (Compartments[i])
			{
				Info.SlotIndex = i;
				Info.Container = Compartments[i];
				ContainerInfos.Add(Info);
			}
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

ASomnusContainerActor* USomnusContainerEquipComponent::GetEquippedContainer(FGameplayTag SlotTag) const
{
	return GetEquippedInstance(SlotTag).ContainerActor;
}

void USomnusContainerEquipComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Not BeginPlay: this component grants starting equipment there, and the weapon slots on the
	// equipment component next door have to already know what they take by then. Every component
	// on an actor is initialised before any of them begins play, so this is the one place the two
	// cannot race.
	//
	// On every machine, not just the authority: a client that does not know what a slot admits
	// waves every drag through until the server says otherwise.
	if (PocketSlot)
	{
		// The one locked slot. Everything else about it is a rig: a container item, worn, with
		// compartments that come from its data asset.
		PocketSlot->InitializeSlot(SomnusTags::Equipment_Slot_Pockets,
			FGameplayTagContainer(SomnusTags::Item_Equipment_Container_Pockets),
			FText::GetEmpty(), /*bLocked*/ true);
	}

	if (BackpackSlot)
	{
		BackpackSlot->InitializeSlot(SomnusTags::Equipment_Slot_Backpack,
			FGameplayTagContainer(SomnusTags::Item_Equipment_Container_Backpack),
			NSLOCTEXT("Somnus", "SlotBackpack", "BACKPACK"));
	}

	if (RigSlot)
	{
		RigSlot->InitializeSlot(SomnusTags::Equipment_Slot_Rig,
			FGameplayTagContainer(SomnusTags::Item_Equipment_Container_Rig),
			NSLOCTEXT("Somnus", "SlotRig", "RIG"));
	}
}

// Called when the game starts
void USomnusContainerEquipComponent::BeginPlay()
{
	Super::BeginPlay();

	// Before the authority gate on purpose: a client's panels have to redraw when what is worn
	// changes, and a client never reaches anything past it.
	for (USomnusEquipmentSlotComponent* Slot : GetStorageSlots())
	{
		if (!Slot)
		{
			continue;
		}
		Slot->OnItemAddedDelegate.AddDynamic(this, &USomnusContainerEquipComponent::HandleSlotContentsChanged);
		Slot->OnItemRemovedDelegate.AddDynamic(this, &USomnusContainerEquipComponent::HandleSlotContentsChanged);
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// Only what cannot be taken off. The rest waits for GrantStartingKit, which the game mode calls
	// on a first spawn and not on a respawn - a pawn that came back from the dead reaches this line
	// looking exactly like one that never died, so the distinction cannot be made here.
	GrantEquipment(/*bLockedSlotsOnly*/ true);
}

void USomnusContainerEquipComponent::GrantStartingKit()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (bStartingKitGranted)
	{
		return;
	}
	bStartingKitGranted = true;

	GrantEquipment(/*bLockedSlotsOnly*/ false);

	// Loose items go in last, so they can land in the storage the call above just granted.
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

void USomnusContainerEquipComponent::GrantEquipment(bool bLockedSlotsOnly)
{
	for (const TObjectPtr<USomnusContainerDataAsset>& EquipmentData : DefaultEquipment)
	{
		if (!EquipmentData)
		{
			continue;
		}

		// Which half this is decided by asking where the thing would go, not by a second list to
		// keep in step with the first. Anything already worn leaves no empty slot that admits it,
		// so FindSlotFor returns null and the second pass skips what the first granted.
		const USomnusEquipmentSlotComponent* Slot = FindSlotFor(EquipmentData);
		if (!Slot || Slot->IsLocked() != bLockedSlotsOnly)
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
}

USomnusEquipmentSlotComponent* USomnusContainerEquipComponent::FindSlotFor(USomnusItemDataAsset* ItemData) const
{
	if (!ItemData || !GetOwner())
	{
		return nullptr;
	}

	// Routed by asking each slot rather than by looking the item's kind up in a table. AcceptsItem
	// is the same question a drag asks, so granting and dragging can never disagree about where
	// something belongs, and a new kind of worn thing needs no entry anywhere. Every slot on the
	// actor, not only this component's, because a slot is declared by whoever owns its meaning.
	TArray<USomnusEquipmentSlotComponent*> Slots;
	GetOwner()->GetComponents<USomnusEquipmentSlotComponent>(Slots);

	for (USomnusEquipmentSlotComponent* Slot : Slots)
	{
		// Empty as well as willing: two weapon slots take the same things, so a kit granting two
		// weapons has to land the second one somewhere other than where the first went.
		if (Slot && Slot->AcceptsItem(ItemData) && Slot->GetAllItems().IsEmpty())
		{
			return Slot;
		}
	}

	return nullptr;
}

bool USomnusContainerEquipComponent::EquipInstance(const FSomnusItemInstance& Instance)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	// Silently, because "not something I wear" is the ordinary answer now. Anything picked up off
	// the floor is offered here first, and most of it is bandages.
	USomnusContainerDataAsset* ContainerDataAsset = Cast<USomnusContainerDataAsset>(Instance.ItemData);
	if (!ContainerDataAsset)
	{
		return false;
	}

	if (!Instance.ContainerActor)
	{
		UE_LOG(LogSomnusInventory, Error,
			TEXT("EquipInstance: %s has no container actor - refusing to equip storage that does not exist."),
			*ContainerDataAsset->GetName());
		return false;
	}

	USomnusEquipmentSlotComponent* TargetSlot = FindSlotFor(ContainerDataAsset);
	if (!TargetSlot)
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("EquipInstance: nothing on %s wears %s."),
			*GetOwner()->GetName(), *ContainerDataAsset->GetName());
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

FSomnusItemInstance USomnusContainerEquipComponent::GetEquippedInstance(FGameplayTag SlotTag) const
{
	const USomnusEquipmentSlotComponent* Slot =
		USomnusEquipmentSlotComponent::FindSlot(GetOwner(), SlotTag);
	if (!Slot)
	{
		return FSomnusItemInstance();
	}

	// A slot holds one item or none, so the first entry is the whole answer.
	const TArray<FSomnusItemInstance> Worn = Slot->GetAllItems();
	return Worn.Num() > 0 ? Worn[0] : FSomnusItemInstance();
}

void USomnusContainerEquipComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// The pocket container actor used to be torn down here by name. It is an ordinary worn item
	// now, so it leaks exactly the way a rig does - which is the same one bug rather than two, and
	// gets fixed once, wherever worn containers learn to clean up after their wearer.
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
	// Every slot on the character, not a hard-coded pair: weapon slots are somewhere else entirely
	// and armour will be too, and a drop that only knew about the rig and the backpack would go on
	// quietly failing for each of them.
	USomnusEquipmentSlotComponent* WornSlot = nullptr;
	if (!HoldingContainer)
	{
		TArray<USomnusEquipmentSlotComponent*> Slots;
		OwnerActor->GetComponents<USomnusEquipmentSlotComponent>(Slots);

		for (USomnusEquipmentSlotComponent* Slot : Slots)
		{
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
