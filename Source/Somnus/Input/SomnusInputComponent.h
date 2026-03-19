// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "SomnusInputConfig.h"
#include "SomnusInputComponent.generated.h"

/**
 * Enhanced input component that binds actions from a USomnusInputConfig data asset.
 * Modeled after Lyra's input component pattern.
 */
UCLASS(Config = Input)
class SOMNUS_API USomnusInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	USomnusInputComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * Binds a native input action (looked up by tag from the config) to a C++ member function.
	 */
	template<class UserClass, typename FuncType>
	void BindNativeAction(const USomnusInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func);

	/**
	 * Binds all ability input actions from the config. On press/release, calls the
	 * provided functions with the matching FGameplayTag as payload.
	 */
	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const USomnusInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc);
};

template<class UserClass, typename FuncType>
void USomnusInputComponent::BindNativeAction(const USomnusInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
{
	check(InputConfig);

	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void USomnusInputComponent::BindAbilityActions(const USomnusInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
{
	check(InputConfig);

	for (const FSomnusInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (!Action.InputAction || !Action.InputTag.IsValid())
		{
			continue;
		}

		if (PressedFunc)
		{
			BindAction(Action.InputAction, ETriggerEvent::Started, Object, PressedFunc, Action.InputTag);
		}

		if (ReleasedFunc)
		{
			BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag);
		}
	}
}
