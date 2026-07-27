// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SomnusItemTypes.h"
#include "Components/ActorComponent.h"
#include "Inventory/SomnusItemInstance.h"
#include "SomnusContainerEquipComponent.generated.h"

class USomnusContainerDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SOMNUS_API USomnusContainerEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USomnusContainerEquipComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FSomnusActiveContainerInfo> GetActiveContainers() const;

	/** Puts an already-formed container instance into the slot its data asset declares.
	 *  Returns false and changes nothing when the item is not a container, declares
	 *  Pockets, or the target slot is already occupied. Server only. */
	bool EquipInstance(const FSomnusItemInstance& Instance);

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
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	TObjectPtr<ASomnusContainerActor> Pocket;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedRig)
	FSomnusItemInstance EquippedRig;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedBackpack)
	FSomnusItemInstance EquippedBackpack;
	
	UFUNCTION()
	void OnRep_EquippedRig(FSomnusItemInstance Old);

	UFUNCTION()
	void OnRep_EquippedBackpack(FSomnusItemInstance Old);

	/** Reaction to an equipped slot changing. Driven by the OnReps on clients, and called
	 *  directly on the server, which never receives them. */
	void HandleEquippedChanged(EContainerSlotType SlotType, const FSomnusItemInstance& OldInstance);
};
