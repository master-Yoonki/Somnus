// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SomnusItemInstance.generated.h"

struct FSomnusInventoryList;

/**
 * A single entry in the inventory list.
 */
USTRUCT(BlueprintType)
struct FSomnusItemInstance : public FFastArraySerializerItem
{
	GENERATED_BODY()

	void PreReplicatedRemove(const struct FSomnusInventoryList& InArraySerializer);
	void PostReplicatedAdd(const struct FSomnusInventoryList& InArraySerializer);
	void PostReplicatedChange(const struct FSomnusInventoryList& InArraySerializer);

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<class USomnusItemDataAsset> ItemData;
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 StackCount = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 CurrentDurability = -1;
	UPROPERTY(BlueprintReadOnly, Category = "Item|Grid")
	FIntPoint GridPosition = FIntPoint(-1, -1);
	UPROPERTY(BlueprintReadOnly, Category = "Item|Grid")
	bool bRotated = false;
	// server-generated; primary key for moves/removes 
	UPROPERTY(BlueprintReadOnly, Category = "Item|Grid")
	FGuid InstanceID;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class ASomnusContainerActor> ContainerActor = nullptr;
};

/**
 * Wrapper for the array of item instances.
 */
USTRUCT()
struct FSomnusInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FSomnusItemInstance> Items;
	
	// Internal back-pointer — set by the component, not replicated, not exposed to BP.
	UPROPERTY(NotReplicated)
	TObjectPtr<class USomnusInventoryComponent> OwnerComponent = nullptr;
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FSomnusItemInstance, FSomnusInventoryList>
		( Items, DeltaParms, *this );
	}
};

template<>
struct TStructOpsTypeTraits<FSomnusInventoryList> : public TStructOpsTypeTraitsBase2<FSomnusInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};
