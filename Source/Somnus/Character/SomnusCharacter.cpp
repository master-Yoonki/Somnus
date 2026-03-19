// Fill out your copyright notice in the Description page of Project Settings.


#include "SomnusCharacter.h"
#include "Core/SomnusPlayerState.h"
#include "AbilitySystemComponent.h"
// --- Add these headers for Camera and Movement ---
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/SomnusInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Animation/SomnusAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Core/SomnusGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Equipment/SomnusWeapon.h"

ASomnusCharacter::ASomnusCharacter()
{
	// Use our custom input component for Lyra-style input binding
	OverrideInputComponentClass = USomnusInputComponent::StaticClass();

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

	CurrentGait = ESomnusGait::None;
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
			AddInputMappingContext();
			InitHUD();
		}
	}
}

void ASomnusCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASomnusCharacter, WeaponInventory);
	DOREPLIFETIME(ASomnusCharacter, EquippedWeapon);
}

void ASomnusCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Spawn all weapons into inventory (Server Only)
	if (HasAuthority())
	{
		for (TSubclassOf<ASomnusWeapon> WeaponClass : WeaponClasses)
		{
			if (!WeaponClass) continue;

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;

			ASomnusWeapon* SpawnedWeapon = GetWorld()->SpawnActor<ASomnusWeapon>(
				WeaponClass,
				GetActorLocation(),
				GetActorRotation(),
				SpawnParams
			);

			if (SpawnedWeapon)
			{
				SpawnedWeapon->SetActorHiddenInGame(true);
				WeaponInventory.Add(SpawnedWeapon);
			}
		}
	}
}

void ASomnusCharacter::SwitchWeapon(int32 SlotIndex)
{
	if (!HasAuthority()) return;

	ASomnusWeapon* OldWeapon = EquippedWeapon;

	// Unequip current weapon
	if (EquippedWeapon)
	{
		EquippedWeapon->Unequip();
		EquippedWeapon->SetActorHiddenInGame(true);
		EquippedWeapon = nullptr;
	}

	// SlotIndex 0 = unarmed, 1+ = weapon index
	if (SlotIndex > 0 && WeaponInventory.IsValidIndex(SlotIndex - 1))
	{
		ASomnusWeapon* NewWeapon = WeaponInventory[SlotIndex - 1];
		if (NewWeapon)
		{
			NewWeapon->SetActorHiddenInGame(false);
			NewWeapon->Equip(this);
			EquippedWeapon = NewWeapon;
		}
	}

	// Anim layers — OnRep doesn't fire on the server, so call manually
	UpdateWeaponAnimLayers(OldWeapon, EquippedWeapon);
}

void ASomnusCharacter::OnRep_EquippedWeapon(ASomnusWeapon* OldWeapon)
{
	// Hide old weapon, show new
	if (OldWeapon)
	{
		OldWeapon->SetActorHiddenInGame(true);
	}
	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(false);
	}

	UpdateWeaponAnimLayers(OldWeapon, EquippedWeapon);
}

void ASomnusCharacter::UpdateWeaponAnimLayers(ASomnusWeapon* OldWeapon, ASomnusWeapon* NewWeapon)
{
	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (!SkelMesh) return;

	// 1. Always re-link default locomotion (restores unarmed baseline)
	if (DefaultLocomotionLayerClass)
	{
		SkelMesh->LinkAnimClassLayers(DefaultLocomotionLayerClass);
	}

	// 2. Link new weapon's layers (overrides defaults for matching interfaces)
	//    Old weapon's layers are NOT unlinked — AnimGraph uses bHasUpperBodyLayer
	//    to skip evaluation when no weapon is equipped.
	//    When a new weapon equips, LinkAnimClassLayers replaces the old implementation.
	if (NewWeapon)
	{
		NewWeapon->LinkAnimLayers(SkelMesh);
	}
}

void ASomnusCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	USomnusInputComponent* SomnusIC = Cast<USomnusInputComponent>(PlayerInputComponent);
	check(SomnusIC && InputConfig);

	// Native actions
	SomnusIC->BindNativeAction(InputConfig, SomnusTags::Input_Native_Move, ETriggerEvent::Triggered, this, &ASomnusCharacter::Move);
	SomnusIC->BindNativeAction(InputConfig, SomnusTags::Input_Native_Look, ETriggerEvent::Triggered, this, &ASomnusCharacter::Look);
	SomnusIC->BindNativeAction(InputConfig, SomnusTags::Input_Native_Jump, ETriggerEvent::Started, this, &ACharacter::Jump);
	SomnusIC->BindNativeAction(InputConfig, SomnusTags::Input_Native_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

	// Ability actions
	SomnusIC->BindAbilityActions(InputConfig, this, &ASomnusCharacter::AbilityInputTagPressed, &ASomnusCharacter::AbilityInputTagReleased);
}

void ASomnusCharacter::Move(const FInputActionValue& Value)
{
	// If in a movement-cancellable window, cancel melee abilities
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(SomnusTags::State_MovementCancellable))
		{
			FGameplayTagContainer MeleeTags;
			MeleeTags.AddTag(SomnusTags::Ability_Melee_Heavy);
			MeleeTags.AddTag(SomnusTags::Ability_Melee_Light);
			ASC->CancelAbilities(&MeleeTags);
		}
	}

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

void ASomnusCharacter::AbilityInputTagPressed(FGameplayTag InputTag)
{
	UE_LOG(LogTemp, Warning, TEXT("[AbilityInput] Pressed: %s"), *InputTag.ToString());

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[AbilityInput] ASC is null!"));
		return;
	}

	const FGameplayTagContainer* AbilityTags = InputTagToAbilityTags.Find(InputTag);
	if (!AbilityTags)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityInput] No mapping found for InputTag: %s"), *InputTag.ToString());
		return;
	}

	// Try each ability tag individually — TryActivateAbilitiesByTag with multiple tags
	// requires ALL tags to match a single ability, so we iterate instead.
	for (const FGameplayTag& Tag : *AbilityTags)
	{
		FGameplayTagContainer SingleTag;
		SingleTag.AddTag(Tag);
		if (ASC->TryActivateAbilitiesByTag(SingleTag))
		{
			UE_LOG(LogTemp, Warning, TEXT("[AbilityInput] Activated ability with tag: %s"), *Tag.ToString());
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[AbilityInput] No ability activated for tags: %s"), *AbilityTags->ToString());
}

void ASomnusCharacter::AbilityInputTagReleased(FGameplayTag InputTag)
{
	// Only cancel abilities for hold-type inputs (configured in BP)
	if (!HoldInputTags.HasTagExact(InputTag))
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (const FGameplayTagContainer* AbilityTags = InputTagToAbilityTags.Find(InputTag))
		{
			ASC->CancelAbilities(AbilityTags);
		}
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
			AddInputMappingContext();
			InitHUD();
		}
	}
}

void ASomnusCharacter::AddInputMappingContext() const
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASomnusCharacter::BindAttributeCallbacks()
{
	ASomnusPlayerState* PS = GetPlayerState<ASomnusPlayerState>();
	if (!PS) return;

	USomnusAttributeSet* AS = const_cast<USomnusAttributeSet*>(PS->GetAttributeSet());
	if (!AS) return;

	TWeakObjectPtr<ASomnusCharacter> WeakThis(this);

	AS->OnHealthChanged.AddLambda([WeakThis](float Health, float MaxHealth) {
		if (WeakThis.IsValid())
		{
			WeakThis->UpdateHealthUI(Health, MaxHealth);
		}
	});

	AS->OnStaminaChanged.AddLambda([WeakThis](float Stamina, float MaxStamina) {
		if (WeakThis.IsValid())
		{
			WeakThis->UpdateStaminaUI(Stamina, MaxStamina);
		}
	});

	UpdateHealthUI(AS->GetHealth(), AS->GetMaxHealth());
	UpdateStaminaUI(AS->GetStamina(), AS->GetMaxStamina());
}
