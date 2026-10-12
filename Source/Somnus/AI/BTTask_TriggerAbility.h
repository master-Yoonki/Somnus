// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_TriggerAbility.generated.h"

class UAbilitySystemComponent;
struct FAbilityEndedData;

/**
 * Activates an ability on the controlled pawn by tag, and either reports straight away or holds the
 * branch until the ability ends - succeeding if it ran to the end, failing if it was cancelled.
 *
 * The pawn's ability system is found through its IAbilitySystemInterface, so any pawn that has one
 * works, not only zombies.
 */
UCLASS(DisplayName = "Trigger Ability")
class SOMNUS_API UBTTask_TriggerAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_TriggerAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Activates every granted ability carrying this tag. */
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTag AbilityTagToActivate;

	/** Hold the branch until the ability ends. Off reports success as soon as it activates, for an
	 *  ability the tree should not wait on - a scream fired while the chase carries on, say. */
	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bWaitForAbilityEnd = true;

private:
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	/** Drops the ability-ended binding, wherever the task ended from. */
	void StopListening();

	// One instance per tree (bCreateNodeInstance), so these are this task's alone. Weak, because the
	// pawn - and its ability system - can be destroyed while the task is still waiting.
	TWeakObjectPtr<UAbilitySystemComponent> ListenedASC;
	TWeakObjectPtr<UBehaviorTreeComponent> WaitingTree;
	FDelegateHandle AbilityEndedHandle;

	/** Set while TryActivate is running, so an ability that ends inside its own activation is
	 *  reported once activation returns rather than finishing a task that has not started yet. */
	bool bActivating = false;
	bool bEndedDuringActivation = false;
	bool bEndedCancelled = false;
};
