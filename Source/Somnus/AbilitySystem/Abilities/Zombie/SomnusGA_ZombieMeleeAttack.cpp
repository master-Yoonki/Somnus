// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Zombie/SomnusGA_ZombieMeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/Tasks/SomnusAT_PlayMontageAndWaitForEvent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

USomnusGA_ZombieMeleeAttack::USomnusGA_ZombieMeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(SomnusTags::Ability_Zombie_Melee));
	CancelAbilitiesWithTag.AddTag(SomnusTags::State_Dead);
}

void USomnusGA_ZombieMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// Combined task: plays montage AND listens for melee hit events simultaneously
	FGameplayTagContainer EventTagFilter;
	EventTagFilter.AddTag(SomnusTags::Event_Melee_Hit);
	
	AActor* TargetActor = nullptr;
	// 1. First, try to get it from the Gameplay Event payload (Studio Standard)
	if (TriggerEventData && TriggerEventData->Target)
	{
		TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	}
	// 2. Fallback: If no payload was sent, grab it from the AI Blackboard
	else if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		// Make sure we get the AI Controller
		if (AAIController* AIC = Cast<AAIController>(Avatar->GetInstigatorController()))
		{
			if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				// IMPORTANT: Change "TargetActor" to whatever your Blackboard Key name actually is!
				TargetActor = Cast<AActor>(BB->GetValueAsObject(FName("TargetActor")));
			}
		}
	}
	UAnimMontage* AttackMontage = SelectAttackMontage(TargetActor); 

	USomnusAT_PlayMontageAndWaitForEvent* Task = USomnusAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this, NAME_None, AttackMontage, EventTagFilter, MontagePlayRate);

	Task->OnCompleted.AddDynamic(this, &USomnusGA_ZombieMeleeAttack::OnMontageCompleted);
	Task->OnBlendOut.AddDynamic(this, &USomnusGA_ZombieMeleeAttack::OnMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &USomnusGA_ZombieMeleeAttack::OnMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &USomnusGA_ZombieMeleeAttack::OnMontageCancelled);
	Task->EventReceived.AddDynamic(this, &USomnusGA_ZombieMeleeAttack::OnMeleeHit);

	Task->ReadyForActivation();
}

UAnimMontage* USomnusGA_ZombieMeleeAttack::SelectAttackMontage(AActor* TargetActor) {
	AActor* SourceActor = CurrentActorInfo->OwnerActor.Get();
	if (!SourceActor) return nullptr;
	if (!TargetActor) return nullptr;
	
	const float Distance = FVector::Dist2D(SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
	FRotator DirectionToTarget = UKismetMathLibrary::FindLookAtRotation(SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(DirectionToTarget, SourceActor->GetActorRotation());
	float AngleToTarget = DeltaRot.Yaw;
	
	TArray<FSomnusZombieAttackMontage*> ValidMontages;
	for (FSomnusZombieAttackMontage& MontageConfig : AttackMontages)
	{
		bool bAngleValid = false;

		if (MontageConfig.MinAngle <= MontageConfig.MaxAngle)
		{
			// Normal forward-facing case (e.g., -45 to 45)
			// Needs to be between both values
			bAngleValid = (AngleToTarget >= MontageConfig.MinAngle && AngleToTarget <= MontageConfig.MaxAngle);
		}
		else
		{
			// Wrapped case (e.g., Min is 135, Max is -135)
			// Needs to be greater than Min OR less than Max
			bAngleValid = (AngleToTarget >= MontageConfig.MinAngle || AngleToTarget <= MontageConfig.MaxAngle);
		}
	
		bool bIsRangeValid = false;
		
		if (FMath::IsWithin(Distance, MontageConfig.MinDistance, MontageConfig.MaxDistance))
		
			if (bAngleValid && bIsRangeValid)
			{
				ValidMontages.Add(&MontageConfig);
			}
	}
	
	if (ValidMontages.IsEmpty())
	{
		ValidMontages.Empty();
		for (FSomnusZombieAttackMontage& MontageConfig : AttackMontages)
		{
			ValidMontages.Add(&MontageConfig);
		}
		return PickByPriority(ValidMontages)->Montage;
	}
	return PickByPriority(ValidMontages)->Montage;
}

FSomnusZombieAttackMontage* USomnusGA_ZombieMeleeAttack::PickByPriority(
	const TArray<FSomnusZombieAttackMontage*>& MontagesArray)
{
	int32 HighestPriority = MAX_int32;
		
	for (FSomnusZombieAttackMontage* Montage : MontagesArray)
	{
		if (Montage->Priority < HighestPriority)
		{
			HighestPriority = Montage->Priority;
		}
	}
	
	TArray<FSomnusZombieAttackMontage*> ValidMontages;
	for (FSomnusZombieAttackMontage* Montage : MontagesArray)
	{
		if (Montage->Priority == HighestPriority)
		{
			ValidMontages.Add(Montage);
		}
	}
	
	if (ValidMontages.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, ValidMontages.Num() - 1);
		return ValidMontages[RandomIndex];
	}
	
	return nullptr;
}

void USomnusGA_ZombieMeleeAttack::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USomnusGA_ZombieMeleeAttack::OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USomnusGA_ZombieMeleeAttack::OnMeleeHit(FGameplayTag EventTag, FGameplayEventData EventData)
{
	if (!DamageEffectClass || !EventData.Target)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
		const_cast<AActor*>(EventData.Target.Get()));
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SomnusTags::Data_Damage, DamageAmount);
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
