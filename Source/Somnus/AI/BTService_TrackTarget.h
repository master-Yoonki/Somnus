// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_TrackTarget.generated.h"

/**
 * 
 */
UCLASS()
class SOMNUS_API UBTService_TrackTarget : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_TrackTarget();
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
