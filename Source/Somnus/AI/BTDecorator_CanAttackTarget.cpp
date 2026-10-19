// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTDecorator_CanAttackTarget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "AbilitySystem/Abilities/Zombie/SomnusGA_ZombieMeleeAttack.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/SomnusGameplayTags.h"

UBTDecorator_CanAttackTarget::UBTDecorator_CanAttackTarget()
{
	NodeName = TEXT("Can Attack Target");
	AbilityTag = SomnusTags::Ability_Zombie_Melee;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_CanAttackTarget, TargetActorKey),
		AActor::StaticClass());

	// Sets bNotifyTick from the TickNode override, so the condition is polled while the branch is live.
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

void UBTDecorator_CanAttackTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

bool UBTDecorator_CanAttackTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !Blackboard) return false;

	const AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	const USomnusGA_ZombieMeleeAttack* Attack = FindAttackAbility(Pawn);
	if (!Target || !Attack) return false;

	return Attack->HasReachableMontage(Pawn, Target);
}

void UBTDecorator_CanAttackTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// Only asks the tree to re-evaluate when the answer has actually flipped.
	ConditionalFlowAbort(OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}

const USomnusGA_ZombieMeleeAttack* UBTDecorator_CanAttackTarget::FindAttackAbility(const APawn* Pawn) const
{
	const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(Pawn);
	const UAbilitySystemComponent* ASC = AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
	if (!ASC) return nullptr;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		// The class default is enough: the reach check reads only the montage table, never state
		// from an activation.
		const USomnusGA_ZombieMeleeAttack* Attack = Cast<USomnusGA_ZombieMeleeAttack>(Spec.Ability);
		if (Attack && Attack->GetAssetTags().HasTag(AbilityTag))
		{
			return Attack;
		}
	}
	return nullptr;
}

FString UBTDecorator_CanAttackTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s can reach %s"), *Super::GetStaticDescription(),
		*AbilityTag.ToString(), *TargetActorKey.SelectedKeyName.ToString());
}
