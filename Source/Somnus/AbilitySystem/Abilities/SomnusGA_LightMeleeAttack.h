// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SomnusGA_MeleeAttack.h"
#include "SomnusGA_LightMeleeAttack.generated.h"

/**
 * Light melee attack — activates while aiming (State.Aiming required).
 * Quick, precise strikes used in aim mode.
 */
UCLASS()
class SOMNUS_API USomnusGA_LightMeleeAttack : public USomnusGA_MeleeAttack
{
	GENERATED_BODY()

public:
	USomnusGA_LightMeleeAttack();
};
