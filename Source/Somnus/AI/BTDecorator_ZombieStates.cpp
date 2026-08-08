// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTDecorator_ZombieStates.h"

#include "BehaviorTree/BlackboardComponent.h"

bool UBTDecorator_ZombieStates::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
	{
		EZombieState CurrentState = static_cast<EZombieState>(BlackboardComponent->GetValueAsEnum(GetSelectedBlackboardKey()));
		return AllowedZombieStates.Contains(CurrentState);
	}
	
	return true;
}
