// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SomnusGE_MeleeDamage.generated.h"

/**
 * Instant damage GE that sets IncomingDamage on the target.
 * PostGameplayEffectExecute on USomnusAttributeSet converts this to Health reduction.
 */
UCLASS()
class SOMNUS_API USomnusGE_MeleeDamage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USomnusGE_MeleeDamage();
};
