// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SomnusInteractable.generated.h"

/** Custom depth stencil values, which are the contract between SetHighlighted and the outline
 *  post-process material. A second outline colour later means a new value here and a branch
 *  there - not a second material. */
namespace SomnusStencil
{
	constexpr int32 None = 0;
	constexpr int32 Interactable = 1;
}

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USomnusInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SOMNUS_API ISomnusInteractable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Interact(AActor* Interactor);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void SetHighlighted(bool bHighlighted);
};
