// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusContainerDataAsset.h"

#include "Core/SomnusGameplayTags.h"

USomnusContainerDataAsset::USomnusContainerDataAsset()
{
	Category = EItemCategory::Equipment;
	MenuActions.AddTag(SomnusTags::ItemAction_Equip);
	MaxStackCount = 1;
}
