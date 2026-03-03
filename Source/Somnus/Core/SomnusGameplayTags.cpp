// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/SomnusGameplayTags.h"

namespace SomnusTags
{
	// Define the equipment tags
	UE_DEFINE_GAMEPLAY_TAG(Equipped_Weapon_Bat, "Equipped.Weapon.Bat");
	
	// Define the event tags
	UE_DEFINE_GAMEPLAY_TAG(Event_Melee_Hit, "Event.Melee.Hit");
	
	// Define the enable tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enable_Swing, "Ability.Enable.Swing");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enable_Aim, "Ability.Enable.Aim");

	// Define the action tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Swing, "Ability.Action.Swing");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Aim, "Ability.Action.Aim");
}