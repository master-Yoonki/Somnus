// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SomnusItemTypes.generated.h"

class USomnusInventoryComponent;
class USomnusItemDataAsset;

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None         UMETA(DisplayName = "None"),
	Weapon       UMETA(DisplayName = "Weapon"),
	Consumable   UMETA(DisplayName = "Consumable"),
	Construction UMETA(DisplayName = "Construction"),
	Medical      UMETA(DisplayName = "Medical"),
	Food         UMETA(DisplayName = "Food"),
	Equipment    UMETA(DisplayName = "Equipment"),
};

UENUM(BlueprintType)
enum class EMedicalEffectType : uint8
{
	Heal         UMETA(DisplayName = "Heal"),
	StopBleeding UMETA(DisplayName = "Stop Bleeding"),
	CurePoison   UMETA(DisplayName = "Cure Poison"),
	BoostSanity  UMETA(DisplayName = "Boost Sanity")
};


USTRUCT(BlueprintType)
struct FSomnusActiveContainerInfo
{
	GENERATED_BODY()

	/** Which slot the storage hangs off, as a tag rather than an enum. An enum had to name every
	 *  slot in one list, which is how a value called Pockets ended up in it before pockets were a
	 *  slot at all, and every new slot meant a new value and a new case in whatever switched on
	 *  it. Tags nest instead: Equipment.Slot.Weapon.Primary is a weapon slot without anyone
	 *  writing that down twice. */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag SlotTag;

	// if multi - compartment container
	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USomnusInventoryComponent> Container = nullptr;

	/** The item this storage came from, for UI that needs to name or picture it. Every
	 *  compartment of one container reports the same asset, and pockets are an item like the
	 *  rest now, so there is no entry without one. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USomnusItemDataAsset> SourceItemData = nullptr;
};