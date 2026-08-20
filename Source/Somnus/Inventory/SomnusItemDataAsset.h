// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Inventory/SomnusItemTypes.h"
#include "SomnusItemDataAsset.generated.h"

class UTexture2D;
class UStaticMesh;
class UGameplayEffect;

/**
 * Read-only template describing a single item kind.
 * Runtime mutable state (current durability, freshness, stack count)
 * lives on FItemInstance, not here.
 */
UCLASS(Abstract, BlueprintType)
class SOMNUS_API USomnusItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	/** Everything this item offers, which is MenuActions plus the ones every item has. Ask this
	 *  rather than reading MenuActions: what is stored is only the part that varies, and a menu
	 *  built from the field alone would be missing the entries nobody had to type. */
	UFUNCTION(BlueprintPure, Category = "Item|Actions")
	FGameplayTagContainer GetMenuActions() const;

	/** Whether this item offers Action - the one question the menu and the server both ask, so
	 *  neither has to know that some actions are stored and some are implied. */
	UFUNCTION(BlueprintPure, Category = "Item|Actions")
	bool SupportsAction(FGameplayTag Action) const;

	/** Returns the dimensions of the item, accounting for 90-degree rotation */
	UFUNCTION(BlueprintPure, Category = "Item|Grid")
	FIntPoint GetEffectiveSize(bool bRotated) const;
	
	/** Checks if a specific cell relative to the item's top-left corner is occupied */
	UFUNCTION(BlueprintPure, Category = "Item|Grid")
	bool IsCellOccupied(int32 LocalX, int32 LocalY, bool bIsRotated) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity")
	FName ItemId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stats", meta = (ClampMin = "0.0", Units = "kg"))
	float Weight = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Classification")
	EItemCategory Category = EItemCategory::None;
	
	/** What this item is, matched against a grid's AcceptedTags. Hierarchical, so a slot that
	 *  accepts Item.Equipment.Weapon takes every weapon kind under it. Leave the leaves to the
	 *  editor's tag table: no code asks whether something is a helmet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Classification")
	FGameplayTag ItemTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stats", meta = (ClampMin = "1"))
	int32 MaxStackCount = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Status", meta = (ClampMin = "1"))
	FIntPoint Size = { 1, 1 };
	
	/** Defines the internal shape of the item. Empty array means the item is a solid rectangle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, EditFixedSize, Category = "Item|Status", meta=(EditFixedOrder))
	TArray<bool> ShapeMask;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TObjectPtr<UStaticMesh> WorldMesh;

	/** What the context menu offers for this item, beyond what every item offers.
	 *
	 *  ItemAction.Drop is deliberately not here: everything in a grid can be put on the floor, so
	 *  storing it would be a row to tick on every asset ever made and a row someone eventually
	 *  forgets. GetMenuActions adds it. If an item that cannot be dropped ever exists, it wants a
	 *  flag saying so rather than the absence of a tag - a missing entry and a refusal read the
	 *  same in data and mean different things.
	 *
	 *  A container rather than a set of bools because the menu iterates it: a new verb is a tag
	 *  and a button, with nothing here to widen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Actions", meta = (Categories = "ItemAction"))
	FGameplayTagContainer MenuActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|GAS")
	TArray<TSubclassOf<UGameplayEffect>> UseEffects;
};
