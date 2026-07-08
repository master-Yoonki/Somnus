// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SomnusZombieAIController.h"

#include "NavigationSystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/SomnusCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

ASomnusZombieAIController::ASomnusZombieAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerceptionComponent");
	
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("SightConfig");
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ASomnusZombieAIController::OnTargetDetected);
}

void ASomnusZombieAIController::SetCurrentTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject("TargetActor", NewTarget);
	}
}

void ASomnusZombieAIController::UpdateTrackingData(FVector Location, FVector Velocity, float Time)
{
	LastKnownLocation = Location;
	LastKnownVelocity = Velocity;
	LastSeenTime = Time;
}

void ASomnusZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTree)
	{
		RunBehaviorTree(BehaviorTree);
		SetState(EZombieState::Unaware);
	}
	
}

void ASomnusZombieAIController::OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus)
{
	// Make sure we actually saw a Player character
	if (ASomnusCharacter* Player = Cast<ASomnusCharacter>(Actor))
	{
		// We also need to check if we just spotted them,
		// rather than the engine telling us we just lost sight of them!
		UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
		if (!BlackboardComponent) return;
		APawn* ControlledPawn = GetPawn();
		if (!ControlledPawn) return;
		FVector ActorLocation = Player->GetActorLocation();
		
		if (Stimulus.WasSuccessfullySensed())
		{
			const float DistanceToPlayer = FVector::Distance(ControlledPawn->GetActorLocation(), ActorLocation);
			if (DistanceToPlayer < HuntStartDistance)
			{
				if (CurrentState == EZombieState::Unaware || CurrentState == EZombieState::Aware)
				{
					CurrentTarget = Actor;
					BlackboardComponent->SetValueAsObject("TargetActor", Actor);
					LastKnownLocation = CurrentTarget->GetActorLocation();
					LastKnownVelocity = CurrentTarget->GetVelocity();
					LastSeenTime = GetWorld()->GetTimeSeconds();
					SetState(EZombieState::Hunt_Certain);
				}
			}
			else
			{
				if (CurrentState == EZombieState::Unaware || CurrentState == EZombieState::Aware)
				{
					// Just remember WHERE we saw them, but don't lock onto the Actor
					LastKnownLocation = ActorLocation;
					
					BlackboardComponent->SetValueAsVector("MemoryLocation", LastKnownLocation);
					// Transition to Aware
					SetState(EZombieState::Aware);
				}
			}

			// Re-acquisition: spotted player again while predicting
			if (CurrentState == EZombieState::Hunt_Predict)
			{
				CurrentTarget = Actor;
				BlackboardComponent->SetValueAsObject("TargetActor", Actor);
				SetState(EZombieState::Hunt_Certain);
			}
		}
		else
		{
			// Sight lost — only care if we lost our current chase target while in HuntCertain
			if (Actor == CurrentTarget && CurrentState == EZombieState::Hunt_Certain)
			{
				UpdateTrackingData(Actor->GetActorLocation(), Actor->GetVelocity(), GetWorld()->GetTimeSeconds());
				GetWorldTimerManager().SetTimer(GraceTimerHandle, this, &ASomnusZombieAIController::OnGraceDurationExpired, CertainGraceTime, false);
			}
		}
	}
}

void ASomnusZombieAIController::SetState(EZombieState NewState)
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
			// Exit logic for OLD state
		switch (CurrentState)
		{
		case EZombieState::Aware:
			{
				if (NewState != EZombieState::Aware)
				{
					GetWorldTimerManager().ClearTimer(AwareTimerHandle);
				}
			}
			break;
			
		case EZombieState::Hunt_Certain:
			{
				// Clear grace timer if re-acquiring target or leaving chase
				GetWorldTimerManager().ClearTimer(GraceTimerHandle);
			}
			break;
		default: break;
		}
		
		// Enter logic for NEW state
		switch (NewState)
		{
		case EZombieState::Aware:
			{
				// SetTimer on an existing handle automatically clears the old timer,
				// so this handles both fresh entry AND re-entry (timer reset)
				GetWorldTimerManager().SetTimer(AwareTimerHandle, this, &ASomnusZombieAIController::OnAwareDurationExpired, AwareDuration, false);
			}
			break;
		case EZombieState::Hunt_Predict:
			{
				// Extrapolate where the player likely went (overshoot by 2.5x to be aggressive)
				const float ElapsedTime = GetWorld()->GetTimeSeconds() - LastSeenTime;
				FVector TargetSpot = LastKnownLocation + (LastKnownVelocity * ElapsedTime * 2.5f);

				// Prevent projecting through walls by casting a ray
				FHitResult HitResult;
				FCollisionQueryParams QueryParams;
				if (APawn* ControlledPawn = GetPawn())
				{
					QueryParams.AddIgnoredActor(ControlledPawn);
				}
				
				if (GetWorld()->LineTraceSingleByChannel(HitResult, LastKnownLocation, TargetSpot, ECC_Visibility, QueryParams))
				{
					// We hit a wall! Cap the prediction at the wall
					TargetSpot = HitResult.Location;
				}

				// Snap to NavMesh — if extrapolation landed off-mesh, fall back to last known position
				FNavLocation ProjectedLocation;
				FVector FinalLocation;
				if (UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(TargetSpot, ProjectedLocation))
				{
					FinalLocation = ProjectedLocation.Location;
				}
				else
				{
					FinalLocation = LastKnownLocation;
				}

				BlackboardComponent->SetValueAsVector("PredictedLocation", FinalLocation);
			}
			break;
		default: break;
		}
		BlackboardComponent->SetValueAsEnum("ZombieState", static_cast<uint8>(NewState));
		CurrentState = NewState;
	}
}

void ASomnusZombieAIController::OnAwareDurationExpired()
{
	SetState(EZombieState::Unaware);
}

void ASomnusZombieAIController::OnGraceDurationExpired()
{
	SetState(EZombieState::Hunt_Predict);
}

