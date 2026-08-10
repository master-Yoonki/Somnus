// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/SomnusZombieAIController.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_SetZombieMoveState.generated.h"

/**
 * Writes a movement state onto the blackboard.
 *
 * The engine's own "Set Enum Key" only offers an asset picker for its enum, which lists Blueprint
 * enum assets and nothing else - a native UENUM cannot be chosen there at all. This exists to
 * name the type at compile time, which also buys a real dropdown of values on the node.
 */
UCLASS(DisplayName = "Set Zombie Move State")
class SOMNUS_API UBTTask_SetZombieMoveState : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_SetZombieMoveState();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	ESomnusZombieMoveState MoveState = ESomnusZombieMoveState::Idle;
};
