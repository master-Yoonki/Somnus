// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_ProximityCheck.generated.h"

/**
 * Checks distance to currently perceived actors.
 * If any player is within HuntStartDistance, transitions to HuntCertain.
 * Attach to the Aware branch.
 */
UCLASS()
class SOMNUS_API UBTService_ProximityCheck : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_ProximityCheck();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
