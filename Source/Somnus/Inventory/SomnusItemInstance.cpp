// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/SomnusItemInstance.h"
#include "Inventory/SomnusInventoryComponent.h"

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
