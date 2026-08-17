// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Inventory/SomnusItemInstance.h"
#include "SomnusEquipmentComponent.generated.h"

class ASomnusWeapon;
class USomnusEquipmentSlotComponent;

/**
 * The actor standing in the world for what a weapon slot holds.
 *
 * The instance ID rides along because the slot on its own cannot answer "is this still the same
 * weapon". Two bats look alike from the outside, and an actor built for one of them is not reusable
 * for the other: it carries no per-instance state, so keeping it would leave the previous item's
 * actor answering for the current item's entry.
 */
USTRUCT()
struct FSomnusCarriedWeapon
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid InstanceID;

	UPROPERTY()
	TObjectPtr<ASomnusWeapon> Actor = nullptr;
};

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
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
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

	/** The actor carried for whatever the slot at SlotIndex holds, or null when the slot is empty
	 *  or its item never named an actor class. Carried is not drawn: the actor returned here is
	 *  hidden and riding along until something puts it in a hand.
	 *
	 *  Server only, and deliberately so - the bookkeeping behind it is not replicated, because a
	 *  client has no question that needs it. What is in the slot arrives as the grid contents it
	 *  is, the actor replicates itself, and which one is drawn is already a replicated property on
	 *  the character. */
	ASomnusWeapon* GetCarriedWeapon(int32 SlotIndex) const;

	/** Puts an already-formed instance into the first slot here that admits it and has nothing in
	 *  it, in GetWeaponSlots order - so a gun with both hands free goes to the primary rather than
	 *  to whichever slot the component array happened to yield first. Returns false and changes
	 *  nothing when no slot will have it, which is the ordinary answer for most items and not a
	 *  fault. Server only. */
	bool EquipInstance(const FSomnusItemInstance& Instance);

protected:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION(Server, Reliable, Category = "Equipment")
	void Server_EquipInstance(const FSomnusItemInstance& Instance);

	TArray<USomnusEquipmentSlotComponent*> GetSlots() const;

	/** Constructor subobjects, and named members rather than an array: a plain object property
	 *  inside a container is not something the instancing graph is guaranteed to remap, and a
	 *  component reference left pointing at the class default object is exactly the failure that
	 *  cost a session to find once already. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USomnusEquipmentSlotComponent> PrimaryWeaponSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USomnusEquipmentSlotComponent> SecondaryWeaponSlot;

	/** The two above, in index order. */
	TArray<USomnusEquipmentSlotComponent*> GetWeaponSlots() const;

	/** Bound to every weapon slot's add and remove delegates on the authority.
	 *
	 *  The item is ignored, which is the point. A dynamic multicast does not say who sent it, so
	 *  the handler cannot tell which slot changed - and on a removal the item is already out of the
	 *  grid that would have identified it, so no amount of searching would recover it either. The
	 *  one answer that serves both directions is to stop reacting to the event and re-read the
	 *  slots instead. */
	UFUNCTION()
	void HandleWeaponSlotChanged(const FSomnusItemInstance& Item);

	/** Makes the carried actors agree with what the slots hold, whatever they held before.
	 *
	 *  Idempotent on purpose. It is both the delegate handler's whole body and the pass BeginPlay
	 *  runs once over slots that were filled before anything was listening, so a duplicated
	 *  broadcast cannot spawn a second actor and a missed one is corrected by the next change.
	 *  Server only. */
	void SyncCarriedWeapons();

private:
	/** Spawns, hides and attaches the actor for Item, and records it against SlotTag. Null when the
	 *  item is not a weapon, names no actor class, or the spawn fails - each of which is logged,
	 *  because every one of them reads as "the slot took it and nothing happened". */
	ASomnusWeapon* SpawnCarriedWeapon(FGameplayTag SlotTag, const FSomnusItemInstance& Item);

	/** Tears down whatever SlotTag carries. Unequip runs before the destroy, while the actor is
	 *  still alive to hand its abilities and loose tags back to the ability system it granted them
	 *  to; destroying first would leave both behind permanently. The wearer is told in between,
	 *  because an actor that is about to stop existing is still the one their hand is pointing at. */
	void RetireCarriedWeapon(FGameplayTag SlotTag);

	/** Keyed by slot tag rather than by index, so the mapping from index to slot lives in
	 *  GetWeaponSlot and nowhere else. Not replicated - see GetCarriedWeapon. */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FSomnusCarriedWeapon> CarriedWeapons;
};
