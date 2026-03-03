// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/SomnusWeapon.h"
#include "SomnusMeleeWeapon.generated.h"

/**
 * 
 */
UCLASS()
class SOMNUS_API ASomnusMeleeWeapon : public ASomnusWeapon
{
	GENERATED_BODY()
	
public:
	ASomnusMeleeWeapon();

	// --- Getters for the AnimNotifyState to read ---
	FName GetBaseSocketName() const { return BaseSocketName; }
	FName GetTipSocketName() const { return TipSocketName; }
	float GetTraceRadius() const { return TraceRadius; }
	class UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

protected:
	// Socket at the handle
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace Data")
	FName BaseSocketName;

	// Socket at the tip/end of the blade
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace Data")
	FName TipSocketName;

	// Thickness of the weapon for the sphere trace
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace Data")
	float TraceRadius;
};
