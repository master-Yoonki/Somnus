// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/SomnusInventoryComponent.h"
#include "SomnusEquipmentSlotComponent.generated.h"

/**
 * An inventory grid of one cell that admits an item regardless of how large it is.
 *
 * Every path that adds or moves an item in the base class narrows to CanFitAt or FindFirstFit and
 * then settles through RebuildOccupationGrid, so overriding those three is the entire class. A 3x4
 * backpack lands in a slot by the same code that lands it in a pocket, and everything the base
 * already does - stack merging, cross-grid moves, replication, the add and remove delegates that
 * are where a slot's meaning gets attached - is inherited untouched.
 *
 * Item dimensions and bRotated go deliberately unread: what admits an item here is whether the
 * slot is free, never whether it has room. GridWidth and GridHeight are held at 1x1 so the
 * inherited helpers keep answering sensibly, but nothing here decides anything from them.
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class SOMNUS_API USomnusEquipmentSlotComponent : public USomnusInventoryComponent
{
	GENERATED_BODY()
public:
	USomnusEquipmentSlotComponent();

	/** Everything that makes one slot different from another, set once by whoever declares it.
	 *  Three values rather than three calls because they are one decision: this is the rig slot,
	 *  it takes rigs, and it says RIG when empty. */
	void InitializeSlot(FGameplayTag InSlotTag, const FGameplayTagContainer& InAcceptedTags,
		const FText& InLabel, bool bInLocked = false);

	/** A locked slot keeps what it is given. Only the two client entry points into storage consult
	 *  this - what the player may ask for is the whole question, and server-side code granting or
	 *  emptying a character still needs to reach in. Pockets are the reason it exists: they are an
	 *  ordinary container item in an ordinary slot, and the single thing that sets them apart is
	 *  that no drag can take them out. */
	UFUNCTION(BlueprintPure, Category = "Slot")
	bool IsLocked() const { return bLocked; }

	/** Which place this is, as opposed to what it admits. The two weapon slots accept the same
	 *  things and are still not each other. */
	UFUNCTION(BlueprintPure, Category = "Slot")
	FGameplayTag GetSlotTag() const { return SlotTag; }

	/** Shown behind an empty slot. Empty means show nothing, which is how a grid that is not a
	 *  slot at all - a pocket compartment - ends up needing no special case in the widget. */
	UFUNCTION(BlueprintPure, Category = "Slot")
	FText GetSlotLabel() const { return SlotLabel; }

	/** The slot on Owner answering to SlotTag, whichever component happens to declare it. Asked of
	 *  the actor rather than of a component because slots are spread across more than one owner by
	 *  design - storage declares its own, equipment declares the rest - and no caller should have
	 *  to know which. */
	UFUNCTION(BlueprintPure, Category = "Slot")
	static USomnusEquipmentSlotComponent* FindSlot(const AActor* Owner, FGameplayTag InSlotTag);

	/** IgnoreItemID carries more weight here than in a grid. A slot holding one item is full by
	 *  definition, so re-placing that same item - which TryMoveItem does on every drop that lands
	 *  where it started - would otherwise be refused by its own presence. */
	virtual bool CanFitAt(class USomnusItemDataAsset* ItemData,
		int32 TopLeftX, int32 TopLeftY, bool bRotated,
		FGuid IgnoreItemID = FGuid()) const override;

	virtual bool FindFirstFit(class USomnusItemDataAsset* ItemData, int32& OutX, int32& OutY, bool& bOutRotated) const override;

	/** Inverted from the grid's reading of an empty AcceptedTags. For a pocket, declaring nothing
	 *  means no restriction; for a slot it means nobody has said what this is for yet, and a slot
	 *  that admits anything until told otherwise will take the first thing offered - which is how
	 *  a weapon slot ended up wearing the pockets, purely because its owning component's BeginPlay
	 *  had not run when the other's granted them. */
	virtual bool AcceptsItem(class USomnusItemDataAsset* ItemData) const override;

protected:
	virtual void RebuildOccupationGrid() override;

	// Not replicated: InitializeSlot runs on every machine, so there is nothing to send.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot")
	FGameplayTag SlotTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot|UI")
	FText SlotLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot")
	bool bLocked = false;
};
