// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusContainerEquipComponent.h"

#include "SomnusContainerActor.h"
#include "SomnusContainerDataAsset.h"
#include "SomnusInventoryComponent.h"
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
	
	auto AddCompartmentsInfo = [this, &ContainerInfos](EContainerSlotType SlotType, TArray<USomnusInventoryComponent*>& Compartments)
	{
		const int32 NumCompartments = Compartments.Num();
		FSomnusActiveContainerInfo CompartmentInfo;
		CompartmentInfo.SlotType = SlotType;
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
		AddCompartmentsInfo(EContainerSlotType::Pockets, Compartments);
	}
	
	if (EquippedBackpack.InstanceID.IsValid())
	{
		if (TObjectPtr<ASomnusContainerActor> Container = EquippedBackpack.ContainerActor)
		{
			TArray<USomnusInventoryComponent*> Compartments = Container->GetCompartments();
			AddCompartmentsInfo(EContainerSlotType::Backpack, Compartments);
		}
	}
	
	if (EquippedRig.InstanceID.IsValid())
	{
		if (TObjectPtr<ASomnusContainerActor> Container = EquippedRig.ContainerActor)
		{
			TArray<USomnusInventoryComponent*> Compartments = Container->GetCompartments();
			AddCompartmentsInfo(EContainerSlotType::Rig, Compartments);
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

	Pocket->Initialize(PocketData);

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
	*TargetSlot = Instance;

	HandleEquippedChanged(ContainerDataAsset->SlotType, OldInstance);
	return true;
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
	// The set of active containers just changed. UI refresh hooks up here once the
	// inventory widgets exist; OldInstance is what needs tearing down.
}
