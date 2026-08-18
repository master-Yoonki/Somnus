// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SomnusItemTypes.h"
#include "Components/ActorComponent.h"
#include "Inventory/SomnusItemInstance.h"
#include "SomnusContainerEquipComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActiveContainersChanged);

class USomnusContainerDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SOMNUS_API USomnusContainerEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPROPERTY(BlueprintAssignable, Category = "Container|Events")
	FOnActiveContainersChanged OnActiveContainersChangedDelegate;
	
	// Sets default values for this component's properties
	USomnusContainerEquipComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FSomnusActiveContainerInfo> GetActiveContainers() const;

	/** Finds which container this character can reach holds InstanceID, and copies the instance
	 *  out. Null when nothing reachable holds it - including whatever is worn, which lives in a
	 *  slot rather than in a grid, so a caller that means to cover equipment has to ask
	 *  GetEquippedInstance separately. The copy is deliberate: callers that go on to remove or
	 *  move the item would be left holding a pointer into an array they just resized. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	USomnusInventoryComponent* FindContainerHolding(FGuid InstanceID, FSomnusItemInstance& OutInstance) const;

	/** Puts an already-formed container instance into whichever slot accepts it. Granting rather
	 *  than equipping: the instance has just been minted and is in no grid yet, which is why this
	 *  is not a move like every other way something reaches a slot. The routing is the item's own
	 *  tag against each slot's - no table mapping one to the other, so a new kind of worn thing is
	 *  a data asset and a slot and nothing else. Returns false and changes nothing when no slot
	 *  will have it - including for items this component does not wear at all, which is the usual
	 *  answer and not a fault. Server only. */
	bool EquipInstance(const FSomnusItemInstance& Instance);

	/** The equipment and loose items a player starts a life with, as opposed to what they always
	 *  have. Called once per player by the game mode on their first spawn, never on a respawn -
	 *  coming back empty-handed is the point of dying, and BeginPlay cannot tell the two apart
	 *  because a respawned pawn is as new as a first one. Server only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void GrantStartingKit();

	// Nothing here equips or unequips any more. A worn slot is a grid, so a drag onto one or off
	// one is the same cross-grid move as any other, and it goes through USomnusLootComponent's
	// Server_MoveItem like the rest - which also means it is validated like the rest, where the
	// pair of RPCs that used to live here took a client's word for the grid it named.


	/** Takes an item away from this character and leaves it in the world in front of them,
	 *  contents and all. Covers both places an item can be - loose in one of the grids, or worn
	 *  in the rig or backpack slot. Returns false and changes nothing when nothing this
	 *  character holds answers to InstanceID, or when the pickup could not be spawned.
	 *  Server only. */
	bool DropItem(FGuid InstanceID);

	/** Mints a new item and spreads it across every container this character can reach, in
	 *  GetActiveContainers() order, merging into existing stacks everywhere before opening
	 *  any new cell. Returns the quantity that found no room. Server only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	int32 TryAddItemAnywhere(USomnusItemDataAsset* ItemDataAsset, int32 Quantity);

	/** Existing-instance counterpart. Consumes from ItemInstance in place and returns what is
	 *  left of it - on a non-zero return the caller still owns the remainder, and its
	 *  ContainerActor, and has to decide what happens to it. Server only. */
	int32 TryAddExistingItemAnywhere(FSomnusItemInstance& ItemInstance);
	
	/** What is worn in the slot answering to SlotTag, or a default-constructed instance when
	 *  nothing is - test InstanceID.IsValid() to tell the two apart, never StackCount, which
	 *  defaults to 1. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FSomnusItemInstance GetEquippedInstance(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	class ASomnusContainerActor* GetEquippedContainer(FGameplayTag SlotTag) const;

	// No GetSlot here any more. USomnusEquipmentSlotComponent::FindSlot asks the actor, which is
	// the only level the question has one answer at: slots are declared by more than one
	// component on purpose, and a caller wanting the primary weapon slot should not have to know
	// that storage is not where it lives.


protected:
	virtual void InitializeComponent() override;

	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<class ASomnusPickupActor> PickupActorClass;

	/** How far in front of the character a dropped item appears. Far enough that it does not
	 *  spawn inside the capsule and get shoved out sideways; near enough to still be in reach. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment", meta = (ClampMin = "0.0"))
	float DropDistance = 120.f;

	/** Radius of the circle a drop is scattered within. Emptying a container otherwise stacks
	 *  every item on one spot, where the bodies interpenetrate and shove each other apart. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment", meta = (ClampMin = "0.0"))
	float DropScatterRadius = 30.f;

	/** Containers worn from the start, routed by each asset's own item tag. Pockets belong here
	 *  like anything else now - they are granted, worn and reported by the same code as a rig,
	 *  and differ only in sitting in a slot nothing can drag them out of. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<TObjectPtr<USomnusContainerDataAsset>> DefaultEquipment;

	/** Loose items granted at spawn, placed wherever they fit across everything the character
	 *  can reach by then - pockets plus whatever DefaultEquipment granted. A quantity above
	 *  the item's MaxStackCount splits into as many stacks as it needs. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TMap<TObjectPtr<USomnusItemDataAsset>, int32> DefaultItems;
	
	/** The storage slots, in the order auto-placement should try them. Named members rather than
	 *  anything gathered from the actor because that order is a decision: a picked-up bandage goes
	 *  to a pocket before it opens a cell in the backpack, and GetComponents would hand them back
	 *  in whatever order they happen to sit in.
	 *
	 *  Constructor subobjects and nothing replicated: both machines build their own at the same
	 *  path, so the pointers are valid everywhere from construction, and what is worn replicates
	 *  as the grid contents each slot already is. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<class USomnusEquipmentSlotComponent> PocketSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<class USomnusEquipmentSlotComponent> BackpackSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<class USomnusEquipmentSlotComponent> RigSlot;

	/** The three above, in that order. */
	TArray<class USomnusEquipmentSlotComponent*> GetStorageSlots() const;

	/** Bound to both slots' add and remove delegates, on every machine. Replaces the hand-called
	 *  notify the replicated slots needed: a grid tells its listeners itself, and it tells them on
	 *  the authority too, so the listen server host stops being the one window that never
	 *  refreshed. Teardown for equipment leaving a slot hooks up here. */
	UFUNCTION()
	void HandleSlotContentsChanged(const FSomnusItemInstance& Item);

	// No writable counterpart to GetEquippedInstance any more. What is worn is an entry in a
	// replicated grid now, so it is written the way every other item is - through the slot's own
	// Add/Move/Remove, which mark the list dirty and notify. A raw pointer into the array would
	// skip both.

private:
	/** Destroys every container actor this character is still holding, worn ones and everything
	 *  nested inside them. Reached by descending through the slots rather than by asking the world
	 *  what this actor owns, which is what keeps a looter's prize out of it: a backpack that was
	 *  carried off stopped being reachable from these slots the moment it moved, and a body must
	 *  not take it down with itself. Server only. */
	void DestroyHeldContainers();

	/** The first slot on this actor that admits ItemData and has nothing in it, or null. The one
	 *  place the routing rule lives, so granting and the two-pass split below cannot come to
	 *  different answers about where something belongs. */
	class USomnusEquipmentSlotComponent* FindSlotFor(class USomnusItemDataAsset* ItemData) const;

	/** Grants the half of DefaultEquipment that lands in locked slots, or the half that does not.
	 *
	 *  One list read twice rather than two lists, because the difference is already recorded: a
	 *  slot nothing can drag an item out of holds something the wearer cannot lose, so it is
	 *  granted on every spawn, and everything else is what a death is meant to cost. Server only. */
	void GrantEquipment(bool bLockedSlotsOnly);

	/** Guards against a second GrantStartingKit. The game mode calls it once per player, but a
	 *  repeat would mint another set of loose items into whatever room was left rather than fail
	 *  visibly. Not replicated - the question only ever arises on the authority. */
	bool bStartingKitGranted = false;
};
