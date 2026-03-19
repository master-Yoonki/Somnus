// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SomnusGameplayAbility.generated.h"

/**
 * Base GameplayAbility for Project Somnus.
 * All project abilities should inherit from this class.
 */
UCLASS(Abstract)
class SOMNUS_API USomnusGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USomnusGameplayAbility();
};
