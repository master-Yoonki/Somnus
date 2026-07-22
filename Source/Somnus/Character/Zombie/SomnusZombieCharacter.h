// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Core/SomnusStrikeSource.h"
#include "GameFramework/Character.h"
#include "SomnusZombieCharacter.generated.h"

class USomnusAttributeSet;

UCLASS()
class SOMNUS_API ASomnusZombieCharacter : public ACharacter, public IAbilitySystemInterface, public ISomnusStrikeSource
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASomnusZombieCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Called when the server assigns a controller to this character
	virtual void PossessedBy(AController* NewController) override;
	
	/* Ability System Component */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }
	USomnusAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
	// Abilities granted at possession time (innate abilities like Jump)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<class UGameplayAbility>> DefaultAbilities;
	
	virtual TArray<FSomnusStrikeSourceInfo> GetStrikeSources() const override;

	// Testing utility: when set on a placed instance, the AI controller possesses
	// as normal (GAS/abilities init via PossessedBy) but skips starting the
	// behavior tree, so the zombie just stands still.
	bool ShouldDisableAI() const { return bDisableAI; }
protected:
	UPROPERTY(EditInstanceOnly, Category = "AI|Testing")
	bool bDisableAI = false;

	// The core component that handles all GAS logic
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<USomnusAttributeSet> AttributeSet;

	// Physics-based flinch on non-lethal hits
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class USomnusHitReactComponent> HitReact;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<class UPhysicsControlComponent> PhysicsControl;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<class UGameplayEffect>> DefaultGameplayEffects;
	
	// Called when Health reaches zero (server). Runs GAS cleanup then multicasts the ragdoll.
	UFUNCTION(BlueprintCallable, Category = "GAS")
	virtual void Die();

	// Visual death applied on every machine: disables collision, stops movement, ragdolls the mesh.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastZombieDeath();

	void OnHealthChanged(float CurrentValue, float MaxValue);
};
