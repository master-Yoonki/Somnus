// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_SetZombieMoveState.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SetZombieMoveState::UBTTask_SetZombieMoveState()
{
	NodeName = TEXT("Set Zombie Move State");

	// Restricts the key dropdown to enum keys of this exact type, so a tree cannot quietly point
	// this task at some unrelated key and write a number that means nothing there.
	BlackboardKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SetZombieMoveState, BlackboardKey),
		StaticEnum<ESomnusZombieMoveState>());
}

EBTNodeResult::Type UBTTask_SetZombieMoveState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	BlackboardComp->SetValueAsEnum(BlackboardKey.SelectedKeyName, static_cast<uint8>(MoveState));
	return EBTNodeResult::Succeeded;
}

FString UBTTask_SetZombieMoveState::GetStaticDescription() const
{
	return FString::Printf(TEXT("Set %s to %s"),
		*BlackboardKey.SelectedKeyName.ToString(),
		*UEnum::GetDisplayValueAsText(MoveState).ToString());
}
