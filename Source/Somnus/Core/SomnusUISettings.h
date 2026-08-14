// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SomnusUISettings.generated.h"

/**
 * Numbers the interface is laid out from, held once for the whole project.
 *
 * Settings rather than a variable on some actor because these belong to no instance: every panel
 * in every window reads the same value, and a widget that has to reach an actor to find out how
 * wide a cell is has taken on a dependency for a constant. Nothing here is replicated or
 * per-player, which is the test for whether a value belongs in this class at all.
 *
 * Edited under Project Settings > Game > Somnus UI, saved to DefaultGame.ini.
 */
UCLASS(config = Game, defaultconfig, BlueprintType, meta = (DisplayName = "Somnus UI"))
class SOMNUS_API USomnusUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USomnusUISettings();

	/** The settings object itself, so a new field below needs no accessor of its own. */
	UFUNCTION(BlueprintPure, Category = "Somnus|UI", meta = (DisplayName = "Get Somnus UI Settings"))
	static USomnusUISettings* Get();

	/** Side of one inventory grid cell, in slate units. Grids, slots and item widgets all size
	 *  themselves from this, so it is the one number that has to agree across every panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Inventory", meta = (ClampMin = "1.0"))
	float CellSize = 64.0f;
};
