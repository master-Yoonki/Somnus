// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SomnusItemInstance.h"
#include "Core/SomnusInteractable.h"
#include "GameFramework/Actor.h"
#include "SomnusPickupActor.generated.h"

UCLASS()
class SOMNUS_API ASomnusPickupActor : public AActor, public ISomnusInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASomnusPickupActor();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** Already runs on the server: the trace that finds this actor is server-side, so there is
	 *  no client hop left to make. A Server RPC on this actor would be dropped anyway - a
	 *  client owns its own pawn, never loose world loot. */
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual void SetHighlighted_Implementation(bool bHighlighted) override;
	
	void InitializeFromInstance(const FSomnusItemInstance* Instance);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Puts PickupItem on Interactor if anything they wear has an empty place for it, and hands it
	 *  to their storage otherwise, contents and all. Destroys this actor once nothing is left of
	 *  the stack; a partial take leaves the remainder standing in the world. Server only. */
	bool PickUp(AActor* Interactor);

	/** Offers PickupItem to each of Interactor's equipment components in turn. True when one of
	 *  them took it, in which case the item is worn and this actor has nothing left to hold. */
	bool TryWearItem(AActor* Interactor);

	/** Turns the body loose. Deliberately not called from BeginPlay - see the comment there. */
	void BeginPhysicsSimulation();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Points the mesh component at whatever this pickup currently describes - the live instance
	 *  at runtime, SpawnItemData while it is only placed. Safe to call more than once and on
	 *  either side of the network; every entry point does. */
	void RefreshPickupMesh();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_PickupItem)
	FSomnusItemInstance PickupItem;

	/** Stands in for loot whose ItemData declares no WorldMesh, so it is still visible and
	 *  interactable rather than an invisible actor the trace can hit. Defaults to the engine
	 *  cube; override it per Blueprint for something less placeholder. */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TObjectPtr<UStaticMesh> FallbackMesh;

#if WITH_EDITORONLY_DATA
	/** Marks hand-placed loot in the viewport. The pickup mesh is no help there: it is resolved
	 *  from PickupItem at BeginPlay, so before PIE the actor may be showing nothing at all. */
	UPROPERTY()
	TObjectPtr<class USphereComponent> EditorVisualizer;
#endif

	/** Loot placed by hand in a level describes itself with these two and mints its instance on
	 *  BeginPlay. Loot dropped out of an inventory arrives with PickupItem already formed and
	 *  ignores them. */
	UPROPERTY(EditAnywhere)
	TObjectPtr<USomnusItemDataAsset> SpawnItemData;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
	int32 SpawnQuantity = 1;
	
private:
	UFUNCTION()
	void OnRep_PickupItem();
};
