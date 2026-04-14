// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AsyncTasks/SomnusAsyncTask_ListenForAttributeChange.h"

USomnusAsyncTask_ListenForAttributeChange* USomnusAsyncTask_ListenForAttributeChange::ListenForAttributeChange(
	UAbilitySystemComponent* AbilitySystemComponent,
	FGameplayAttribute Attribute)
{
	USomnusAsyncTask_ListenForAttributeChange* Task = NewObject<USomnusAsyncTask_ListenForAttributeChange>();
	Task->ASC = AbilitySystemComponent;
	Task->AttributeToListenFor = Attribute;

	if (!IsValid(AbilitySystemComponent) || !Attribute.IsValid())
	{
		Task->RemoveFromRoot();
		return nullptr;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute)
		.AddUObject(Task, &USomnusAsyncTask_ListenForAttributeChange::AttributeChanged);

	return Task;
}

USomnusAsyncTask_ListenForAttributeChange* USomnusAsyncTask_ListenForAttributeChange::ListenForAttributesChange(
	UAbilitySystemComponent* AbilitySystemComponent,
	TArray<FGameplayAttribute> Attributes)
{
	USomnusAsyncTask_ListenForAttributeChange* Task = NewObject<USomnusAsyncTask_ListenForAttributeChange>();
	Task->ASC = AbilitySystemComponent;
	Task->AttributesToListenFor = Attributes;

	if (!IsValid(AbilitySystemComponent))
	{
		Task->RemoveFromRoot();
		return nullptr;
	}

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		if (Attribute.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute)
				.AddUObject(Task, &USomnusAsyncTask_ListenForAttributeChange::AttributeChanged);
		}
	}

	return Task;
}

void USomnusAsyncTask_ListenForAttributeChange::EndTask()
{
	if (IsValid(ASC))
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeToListenFor).RemoveAll(this);

		for (const FGameplayAttribute& Attribute : AttributesToListenFor)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
		}
	}

	SetReadyToDestroy();
	MarkAsGarbage();
}

void USomnusAsyncTask_ListenForAttributeChange::AttributeChanged(const FOnAttributeChangeData& Data)
{
	OnAttributeChanged.Broadcast(Data.Attribute, Data.NewValue, Data.OldValue);
}
