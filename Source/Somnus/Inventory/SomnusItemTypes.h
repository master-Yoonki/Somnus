// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SomnusItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None         UMETA(DisplayName = "None"),
	Weapon       UMETA(DisplayName = "Weapon"),
	Consumable   UMETA(DisplayName = "Consumable"),
	Construction UMETA(DisplayName = "Construction"),
	Medical      UMETA(DisplayName = "Medical"),
	Food         UMETA(DisplayName = "Food")
};

UENUM(BlueprintType)
enum class EMedicalEffectType : uint8
{
	Heal         UMETA(DisplayName = "Heal"),
	StopBleeding UMETA(DisplayName = "Stop Bleeding"),
	CurePoison   UMETA(DisplayName = "Cure Poison"),
	BoostSanity  UMETA(DisplayName = "Boost Sanity")
};
