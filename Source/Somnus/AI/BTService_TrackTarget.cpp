// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_TrackTarget.h"

#include "AIController.h"
#include "SomnusZombieAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_TrackTarget::UBTService_TrackTarget()
{
	NodeName = TEXT("Track Target");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTService_TrackTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ASomnusZombieAIController* ZombieAIController = Cast<ASomnusZombieAIController>(AIController))
		{
			AActor* TargetActor = ZombieAIController->GetCurrentTarget();
			if (!TargetActor) return;
			
			ZombieAIController->UpdateTrackingData(
				TargetActor->GetActorLocation(), 
				TargetActor->GetVelocity(), 
				GetWorld()->GetTimeSeconds()
				);

			if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
			{
				BB->SetValueAsVector("CurrentChaseLocation", TargetActor->GetActorLocation());
			}
				
			APawn* ControlledPawn = ZombieAIController->GetPawn();
			if (ControlledPawn)
			{
				const float Distance = FVector::Distance(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
				if (Distance > ZombieAIController->GetHuntDisengageDistance())
				{
					ZombieAIController->SetState(EZombieState::Aware);
				}
			}
		}
	}
}
