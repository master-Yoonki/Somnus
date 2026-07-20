// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Zombie/SomnusGA_ZombieMeleeAttack.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/SomnusGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"

USomnusGA_ZombieMeleeAttack::USomnusGA_ZombieMeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(SomnusTags::Ability_Zombie_Melee));
	CancelAbilitiesWithTag.AddTag(SomnusTags::State_Dead);
}

UAnimMontage* USomnusGA_ZombieMeleeAttack::GetMontageToPlay(const FGameplayEventData* TriggerEventData)
{
	AActor* TargetActor = nullptr;
	// 1. Prefer the target sent with the Gameplay Event payload.
	if (TriggerEventData && TriggerEventData->Target)
	{
		TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	}
	// 2. Fallback: no payload (e.g. activated from the Behavior Tree) — read the AI Blackboard.
	else if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		if (AAIController* AIC = Cast<AAIController>(Avatar->GetInstigatorController()))
		{
			if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				// IMPORTANT: Change "TargetActor" to whatever your Blackboard Key name actually is!
				TargetActor = Cast<AActor>(BB->GetValueAsObject(FName("TargetActor")));
			}
		}
	}

	return SelectAttackMontage(TargetActor);
}

UAnimMontage* USomnusGA_ZombieMeleeAttack::SelectAttackMontage(const AActor* TargetActor)
{
	AActor* SourceActor = CurrentActorInfo->OwnerActor.Get();
	if (!SourceActor) return nullptr;
	if (!TargetActor) return nullptr;
	if (AttackMontages.IsEmpty()) return nullptr;

	const float Distance = FVector::Dist2D(SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
	FRotator DirectionToTarget = UKismetMathLibrary::FindLookAtRotation(SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(DirectionToTarget, SourceActor->GetActorRotation());
	const float AngleToTarget = DeltaRot.Yaw;

	TArray<FSomnusZombieAttackMontage> ValidMontages;
	for (const FSomnusZombieAttackMontage& MontageConfig : AttackMontages)
	{
		bool bAngleValid;
		if (MontageConfig.MinAngle <= MontageConfig.MaxAngle)
		{
			// Normal forward-facing case (e.g., -45 to 45): between both values.
			bAngleValid = (AngleToTarget >= MontageConfig.MinAngle && AngleToTarget <= MontageConfig.MaxAngle);
		}
		else
		{
			// Wrapped case (e.g., Min 135, Max -135): greater than Min OR less than Max.
			bAngleValid = (AngleToTarget >= MontageConfig.MinAngle || AngleToTarget <= MontageConfig.MaxAngle);
		}

		const bool bIsRangeValid = FMath::IsWithin(Distance, MontageConfig.MinDistance, MontageConfig.MaxDistance);

		if (bAngleValid && bIsRangeValid)
		{
			ValidMontages.Add(MontageConfig);
		}
	}

	// No directional match — fall back to considering every montage.
	if (ValidMontages.IsEmpty())
	{
		ValidMontages = AttackMontages;
	}

	FSomnusZombieAttackMontage* Picked = PickByPriority(&ValidMontages);
	return Picked ? Picked->Montage : nullptr;
}

FSomnusZombieAttackMontage* USomnusGA_ZombieMeleeAttack::PickByPriority(
	TArray<FSomnusZombieAttackMontage>* MontagesArray)
{
	if (!MontagesArray || MontagesArray->IsEmpty())
	{
		return nullptr;
	}

	int32 HighestPriority = MAX_int32;
	for (const FSomnusZombieAttackMontage& Montage : *MontagesArray)
	{
		HighestPriority = FMath::Min(HighestPriority, Montage.Priority);
	}

	TArray<FSomnusZombieAttackMontage*> Candidates;
	for (FSomnusZombieAttackMontage& Montage : *MontagesArray)
	{
		if (Montage.Priority == HighestPriority)
		{
			Candidates.Add(&Montage);
		}
	}

	return Candidates.Num() > 0 ? Candidates[FMath::RandRange(0, Candidates.Num() - 1)] : nullptr;
}
