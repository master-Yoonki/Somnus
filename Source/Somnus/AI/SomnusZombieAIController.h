// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SomnusZombieAIController.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogZombieAI, Log, All)

UENUM(BlueprintType)
enum class EZombieState : uint8
{
	Unaware,
	Aware,
	Hunt_Certain,
	Hunt_Predict,
	Attack
};
/**
 * 
 */
UCLASS()
class SOMNUS_API ASomnusZombieAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASomnusZombieAIController();

	// --- Public API (called by BT nodes) ---

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetState(EZombieState NewState, const FString& Reason = TEXT("UnSpecified"));

	TObjectPtr<AActor> GetCurrentTarget() const { return CurrentTarget; }
	void SetCurrentTarget(AActor* NewTarget);
	float GetHuntStartDistance() const { return HuntStartDistance; }
	float GetHuntDisengageDistance() const { return HuntDisengageDistance; }
	UAIPerceptionComponent* GetAIPerceptionComponent() const { return AIPerceptionComponent; }
	void UpdateTrackingData(FVector Location, FVector Velocity, float Time);

	
	UFUNCTION(BlueprintPure, Category = "AI|State")
	EZombieState GetCurrentState() const { return CurrentState; }
protected:
	// --- Overrides ---

	virtual void OnPossess(APawn* InPawn) override;

private:
	// --- Components ---
	void ExecuteTransitionLogic(UBlackboardComponent* BlackboardComponent, EZombieState OldState, EZombieState NewState);
	UPROPERTY(EditDefaultsOnly, Category = "AI|Components")
	TObjectPtr<class UBehaviorTree> BehaviorTree;

	UPROPERTY(VisibleAnywhere, Category = "AI|Components")
	TObjectPtr<class UAIPerceptionComponent> AIPerceptionComponent;

	TObjectPtr<class UAISenseConfig_Sight> SightConfig;

	// --- Perception ---

	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus);

	// --- State ---

	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AI|State", meta = (AllowPrivateAccess = "true"))
	EZombieState CurrentState;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AI|State", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> CurrentTarget;

	// --- Tracking Data (updated by BTService_TrackTarget) ---

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AI|Tracking", meta = (AllowPrivateAccess = "true"))
	FVector LastKnownLocation;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AI|Tracking", meta = (AllowPrivateAccess = "true"))
	FVector LastKnownVelocity;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AI|Tracking", meta = (AllowPrivateAccess = "true"))
	float LastSeenTime;

	// --- Timers ---

	UPROPERTY()
	FTimerHandle AwareTimerHandle;

	UFUNCTION()
	void OnAwareDurationExpired();

	UPROPERTY()
	FTimerHandle GraceTimerHandle;

	UFUNCTION()
	void OnGraceDurationExpired();

	// --- Configuration ---

	UPROPERTY(EditDefaultsOnly, Category = "AI|Config")
	float MemoryTimeout = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Config")
	float AwareDuration = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Config")
	float CertainGraceTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config", meta = (AllowPrivateAccess = "true"))
	float HuntStartDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Config")
	float HuntDisengageDistance = 1000.f;
};