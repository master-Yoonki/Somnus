// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SomnusItemDataAsset.h"
#include "Engine/DataAsset.h"
#include "Inventory/SomnusItemTypes.h"
#include "SomnusContainerDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class SOMNUS_API USomnusContainerDataAsset : public USomnusItemDataAsset
{
	GENERATED_BODY()

public:
	USomnusContainerDataAsset();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container|Configuration")
	EContainerSlotType SlotType = EContainerSlotType::Backpack;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container|Configuration")
	TArray<FIntPoint> CompartmentSizes;
};
