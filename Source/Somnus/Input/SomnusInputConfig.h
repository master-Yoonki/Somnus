// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SomnusInputConfig.generated.h"

class UInputAction;

/**
 * Pairs an InputAction with a gameplay tag for routing.
 */
USTRUCT(BlueprintType)
struct FSomnusInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "Input"))
	FGameplayTag InputTag;
};

/**
 * Data asset that defines all input-to-tag mappings.
 * NativeInputActions map to C++ callbacks (Move, Look, Jump).
 * AbilityInputActions map to GAS ability activation by tag.
 */
UCLASS(BlueprintType, Const)
class SOMNUS_API USomnusInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// Actions bound to C++ callbacks (Move, Look, Jump)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FSomnusInputAction> NativeInputActions;

	// Actions bound to GAS ability activation by tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FSomnusInputAction> AbilityInputActions;

	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag) const;
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag) const;
};
