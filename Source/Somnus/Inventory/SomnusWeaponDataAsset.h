// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/SomnusItemDataAsset.h"
#include "SomnusWeaponDataAsset.generated.h"

UCLASS(BlueprintType)
class SOMNUS_API USomnusWeaponDataAsset : public USomnusItemDataAsset
{
	GENERATED_BODY()

public:
	USomnusWeaponDataAsset();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class ASomnusWeapon> WeaponActorClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats", meta = (ClampMin = "1"))
	int32 MaxDurability = 100;
};
