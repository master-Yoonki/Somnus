// Fill out your copyright notice in the Description page of Project Settings.


#include "SomnusCharacter.h"
#include "Core/SomnusPlayerState.h"
#include "AbilitySystemComponent.h"
// --- Add these headers for Camera and Movement ---
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Components/CapsuleComponent.h"

ASomnusCharacter::ASomnusCharacter()
{
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement to face the direction of movement
	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); 

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
}

UAbilitySystemComponent* ASomnusCharacter::GetAbilitySystemComponent() const
{
	// Safely retrieve the ASC from the PlayerState
	if (ASomnusPlayerState* PS = GetPlayerState<ASomnusPlayerState>())
	{
		return PS->GetAbilitySystemComponent();
	}
	return nullptr;
}

void ASomnusCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// [Server Side] Initialize GAS Actor Info
	// Owner = PlayerState, Avatar = Character
	if (ASomnusPlayerState* PS = GetPlayerState<ASomnusPlayerState>())
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
		if (IsLocallyControlled())
		{
			InitHUD();
		}
	}
}

void ASomnusCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASomnusCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASomnusCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASomnusCharacter::Look);
	}
}

void ASomnusCharacter::Move(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    
		// Get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASomnusCharacter::Look(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASomnusCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [Client Side] Initialize GAS Actor Info once the PlayerState arrives
	if (ASomnusPlayerState* PS = GetPlayerState<ASomnusPlayerState>())
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
		if (IsLocallyControlled())
		{
			InitHUD();
		}
	}
}

void ASomnusCharacter::BindAttributeCallbacks()
{
	ASomnusPlayerState* PS = GetPlayerState<ASomnusPlayerState>();
	if (!PS) return;

	USomnusAttributeSet* AS = const_cast<USomnusAttributeSet*>(PS->GetAttributeSet());
	if (!AS) return;
	
	AS->OnHealthChanged.AddLambda([this](float Health, float MaxHealth) {
		UpdateHealthUI(Health, MaxHealth);
	});

	AS->OnStaminaChanged.AddLambda([this](float Stamina, float MaxStamina) {
		UpdateStaminaUI(Stamina, MaxStamina);
	});
	
	UpdateHealthUI(AS->GetHealth(), AS->GetMaxHealth());
	UpdateStaminaUI(AS->GetStamina(), AS->GetMaxStamina());
}
