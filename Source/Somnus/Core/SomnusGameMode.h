// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SomnusGameMode.generated.h"

/**
 * Core game mode for Project Somnus.
 * Manages default classes and match rules.
 */
UCLASS()
class SOMNUS_API ASomnusGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ASomnusGameMode();
};
