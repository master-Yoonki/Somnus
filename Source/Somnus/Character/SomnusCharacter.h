// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "SomnusCharacter.generated.h"

class ASomnusWeapon;
class UGameplayEffect;
class USomnusInputConfig;
enum class ESomnusGait : uint8;

/**
 * Base character class for Project Somnus.
 * Acts as the physical avatar for the GAS component stored in the PlayerState.
 */
UCLASS()
class SOMNUS_API ASomnusCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASomnusCharacter();

	// Implement IAbilitySystemInterface to fetch ASC from PlayerState
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// Called when the server assigns a controller to this character
	virtual void PossessedBy(AController* NewController) override;

	// Replicate this actor for multiplayer
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Input mapping context added in BeginPlay
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	// Data asset that defines all input→tag mappings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USomnusInputConfig> InputConfig;

	// Maps an input tag (e.g. Input.Ability.Attack) to the ability tags to activate
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Abilities", meta = (AllowPrivateAccess = "true"))
	TMap<FGameplayTag, FGameplayTagContainer> InputTagToAbilityTags;

	// Input tags that cancel their abilities on release (hold-type inputs like Aim, Block, etc.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Abilities", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer HoldInputTags;

	// Native input callbacks
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// Ability input callbacks (called with the InputTag as payload)
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);

	// Adds the DefaultMappingContext to the local player's Enhanced Input subsystem
	void AddInputMappingContext() const;

protected:
	// Called on the client when the PlayerState is successfully replicated
	virtual void OnRep_PlayerState() override;

protected:
    // Camera boom positioning the camera behind the character
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* CameraBoom;

    // Follow camera
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* FollowCamera;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void InitHUD();

	UFUNCTION(BlueprintCallable, Category = "GAS|UI")
	void BindAttributeCallbacks();

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|UI")
	void UpdateHealthUI(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|UI")
	void UpdateStaminaUI(float CurrentStamina, float MaxStamina);

	// Getter function for AnimNotify and Abilities to read the weapon data
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ASomnusWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

	// Switch weapon slot: 0 = unarmed, 1+ = weapon index in WeaponClasses
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeapon(int32 SlotIndex);

	UFUNCTION()
	ESomnusGait GetCurrentGait() const { return CurrentGait; }

protected:
	// GEs applied to the ASC at possession (e.g., stamina regen, passive buffs)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

	// Default full body locomotion layer (ABP_UnarmedLocomotion — always re-linked on unequip)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TSubclassOf<UAnimInstance> DefaultLocomotionLayerClass;

	// Weapon classes available (configured in BP)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TSubclassOf<ASomnusWeapon>> WeaponClasses;

	// Spawned weapon instances (persistent inventory)
	UPROPERTY(Transient, Replicated)
	TArray<TObjectPtr<ASomnusWeapon>> WeaponInventory;

	// Currently equipped weapon (null = unarmed)
	UPROPERTY(Transient, ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<ASomnusWeapon> EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon(ASomnusWeapon* OldWeapon);

	// Handles anim layer swap — called on both server and client
	void UpdateWeaponAnimLayers(ASomnusWeapon* OldWeapon, ASomnusWeapon* NewWeapon);

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	ESomnusGait CurrentGait;
};
