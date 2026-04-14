// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AsyncTasks/SomnusAsyncTask_ListenForCooldownChange.h"

USomnusAsyncTask_ListenForCooldownChange* USomnusAsyncTask_ListenForCooldownChange::ListenForCooldownChange(
	UAbilitySystemComponent* AbilitySystemComponent,
	FGameplayTagContainer InCooldownTags,
	bool bInUseServerCooldown)
{
	USomnusAsyncTask_ListenForCooldownChange* Task = NewObject<USomnusAsyncTask_ListenForCooldownChange>();
	Task->ASC = AbilitySystemComponent;
	Task->CooldownTags = InCooldownTags;
	Task->bUseServerCooldown = bInUseServerCooldown;

	if (!IsValid(AbilitySystemComponent) || InCooldownTags.Num() < 1)
	{
		Task->RemoveFromRoot();
		return nullptr;
	}

	// Listen for new GE applications to detect cooldown start
	AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(
		Task, &USomnusAsyncTask_ListenForCooldownChange::OnActiveGameplayEffectAdded);

	// Listen for cooldown tag changes to detect cooldown end
	for (const FGameplayTag& Tag : InCooldownTags)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(Task, &USomnusAsyncTask_ListenForCooldownChange::CooldownTagChanged);
	}

	return Task;
}

void USomnusAsyncTask_ListenForCooldownChange::EndTask()
{
	if (IsValid(ASC))
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);

		for (const FGameplayTag& Tag : CooldownTags)
		{
			ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		}
	}

	SetReadyToDestroy();
	MarkAsGarbage();
}

void USomnusAsyncTask_ListenForCooldownChange::OnActiveGameplayEffectAdded(
	UAbilitySystemComponent* Target,
	const FGameplayEffectSpec& SpecApplied,
	FActiveGameplayEffectHandle ActiveHandle)
{
	FGameplayTagContainer AssetTags;
	SpecApplied.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	SpecApplied.GetAllGrantedTags(GrantedTags);

	// Check if this GE grants any of our cooldown tags
	for (const FGameplayTag& Tag : CooldownTags)
	{
		if (GrantedTags.HasTagExact(Tag) || AssetTags.HasTagExact(Tag))
		{
			float TimeRemaining = 0.0f;
			float Duration = 0.0f;

			// Get the remaining time from the active GE handle
			const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandle);
			if (ActiveGE)
			{
				TimeRemaining = ActiveGE->GetTimeRemaining(ASC->GetWorld()->GetTimeSeconds());
				Duration = ActiveGE->GetDuration();
			}

			if (Duration > 0.0f)
			{
				// On server, always broadcast. On client, only if locally predicted or using server cooldown.
				if (ASC->GetOwnerRole() == ROLE_Authority
					|| (bUseServerCooldown && ASC->GetOwnerRole() != ROLE_Authority)
					|| (!bUseServerCooldown && ASC->IsOwnerActorAuthoritative()))
				{
					OnCooldownBegin.Broadcast(Tag, TimeRemaining, Duration);
				}
			}
		}
	}
}

void USomnusAsyncTask_ListenForCooldownChange::CooldownTagChanged(const FGameplayTag CooldownTag, int32 NewCount)
{
	if (NewCount == 0)
	{
		OnCooldownEnd.Broadcast(CooldownTag, 0.0f, 0.0f);
	}
}

bool USomnusAsyncTask_ListenForCooldownChange::GetCooldownRemainingForTag(
	const FGameplayTagContainer& Tags,
	float& TimeRemaining,
	float& Duration) const
{
	if (!IsValid(ASC) || Tags.Num() == 0)
	{
		TimeRemaining = 0.0f;
		Duration = 0.0f;
		return false;
	}

	TimeRemaining = 0.0f;
	Duration = 0.0f;

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tags);
	TArray<TPair<float, float>> DurationAndTimeRemaining = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	for (const auto& Pair : DurationAndTimeRemaining)
	{
		if (Pair.Key > TimeRemaining)
		{
			TimeRemaining = Pair.Key;
			Duration = Pair.Value;
		}
	}

	return TimeRemaining > 0.0f;
}
