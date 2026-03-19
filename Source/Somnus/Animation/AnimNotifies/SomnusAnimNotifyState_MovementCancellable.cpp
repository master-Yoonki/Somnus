// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotifies/SomnusAnimNotifyState_MovementCancellable.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/SomnusGameplayTags.h"

void USomnusAnimNotifyState_MovementCancellable::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				ASC->AddLooseGameplayTag(SomnusTags::State_MovementCancellable);
			}
		}
	}
}

void USomnusAnimNotifyState_MovementCancellable::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				ASC->RemoveLooseGameplayTag(SomnusTags::State_MovementCancellable);
			}
		}
	}
}
