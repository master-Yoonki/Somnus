// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_TriggerAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_TriggerAbility::UBTTask_TriggerAbility()
{
	NodeName = TEXT("Trigger Ability");

	// The binding and the tree it reports to are kept on the node, so each tree needs its own copy.
	bCreateNodeInstance = true;

	// OnTaskFinished is only called for nodes that ask for it, and it is where the binding is cleared.
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_TriggerAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	UAbilitySystemComponent* ASC = AIController
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AIController->GetPawn())
		: nullptr;
	if (!ASC || !AbilityTagToActivate.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	const FGameplayTagContainer AbilityTags(AbilityTagToActivate);

	if (!bWaitForAbilityEnd)
	{
		return ASC->TryActivateAbilitiesByTag(AbilityTags) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
	}

	// Listen before activating. An ability can end inside its own activation - failing a check, or
	// finishing at once - and a listener bound afterwards would wait for an end that already came.
	ListenedASC = ASC;
	WaitingTree = &OwnerComp;
	AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &UBTTask_TriggerAbility::OnAbilityEnded);

	bActivating = true;
	bEndedDuringActivation = false;
	const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags);
	bActivating = false;

	if (!bActivated)
	{
		return EBTNodeResult::Failed;
	}
	if (bEndedDuringActivation)
	{
		return bEndedCancelled ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::InProgress;
}

void UBTTask_TriggerAbility::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	// Every ability on the component reports here; only the one this task started counts.
	if (!AbilityEndedData.AbilityThatEnded || !AbilityEndedData.AbilityThatEnded->GetAssetTags().HasTag(AbilityTagToActivate))
	{
		return;
	}

	if (bActivating)
	{
		bEndedDuringActivation = true;
		bEndedCancelled = AbilityEndedData.bWasCancelled;
		return;
	}

	UBehaviorTreeComponent* Tree = WaitingTree.Get();
	StopListening();
	if (Tree)
	{
		FinishLatentTask(*Tree, AbilityEndedData.bWasCancelled ? EBTNodeResult::Failed : EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_TriggerAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Stop listening first: cancelling below ends the ability, and its end must not come back in as
	// a finish for a task that is already being aborted.
	UAbilitySystemComponent* ASC = ListenedASC.Get();
	StopListening();

	if (ASC)
	{
		const FGameplayTagContainer AbilityTags(AbilityTagToActivate);
		ASC->CancelAbilities(&AbilityTags);
	}
	return EBTNodeResult::Aborted;
}

void UBTTask_TriggerAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// Every way out lands here - succeeded, failed, aborted, or the activation never starting - so
	// this is where the binding is guaranteed to go.
	StopListening();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_TriggerAbility::StopListening()
{
	if (UAbilitySystemComponent* ASC = ListenedASC.Get())
	{
		ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
	}
	AbilityEndedHandle.Reset();
	ListenedASC.Reset();
	WaitingTree.Reset();
}

FString UBTTask_TriggerAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Trigger %s%s"),
		*AbilityTagToActivate.ToString(),
		bWaitForAbilityEnd ? TEXT(", wait for end") : TEXT(""));
}
