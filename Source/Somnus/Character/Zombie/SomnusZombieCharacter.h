// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SomnusCharacterBase.h"
#include "Core/SomnusStrikeSource.h"
#include "SomnusZombieCharacter.generated.h"

class USomnusAttributeSet;

UCLASS()
class SOMNUS_API ASomnusZombieCharacter : public ASomnusCharacterBase, public ISomnusStrikeSource
{
	GENERATED_BODY()

public:
	ASomnusZombieCharacter();

	// Called when the server assigns a controller to this character
	virtual void PossessedBy(AController* NewController) override;

	/** A zombie holds its own ability system - there is no PlayerState to keep one for it. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }
	USomnusAttributeSet* GetAttributeSet() const { return AttributeSet; }

	virtual TArray<FSomnusStrikeSourceInfo> GetStrikeSources() const override;

	// Testing utility: when set on a placed instance, the AI controller possesses
	// as normal (GAS/abilities init via PossessedBy) but skips starting the
	// behavior tree, so the zombie just stands still.
	bool ShouldDisableAI() const { return bDisableAI; }

protected:
	/** Nothing searches a zombie's body, so it hands back its controller and clears away. */
	virtual void HandleServerDeath() override;

	UPROPERTY(EditInstanceOnly, Category = "AI|Testing")
	bool bDisableAI = false;

	// The core component that handles all GAS logic
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<USomnusAttributeSet> AttributeSet;

	/** How long a body lies before it is cleared away. */
	UPROPERTY(EditDefaultsOnly, Category = "GAS", meta = (ClampMin = "0", Units = "Seconds"))
	float CorpseLifeSpan = 5.f;
};
