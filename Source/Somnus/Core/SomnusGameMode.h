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

	/** Takes over a body's lifetime so it can be looted. Once the budget is exceeded the oldest
	 *  corpse is destroyed, which is why this is here rather than a lifespan on the pawn: the cap
	 *  is a property of the match, not of any one body. Takes APawn so zombies use it unchanged. */
	void RegisterCorpse(APawn* Corpse);

protected:
	/** Every way a controller gets a body arrives here - the first spawn, the one a starting match
	 *  issues, and RequestRespawn - which is why the starting kit is handed out from this override
	 *  and gated on the player state rather than hung off whichever of those paths looks like the
	 *  first one. */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** How many bodies may exist at once. */
	UPROPERTY(EditDefaultsOnly, Category = "Corpses", meta = (ClampMin = "1"))
	int32 MaxCorpses = 16;

private:
	/** Oldest first. */
	UPROPERTY()
	TArray<TObjectPtr<APawn>> Corpses;
};
