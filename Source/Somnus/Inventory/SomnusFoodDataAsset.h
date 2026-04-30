// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/SomnusItemDataAsset.h"
#include "SomnusFoodDataAsset.generated.h"

UCLASS(BlueprintType)
class SOMNUS_API USomnusFoodDataAsset : public USomnusItemDataAsset
{
	GENERATED_BODY()

public:
	USomnusFoodDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Food|Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxFreshness = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Food|Stats", meta = (ClampMin = "0.0"))
	float HungerRestore = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Food|Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SicknessChance = 0.f;
};
