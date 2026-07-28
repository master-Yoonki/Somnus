// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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


// For player equipped container
UENUM(BlueprintType)
enum class EContainerSlotType : uint8
{
	Pockets		 UMETA(DisplayName = "Pockets"),
	Backpack	 UMETA(DisplayName = "Backpack"),
	Rig			 UMETA(DisplayName = "Rig")
};

USTRUCT(BlueprintType)
struct FSomnusActiveContainerInfo
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	EContainerSlotType SlotType = EContainerSlotType::Pockets;
	
	// if multi - compartment container
	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USomnusInventoryComponent> Container = nullptr;

	/** The item this storage came from, for UI that needs to name or picture it. Every
	 *  compartment of one container reports the same asset. Never null for a well-formed
	 *  entry - pockets report their PocketData, worn slots report the equipped item. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USomnusItemDataAsset> SourceItemData = nullptr;
};