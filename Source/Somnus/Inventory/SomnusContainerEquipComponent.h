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

	/** Puts an already-formed container instance into the slot its data asset declares.
	 *  Returns false and changes nothing when the item is not a container, declares
	 *  Pockets, or the target slot is already occupied. Server only. */
	bool EquipInstance(const FSomnusItemInstance& Instance);
	
	/** Mints a new item and spreads it across every container this character can reach, in
	 *  GetActiveContainers() order, merging into existing stacks everywhere before opening
	 *  any new cell. Returns the quantity that found no room. Server only. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	int32 TryAddItemAnywhere(USomnusItemDataAsset* ItemDataAsset, int32 Quantity);

	/** Existing-instance counterpart. Consumes from ItemInstance in place and returns what is
	 *  left of it - on a non-zero return the caller still owns the remainder, and its
	 *  ContainerActor, and has to decide what happens to it. Server only. */
	int32 TryAddExistingItemAnywhere(FSomnusItemInstance& ItemInstance);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TObjectPtr<USomnusContainerDataAsset> PocketData;

	/** Containers worn from the start. Routed by each asset's own SlotType.
	 *  Leave empty to spawn with nothing worn. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<TObjectPtr<USomnusContainerDataAsset>> DefaultEquipment;

	/** Loose items granted at spawn, placed wherever they fit across everything the character
	 *  can reach by then - pockets plus whatever DefaultEquipment granted. A quantity above
	 *  the item's MaxStackCount splits into as many stacks as it needs. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TMap<TObjectPtr<USomnusItemDataAsset>, int32> DefaultItems;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Pocket)
	TObjectPtr<ASomnusContainerActor> Pocket;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedRig)
	FSomnusItemInstance EquippedRig;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedBackpack)
	FSomnusItemInstance EquippedBackpack;
	
	UFUNCTION()
	void OnRep_Pocket();
	
	UFUNCTION()
	void OnRep_EquippedRig(FSomnusItemInstance Old);

	UFUNCTION()
	void OnRep_EquippedBackpack(FSomnusItemInstance Old);
	

	/** Reaction to an equipped slot changing. Driven by the OnReps on clients, and called
	 *  directly on the server, which never receives them. */
	void HandleEquippedChanged(EContainerSlotType SlotType, const FSomnusItemInstance& OldInstance);
};
