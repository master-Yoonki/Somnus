// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusItemDataAsset.h"

FPrimaryAssetId USomnusItemDataAsset::GetPrimaryAssetId() const
{
	const FName AssetName = ItemId.IsNone() ? GetFName() : ItemId;
	return FPrimaryAssetId(TEXT("SomnusItem"), AssetName);
}

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
