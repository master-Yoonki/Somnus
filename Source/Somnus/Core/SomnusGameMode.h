// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SomnusGameMode.generated.h"

/**
 * Core game mode for Project Somnus.
 * Manages default classes and respawn flow.
 */
UCLASS()
class SOMNUS_API ASomnusGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASomnusGameMode();

	// Called by the character when the player requests to respawn (e.g., presses any button while dead)
	void RequestRespawn(AController* Controller);
};
