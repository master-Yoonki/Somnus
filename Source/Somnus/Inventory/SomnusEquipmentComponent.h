// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SomnusEquipmentComponent.generated.h"

class USomnusEquipmentSlotComponent;

/**
 * The slots a character wears that are not storage.
 *
 * Separate from USomnusContainerEquipComponent because the two answer different questions. That
 * one is about storage - which grids exist, where a loose item can go, what the panels show - and
 * carries a fair amount of machinery for it. This one is about what is worn and what wearing it
 * does, and the slots themselves are the same class either way.
 *
 * Weapons now, armour later, and later is meant to cost nothing: what a slot admits is a tag on
 * the slot, and what putting something in it does will be an actor to spawn and a set of effects
 * to apply, both declared by the item. Neither is a reason for another component.
 */
UCLASS(ClassGroup = (Somnus), meta = (BlueprintSpawnableComponent))
class SOMNUS_API USomnusEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USomnusEquipmentComponent();

	/** Weapon slots by index, because that is what SwitchWeapon and the hotbar already speak in.
	 *  Null for an index this character does not have. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	USomnusEquipmentSlotComponent* GetWeaponSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	int32 GetNumWeaponSlots() const;

protected:
	virtual void InitializeComponent() override;

	/** Constructor subobjects, and named members rather than an array: a plain object property
	 *  inside a container is not something the instancing graph is guaranteed to remap, and a
	 *  component reference left pointing at the class default object is exactly the failure that
	 *  cost a session to find once already. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USomnusEquipmentSlotComponent> PrimaryWeaponSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USomnusEquipmentSlotComponent> SecondaryWeaponSlot;
};
