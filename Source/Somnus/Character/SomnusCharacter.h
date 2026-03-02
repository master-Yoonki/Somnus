// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "InputActionValue.h"
#include "SomnusCharacter.generated.h"

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
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	// Input Callbacks
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	
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
};