// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "DetourCrowdAIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "SomnusZombieAIController.generated.h"

/** Coarse movement intent. The behavior tree picks one; what it implies lives on the controller. */
UENUM(BlueprintType)
enum class ESomnusZombieMoveState : uint8
{
	Idle,
	Wander,
	Alert,
	Chase
};

/** Everything about how a zombie moves in one state. Blended as a unit so speed, acceleration and
 *  turn rate never disagree about which state the zombie is in. */
USTRUCT(BlueprintType)
struct FSomnusZombieMoveConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Move", meta = (ClampMin = "0.0"))
	float MaxWalkSpeed = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Move", meta = (ClampMin = "0.0"))
	float MaxAcceleration = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Move", meta = (ClampMin = "0.0"))
	float BrakingDecelerationWalking = 500.0f;

	/** Yaw degrees per second. This is the axis that reads as "how much it wants you". */
	UPROPERTY(EditDefaultsOnly, Category = "Move", meta = (ClampMin = "0.0"))
	float RotationRateYaw = 180.0f;
};

/**
 * Zombie controller: runs a behavior tree and owns everything the tree cannot see for itself -
 * who is being chased, how long ago they were lost, and how the pawn moves in each state.
 *
 * The division is that the tree decides what to do and the controller decides what is true.
 * TargetActor in particular has exactly one writer, here, because acquiring and releasing a target
 * are both perception events; a tree service could only poll for a transition already delivered as
 * an event, and would stop ticking on the branch that needs it most.
 *
 * Sight is tuned on the AI Perception component in the blueprint (Senses Config -> AI Sight),
 * not from here. The constructor only seeds sane starting values.
 */
UCLASS()
class SOMNUS_API ASomnusZombieAIController : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	ASomnusZombieAIController();

	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	virtual FPathFollowingRequestResult MoveTo(const FAIMoveRequest& MoveRequest,
	                                           FNavPathSharedPtr* OutPath = nullptr) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<class UBehaviorTree> BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	bool bLogSightEvents = true;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Animation")
	FName ScanYawCurveName = FName("SightScanYawOffset");

	UPROPERTY(EditDefaultsOnly, Category = "AI|Animation")
	bool bLogScanYaw = true;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	TSubclassOf<AActor> DetectClassFilter;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", Units = "Seconds"))
	float GraceTime = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FName TargetActorKeyName = FName("TargetActor");

	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FName LastKnownLocationKeyName = FName("LastKnownLocation");

	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FName HasReactedKeyName = FName("bHasReactedToTarget");
	
	/** How far a chased actor must stray from where its path was built before the path is
	 *  rebuilt. The engine hardcodes 100, which a strafing player clears in a single step -
	 *  the route then flips sides around cover every time they change direction. 0 keeps
	 *  the engine default. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Movement", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float GoalTetherDistance = 400.0f;

	/** Blackboard enum key naming the current movement state. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Movement")
	FName MoveStateKeyName = FName("MoveState");

	/** Movement per state. The tree only ever sets the state, never a number, so retuning every
	 *  zombie is one blueprint edit and nobody has to open the tree to find where 150 came from. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Movement")
	TMap<ESomnusZombieMoveState, FSomnusZombieMoveConfig> MoveConfigByState;

	/** FInterpTo rate used when the new state is faster than the current one. Kept high so a
	 *  zombie that spots you lunges rather than easing into it. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float MoveConfigRampUpRate = 6.0f;

	/** FInterpTo rate used when the new state is slower. Kept low so losing you bleeds off over
	 *  a few seconds instead of the zombie visibly giving up the instant you break line of sight. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float MoveConfigRampDownRate = 1.2f;
public:
	virtual void Tick(float DeltaSeconds) override;

private:
	EBlackboardNotificationResult OnMoveStateChanged(const UBlackboardComponent& BlackboardComp, FBlackboard::FKey KeyID);

	/** Reads the blackboard state and points TargetMoveConfig at its entry. */
	void RefreshTargetMoveConfig();

	/** Writes LiveMoveConfig onto the movement component. */
	void ApplyLiveMoveConfig();

	/** Where the current state wants the zombie to end up. */
	FSomnusZombieMoveConfig TargetMoveConfig;

	/** What the movement component is actually set to right now, chasing Target over time. */
	FSomnusZombieMoveConfig LiveMoveConfig;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	/** Re-reads the perception set, keeps or re-picks the target, and starts the grace timer once
	 *  nobody is left in sight. Because a target leaving the set is re-picked here immediately, the
	 *  set can only empty on the loss of the current target - which is what lets the stimulus that
	 *  emptied it be trusted as the place to search. */
	void RefreshTarget(const FAIStimulus& Stimulus);

	/** Picks a target from the perceived set. Only called when there is no target worth keeping. */
	AActor* SelectTarget(const TArray<AActor*>& Candidates) const;

	/** Hands the target over to the search branch. All three writes belong to the single event of
	 *  going back to seeing nobody, so they live together rather than being spread over the tree. */
	void OnGraceExpired();

	FTimerHandle GraceTimerHandle;

	/** Where the target stood when sight broke. Written once per loss and never refreshed, so the
	 *  search destination cannot creep along with a player the zombie can no longer see. */
	FVector CachedLostLocation = FVector::ZeroVector;
};
