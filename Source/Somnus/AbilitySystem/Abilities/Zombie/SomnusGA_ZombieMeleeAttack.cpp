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

	// Held for the whole swing, so anything that must not pull the body away mid-attack (the ring
	// reassigning its slot) can tell without knowing which ability is running.
	ActivationOwnedTags.AddTag(SomnusTags::State_Attacking);
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
				TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKeyName));
			}
		}
	}

	return SelectAttackMontage(TargetActor);
}

UAnimMontage* USomnusGA_ZombieMeleeAttack::SelectAttackMontage(const AActor* TargetActor)
{
	const AActor* SourceActor = CurrentActorInfo ? CurrentActorInfo->OwnerActor.Get() : nullptr;
	if (!SourceActor || !TargetActor) return nullptr;

	float Distance, Angle;
	MeasureTarget(SourceActor, TargetActor, Distance, Angle);

	TArray<FSomnusZombieAttackMontage> ValidMontages;
	for (const FSomnusZombieAttackMontage& MontageConfig : AttackMontages)
	{
		if (CanSwing(MontageConfig, Distance, Angle))
		{
			ValidMontages.Add(MontageConfig);
		}
	}

	// Nothing reaches from here: no montage, so the ability ends without swinging at air.
	FSomnusZombieAttackMontage* Picked = PickByPriority(&ValidMontages);
	if (Picked)
	{
		WarpContactDistance = Picked->WarpContactDistance;
		WarpTarget = TargetActor;
	}
	else
	{
		WarpContactDistance = 0.f;
		WarpTarget.Reset();
	}
	return Picked ? Picked->Montage : nullptr;
}

bool USomnusGA_ZombieMeleeAttack::HasReachableMontage(const AActor* Source, const AActor* Target) const
{
	if (!Source || !Target) return false;

	float Distance, Angle;
	MeasureTarget(Source, Target, Distance, Angle);

	// Asked every tick by the tree, so it answers without building the candidate list.
	return AttackMontages.ContainsByPredicate([Distance, Angle](const FSomnusZombieAttackMontage& MontageConfig)
	{
		return CanSwing(MontageConfig, Distance, Angle);
	});
}

void USomnusGA_ZombieMeleeAttack::MeasureTarget(const AActor* Source, const AActor* Target, float& OutDistance,
	float& OutAngle)
{
	const FVector SourceLocation = Source->GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();

	OutDistance = FVector::Dist2D(SourceLocation, TargetLocation);

	const FRotator DirectionToTarget = UKismetMathLibrary::FindLookAtRotation(SourceLocation, TargetLocation);
	OutAngle = UKismetMathLibrary::NormalizedDeltaRotator(DirectionToTarget, Source->GetActorRotation()).Yaw;
}

bool USomnusGA_ZombieMeleeAttack::CanSwing(const FSomnusZombieAttackMontage& MontageConfig, float Distance, float Angle)
{
	bool bAngleValid;
	if (MontageConfig.MinAngle <= MontageConfig.MaxAngle)
	{
		// Normal forward-facing case (e.g., -45 to 45): between both values.
		bAngleValid = (Angle >= MontageConfig.MinAngle && Angle <= MontageConfig.MaxAngle);
	}
	else
	{
		// Wrapped case (e.g., Min 135, Max -135): greater than Min OR less than Max.
		bAngleValid = (Angle >= MontageConfig.MinAngle || Angle <= MontageConfig.MaxAngle);
	}

	const bool bIsRangeValid = FMath::IsWithin(Distance, MontageConfig.MinDistance, MontageConfig.MaxDistance);

	return MontageConfig.Montage && bAngleValid && bIsRangeValid;
}

bool USomnusGA_ZombieMeleeAttack::GetWarpTarget(const AActor*& OutTarget, float& OutContactDistance) const
{
	if (WarpTarget.IsValid())
	{
		OutTarget = WarpTarget.Get();
		OutContactDistance = WarpContactDistance;
		return true;
	}
	return false;
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
