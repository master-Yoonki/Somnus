// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SomnusGA_MeleeAttack.h"
#include "SomnusGA_HeavyMeleeAttack.generated.h"

/**
 * Heavy melee attack — activates when NOT aiming.
 * Powerful swings used outside of aim mode.
 */
UCLASS()
class SOMNUS_API USomnusGA_HeavyMeleeAttack : public USomnusGA_MeleeAttack
{
	GENERATED_BODY()

public:
	USomnusGA_HeavyMeleeAttack();
};
