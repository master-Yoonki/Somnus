// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Zombie/SomnusGA_ZombieMeleeAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
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
	
	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, 1.f);
	DelayTask->OnFinish.AddDynamic(this, &USomnusGA_ZombieMeleeAttack::OnDelayFinished);
	DelayTask->ReadyForActivation();
}

void USomnusGA_ZombieMeleeAttack::OnDelayFinished()
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!SourceActor) return;
	
	const FVector Start = SourceActor->GetActorLocation();
	const FVector End = Start + SourceActor->GetActorForwardVector().GetSafeNormal() * 200.f;
	
	TArray<AActor*> ActorsToIgnore = {SourceActor};
	TArray<FHitResult> HitResults;
	
	UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		Start,
		End,
		45.f,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::Type::ForDuration,
		HitResults,
		true
		);
	
	for (const FHitResult& HitResult : HitResults)
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			if (ASomnusCharacter* SomnusCharacter = Cast<ASomnusCharacter>(HitActor))
			{
				UAbilitySystemComponent* TargetASC = SomnusCharacter->GetAbilitySystemComponent();
				if (!TargetASC) return;
				
				FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass);
				if (SpecHandle.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(SomnusTags::Data_Damage, 20.f);
					TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					// Prevents damage multiple characters
					break;
				}
			}
		}
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
