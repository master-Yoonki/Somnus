// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effects/SomnusGE_Cooldown.h"
#include "Core/SomnusGameplayTags.h"

USomnusGE_Cooldown::USomnusGE_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// Duration magnitude comes from SetByCaller — each ability sets its own CooldownDuration.
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = SomnusTags::Data_CooldownDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}
