// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_TriggerAbility.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"

#include "Character/Zombie/SomnusZombieCharacter.h"

UBTTask_TriggerAbility::UBTTask_TriggerAbility()
{
	// Required to prevent memory sharing between multiple zombie instances so we can safely cache the BTComponent 
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_TriggerAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;
	ACharacter* Character = AIController->GetCharacter();
	if (!Character) return EBTNodeResult::Failed;
	ASomnusZombieCharacter* ZombieCharacter = Cast<ASomnusZombieCharacter>(Character);
	if (!ZombieCharacter) return EBTNodeResult::Failed;
	UAbilitySystemComponent* ASC = ZombieCharacter->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;
	
	FGameplayTagContainer SingleTag;
	SingleTag.AddTag(AbilityTagToActivate);
	if (ASC->TryActivateAbilitiesByTag(SingleTag))
	{
		ASC->OnAbilityEnded.AddUObject(this, &UBTTask_TriggerAbility::OnAbilityEnded);
		CachedASC = ASC;
		CachedBTComponent = &OwnerComp;
		return EBTNodeResult::InProgress;
	}
	else
	{
		return EBTNodeResult::Failed;
	}
	
	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

void UBTTask_TriggerAbility::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (!CachedASC || !CachedBTComponent) return;
	if (AbilityEndedData.AbilityThatEnded->GetAssetTags().HasTag(AbilityTagToActivate))
	{
		CachedASC->OnAbilityEnded.RemoveAll(this);
		EBTNodeResult::Type NodeResult = AbilityEndedData.bWasCancelled ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
		FinishLatentTask(*CachedBTComponent, NodeResult);
	}
}
