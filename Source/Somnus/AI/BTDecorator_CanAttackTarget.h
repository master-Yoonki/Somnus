// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_CanAttackTarget.generated.h"

class USomnusGA_ZombieMeleeAttack;

/**
 * Passes while the zombie's attack ability has a montage that reaches the target from where it
 * stands now. It asks the ability itself rather than holding a range of its own, so the tree can
 * never start an attack the ability would then refuse to swing.
 *
 * Re-checked every tick, since distance and angle change without any blackboard key changing; with
 * Observer Aborts set to Lower Priority it cuts into an approach the moment the target is in reach.
 */
UCLASS(DisplayName = "Can Attack Target")
class SOMNUS_API UBTDecorator_CanAttackTarget : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CanAttackTarget();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** The attack ability granted to this pawn whose asset tags carry AbilityTag, or null. */
	const USomnusGA_ZombieMeleeAttack* FindAttackAbility(const APawn* Pawn) const;

	/** Who is being attacked. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Identifies the attack ability among those the pawn was granted. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag AbilityTag;
};
