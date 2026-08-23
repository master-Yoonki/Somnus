// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusItemDataAsset.h"

#include "Core/SomnusGameplayTags.h"

FGameplayTagContainer USomnusItemDataAsset::GetMenuActions() const
{
	FGameplayTagContainer Actions = MenuActions;
	return Actions;
}

bool USomnusItemDataAsset::SupportsAction(FGameplayTag Action) const
{
	// Built rather than tested against MenuActions, so the implied entries answer here too. The
	// container is a handful of tags and this is asked once per menu, not per frame.
	return GetMenuActions().HasTag(Action);
}

USomnusItemDataAsset::USomnusItemDataAsset()
{
	MenuActions.AddTag(SomnusTags::ItemAction_Drop);
}

FPrimaryAssetId USomnusItemDataAsset::GetPrimaryAssetId() const
{
	const FName AssetName = ItemId.IsNone() ? GetFName() : ItemId;
	return FPrimaryAssetId(TEXT("SomnusItem"), AssetName);
}

#if WITH_EDITOR
void USomnusItemDataAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(USomnusItemDataAsset, Size))
	{
		bool ArrayInitVal = true;
		ShapeMask.Init(true, Size.X * Size.Y);
	}
}
#endif

FIntPoint USomnusItemDataAsset::GetEffectiveSize(bool bRotated) const
{
	// Swap Width and Height if rotated 90 degrees
	return bRotated ? FIntPoint(Size.Y, Size.X) : Size;
}

bool USomnusItemDataAsset::IsCellOccupied(int32 LocalX, int32 LocalY, bool bIsRotated) const
{
	const FIntPoint EffSize = GetEffectiveSize(bIsRotated);
	
	// Basic bounds check against current orientation
	if (LocalX < 0 || LocalX >= EffSize.X || LocalY < 0 || LocalY >= EffSize.Y)
	{
		return false;
	}

	// If no mask is defined, assume the item is a solid block
	if (ShapeMask.Num() == 0)
	{
		return true;
	}
	
	int32 OrgX = LocalX;
	int32 OrgY = LocalY;

	if (bIsRotated)
	{
		/** 
		 * Map rotated coordinates back to original data space.
		 * 90-degree CW rotation mapping: (x, y) -> (y, (OrigHeight - 1) - x)
		 */
		OrgX = LocalY;
		OrgY = Size.Y - 1 - LocalX;
	}
	
	const int32 Index = (OrgY * Size.X) + OrgX;
	return ShapeMask.IsValidIndex(Index) ? ShapeMask[Index] : false;
}
