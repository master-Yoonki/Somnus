// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_SetZombieState.h"

#include "AIController.h"

UBTTask_SetZombieState::UBTTask_SetZombieState()
{
	NodeName = TEXT("Set Zombie State");
}

EBTNodeResult::Type UBTTask_SetZombieState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ASomnusZombieAIController* ZombieAIController = Cast<ASomnusZombieAIController>(AIController))
		{
			ZombieAIController->SetState(TargetState);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
