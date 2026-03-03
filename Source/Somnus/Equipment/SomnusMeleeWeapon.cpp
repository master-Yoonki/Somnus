// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/SomnusMeleeWeapon.h"

ASomnusMeleeWeapon::ASomnusMeleeWeapon()
{
	// No Tick needed! Maximum performance.
	PrimaryActorTick.bCanEverTick = false;

	// Default values
	BaseSocketName = FName("BaseSocket");
	TipSocketName = FName("TipSocket");
	TraceRadius = 15.0f;
}
