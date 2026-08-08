// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SomnusGameplayEffectContext.h"

bool FSomnusGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Super::NetSerialize(Ar, Map, bOutSuccess);
	HitImpulse.NetSerialize(Ar, Map, bOutSuccess);
	return true;
}
