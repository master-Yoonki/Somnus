// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_TriggerAbility.generated.h"

/**
 * 
 */
UCLASS()
class SOMNUS_API UBTTask_TriggerAbility : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_TriggerAbility();
	
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTag AbilityTagToActivate;
	
	UFUNCTION()
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	class UBehaviorTreeComponent* CachedBTComponent;
	class UAbilitySystemComponent* CachedASC;
};
