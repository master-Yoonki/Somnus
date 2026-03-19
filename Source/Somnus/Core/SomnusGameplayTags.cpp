// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/SomnusGameplayTags.h"

namespace SomnusTags
{
	// Equipment identity
	UE_DEFINE_GAMEPLAY_TAG(Equipped_Weapon_Bat, "Equipped.Weapon.Bat");

	// Events
	UE_DEFINE_GAMEPLAY_TAG(Event_Melee_Hit, "Event.Melee.Hit");

	// Weapon-granted permission tags
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_HeavyAttack, "Weapon.Enable.HeavyAttack");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_LightAttack, "Weapon.Enable.LightAttack");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_Aim, "Weapon.Enable.Aim");

	// Ability identity tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Melee_Heavy, "Ability.Melee.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Melee_Light, "Ability.Melee.Light");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Aim, "Ability.Aim");

	// Character state
	UE_DEFINE_GAMEPLAY_TAG(State_Aiming, "State.Aiming");
	UE_DEFINE_GAMEPLAY_TAG(State_MovementCancellable, "State.MovementCancellable");

	// Input — Native
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Move, "Input.Native.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Look, "Input.Native.Look");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Jump, "Input.Native.Jump");

	// Input — Ability
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Attack, "Input.Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Aim, "Input.Ability.Aim");
}
