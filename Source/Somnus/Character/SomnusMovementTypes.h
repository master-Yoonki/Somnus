// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SomnusMovementTypes.generated.h"

/**
 * How the character is moving, as the character understands it. Declared beside the character
 * rather than in an animation header because the character owns this state and animation only
 * reads it - an anim instance being replaced should not take these with it.
 */
UENUM(BlueprintType)
enum class ESomnusMovementState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Moving		UMETA(DisplayName = "Moving")
};

UENUM(BlueprintType)
enum class ESomnusMovementMode : uint8
{
	OnGround	UMETA(DisplayName = "OnGround"),
	InAir		UMETA(DisplayName = "InAir"),
	Sliding		UMETA(DisplayName = "Sliding"),
	Traversing	UMETA(DisplayName = "Traversing"),
};

UENUM(BlueprintType)
enum class ESomnusGait : uint8
{
	Walk	UMETA(DisplayName = "Walk"),
	Run		UMETA(DisplayName = "Run"),
	Sprint	UMETA(DisplayName = "Sprint"),
};
