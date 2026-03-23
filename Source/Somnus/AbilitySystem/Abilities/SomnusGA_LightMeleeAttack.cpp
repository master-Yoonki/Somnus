// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/SomnusGA_LightMeleeAttack.h"
#include "Core/SomnusGameplayTags.h"

USomnusGA_LightMeleeAttack::USomnusGA_LightMeleeAttack()
{
	SetAssetTags(FGameplayTagContainer(SomnusTags::Ability_Melee_Light));
	ActivationRequiredTags.AddTag(SomnusTags::Weapon_Enable_LightAttack);
	ActivationRequiredTags.AddTag(SomnusTags::State_Aiming);
	ActivationBlockedTags.AddTag(SomnusTags::Ability_Melee_Light);

	StaminaCost = 15.0f;
}
