// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_AttackSlot.h"

#include "AI/SomnusAttackSlotComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTService_AttackSlot::UBTService_AttackSlot()
{
	NodeName = TEXT("Attack Slot");
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	// Leaving the branch has to give the slot back, or a zombie that wandered off keeps it forever.
	bNotifyCeaseRelevant = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_AttackSlot, TargetActorKey),
		AActor::StaticClass());
	ChaseGoalKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_AttackSlot, ChaseGoalKey));
}

void UBTService_AttackSlot::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// Key selectors declared on the node itself have to be resolved against the tree's blackboard.
	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		ChaseGoalKey.ResolveSelectedKey(*BBAsset);
	}
}

void UBTService_AttackSlot::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Zombie = Controller ? Controller->GetPawn() : nullptr;
	if (!Blackboard || !Zombie) return;

	const AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	USomnusAttackSlotComponent* SlotComponent = FindSlotComponent(Target);
	if (!SlotComponent)
	{
		// A target with no ring is still chased straight at; no target leaves nowhere to go.
		if (Target)
		{
			Blackboard->SetValueAsVector(ChaseGoalKey.SelectedKeyName, Target->GetActorLocation());
		}
		else
		{
			Blackboard->ClearValue(ChaseGoalKey.SelectedKeyName);
		}
		return;
	}

	//check if it already has slot index
	// if it has, but it is not valid, or if not, check range and reassign if needed
	// if it has but outside the range, release
	const float Distance = FVector::Dist2D(Zombie->GetActorLocation(), Target->GetActorLocation());
	
	int32 AssignedKey = SlotComponent->FindSlotOf(Zombie);
	
	FVector OutLocation;
	bool bHasValidKey = 
		AssignedKey != INDEX_NONE &&
			SlotComponent->GetSlotLocation(AssignedKey, OutLocation);
	
	if (bHasValidKey)
	{
		if (Distance > ReleaseRadius)
		{
			SlotComponent->ReleaseSlot(Zombie);
		}
		else
		{
			SlotComponent->RequestSlot(Zombie);
		}
	}
	else
	{
		if (Distance <= RequestRadius)
		{
			SlotComponent->RequestSlot(Zombie);
		}
		else
		{
			SlotComponent->ReleaseSlot(Zombie);
		}
	}
	
	AssignedKey = SlotComponent->FindSlotOf(Zombie);
	if (AssignedKey != INDEX_NONE && SlotComponent->GetSlotLocation(AssignedKey, OutLocation))
	{
		Blackboard->SetValueAsVector(ChaseGoalKey.SelectedKeyName, OutLocation);
	}
	else
	{
		Blackboard->SetValueAsVector(ChaseGoalKey.SelectedKeyName, Target->GetActorLocation());
	}
}

void UBTService_AttackSlot::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Zombie = Controller ? Controller->GetPawn() : nullptr;
	if (!Blackboard || !Zombie) return;

	const AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	USomnusAttackSlotComponent* SlotComponent = FindSlotComponent(Target);
	
	if (!SlotComponent)
	{
		Blackboard->ClearValue(ChaseGoalKey.SelectedKeyName);
		return;
	}
	SlotComponent->ReleaseSlot(Zombie);
	Blackboard->ClearValue(ChaseGoalKey.SelectedKeyName);
}

USomnusAttackSlotComponent* UBTService_AttackSlot::FindSlotComponent(const AActor* Target)
{
	return Target ? Target->FindComponentByClass<USomnusAttackSlotComponent>() : nullptr;
}

FString UBTService_AttackSlot::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nRing of %s, ask within %.0fcm, give back past %.0fcm"),
		*Super::GetStaticDescription(),
		*TargetActorKey.SelectedKeyName.ToString(),
		RequestRadius, ReleaseRadius);
}
