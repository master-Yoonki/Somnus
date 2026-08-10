// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SomnusZombieAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationData.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameFramework/Character.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

ASomnusZombieAIController::ASomnusZombieAIController()
{
	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// Seeds only. SensesConfig is an Instanced array, so a blueprint subclass gets its own deep
	// copy of this object and the engine reads that copy - anything written here at runtime would
	// land on an orphan. Tune sight in the blueprint under AI Perception -> Senses Config.
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;
	SightConfig->SetMaxAge(5.0f);

	// All three affiliation flags default to false, which senses nothing whatsoever. Without a
	// GenericTeamAgent implementation every actor reads as Neutral, so detect every affiliation
	// and narrow this once teams exist.
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*SightConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());

	// AAIController owns a PerceptionComponent pointer of its own, and GetPerceptionComponent()
	// returns that one - not whatever subobject we happen to hold. EQS's PerceivedActors
	// generator and the gameplay debugger both read it, so hand ours over.
	SetPerceptionComponent(*Perception);

	PrimaryActorTick.bCanEverTick = true;

	//                                        Speed  Accel  Braking  YawRate
	MoveConfigByState.Add(ESomnusZombieMoveState::Idle,   {  80.0f, 200.0f, 600.0f,  90.0f });
	MoveConfigByState.Add(ESomnusZombieMoveState::Wander, { 110.0f, 250.0f, 500.0f, 120.0f });
	MoveConfigByState.Add(ESomnusZombieMoveState::Alert,  { 140.0f, 400.0f, 500.0f, 240.0f });
	MoveConfigByState.Add(ESomnusZombieMoveState::Chase,  { 320.0f, 800.0f, 400.0f, 360.0f });
}

void ASomnusZombieAIController::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	Super::GetActorEyesViewPoint(OutLocation, OutRotation);
	
	if (const ACharacter* ZombieChar = Cast<ACharacter>(GetPawn()))
	{
		if (const USkeletalMeshComponent* MeshComp = ZombieChar->GetMesh())
		{
			if (const UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
			{
				const float ScanYaw = AnimInstance->GetCurveValue(ScanYawCurveName);
				OutRotation.Yaw += ScanYaw;

				// This runs on every perception update and every line-of-sight test, so only
				// report while the curve is actually driving something.
				if (bLogScanYaw && !FMath::IsNearlyZero(ScanYaw))
				{
					UE_LOG(LogTemp, Log, TEXT("[Zombie] %s = %.2f  ->  Yaw %.2f"),
						*ScanYawCurveName.ToString(), ScanYaw, OutRotation.Yaw);
				}
			}
		}
	}
}

FPathFollowingRequestResult ASomnusZombieAIController::MoveTo(const FAIMoveRequest& MoveRequest,
                                                                  FNavPathSharedPtr* OutPath)
{
	const FPathFollowingRequestResult Result = Super::MoveTo(MoveRequest, OutPath);

	// Only actor goals are observed for movement at all - a location goal never repaths, so
	// there is no tether to widen.
	if (GoalTetherDistance > 0.0f && MoveRequest.IsMoveToActorRequest())
	{
		if (const UPathFollowingComponent* PathComp = GetPathFollowingComponent())
		{
			if (const FNavPathSharedPtr Path = PathComp->GetPath())
			{
				Path->SetGoalActorTetherDistance(GoalTetherDistance);
			}
		}
	}

	return Result;
}

void ASomnusZombieAIController::BeginPlay()
{
	Super::BeginPlay();

	if (UAIPerceptionComponent* Perception = GetPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ASomnusZombieAIController::HandleTargetPerceptionUpdated);
	}
}

void ASomnusZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTree)
	{
		RunBehaviorTree(BehaviorTree);
	}

	// Registered after RunBehaviorTree because that is what creates the blackboard. Observing the
	// key rather than applying speed from a task means any writer - task, service, C++ - gets the
	// speed change for free, and the blackboard stays the single account of what state we are in.
	if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
	{
		const FBlackboard::FKey KeyID = BlackboardComp->GetKeyID(MoveStateKeyName);
		if (KeyID != FBlackboard::InvalidKey)
		{
			BlackboardComp->RegisterObserver(KeyID, this,
				FOnBlackboardChangeNotification::CreateUObject(this, &ASomnusZombieAIController::OnMoveStateChanged));
		}

		// Snap on possession instead of ramping - otherwise every zombie spends its first seconds
		// accelerating up from zero regardless of the state it spawned in.
		RefreshTargetMoveConfig();
		LiveMoveConfig = TargetMoveConfig;
		ApplyLiveMoveConfig();

		// Possession and BeginPlay do not have a fixed order, so a perception update can land while
		// there is no blackboard to write it to. A default stimulus carries an invalid location and
		// so cannot overwrite anything - this only recovers a target that was already in sight.
		RefreshTarget(FAIStimulus());
	}
}

EBlackboardNotificationResult ASomnusZombieAIController::OnMoveStateChanged(
	const UBlackboardComponent& BlackboardComp, FBlackboard::FKey KeyID)
{
	RefreshTargetMoveConfig();
	return EBlackboardNotificationResult::ContinueObserving;
}

void ASomnusZombieAIController::RefreshTargetMoveConfig()
{
	const UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (!BlackboardComp) return;

	const ESomnusZombieMoveState State =
		static_cast<ESomnusZombieMoveState>(BlackboardComp->GetValueAsEnum(MoveStateKeyName));

	// A state with no entry keeps the previous target rather than defaulting, so a half-filled
	// map degrades into "nothing changes" instead of snapping every zombie to a stop.
	if (const FSomnusZombieMoveConfig* Config = MoveConfigByState.Find(State))
	{
		TargetMoveConfig = *Config;
	}
}

void ASomnusZombieAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Speeding up and slowing down run at different rates on purpose: the lunge should land
	// immediately, the wind-down should linger. Picking the rate off speed alone keeps the whole
	// config moving as one - turn rate must not still be at chase values while speed says idle.
	const bool bRampingUp = TargetMoveConfig.MaxWalkSpeed > LiveMoveConfig.MaxWalkSpeed;
	const float Rate = bRampingUp ? MoveConfigRampUpRate : MoveConfigRampDownRate;

	if (Rate <= 0.0f)
	{
		LiveMoveConfig = TargetMoveConfig;
	}
	else
	{
		LiveMoveConfig.MaxWalkSpeed = FMath::FInterpTo(LiveMoveConfig.MaxWalkSpeed, TargetMoveConfig.MaxWalkSpeed, DeltaSeconds, Rate);
		LiveMoveConfig.MaxAcceleration = FMath::FInterpTo(LiveMoveConfig.MaxAcceleration, TargetMoveConfig.MaxAcceleration, DeltaSeconds, Rate);
		LiveMoveConfig.BrakingDecelerationWalking = FMath::FInterpTo(LiveMoveConfig.BrakingDecelerationWalking, TargetMoveConfig.BrakingDecelerationWalking, DeltaSeconds, Rate);
		LiveMoveConfig.RotationRateYaw = FMath::FInterpTo(LiveMoveConfig.RotationRateYaw, TargetMoveConfig.RotationRateYaw, DeltaSeconds, Rate);
	}

	ApplyLiveMoveConfig();
}

void ASomnusZombieAIController::ApplyLiveMoveConfig()
{
	const ACharacter* ZombieChar = Cast<ACharacter>(GetPawn());
	if (!ZombieChar) return;

	if (UCharacterMovementComponent* Movement = ZombieChar->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = LiveMoveConfig.MaxWalkSpeed;
		Movement->MaxAcceleration = LiveMoveConfig.MaxAcceleration;
		Movement->BrakingDecelerationWalking = LiveMoveConfig.BrakingDecelerationWalking;
		Movement->RotationRate = FRotator(0.0f, LiveMoveConfig.RotationRateYaw, 0.0f);
	}
}

void ASomnusZombieAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Which actor changed is deliberately ignored - the perception set is re-read instead. Deriving
	// "who can I see" from a single actor's transition gets it wrong the moment two players are
	// visible and only one of them is lost.
	RefreshTarget(Stimulus);
}

void ASomnusZombieAIController::RefreshTarget(const FAIStimulus& Stimulus)
{
	// Named Perception rather than PerceptionComponent because AAIController already owns a member
	// by that name (AIController.h:142) and shadowing it is an error under this project's warning level.
	UAIPerceptionComponent* Perception = GetPerceptionComponent();
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!Perception || !BlackboardComponent) return;

	TArray<AActor*> Perceived;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Perceived);

	// An empty filter passes everything on purpose: a controller nobody configured senses too much
	// rather than going silently blind, which is the failure that is actually hard to spot.
	if (DetectClassFilter)
	{
		Perceived.RemoveAll([this](const AActor* Candidate)
		{
			return !Candidate->IsA(DetectClassFilter);
		});
	}

	FTimerManager& TimerManager = GetWorldTimerManager();
	const UObject* CurrentTarget = BlackboardComponent->GetValueAsObject(TargetActorKeyName);

	if (!Perceived.IsEmpty())
	{
		TimerManager.ClearTimer(GraceTimerHandle);

		// Sticky: a target that is still visible is never traded for a closer one. Re-picking on
		// every update makes a zombie oscillate between two players standing at similar distances.
		if (!Perceived.Contains(CurrentTarget))
		{
			BlackboardComponent->SetValueAsObject(TargetActorKeyName, SelectTarget(Perceived));
		}
		return;
	}

	// Nothing in sight. The target is deliberately left set until the grace timer expires - that
	// window is what absorbs a pillar clipping the line of sight for a moment, and it is the only
	// reason the chase does not stop dead on the frame sight breaks.
	if (!CurrentTarget || TimerManager.IsTimerActive(GraceTimerHandle))
	{
		return;
	}

	// Because a target leaving the set is re-selected immediately above, the set can only empty on
	// the loss of the current target - which is what makes this stimulus the right one to remember.
	if (FAISystem::IsValidLocation(Stimulus.StimulusLocation))
	{
		CachedLostLocation = Stimulus.StimulusLocation;
	}

	// FTimerManager treats a non-positive rate as a request to clear (TimerManager.cpp:617), so a
	// zero grace would leave the target latched forever instead of releasing it at once.
	if (GraceTime > 0.0f)
	{
		TimerManager.SetTimer(GraceTimerHandle, this,
			&ASomnusZombieAIController::OnGraceExpired, GraceTime, false);
	}
	else
	{
		OnGraceExpired();
	}
}

AActor* ASomnusZombieAIController::SelectTarget(const TArray<AActor*>& Candidates) const
{
	// Co-op will want nearest, or threat once damage can vote. Stickiness lives in the caller, so
	// this only ever runs when there is no target to keep.
	return Candidates.IsEmpty() ? nullptr : Candidates[0];
}

void ASomnusZombieAIController::OnGraceExpired()
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent) return;

	// Order matters. Clearing the target is what aborts the chase branch, so the search branch has
	// to already have somewhere to go by the time it starts running.
	BlackboardComponent->SetValueAsVector(LastKnownLocationKeyName, CachedLostLocation);
	BlackboardComponent->SetValueAsBool(HasReactedKeyName, false);
	BlackboardComponent->ClearValue(TargetActorKeyName);

	if (bLogSightEvents)
	{
		UE_LOG(LogTemp, Log, TEXT("[Zombie] target released, searching from %s"),
			*CachedLostLocation.ToCompactString());
	}
}
