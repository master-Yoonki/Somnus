// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/SomnusGameplayTags.h"

namespace SomnusTags
{
	// Item type
	UE_DEFINE_GAMEPLAY_TAG(Item, "Item");
	UE_DEFINE_GAMEPLAY_TAG(Item_Equipment_Container, "Item.Equipment.Container");
	UE_DEFINE_GAMEPLAY_TAG(Item_Equipment_Container_Rig, "Item.Equipment.Container.Rig");
	UE_DEFINE_GAMEPLAY_TAG(Item_Equipment_Container_Backpack, "Item.Equipment.Container.Backpack");
	UE_DEFINE_GAMEPLAY_TAG(Item_Equipment_Container_Pockets, "Item.Equipment.Container.Pockets");
	UE_DEFINE_GAMEPLAY_TAG(Item_Equipment_Weapon, "Item.Equipment.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Item_Equipment_Armor, "Item.Equipment.Armor");

	// Slot identity
	UE_DEFINE_GAMEPLAY_TAG(Equipment_Slot_Pockets, "Equipment.Slot.Pockets");
	UE_DEFINE_GAMEPLAY_TAG(Equipment_Slot_Rig, "Equipment.Slot.Rig");
	UE_DEFINE_GAMEPLAY_TAG(Equipment_Slot_Backpack, "Equipment.Slot.Backpack");
	UE_DEFINE_GAMEPLAY_TAG(Equipment_Slot_Weapon_Primary, "Equipment.Slot.Weapon.Primary");
	UE_DEFINE_GAMEPLAY_TAG(Equipment_Slot_Weapon_Secondary, "Equipment.Slot.Weapon.Secondary");

	// Equipment identity
	UE_DEFINE_GAMEPLAY_TAG(Equipped_Weapon_Bat, "Equipped.Weapon.Bat");

	// Data — SetByCaller keys
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_StaminaCost, "Data.StaminaCost");

	// Events
	UE_DEFINE_GAMEPLAY_TAG(Event_Melee_Hit, "Event.Melee.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Death, "Event.Death");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact, "Event.HitReact");

	// Weapon-granted permission tags
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_HeavyAttack, "Weapon.Enable.HeavyAttack");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_LightAttack, "Weapon.Enable.LightAttack");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Enable_Aim, "Weapon.Enable.Aim");

	// Ability identity tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Melee_Heavy, "Ability.Melee.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Melee_Light, "Ability.Melee.Light");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Aim, "Ability.Aim");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Jump, "Ability.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Ability_HitReact, "Ability.HitReact");

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
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Interact, "Input.Native.Interact");

	// Input — Ability
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Attack, "Input.Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Aim, "Input.Ability.Aim");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Jump, "Input.Ability.Jump");
	
	// Zombie
	// Zombie Ability identity tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Zombie_Melee, "Ability.Zombie.Melee");
}
