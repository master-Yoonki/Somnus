// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/SomnusItemInstance.h"

#include "SomnusContainerActor.h"
#include "SomnusContainerDataAsset.h"
#include "SomnusItemDataAsset.h"
#include "Inventory/SomnusInventoryComponent.h"

FSomnusItemInstance FSomnusItemInstance::MakeItemInstance(UWorld* World, USomnusItemDataAsset* Data, int32 Quantity)
{
	FSomnusItemInstance Instance;

	if (!World || !Data || Quantity <= 0)
	{
		return Instance;
	}

	Instance.ItemData = Data;
	Instance.StackCount = Quantity;
	Instance.InstanceID = FGuid::NewGuid();

	if (USomnusContainerDataAsset* ContainerDataAsset = Cast<USomnusContainerDataAsset>(Data))
	{
		ASomnusContainerActor* ContainerActor = World->SpawnActor<ASomnusContainerActor>();
		if (!ContainerActor)
		{
			UE_LOG(LogSomnusInventory, Error,
				TEXT("Failed to spawn the container actor for %s."), *Data->GetName());
			return FSomnusItemInstance();
		}

		ContainerActor->Initialize(ContainerDataAsset);
		Instance.ContainerActor = ContainerActor;
	}

	return Instance;
}

void FSomnusItemInstance::PreReplicatedRemove(const FSomnusInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnItemRemoved(*this);
	}
}

void FSomnusItemInstance::PostReplicatedAdd(const FSomnusInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnItemAdded(*this);
	}
}

void FSomnusItemInstance::PostReplicatedChange(const FSomnusInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnItemChanged(*this);
	}
}
