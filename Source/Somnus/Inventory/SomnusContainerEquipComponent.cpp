// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusContainerEquipComponent.h"

#include "SomnusContainerActor.h"
#include "SomnusContainerDataAsset.h"
#include "SomnusInventoryComponent.h"
#include "SomnusItemDataAsset.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
USomnusContainerEquipComponent::USomnusContainerEquipComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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
	OnActiveContainersChangedDelegate.Broadcast();
}

void USomnusContainerEquipComponent::OnRep_EquippedBackpack(FSomnusItemInstance Old)
{
	HandleEquippedChanged(EContainerSlotType::Backpack, Old);
	OnActiveContainersChangedDelegate.Broadcast();
}

void USomnusContainerEquipComponent::HandleEquippedChanged(EContainerSlotType SlotType, const FSomnusItemInstance& OldInstance)
{
	// The set of active containers just changed. UI refresh hooks up here once the
	// inventory widgets exist; OldInstance is what needs tearing down.
}
