// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/SomnusGameplayTags.h"

namespace SomnusTags
{
	// Equipment identity
	UE_DEFINE_GAMEPLAY_TAG(Equipped_Weapon_Bat, "Equipped.Weapon.Bat");

	// Data — SetByCaller keys
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_StaminaCost, "Data.StaminaCost");

	// Events
	UE_DEFINE_GAMEPLAY_TAG(Event_Melee_Hit, "Event.Melee.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Death, "Event.Death");

	// Weapon-granted permission tags
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_HeavyAttack, "Weapon.Enable.HeavyAttack");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_LightAttack, "Weapon.Enable.LightAttack");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_Aim, "Weapon.Enable.Aim");

	// Ability identity tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Melee_Heavy, "Ability.Melee.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Melee_Light, "Ability.Melee.Light");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Aim, "Ability.Aim");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Jump, "Ability.Jump");

	// Cooldown tags
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Ability_Jump, "Cooldown.Ability.Jump");

	// Data — SetByCaller keys (cooldown)
	UE_DEFINE_GAMEPLAY_TAG(Data_CooldownDuration, "Data.CooldownDuration");

	// Character state
	UE_DEFINE_GAMEPLAY_TAG(State_Aiming, "State.Aiming");
	UE_DEFINE_GAMEPLAY_TAG(State_MovementCancellable, "State.MovementCancellable");
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");

	// Effect policy tags
	UE_DEFINE_GAMEPLAY_TAG(Effect_RemoveOnDeath, "Effect.RemoveOnDeath");

	// Input — Native
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Move, "Input.Native.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Look, "Input.Native.Look");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Jump, "Input.Native.Jump");

	// Input — Ability
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Attack, "Input.Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Aim, "Input.Ability.Aim");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Jump, "Input.Ability.Jump");
	
	// Zombie
	// Zombie Ability identity tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Zombie_Melee, "Ability.Zombie.Melee");
}
