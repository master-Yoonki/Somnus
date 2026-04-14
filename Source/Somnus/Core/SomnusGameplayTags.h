// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
/**
 *
 */
namespace SomnusTags
{
	// Equipment identity
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipped_Weapon_Bat);

	// Data — SetByCaller keys
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StaminaCost);

	// Events
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee_Hit);

	// Weapon-granted permission tags (added/removed on equip/unequip)
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Enable_HeavyAttack);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Enable_LightAttack);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Enable_Aim);

	// Ability identity tags
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Melee_Heavy);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Melee_Light);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aim);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Jump);

	// Cooldown tags
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_Jump);

	// Data — SetByCaller keys (cooldown)
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_CooldownDuration);

	// Character state
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Aiming);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_MovementCancellable);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	// Effect policy tags — used to batch-remove effects on death
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_RemoveOnDeath);

	// Input — Native
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Move);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Look);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Jump);

	// Input — Ability
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Attack);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Aim);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Jump);
};
