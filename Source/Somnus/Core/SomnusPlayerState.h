// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "SomnusPlayerState.generated.h"

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

	/** Whether this player has already been handed the equipment a life starts with.
	 *
	 *  Kept here rather than on the pawn or one of its components because it has to outlive the
	 *  pawn - that is the entire question it answers, and a respawn builds a component that would
	 *  say "not yet" every time. Server only and not replicated: the game mode is the only thing
	 *  that asks, and only ever on the authority. */
	bool bHasReceivedStartingKit = false;

protected:
	// The core component that handles all GAS logic
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<USomnusAttributeSet> AttributeSet;
};
