// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "AI/SomnusZombieAIController.h"
#include "BTDecorator_ZombieStates.generated.h"

/**
 * 
 */
UCLASS()
class SOMNUS_API UBTDecorator_ZombieStates : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "State")
	TArray<EZombieState> AllowedZombieStates;
	
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
