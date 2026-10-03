// Fill out your copyright notice in the Description page of Project Settings.


#include "SomnusCharacter.h"
#include "Core/SomnusPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Input/SomnusInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Core/SomnusGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Equipment/SomnusWeapon.h"
#include "Animation/SomnusItemAnimLayers.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "Core/SomnusGameMode.h"
#include "Inventory/SomnusInventoryComponent.h"
#include "Inventory/SomnusContainerActor.h"
#include "Inventory/SomnusContainerDataAsset.h"
#include "Inventory/SomnusItemTypes.h"
#include "Inventory/SomnusItemDataAsset.h"
#include "Inventory/SomnusContainerEquipComponent.h"
#include "Inventory/SomnusEquipmentComponent.h"
#include "Inventory/SomnusLootComponent.h"
#include "EngineUtils.h"
#include "Core/SomnusInteractable.h"
#include "Core/SomnusCollisionChannels.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/SomnusHitReactComponent.h"
#include "Character/SomnusCharacterMovementComponent.h"
#include "Equipment/SomnusMeleeWeapon.h"
#include "PhysicsControlComponent.h"
#include "Core/SomnusInteractorComponent.h"
#include "Kismet/KismetMathLibrary.h"

ASomnusCharacter::ASomnusCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USomnusCharacterMovementComponent>(CharacterMovementComponentName))
{
	// Use our custom input component for Lyra-style input binding
	OverrideInputComponentClass = USomnusInputComponent::StaticClass();

	// In Lyra-style Turn In Place, we let the Actor rotate with the controller,
	// and the AnimBP handles the visual counter-rotation via RootYawOffset.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement to face the direction of movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	// Arm length, offset and field of view are not set here: they belong to the framing table, and
	// a constructor value would only be the one the camera blends away from on the first frame.

	ExploreCamera   = { 320.f, FVector(0.f,  0.f,  0.f), 88.f, 0.35f };
	StrafeCamera    = { 200.f, FVector(0.f, 25.f, 45.f), 80.f, 0.25f };
	MeleeAimCamera  = { 180.f, FVector(0.f, 40.f, 50.f), 75.f, 0.15f };
	GunAimCamera    = { 160.f, FVector(0.f, 55.f, 55.f), 72.f, 0.20f };

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	ContainerEquipmentComponent = CreateDefaultSubobject<USomnusContainerEquipComponent>(TEXT("ContainerEquip"));

	EquipmentComponent = CreateDefaultSubobject<USomnusEquipmentComponent>(TEXT("Equipment"));

	LootComponent = CreateDefaultSubobject<USomnusLootComponent>(TEXT("Loot"));
	
	InteractorComponent = CreateDefaultSubobject<USomnusInteractorComponent>(TEXT("InteractorComponent"));

	Gait = ESomnusGait::Walk;
}

void ASomnusCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	LastUpdateVelocity = GetCharacterMovement()->GetLastUpdateVelocity();

	// The ability system lives on the player state, so it is missing until one is assigned.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	bIsAiming = ASC && ASC->HasMatchingGameplayTag(SomnusTags::State_Aiming);

	UpdateLocomotionState();
	if (HasAuthority())
	{
		PushOverlappingZombies(DeltaTime);
	}
	UpdateAimStanceTurn(DeltaTime);
	UpdateCamera(DeltaTime);

	// // Toggle rotation mode based on aiming state
	// if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	// {
	// 	const bool bAiming = ASC->HasMatchingGameplayTag(SomnusTags::State_Aiming);
	// 	UCharacterMovementComponent* CMC = GetCharacterMovement();
	//
	// 	if (bAiming)
	// 	{
	// 		CMC->bOrientRotationToMovement = false;
	// 		CMC->bUseControllerDesiredRotation = true;
	// 	}
	// 	else
	// 	{
	// 		CMC->bOrientRotationToMovement = true;
	// 		CMC->bUseControllerDesiredRotation = false;
	// 	}
	// }
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
		UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
		ASC->InitAbilityActorInfo(PS, this);
		GrantDefaults(ASC);
		RefreshInAirTag();

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
	DOREPLIFETIME(ASomnusCharacter, EquippedWeapon);

	// The owner works both out from its own input a round trip before the server's copy could
	// arrive, so only the machines that merely watch are sent them.
	DOREPLIFETIME_CONDITION(ASomnusCharacter, Gait, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(ASomnusCharacter, bIsStrafing, COND_SimulatedOnly);
}

TArray<FSomnusStrikeSourceInfo> ASomnusCharacter::GetStrikeSources() const
{
	if (ASomnusMeleeWeapon* MeleeWeapon = Cast<ASomnusMeleeWeapon>(EquippedWeapon))
	{
		return MeleeWeapon->GetStrikeSources();
	}
	return {};
}

void ASomnusCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseMeshRelativeRotation = GetMesh()->GetRelativeRotation().Quaternion();

	// Start in the framing rather than blending into it from whatever the components were built with.
	const FSomnusCameraMode& StartingFraming = GetCameraMode(CameraFraming);
	ApplyCameraMode(StartingFraming, StartingFraming, 1.f);
	CameraBlendStart = StartingFraming;
}

const FSomnusCameraMode& ASomnusCharacter::GetCameraMode(ESomnusCameraFraming Framing) const
{
	switch (Framing)
	{
	case ESomnusCameraFraming::Strafe:	 return StrafeCamera;
	case ESomnusCameraFraming::MeleeAim: return MeleeAimCamera;
	case ESomnusCameraFraming::GunAim:	 return GunAimCamera;
	default:							 return ExploreCamera;
	}
}

void ASomnusCharacter::UpdateCamera(float DeltaTime)
{
	// Nobody looks through a proxy's camera, and moving one would only cost the frame time.
	if (!IsLocallyControlled())
	{
		return;
	}

	const FSomnusCameraMode& Target = GetCameraMode(CameraFraming);

	if (CameraFraming != CameraFraming_LastFrame)
	{
		CameraBlendStart.TargetArmLength = CameraBoom->TargetArmLength;
		CameraBlendStart.SocketOffset = CameraBoom->SocketOffset;
		CameraBlendStart.FieldOfView = FollowCamera->FieldOfView;
		CameraBlendElapsed = 0.f;
		CameraFraming_LastFrame = CameraFraming;
	}

	CameraBlendElapsed += DeltaTime;
	const float Alpha = Target.BlendTime > 0.f
		? FMath::Clamp(CameraBlendElapsed / Target.BlendTime, 0.f, 1.f)
		: 1.f;

	// Eased rather than linear: a camera that starts and stops moving at full speed reads as a cut
	// with extra steps, however long the blend is given.
	ApplyCameraMode(CameraBlendStart, Target, FMath::SmoothStep(0.f, 1.f, Alpha));
}

void ASomnusCharacter::ApplyCameraMode(const FSomnusCameraMode& From, const FSomnusCameraMode& To, float Alpha)
{
	CameraBoom->TargetArmLength = FMath::Lerp(From.TargetArmLength, To.TargetArmLength, Alpha);
	CameraBoom->SocketOffset = FMath::Lerp(From.SocketOffset, To.SocketOffset, Alpha);
	FollowCamera->SetFieldOfView(FMath::Lerp(From.FieldOfView, To.FieldOfView, Alpha));
}

void ASomnusCharacter::UpdateAimStanceTurn(float DeltaTime)
{
	// A corpse's mesh is simulating; turning it would teleport the ragdoll.
	if (IsDead())
	{
		return;
	}

	// Runs on every machine, the server included - melee traces sweep the weapon on this mesh, so
	// the server has to turn it the same way the players see it turned.
	const float TargetYaw = bIsAiming && EquippedWeapon ? EquippedWeapon->GetAimStanceYaw() : 0.f;

	const float PreviousYaw = AimStanceYaw;
	FMath::ExponentialSmoothingApprox(AimStanceYaw, TargetYaw, DeltaTime, AimStanceTurnSmoothingTime);
	if (FMath::IsNearlyEqual(AimStanceYaw, TargetYaw, 0.05f))
	{
		AimStanceYaw = TargetYaw;
	}

	if (AimStanceYaw != PreviousYaw)
	{
		GetMesh()->SetRelativeRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(AimStanceYaw)) * BaseMeshRelativeRotation);
	}
}

TArray<FSomnusActiveContainerInfo> ASomnusCharacter::GetActiveContainers() const
{
	if (ContainerEquipmentComponent)
	{
		return ContainerEquipmentComponent->GetActiveContainers();	
	}
	
	return {};
}

void ASomnusCharacter::SetHighlighted_Implementation(bool bHighlighted)
{
	// The same question Interact asks, for the same reason. A living character is something the
	// trace can find but has nothing to offer, so lighting one up would promise a search that
	// never happens. Refusing here rather than in the interactor keeps the rule with the class
	// that owns it - only a body knows when it became one.
	if (!IsDead())
	{
		return;
	}

	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (!BodyMesh)
	{
		return;
	}

	BodyMesh->SetCustomDepthStencilValue(USomnusInteractionLibrary::HighlightStencil(bHighlighted));
	BodyMesh->SetRenderCustomDepth(bHighlighted);
}

void ASomnusCharacter::SwitchWeapon(int32 SlotIndex)
{
	// Route through server RPC. On the server this executes locally; on a client it sends an RPC.
	ServerSwitchWeapon(SlotIndex);
}

void ASomnusCharacter::ServerSwitchWeapon_Implementation(int32 SlotIndex)
{
	if (!EquipmentComponent) return;

	// An index no slot answers to yields null, which is how holstering is asked for. Asking for
	// what is already drawn is not a re-equip - without this the draw animation replays and the
	// weapon's abilities are handed back and granted again for nothing.
	ASomnusWeapon* NewWeapon = EquipmentComponent->GetCarriedWeapon(SlotIndex);
	if (NewWeapon == EquippedWeapon) return;

	ASomnusWeapon* OldWeapon = EquippedWeapon;
	if (IsValid(OldWeapon))
	{
		OldWeapon->Unequip();
	}

	EquippedWeapon = NewWeapon;
	if (NewWeapon)
	{
		NewWeapon->Equip(this);
	}

	UpdateWeaponAnimLayers(NewWeapon);
}

void ASomnusCharacter::NotifyCarriedWeaponRetiring(ASomnusWeapon* Weapon)
{
	if (!Weapon || EquippedWeapon != Weapon)
	{
		return;
	}

	EquippedWeapon = nullptr;
	UpdateWeaponAnimLayers(nullptr);
}

void ASomnusCharacter::OnRep_EquippedWeapon(ASomnusWeapon* OldWeapon)
{
	// Hide old weapon, show new. IsValid rather than a null test: the value that arrived here can
	// be a weapon the server destroyed, and this machine may have torn it down already.
	if (IsValid(OldWeapon))
	{
		OldWeapon->SetActorHiddenInGame(true);
	}
	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(false);
	}

	UpdateWeaponAnimLayers(EquippedWeapon);
}

void ASomnusCharacter::UpdateWeaponAnimLayers(const ASomnusWeapon* NewWeapon)
{
	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (!SkelMesh) return;

	const TSubclassOf<USomnusItemAnimLayers> NewLayerClass = NewWeapon ? NewWeapon->GetAnimLayerClass() : nullptr;

	// Weapons sharing a layer (a bat and a pipe) keep the running instance. Relinking the same
	// class would spawn a new one, snapping the pose and resetting whatever the layer carries.
	if (NewLayerClass == LinkedItemLayerClass)
	{
		return;
	}

	// With nothing linked, the main blueprint's own implementation of the layer takes over, which
	// is what unarmed looks like.
	if (LinkedItemLayerClass)
	{
		SkelMesh->UnlinkAnimClassLayers(LinkedItemLayerClass);
	}
	if (NewLayerClass)
	{
		SkelMesh->LinkAnimClassLayers(NewLayerClass);
	}
	LinkedItemLayerClass = NewLayerClass;
}

USomnusCharacterMovementComponent* ASomnusCharacter::GetSomnusMovement() const
{
	return CastChecked<USomnusCharacterMovementComponent>(GetCharacterMovement());
}

void ASomnusCharacter::UpdateLocomotionState()
{
	USomnusCharacterMovementComponent* Movement = GetSomnusMovement();

	// A watching machine has neither the wishes nor the moves, so it takes both from the server.
	// It runs no moves either, so nothing else sets its turn flags - and its animation reads them
	// to predict where the body will face. Set every tick rather than on arrival: the starting
	// value is never sent, and the flags start as whatever the blueprint left them.
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		Movement->ApplyRotationMode(bIsStrafing);
		return;
	}

	Gait = Movement->ResolveDisplayedGait(Gait);
	bIsStrafing = Movement->WantsToStrafe();
}

ESomnusMovementMode ASomnusCharacter::GetMovementMode() const
{
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (!CM) return ESomnusMovementMode::OnGround;
	EMovementMode CM_MovementMode = CM->MovementMode;
	switch (CM_MovementMode)
	{
	case MOVE_None:
	case MOVE_NavWalking:
	case MOVE_Swimming:
	case MOVE_Flying:
	case MOVE_Custom:
	case MOVE_MAX:
	case MOVE_Walking:
		return ESomnusMovementMode::OnGround;
	case MOVE_Falling:
		return ESomnusMovementMode::InAir;
	}
	return ESomnusMovementMode::OnGround;
}

bool ASomnusCharacter::IsMoving() const
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		bool bIsVelocityNonZero = 
			UKismetMathLibrary::NotEqual_VectorVector(Movement->Velocity, FVector::ZeroVector, 0.1);
		bool bIsAccelerationNonZero = 
			UKismetMathLibrary::NotEqual_VectorVector(Movement->GetCurrentAcceleration(), FVector::ZeroVector, 0.1);
		
		return bIsVelocityNonZero && bIsAccelerationNonZero;
	}
	return false;
}

void ASomnusCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	USomnusInputComponent* SomnusIC = Cast<USomnusInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(SomnusIC && InputConfig, TEXT("%s has no InputConfig set - input is not bound"), *GetClass()->GetName()))
	{
		return;
	}

	// Native actions
	SomnusIC->BindNativeAction(InputConfig, SomnusTags::Input_Native_Move, ETriggerEvent::Triggered, this, &ASomnusCharacter::Move);
	SomnusIC->BindNativeAction(InputConfig, SomnusTags::Input_Native_Look, ETriggerEvent::Triggered, this, &ASomnusCharacter::Look);
	SomnusIC->BindNativeAction(InputConfig, SomnusTags::Input_Native_Interact, ETriggerEvent::Started, this, &ASomnusCharacter::Interact);
	
	// Ability actions (Jump is routed here too — see GA_Jump for InputReleased handling)
	SomnusIC->BindAbilityActions(InputConfig, this, &ASomnusCharacter::AbilityInputTagPressed, &ASomnusCharacter::AbilityInputTagReleased);
}

void ASomnusCharacter::Interact(const FInputActionValue& Value)
{
	Server_Interact();
}

void ASomnusCharacter::Server_Interact_Implementation()
{
	FHitResult Hit;
	if (!InteractorComponent || !InteractorComponent->TraceForInteractable(Hit))
	{
		return;
	}

	// Execute_Interact asserts on an actor that does not implement the interface. The channel
	// being interact-only makes that unlikely rather than impossible, and an assert is too
	// expensive a way to find out.
	AActor* HitActor = Hit.GetActor();
	if (HitActor && HitActor->Implements<USomnusInteractable>())
	{
		ISomnusInteractable::Execute_Interact(HitActor, this);
	}
}

void ASomnusCharacter::Interact_Implementation(AActor* Interactor)
{
	// Reached only on the server - Server_Interact owns the trace. A living character being
	// interacted with is not an error, it simply has nothing to offer.
	if (!IsDead() || !Interactor || Interactor == this) return;

	// Asking for the component rather than casting to a character is what lets anything that can
	// carry storage do the searching later, without this line changing.
	if (USomnusLootComponent* SearcherLoot = Interactor->FindComponentByClass<USomnusLootComponent>())
	{
		SearcherLoot->OpenLoot(this);
	}
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

	// Passed through as it comes. Gait is the movement component's to decide, from the walk and
	// sprint wishes, so the input only says which way and how hard; the component clamps a
	// keyboard diagonal back to unit length itself (ScaleInputAcceleration).
	const FVector2D MovementVector = Value.Get<FVector2D>();
	
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
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	const FGameplayTagContainer* AbilityTags = InputTagToAbilityTags.Find(InputTag);
	if (!AbilityTags) return;

	// Try each ability tag individually — TryActivateAbilitiesByTag with multiple tags
	// requires ALL tags to match a single ability, so we iterate instead.
	for (const FGameplayTag& Tag : *AbilityTags)
	{
		FGameplayTagContainer SingleTag;
		SingleTag.AddTag(Tag);
		if (ASC->TryActivateAbilitiesByTag(SingleTag))
		{
			return;
		}
	}
}

void ASomnusCharacter::AbilityInputTagReleased(FGameplayTag InputTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	const FGameplayTagContainer* AbilityTags = InputTagToAbilityTags.Find(InputTag);
	if (!AbilityTags) return;

	// Notify all matching active abilities of input release.
	// This fires InputReleased() on abilities (e.g., GA_Jump calls StopJumping).
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && AbilityTags->HasAny(Spec.Ability->GetAssetTags()))
		{
			Spec.InputPressed = false;
			ASC->AbilitySpecInputReleased(Spec);
		}
	}

	// Only cancel abilities for hold-type inputs (Aim, Block, etc.)
	if (HoldInputTags.HasTagExact(InputTag))
	{
		ASC->CancelAbilities(AbilityTags);
	}
}

void ASomnusCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [Client Side] Initialize GAS Actor Info once the PlayerState arrives
	if (ASomnusPlayerState* PS = GetPlayerState<ASomnusPlayerState>())
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
		RefreshInAirTag();
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

void ASomnusCharacter::HandleServerDeath()
{
	// A looter who dies stops looting. Being looted is unaffected - that session belongs to
	// whoever opened this body, and their component keeps re-checking it from their side.
	if (LootComponent)
	{
		LootComponent->Server_CloseLoot_Implementation();
	}

	// Cleared as well as unequipped. A body that still names a weapon is a body that answers
	// GetEquippedWeapon with something it is no longer holding - and every melee trace and anim
	// notify asks exactly that question.
	if (EquippedWeapon)
	{
		EquippedWeapon->Unequip();
		EquippedWeapon = nullptr;
		UpdateWeaponAnimLayers(nullptr);
	}
}

void ASomnusCharacter::HandleDeathEvent(const FVector& HitDirection)
{
	// Death UI for the owning client only. The controller has to be a player controller, not
	// merely a local one: IsLocalController() is unconditionally true in standalone
	// (Controller.cpp:90), so an AI-possessed body would put a death screen on the local viewport.
	if (const APlayerController* PC = Cast<APlayerController>(GetController()); PC && PC->IsLocalController())
	{
		OnDeath();
	}
}

void ASomnusCharacter::ServerRequestRespawn_Implementation()
{
	if (!IsDead()) return;

	if (ASomnusGameMode* GM = GetWorld()->GetAuthGameMode<ASomnusGameMode>())
	{
		GM->RequestRespawn(GetController());
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

// Container inventory diagnostics. Every label below is chosen so the same dump taken on two
// machines can be compared line by line - which is the only practical way to tell a replication
// fault apart from a display fault in this system.

static FString SomnusDebug_MachineLabel(const AActor* Actor)
{
	const UWorld* World = Actor ? Actor->GetWorld() : nullptr;
	if (!World)
	{
		return TEXT("???");
	}

	switch (World->GetNetMode())
	{
	case NM_Client:         return TEXT("CLIENT");
	case NM_ListenServer:   return TEXT("LISTEN-SERVER");
	case NM_DedicatedServer:return TEXT("DEDICATED-SERVER");
	default:                return TEXT("STANDALONE");
	}
}

static FString SomnusDebug_RoleLabel(const AActor* Actor)
{
	switch (Actor->GetLocalRole())
	{
	case ROLE_Authority:       return TEXT("Authority");
	case ROLE_AutonomousProxy: return TEXT("AutonomousProxy");
	case ROLE_SimulatedProxy:  return TEXT("SimulatedProxy");
	default:                   return TEXT("None");
	}
}

static FString SomnusDebug_SlotLabel(FGameplayTag SlotTag)
{
	return SlotTag.IsValid() ? SlotTag.ToString() : TEXT("?");
}

// Lists the items of one grid, recursing into a container item's own compartments so
// something rotated inside a worn rig/backpack shows up too, not just top-level items.
// Container items are labelled with their InstanceID, which is server-generated and
// replicated, so the same item can be matched across machines even though actor and
// component names differ.
static void SomnusDebug_DumpGridContents(USomnusInventoryComponent* Grid, const TCHAR* Indent)
{
	for (const FSomnusItemInstance& Item : Grid->GetAllItems())
	{
		const FString ItemName = Item.ItemData ? Item.ItemData->GetName() : TEXT("<null ItemData>");

		const FString InstanceLabel = Item.InstanceID.ToString(EGuidFormats::DigitsWithHyphens).Left(8);

		const FString RotatedSuffix = Item.bRotated ? TEXT("  rotated") : TEXT("");

		if (!Cast<USomnusContainerDataAsset>(Item.ItemData))
		{
			// Plain items carry their id and cell too, so stacks split off the same incoming
			// instance can be told apart - two entries sharing an id is the bug to watch for.
			UE_LOG(LogSomnusInventory, Warning, TEXT("%s%s x%d  [id %s]  at (%d,%d)%s"),
				Indent, *ItemName, Item.StackCount, *InstanceLabel,
				Item.GridPosition.X, Item.GridPosition.Y, *RotatedSuffix);
			continue;
		}

		if (!Item.ContainerActor)
		{
			UE_LOG(LogSomnusInventory, Error,
				TEXT("%s%s [id %s]  ContainerActor is NULL  <-- did not resolve here"),
				Indent, *ItemName, *InstanceLabel);
			continue;
		}

		UE_LOG(LogSomnusInventory, Warning,
			TEXT("%s%s [id %s]  at (%d,%d)%s  ContainerActor=%s, Compartments=%d"),
			Indent, *ItemName, *InstanceLabel,
			Item.GridPosition.X, Item.GridPosition.Y, *RotatedSuffix,
			*Item.ContainerActor->GetName(), Item.ContainerActor->GetCompartments().Num());

		const FString NestedIndent = FString(Indent) + TEXT("    ");
		for (USomnusInventoryComponent* Compartment : Item.ContainerActor->GetCompartments())
		{
			if (Compartment)
			{
				SomnusDebug_DumpGridContents(Compartment, *NestedIndent);
			}
		}
	}
}

static void SomnusDebug_DumpCharacter(ASomnusCharacter* Character)
{
	const APlayerState* PS = Character->GetPlayerState();

	UE_LOG(LogSomnusInventory, Warning, TEXT("  -- %s  Role=%s  PlayerId=%d  LocallyControlled=%d"),
		*Character->GetName(), *SomnusDebug_RoleLabel(Character),
		PS ? PS->GetPlayerId() : -1,
		Character->IsLocallyControlled() ? 1 : 0);

	const USomnusContainerEquipComponent* Equip =
		Character->FindComponentByClass<USomnusContainerEquipComponent>();
	if (!Equip)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("     no USomnusContainerEquipComponent on this character"));
		return;
	}

	const TArray<FSomnusActiveContainerInfo> Active = Equip->GetActiveContainers();
	UE_LOG(LogSomnusInventory, Warning, TEXT("     GetActiveContainers() -> %d"), Active.Num());

	for (int32 FlatIndex = 0; FlatIndex < Active.Num(); ++FlatIndex)
	{
		const FSomnusActiveContainerInfo& Info = Active[FlatIndex];
		USomnusInventoryComponent* Grid = Info.Container;
		if (!Grid)
		{
			UE_LOG(LogSomnusInventory, Error, TEXT("     [%s %d]  Container is NULL  <-- should have been filtered"),
				*SomnusDebug_SlotLabel(Info.SlotTag), Info.SlotIndex);
			continue;
		}

		// The leading number is the flat index into GetActiveContainers(), so a grid seen here
		// can be named unambiguously when talking about a specific one.
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("     #%d  [%s %d]  %s  Registered=%d  Initialized=%d  Items=%d"),
			FlatIndex,
			*SomnusDebug_SlotLabel(Info.SlotTag), Info.SlotIndex, *Grid->GetName(),
			Grid->IsRegistered() ? 1 : 0,
			Grid->HasBeenInitialized() ? 1 : 0,
			Grid->GetAllItems().Num());

		SomnusDebug_DumpGridContents(Grid, TEXT("         "));
	}
}

void ASomnusCharacter::SomnusDumpContainers()
{
	const FString Machine = SomnusDebug_MachineLabel(this);

	UE_LOG(LogSomnusInventory, Warning, TEXT("===== [%s] DumpContainers: every character in this world ====="), *Machine);

	int32 Count = 0;
	for (TActorIterator<ASomnusCharacter> It(GetWorld()); It; ++It)
	{
		SomnusDebug_DumpCharacter(*It);
		++Count;
	}

	if (Count == 0)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("  no ASomnusCharacter found in this world"));
	}

	UE_LOG(LogSomnusInventory, Warning, TEXT("===== [%s] end ====="), *Machine);
}

void ASomnusCharacter::PushOverlappingZombies(float DeltaTime)
{
	if (!GetSomnusMovement()) return;
	TArray<AActor*> OverlappingZombies;
	GetSomnusMovement()->FindOverlappingZombies(OverlappingZombies);

	for (AActor* OverlappingZombie : OverlappingZombies)
	{
		ACharacter* ZombieCharacter = Cast<ACharacter>(OverlappingZombie);
		if (!ZombieCharacter)
		{
			continue;
		}

		const FVector Shove = ComputeZombieShove(ZombieCharacter, DeltaTime);
		if (Shove.IsNearlyZero())
		{
			continue;
		}

		// Swept, so a zombie with a wall or another zombie behind it stays put - and then the
		// movement component's penetration limit is what stops the player.
		FHitResult Hit;
		ZombieCharacter->GetCharacterMovement()->SafeMoveUpdatedComponent(Shove, ZombieCharacter->GetActorQuat(), true, Hit);
	}
}

FVector ASomnusCharacter::ComputeZombieShove(const ACharacter* Zombie, float DeltaTime) const
{
	// Flat on purpose: a push with height in it would lift the zombie off the floor or drive it
	// into it whenever the two stand on different steps.
	const FVector ToZombie = FVector(Zombie->GetActorLocation() - GetActorLocation()) * FVector(1.f, 1.f, 0.f);
	const double Distance = ToZombie.Size();

	const double Penetration = GetCapsuleComponent()->GetScaledCapsuleRadius()
		+ Zombie->GetCapsuleComponent()->GetScaledCapsuleRadius() - Distance;
	if (Penetration <= 0.0)
	{
		return FVector::ZeroVector;
	}

	// Standing exactly on top of each other gives no line to push along; out ahead of the body
	// is the side the player is walking into anyway.
	const FVector TowardZombie = Distance > UE_KINDA_SMALL_NUMBER ? ToZombie / Distance : GetActorForwardVector().GetSafeNormal2D();

	// Carried at the speed this body is heading into the zombie. The movement component has already
	// capped that at what the zombie's resistance leaves, so the two move on together at one speed
	// and the gap between them holds instead of closing into a wall.
	const double SpeedIntoZombie = FVector::DotProduct(GetVelocity() * FVector(1.f, 1.f, 0.f), TowardZombie);
	const double Carry = FMath::Max(SpeedIntoZombie, 0.0) * DeltaTime;

	// Eased out by part of the overlap. Capped at the whole of it, so a long frame settles the zombie
	// at the capsule's edge instead of flinging it past.
	const double EaseOut = Penetration * FMath::Min(CrowdPushStrength * DeltaTime, 1.f);

	// Anything past the penetration limit goes at once. The limit on the player's side only stops the
	// player; a zombie chasing in is stopped by nothing else, and past the centre the line flips and
	// the player would be pushed through it.
	const double Excess = FMath::Max(Penetration - GetSomnusMovement()->GetCrowdMaxPenetration(), 0.0);

	// Easing out and backing out both close the overlap, so the larger of the two is taken rather
	// than their sum. Carrying is separate: it keeps up with the player rather than closing anything.
	return TowardZombie * (Carry + FMath::Max(EaseOut, Excess));
}
