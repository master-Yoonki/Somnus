// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SomnusZombieAIController.h"
#include "BTTask_SetZombieState.generated.h"

/**
 * Simple BT task that transitions the zombie to a specified state.
 * Reusable anywhere a BT-driven state change is needed.
 */
UCLASS()
class SOMNUS_API UBTTask_SetZombieState : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetZombieState();

	UPROPERTY(EditAnywhere, Category = "State")
	EZombieState TargetState;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
