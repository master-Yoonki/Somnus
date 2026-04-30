// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/SomnusItemDataAsset.h"
#include "SomnusMedicalDataAsset.generated.h"

UCLASS(BlueprintType)
class SOMNUS_API USomnusMedicalDataAsset : public USomnusItemDataAsset
{
	GENERATED_BODY()

public:
	USomnusMedicalDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical|Stats", meta = (ClampMin = "0.0"))
	float HealAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical|Stats")
	EMedicalEffectType EffectType = EMedicalEffectType::Heal;
};
