// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

/**
 * Readable names for the project's custom collision channels. The engine only ever exposes
 * these as ECC_GameTraceChannelN, so the mapping lives here instead of being spelled out at
 * every trace - reordering a channel in Project Settings is then a one-line edit.
 *
 * Keep in sync with [/Script/Engine.CollisionProfile] in Config/DefaultEngine.ini.
 */
namespace SomnusCollision
{
	/** Loose world loot, and anything else an interact trace should find. Its default response
	 *  is Ignore, so only actors that opt in are ever hit and the trace needs no filtering of
	 *  its own - unlike Visibility, which the ground-distance trace also rides on. */
	inline constexpr ECollisionChannel Interaction = ECC_GameTraceChannel1;

	/** Weapon damage traces (melee sweeps, future hitscan). Default response is Block, but
	 *  the Pawn profile opts out so a swing passes the capsule and lands on the character
	 *  mesh instead - that is what keeps the reported bone, and the hit reaction, meaningful. */
	inline constexpr ECollisionChannel Weapon = ECC_GameTraceChannel2;

	/** Object type of zombie capsules. Blocks by default, so zombies stay solid to the world and to
	 *  each other, but the Pawn profile overlaps it: the player wades into a crowd instead of being
	 *  walled off, and the zombies are shoved aside rather than stood on. */
	inline constexpr ECollisionChannel Zombie = ECC_GameTraceChannel3;

	/** The collision profile zombie capsules use. */
	inline const FName ZombieProfile = TEXT("Zombie");
}
