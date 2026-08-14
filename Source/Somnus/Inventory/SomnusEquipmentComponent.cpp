// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusEquipmentComponent.h"

#include "Core/SomnusGameplayTags.h"
#include "Inventory/SomnusEquipmentSlotComponent.h"

USomnusEquipmentComponent::USomnusEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	PrimaryWeaponSlot = CreateDefaultSubobject<USomnusEquipmentSlotComponent>(TEXT("PrimaryWeaponSlot"));
	SecondaryWeaponSlot = CreateDefaultSubobject<USomnusEquipmentSlotComponent>(TEXT("SecondaryWeaponSlot"));
}

void USomnusEquipmentComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// InitializeComponent rather than BeginPlay, because another component grants starting
	// equipment in its BeginPlay and the order between the two is not ours to pick. Every
	// component on an actor is initialised before any of them begins play, so a slot is always
	// configured by the time anything looks for one to put something in.
	//
	// Both slots take the parent tag rather than a leaf, so a new weapon kind is a data asset and
	// nothing else - the slot never hears about it. Set on every machine, and not behind an
	// authority check: a client refusing a drag before asking the server depends on knowing this,
	// and it would otherwise wave everything through until the value replicated.
	FGameplayTagContainer WeaponTags;
	WeaponTags.AddTag(SomnusTags::Item_Equipment_Weapon);

	if (PrimaryWeaponSlot)
	{
		PrimaryWeaponSlot->InitializeSlot(SomnusTags::Equipment_Slot_Weapon_Primary, WeaponTags,
			NSLOCTEXT("Somnus", "SlotPrimaryWeapon", "PRIMARY"));
	}

	if (SecondaryWeaponSlot)
	{
		SecondaryWeaponSlot->InitializeSlot(SomnusTags::Equipment_Slot_Weapon_Secondary, WeaponTags,
			NSLOCTEXT("Somnus", "SlotSecondaryWeapon", "SECONDARY"));
	}
}

USomnusEquipmentSlotComponent* USomnusEquipmentComponent::GetWeaponSlot(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0:  return PrimaryWeaponSlot;
	case 1:  return SecondaryWeaponSlot;
	default: return nullptr;
	}
}

int32 USomnusEquipmentComponent::GetNumWeaponSlots() const
{
	return 2;
}
