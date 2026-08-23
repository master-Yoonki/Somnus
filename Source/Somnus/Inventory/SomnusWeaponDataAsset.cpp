// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusWeaponDataAsset.h"

#include "Core/SomnusGameplayTags.h"

USomnusWeaponDataAsset::USomnusWeaponDataAsset()
{
	Category = EItemCategory::Weapon;
	ItemTag = SomnusTags::Item_Equipment_Weapon;
	MenuActions.AddTag(SomnusTags::ItemAction_Equip);
	MaxStackCount = 1;
}
