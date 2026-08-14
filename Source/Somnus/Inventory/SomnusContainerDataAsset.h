// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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
	
	// Which slot this goes in is not declared here any more. ItemTag already said it - a rig is
	// tagged Item.Equipment.Container.Rig and the rig slot accepts exactly that - and two fields
	// answering one question is two fields that can disagree.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container|Configuration")
	FGameplayTagContainer AcceptedTags;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container|Configuration")
	TArray<FIntPoint> CompartmentSizes;
};
