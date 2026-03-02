// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "SomnusPlayerState.generated.h"

class UAttributeSet;
/**
 * Custom PlayerState that holds the AbilitySystemComponent.
 * Essential for replication and GAS functionality in a True FPS environment.
 */
UCLASS()
class SOMNUS_API ASomnusPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	ASomnusPlayerState();

	// Implement IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	USomnusAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	// The core component that handles all GAS logic
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<USomnusAttributeSet> AttributeSet;
};
