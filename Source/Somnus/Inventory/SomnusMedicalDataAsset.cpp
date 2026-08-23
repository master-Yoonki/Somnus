// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusMedicalDataAsset.h"

#include "Core/SomnusGameplayTags.h"

USomnusMedicalDataAsset::USomnusMedicalDataAsset()
{
	Category = EItemCategory::Medical;
	MenuActions.AddTag(SomnusTags::ItemAction_Use);
	ItemTag = SomnusTags::Item_Consumable_Medical;
	
}
