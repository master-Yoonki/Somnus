// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AbilitySystemComponent.h"
#include "SomnusAsyncTask_ListenForAttributeChange.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttributeChanged, FGameplayAttribute, Attribute, float, NewValue, float, OldValue);

/**
 * Blueprint async node that listens for attribute value changes on an ASC.
 * Usage: drag out from any ASC reference and call "Listen for Attribute Change".
 * Call EndTask() (e.g., in widget Destruct) to stop listening.
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class SOMNUS_API USomnusAsyncTask_ListenForAttributeChange : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Listen for changes to a single attribute
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static USomnusAsyncTask_ListenForAttributeChange* ListenForAttributeChange(
		UAbilitySystemComponent* AbilitySystemComponent,
		FGameplayAttribute Attribute);

	// Listen for changes to multiple attributes
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static USomnusAsyncTask_ListenForAttributeChange* ListenForAttributesChange(
		UAbilitySystemComponent* AbilitySystemComponent,
		TArray<FGameplayAttribute> Attributes);

	// Must be called to stop listening and allow GC
	UFUNCTION(BlueprintCallable)
	void EndTask();

	UPROPERTY(BlueprintAssignable)
	FOnAttributeChanged OnAttributeChanged;

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	FGameplayAttribute AttributeToListenFor;
	TArray<FGameplayAttribute> AttributesToListenFor;

	void AttributeChanged(const FOnAttributeChangeData& Data);
};
