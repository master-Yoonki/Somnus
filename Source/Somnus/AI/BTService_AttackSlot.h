// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_AttackSlot.generated.h"

class USomnusAttackSlotComponent;

/**
 * Keeps this zombie's claim on a slot around its target: asks for one when it closes in, gives it
 * back when it falls behind or leaves the branch, and keeps the slot's location on the blackboard
 * for a MoveTo to follow.
 */
UCLASS(DisplayName = "Attack Slot")
class SOMNUS_API UBTService_AttackSlot : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_AttackSlot();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** The ring around this target, or null when it has none (or there is no target). */
	static USomnusAttackSlotComponent* FindSlotComponent(const AActor* Target);

	/** The player whose ring this zombie joins. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Where the chase heads: the held slot's location, or the target's own while no slot is held.
	 *  One key for both, so a single MoveTo re-paths in place instead of being aborted and restarted. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ChaseGoalKey;

	/** Closer than this, a zombie without a slot asks for one. */
	UPROPERTY(EditAnywhere, Category = "AttackSlot", meta = (ClampMin = "0", Units = "cm"))
	float RequestRadius = 600.f;

	/** Farther than this, a held slot is given back. Kept above RequestRadius so a zombie on the
	 *  boundary does not claim and release every update. */
	UPROPERTY(EditAnywhere, Category = "AttackSlot", meta = (ClampMin = "0", Units = "cm"))
	float ReleaseRadius = 800.f;
	
};
