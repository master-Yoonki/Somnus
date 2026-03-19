// Fill out your copyright notice in the Description page of Project Settings.

#include "Input/SomnusInputConfig.h"

const UInputAction* USomnusInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FSomnusInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}

const UInputAction* USomnusInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FSomnusInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}
