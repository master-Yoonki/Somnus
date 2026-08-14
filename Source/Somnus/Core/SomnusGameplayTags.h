// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
/**
 *
 */
namespace SomnusTags
{
	// Item type — what an item IS, matched against a container's AcceptedItemTags.
	//
	// Only what C++ names is declared here. Leaves like Item.Equipment.Weapon.Melee or
	// Item.Consumable.Medical belong in the editor's tag table instead: no code asks whether
	// something is a helmet, and declaring them natively would put the category axis back into
	// C++ - the thing the tag filter exists to get out of it. Intermediate tags come free, since
	// registering a leaf registers every parent along the way.
	//
	// The root. A container holding this accepts anything, which is the default every grid
	// starts with; a slot resets it and is handed something narrower.
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item);

	// Named because a slot is created for each of these by name, and because the storage
	// component has to find these two in particular to work out which compartments exist.
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Container);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Container_Rig);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Container_Backpack);

	// Parents, so a weapon slot accepts every weapon kind and an armour slot every armour kind
	// without either of them hearing about a new one.
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Weapon);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Equipment_Armor);

	// Equipment identity
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipped_Weapon_Bat);

	// Data — SetByCaller keys
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StaminaCost);

	// Events
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee_Hit);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);

	// Weapon-granted permission tags (added/removed on equip/unequip)
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Enable_HeavyAttack);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Enable_LightAttack);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Enable_Aim);

	// Ability identity tags
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Melee_Heavy);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Melee_Light);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aim);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Jump);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReact);

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
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Interact);

	// Input — Ability
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Attack);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Aim);
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Jump);
	
	// Zombie
	// Zombie Ability identity tags
	SOMNUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Zombie_Melee);
};
