// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_ProximityCheck.h"

#include "AIController.h"
#include "SomnusZombieAIController.h"
#include "Character/SomnusCharacter.h"
#include "Perception/AIPerceptionComponent.h"

UBTService_ProximityCheck::UBTService_ProximityCheck()
{
	NodeName = TEXT("Proximity Check");
	Interval = 0.3f;
	RandomDeviation = 0.05f;
}

void UBTService_ProximityCheck::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	ASomnusZombieAIController* ZombieController = Cast<ASomnusZombieAIController>(AIController);
	if (!ZombieController) return;

	APawn* ControlledPawn = ZombieController->GetPawn();
	if (!ControlledPawn) return;

	UAIPerceptionComponent* PerceptionComp = ZombieController->GetAIPerceptionComponent();
	if (!PerceptionComp) return;

	const float HuntDistance = ZombieController->GetHuntStartDistance();
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const float AttackDistance = 150.f;

	// Check all currently perceived actors for proximity
	TArray<AActor*> PerceivedActors;
	PerceptionComp->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);
	
	EZombieState CurrentState = ZombieController->GetCurrentState();
	
	for (AActor* Actor : PerceivedActors)
	{
		if (!Cast<ASomnusCharacter>(Actor)) continue;

		const float Distance = FVector::Distance(PawnLocation, Actor->GetActorLocation());
		// Instant override: If any player gets into melee range, instantly snap to attack regardless of current state.
		if (Distance < AttackDistance)
		{
			ZombieController->SetCurrentTarget(Actor);
			ZombieController->SetState(EZombieState::Attack);
			return;
		}
		if (Distance < HuntDistance)
		{
			// Lock onto this target and start chasing
			ZombieController->SetCurrentTarget(Actor);
			ZombieController->SetState(EZombieState::Hunt_Certain);
			return;
		}
	}
	if (CurrentState == EZombieState::Attack)
	{
		if (ZombieController->GetCurrentTarget() != nullptr)
		{
			ZombieController->SetState(EZombieState::Hunt_Predict);
		}
		else
		{
			ZombieController->SetState(EZombieState::Aware);
		}
	}
}
