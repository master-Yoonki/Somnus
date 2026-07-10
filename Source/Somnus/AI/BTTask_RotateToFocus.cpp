// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_RotateToFocus.h"

#include "SomnusZombieAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_RotateToFocus::UBTTask_RotateToFocus()
{
	BlackboardKey.AddObjectFilter(this, 
		GET_MEMBER_NAME_CHECKED(UBTTask_RotateToFocus, BlackboardKey), 
		{AActor::StaticClass()});
	bNotifyTick = true;
}

uint16 UBTTask_RotateToFocus::GetInstanceMemorySize() const
{
	return sizeof(FBTRotateToFocusMemory);
}

void UBTTask_RotateToFocus::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
}

EBTNodeResult::Type UBTTask_RotateToFocus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent) return EBTNodeResult::Failed;
	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!TargetActor)
	{	
		UE_LOG(LogZombieAI, Warning, TEXT("Target Actor does not exist"));
		return EBTNodeResult::Failed;
	}
	FBTRotateToFocusMemory* RotateToFocusMemory = CastInstanceNodeMemory<FBTRotateToFocusMemory>(NodeMemory);
	if (!RotateToFocusMemory) return EBTNodeResult::Failed;
	
	RotateToFocusMemory->ElapsedTime = 0.f;
	
	AIController->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	
	return EBTNodeResult::InProgress;
}

void UBTTask_RotateToFocus::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	AAIController* AIController = OwnerComp.GetAIOwner();
	
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	auto FailTask = [&OwnerComp, &AIController, this]() 
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	};
	
	APawn* SourcePawn = AIController->GetPawn();
	if (!SourcePawn)
	{
		FailTask();
		return;
	}
	
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		FailTask();
		return;
	}
	
	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!TargetActor)
	{	
		UE_LOG(LogZombieAI, Warning, TEXT("Target Actor does not exist"));
		FailTask();
		return;
	}
	FBTRotateToFocusMemory* RotateToFocusMemory = CastInstanceNodeMemory<FBTRotateToFocusMemory>(NodeMemory);
	if (!RotateToFocusMemory)
	{
		FailTask();
		return;
	}
	
	RotateToFocusMemory->ElapsedTime += DeltaSeconds;
	
	if (RotateToFocusMemory->ElapsedTime > TimeoutInSeconds)
	{
		FailTask();
		return;
	}
	
	FRotator SourceToTargetRotator = 
		UKismetMathLibrary::FindLookAtRotation(
			SourcePawn->GetActorLocation(), 
			TargetActor->GetActorLocation());
	
	FRotator DeltaRotator = 
		UKismetMathLibrary::NormalizedDeltaRotator(
			AIController->GetControlRotation(), 
			SourceToTargetRotator);
	
	float DeltaYaw = DeltaRotator.Yaw;
	if (FMath::Abs(DeltaYaw) < ToleranceInDegrees)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
}

EBTNodeResult::Type UBTTask_RotateToFocus::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	
	if (!AIController)
	{
		return EBTNodeResult::Aborted;
	}
	
	AIController->ClearFocus(EAIFocusPriority::Gameplay);
	return Super::AbortTask(OwnerComp, NodeMemory);
}
