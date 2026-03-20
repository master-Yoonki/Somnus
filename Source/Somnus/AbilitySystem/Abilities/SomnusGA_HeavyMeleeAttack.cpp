// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_HeavyMeleeAttack.h"
#include "Core/SomnusGameplayTags.h"

USomnusGA_HeavyMeleeAttack::USomnusGA_HeavyMeleeAttack()
{
	AbilityTags.AddTag(SomnusTags::Ability_Melee_Heavy);
	ActivationRequiredTags.AddTag(SomnusTags::Weapon_Enable_HeavyAttack);
	ActivationBlockedTags.AddTag(SomnusTags::State_Aiming);
	ActivationBlockedTags.AddTag(SomnusTags::Ability_Melee_Heavy);

	StaminaCost = 25.0f;
}
