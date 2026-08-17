// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusEquipmentComponent.h"

#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "Engine/World.h"
#include "Equipment/SomnusWeapon.h"
#include "GameFramework/Pawn.h"
#include "Inventory/SomnusEquipmentSlotComponent.h"
#include "Inventory/SomnusWeaponDataAsset.h"

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

void USomnusEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// Behind the authority gate, unlike the storage component's matching bind. That one listens on
	// every machine because a client's panels have to redraw; this one only ever spawns actors,
	// which is the server's job, and the actors replicate themselves down.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (USomnusEquipmentSlotComponent* Slot : GetWeaponSlots())
	{
		if (!Slot)
		{
			continue;
		}
		Slot->OnItemAddedDelegate.AddDynamic(this, &USomnusEquipmentComponent::HandleWeaponSlotChanged);
		Slot->OnItemRemovedDelegate.AddDynamic(this, &USomnusEquipmentComponent::HandleWeaponSlotChanged);
	}

	// Delegates only report what happens next. Anything already sitting in a slot by now - granted
	// equipment, a character rebuilt after travel - was put there before there was anyone to tell,
	// and would go without an actor for as long as nobody touched the slot again.
	SyncCarriedWeapons();
}

void USomnusEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Carried actors are separate actors that happen to be attached, so nothing tears them down
	// with their wearer. Keys first: retiring mutates the map being read.
	TArray<FGameplayTag> CarriedSlots;
	CarriedWeapons.GetKeys(CarriedSlots);
	for (const FGameplayTag& SlotTag : CarriedSlots)
	{
		RetireCarriedWeapon(SlotTag);
	}

	Super::EndPlay(EndPlayReason);
}

void USomnusEquipmentComponent::Server_EquipInstance_Implementation(const FSomnusItemInstance& Instance)
{
	EquipInstance(Instance);
}

bool USomnusEquipmentComponent::EquipInstance(const FSomnusItemInstance& Instance)
{	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}
	
	if (!Instance.ItemData) return false;
	
	// Routed by asking each slot rather than by looking the item's kind up in a table. AcceptsItem
	// is the same question a drag asks, so granting and dragging can never disagree about where
	// something belongs, and a new kind of worn thing needs no entry anywhere.
	USomnusEquipmentSlotComponent* TargetSlot = nullptr;
	TArray<USomnusEquipmentSlotComponent*> Slots = GetSlots();
	for (USomnusEquipmentSlotComponent* Slot : Slots)
	{
		// Empty as well as willing: two weapon slots take the same things, so a kit granting two
		// weapons has to land the second one somewhere other than where the first went.
		if (Slot && Slot->AcceptsItem(Instance.ItemData) && Slot->GetAllItems().IsEmpty())
		{
			TargetSlot = Slot;
			break;
		}
	}

	if (!TargetSlot)
	{
		return false;
	}

	// Occupancy is not checked here any more - the slot refuses on its own, because a slot holding
	// one item is full by definition and CanFitAt says so. A copy, because AddExistingItemAt
	// consumes what it is given and Instance is const for the caller's sake.
	FSomnusItemInstance ItemToEquip = Instance;
	if (TargetSlot->AddExistingItemAt(ItemToEquip, 0, 0, false) != 0)
	{
		return false;
	}
	
	return true;
}

TArray<USomnusEquipmentSlotComponent*> USomnusEquipmentComponent::GetSlots() const
{
	return GetWeaponSlots();
}

TArray<USomnusEquipmentSlotComponent*> USomnusEquipmentComponent::GetWeaponSlots() const
{
	return { PrimaryWeaponSlot, SecondaryWeaponSlot };
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

ASomnusWeapon* USomnusEquipmentComponent::GetCarriedWeapon(int32 SlotIndex) const
{
	// Index to slot to tag, rather than indexing the map directly. GetWeaponSlot is the one place
	// that decides what an index means, and a second copy of that decision here would be free to
	// drift from it.
	const USomnusEquipmentSlotComponent* Slot = GetWeaponSlot(SlotIndex);
	if (!Slot)
	{
		return nullptr;
	}

	const FSomnusCarriedWeapon* Carried = CarriedWeapons.Find(Slot->GetSlotTag());
	return Carried ? Carried->Actor.Get() : nullptr;
}

void USomnusEquipmentComponent::HandleWeaponSlotChanged(const FSomnusItemInstance& Item)
{
	SyncCarriedWeapons();
}

void USomnusEquipmentComponent::SyncCarriedWeapons()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (const USomnusEquipmentSlotComponent* Slot : GetWeaponSlots())
	{
		if (!Slot)
		{
			continue;
		}

		const FGameplayTag SlotTag = Slot->GetSlotTag();

		// A slot holds one item or none, so the first entry is the whole answer.
		const TArray<FSomnusItemInstance> Worn = Slot->GetAllItems();
		const FSomnusItemInstance* Desired = Worn.Num() > 0 ? &Worn[0] : nullptr;

		// Nothing wanted and nothing carried is as much an agreement as the two matching, and has to
		// be read as one - a slot that has always been empty otherwise counts as out of step on
		// every pass and works its way down to two branches that both decline to do anything.
		const FSomnusCarriedWeapon* Carried = CarriedWeapons.Find(SlotTag);
		const bool bAlreadyRight = Desired
			? (Carried && Carried->Actor && Carried->InstanceID == Desired->InstanceID)
			: (Carried == nullptr);

		if (bAlreadyRight)
		{
			continue;
		}

		// Both branches, in this order, cover every transition: emptied, filled, and swapped for a
		// different item. Nothing reads Carried past this point - retiring erases the entry it
		// pointed at, and spawning may move the map's storage out from under it.
		if (Carried)
		{
			RetireCarriedWeapon(SlotTag);
		}

		if (Desired)
		{
			SpawnCarriedWeapon(SlotTag, *Desired);
		}
	}
}

ASomnusWeapon* USomnusEquipmentComponent::SpawnCarriedWeapon(FGameplayTag SlotTag, const FSomnusItemInstance& Item)
{
	const USomnusWeaponDataAsset* WeaponData = Cast<USomnusWeaponDataAsset>(Item.ItemData);
	if (!WeaponData)
	{
		// The slot's tag filter should have refused this long before here.
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("SpawnCarriedWeapon: %s is in a weapon slot but is not a weapon."),
			*GetNameSafe(Item.ItemData));
		return nullptr;
	}

	if (!WeaponData->WeaponActorClass)
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("SpawnCarriedWeapon: %s names no WeaponActorClass - it can be worn but never drawn."),
			*WeaponData->GetName());
		return nullptr;
	}

	// A pawn specifically, because Instigator is what routes damage credit back to whoever fired.
	APawn* OwningPawn = GetOwner<APawn>();
	UWorld* World = GetWorld();
	if (!OwningPawn || !World)
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("SpawnCarriedWeapon: %s is not carried by a pawn - nothing to spawn a weapon for."),
			*GetNameSafe(GetOwner()));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwningPawn;
	SpawnParams.Instigator = OwningPawn;

	// The weapon exists because the slot says so. A collision test is not entitled to a say in
	// that, and a refused spawn would leave an item that is worn but has nothing standing for it.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASomnusWeapon* Spawned = World->SpawnActor<ASomnusWeapon>(
		WeaponData->WeaponActorClass,
		OwningPawn->GetActorLocation(),
		OwningPawn->GetActorRotation(),
		SpawnParams);

	if (!Spawned)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("SpawnCarriedWeapon: failed to spawn %s for %s."),
			*GetNameSafe(WeaponData->WeaponActorClass), *OwningPawn->GetName());
		return nullptr;
	}

	// Carried, not drawn. Attached so it travels with its wearer - an actor left standing where it
	// spawned goes out of net relevancy the moment the character walks off, and comes back as a
	// visible hitch when it is finally drawn. To the root rather than to a holster socket, because
	// there is no holster socket yet; hidden means nobody can tell the difference.
	Spawned->AttachToComponent(OwningPawn->GetRootComponent(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Spawned->SetActorHiddenInGame(true);

	FSomnusCarriedWeapon Carried;
	Carried.InstanceID = Item.InstanceID;
	Carried.Actor = Spawned;
	CarriedWeapons.Add(SlotTag, Carried);

	return Spawned;
}

void USomnusEquipmentComponent::RetireCarriedWeapon(FGameplayTag SlotTag)
{
	FSomnusCarriedWeapon Retiring;
	if (!CarriedWeapons.RemoveAndCopyValue(SlotTag, Retiring) || !Retiring.Actor)
	{
		return;
	}

	// Unequip is a no-op on a weapon that was never drawn, and on one that was it is the only
	// thing that gives the abilities and loose tags back before the actor they were granted from
	// stops existing.
	Retiring.Actor->Unequip();

	// Then the wearer, while there is still an actor to name. Dropping a weapon that was in a hand
	// is the case this exists for: the slot empties, this runs, and without the line below the
	// character keeps pointing at destroyed memory and keeps that weapon's anim layers linked.
	if (ASomnusCharacter* Wearer = GetOwner<ASomnusCharacter>())
	{
		Wearer->NotifyCarriedWeaponRetiring(Retiring.Actor);
	}

	Retiring.Actor->Destroy();
}
