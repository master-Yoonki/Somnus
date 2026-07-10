// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_RotateToFocus.generated.h"

/**
 * 
 */

struct FBTRotateToFocusMemory
{
	float ElapsedTime;
};


UCLASS()
class SOMNUS_API UBTTask_RotateToFocus : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
public:
	UBTTask_RotateToFocus();
	
	virtual uint16 GetInstanceMemorySize() const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
protected:
	UPROPERTY(EditAnywhere)
	float ToleranceInDegrees = 20.0f;
	UPROPERTY(EditAnywhere)
	float TimeoutInSeconds = 1.5f;
};
