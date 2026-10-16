// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SomnusAttackSlotComponent.generated.h"

class UNavigationSystemV1;

USTRUCT(BlueprintType)
struct FAttackSlot
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	FVector LocationOffset = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	FVector ProjectedLocation = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TWeakObjectPtr<AActor> AssignedActor;
	
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	bool bIsValid = false;
};

UCLASS(ClassGroup = (Somnus), meta = (BlueprintSpawnableComponent))
class SOMNUS_API USomnusAttackSlotComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USomnusAttackSlotComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "AttackSlot")
	void DebugDraw();

	/** Server only. The slot this zombie holds, swapped for a free one only when that is nearer by
	 *  more than SwitchMargin; an invalid held slot is dropped first. Without one, the nearest free
	 *  valid slot. INDEX_NONE when none is free. */
	int32 RequestSlot(AActor* Zombie);

	/** The index of the slot this zombie holds, or INDEX_NONE. Never claims one. */
	int32 FindSlotOf(const AActor* Zombie) const;

	/** Server only. Frees whatever slot this zombie holds; nothing happens if it holds none. */
	void ReleaseSlot(AActor* Zombie);

	/** Where the slot at this index stands on the navmesh. False for a bad index or an invalid slot. */
	bool GetSlotLocation(int32 SlotIndex, FVector& OutLocation) const;
	
	bool IsValidKey(int32 SlotIndex) const;
	
protected:
	virtual void BeginPlay() override;

	/** Lays the ring out as evenly spaced offsets around the owner. */
	void BuildSlots();

	/** Re-fits every slot to the navmesh around where the owner stands now. */
	void UpdateSlots();

	/**
	 * Where a slot at this offset lands on the navmesh, pulled in short of anything in the way.
	 * False when there is no such place, or it ends up too close for a zombie to fit.
	 */
	bool ResolveSlotLocation(const UNavigationSystemV1& NavSys, const FVector& PlayerNavLocation,
		const FVector& Offset, FVector& OutLocation) const;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AttackSlot")
	float Range = 100.f;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AttackSlot")
	float MinSlotRange = 70.f;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AttackSlot")
	uint8 NumSlots = 6;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AttackSlot")
	FVector ProjectionExtent = FVector(50.f, 50.f, 150.f);
	
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleInstanceOnly, Category = "AttackSlot")
	TArray<FAttackSlot> ActiveSlots;
	
	UPROPERTY(EditAnywhere, Category = "AttackSlot", meta = (ClampMin = "0", Units = "cm"))
	float SwitchMargin = 50.f;
};
