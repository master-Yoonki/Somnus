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
#include "Animation/SomnusAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Core/SomnusGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Equipment/SomnusWeapon.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "Core/SomnusGameMode.h"
#include "Inventory/SomnusInventoryComponent.h"
#include "Inventory/SomnusContainerActor.h"
#include "Inventory/SomnusContainerDataAsset.h"
#include "Inventory/SomnusItemTypes.h"
#include "Inventory/SomnusItemDataAsset.h"
#include "Inventory/SomnusContainerEquipComponent.h"
#include "Inventory/SomnusLootComponent.h"
#include "EngineUtils.h"
#include "Core/SomnusInteractable.h"
#include "Core/SomnusCollisionChannels.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/SomnusHitReactComponent.h"
#include "Equipment/SomnusMeleeWeapon.h"
#include "PhysicsControlComponent.h"
#include "Core/SomnusInteractorComponent.h"
#include "Inventory/SomnusEquipmentSlotComponent.h"
#include "Misc/OutputDevice.h"

ASomnusCharacter::ASomnusCharacter()
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
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	ContainerEquipmentComponent = CreateDefaultSubobject<USomnusContainerEquipComponent>(TEXT("ContainerEquip"));

	LootComponent = CreateDefaultSubobject<USomnusLootComponent>(TEXT("Loot"));
	
	InteractorComponent = CreateDefaultSubobject<USomnusInteractorComponent>(TEXT("InteractorComponent"));

	HitReact = CreateDefaultSubobject<USomnusHitReactComponent>(TEXT("HitReact"));

	PhysicsControl = CreateDefaultSubobject<UPhysicsControlComponent>(TEXT("PhysicsControl"));

	CurrentGait = ESomnusGait::None;
	
	GetMesh()->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;
}

void ASomnusCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Toggle rotation mode based on aiming state
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const bool bAiming = ASC->HasMatchingGameplayTag(SomnusTags::State_Aiming);
		UCharacterMovementComponent* CMC = GetCharacterMovement();

		if (bAiming)
		{
			CMC->bOrientRotationToMovement = false;
			CMC->bUseControllerDesiredRotation = true;
		}
		else
		{
			CMC->bOrientRotationToMovement = true;
			CMC->bUseControllerDesiredRotation = false;
		}
	}
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

		// Apply default GEs (stamina regen, passive buffs, etc.) — guarded against repossession
		if (!bDefaultEffectsApplied)
		{
			for (const TSubclassOf<UGameplayEffect>& GEClass : DefaultGameplayEffects)
			{
				if (!GEClass) continue;
				FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
				ContextHandle.AddSourceObject(this);
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GEClass, 1.0f, ContextHandle);
				if (SpecHandle.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}
			bDefaultEffectsApplied = true;
		}

		// Grant innate abilities (e.g., Jump) — guarded against repossession
		if (!bDefaultAbilitiesGiven)
		{
			for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
			{
				if (!AbilityClass) continue;
				FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
				ASC->GiveAbility(Spec);
			}
			bDefaultAbilitiesGiven = true;
		}

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
	DOREPLIFETIME(ASomnusCharacter, bDead);
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

	BodyMesh->SetCustomDepthStencilValue(bHighlighted ? SomnusStencil::Interactable : SomnusStencil::None);
	BodyMesh->SetRenderCustomDepth(bHighlighted);
}

void ASomnusCharacter::SwitchWeapon(int32 SlotIndex)
{
	// Route through server RPC. On the server this executes locally; on a client it sends an RPC.
	ServerSwitchWeapon(SlotIndex);
}

void ASomnusCharacter::ServerSwitchWeapon_Implementation(int32 SlotIndex)
{
	ASomnusWeapon* OldWeapon = EquippedWeapon;

	// Unequip current weapon
	if (EquippedWeapon)
	{
		EquippedWeapon->Unequip();
		EquippedWeapon = nullptr;
	}

	// SlotIndex 0 = unarmed, 1+ = weapon index
	if (SlotIndex > 0 && WeaponInventory.IsValidIndex(SlotIndex - 1))
	{
		ASomnusWeapon* NewWeapon = WeaponInventory[SlotIndex - 1];
		if (NewWeapon)
		{
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

void ASomnusCharacter::Die(const FVector& HitDirection)
{
	// Death is a server decision, and bDead is replicated - a client must never set it locally.
	if (!HasAuthority() || IsDead()) return;

	// Set before anything that could bail out. Whether this body is a corpse must not depend on
	// whether there happened to be an ability system left to clean up.
	bDead = true;

	// A looter who dies stops looting. Being looted is unaffected - that session belongs to
	// whoever opened this body, and their component keeps re-checking it from their side.
	if (LootComponent)
	{
		LootComponent->Server_CloseLoot_Implementation();
	}

	if (GetEquippedWeapon())
	{
		GetEquippedWeapon()->Unequip();
	}

	// Only clean up an ability system that exists. It lives on the PlayerState, which a character
	// placed in the level never had and an unpossessed corpse no longer has.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();

		// Regen, buffs and anything else that has no business ticking on a body.
		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(SomnusTags::Effect_RemoveOnDeath);
		ASC->RemoveActiveEffectsWithGrantedTags(EffectTagsToRemove);

		// Kept alongside bDead rather than replaced by it: the tag is what blocks abilities while
		// the ability system is still attached, which is a different question from "is this a corpse".
		ASC->AddLooseGameplayTag(SomnusTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
	}

	// Outside the block on purpose - the ragdoll has to happen either way. No hand call to
	// ApplyDeathState here the way OnRep_LootTarget needs one: a multicast runs locally on the
	// authority too (Actor.cpp:5500-5519), so the server gets its half from this line.
	MulticastDeath(HitDirection);
}

void ASomnusCharacter::ApplyDeathState()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// Going slack goes through the hit react component rather than the mesh directly: it owns the
	// physics control body modifiers, and those reassert the movement type. Setting bodies to
	// simulate behind its back leaves them kinematic, which reads on screen as a frozen pose.
	if (HitReact)
	{
		HitReact->SetPhysicsPose(ESomnusPhysicsPose::Limp);
	}
}

void ASomnusCharacter::OnRep_Dead()
{
	// The half of dying that has to survive being missed. A machine that was not watching - out
	// of relevancy, or not yet connected when the body fell - never hears the multicast, and
	// would otherwise be left with a corpse standing up and playing its idle forever.
	if (bDead)
	{
		ApplyDeathState();
	}
}

void ASomnusCharacter::MulticastDeath_Implementation(const FVector& HitDirection)
{
	// Called here as well as from OnRep_Dead because the two have no ordering guarantee between
	// them. Arriving first, this is what has the mesh simulating in time for the impulse below;
	// arriving second, it costs nothing.
	ApplyDeathState();

	// Kept an event rather than moved into the state above, because it only means anything at the
	// instant it happens. A machine that missed it wants the body, not a shove five seconds late.
	if (!HitDirection.IsNearlyZero())
	{
		GetMesh()->AddImpulse(HitDirection * 1500.0f, NAME_None, true);
	}

	// Notify Blueprint for death UI (owning client only). The controller has to be a player
	// controller, not merely a local one: IsLocalController() is unconditionally true in
	// standalone (Controller.cpp:90), so an AI-possessed body would put a death screen on the
	// local player's viewport.
	if (const APlayerController* PC = Cast<APlayerController>(GetController()); PC && PC->IsLocalController())
	{
		OnDeath();
	}
}

bool ASomnusCharacter::IsDead() const
{
	// bDead is checked first because an unpossessed corpse has no PlayerState, and therefore no
	// ability system to ask.
	if (bDead) return true;

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(SomnusTags::State_Dead);
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

static FString SomnusDebug_SlotLabel(EContainerSlotType SlotType)
{
	switch (SlotType)
	{
	case EContainerSlotType::Pockets:  return TEXT("Pockets");
	case EContainerSlotType::Rig:      return TEXT("Rig");
	case EContainerSlotType::Backpack: return TEXT("Backpack");
	default:                           return TEXT("?");
	}
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
				*SomnusDebug_SlotLabel(Info.SlotType), Info.SlotIndex);
			continue;
		}

		// The leading number is the flat index into GetActiveContainers(), so a grid seen here
		// can be named unambiguously when talking about a specific one.
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("     #%d  [%s %d]  %s  Registered=%d  Initialized=%d  Items=%d"),
			FlatIndex,
			*SomnusDebug_SlotLabel(Info.SlotType), Info.SlotIndex, *Grid->GetName(),
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

// =================================================================================================
// TEMPORARY - equip slot isolation proof. Delete this block, its two declarations, and the
// SomnusEquipmentSlotComponent include once the result is recorded.
// =================================================================================================
namespace
{
	/** Counts Error and Fatal lines for as long as it is installed on GLog. This is what makes the
	 *  proof self-checking rather than something read off a scrolling log: a slot that leaves
	 *  RebuildOccupationGrid to the base class still passes every functional assertion below, and
	 *  says so only by logging an out-of-bounds error on every add and every remove. */
	class FSlotProofErrorSink : public FOutputDevice
	{
	public:
		int32 ErrorCount = 0;
		FString FirstError;

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Verbosity <= ELogVerbosity::Error)
			{
				++ErrorCount;
				if (FirstError.IsEmpty())
				{
					FirstError = Message;
				}
			}
		}
	};

	int32 GSlotProofTotal = 0;
	int32 GSlotProofFailed = 0;

	// Warning rather than Error even when the assertion fails, so the harness never contaminates
	// the very count it is taking.
	void SlotProof_Check(const TCHAR* Label, bool bCondition, const FString& Detail = FString())
	{
		++GSlotProofTotal;
		if (!bCondition)
		{
			++GSlotProofFailed;
		}

		UE_LOG(LogSomnusInventory, Warning, TEXT("    [%s] %s%s"),
			bCondition ? TEXT("PASS") : TEXT("FAIL"),
			Label,
			Detail.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("   %s"), *Detail));
	}

	struct FSlotProofCandidate
	{
		USomnusInventoryComponent* Container = nullptr;
		FSomnusItemInstance Item;
		int32 Area = 0;
	};

	/** Every reachable grid, gathered again for each slot because the previous slot's run moved
	 *  things. Not just the pockets: those are four 1x1 compartments, so nothing large enough to
	 *  be worth testing can ever be in one. Largest first - the claim under test is that size
	 *  stops mattering, which a subject that would have fit a 1x1 grid anyway cannot demonstrate. */
	void SlotProof_GatherCandidates(const ASomnusCharacter* Character,
		TArray<FSlotProofCandidate>& OutCandidates, int32& OutGridCount)
	{
		OutCandidates.Reset();
		OutGridCount = 0;

		for (const FSomnusActiveContainerInfo& Info : Character->GetActiveContainers())
		{
			if (!Info.Container)
			{
				continue;
			}
			++OutGridCount;

			for (const FSomnusItemInstance& Item : Info.Container->GetAllItems())
			{
				if (!Item.ItemData)
				{
					continue;
				}
				const FIntPoint Size = Item.ItemData->GetEffectiveSize(Item.bRotated);
				OutCandidates.Add({ Info.Container, Item, Size.X * Size.Y });
			}
		}

		OutCandidates.Sort([](const FSlotProofCandidate& A, const FSlotProofCandidate& B)
		{
			return A.Area > B.Area;
		});
	}

	void SlotProof_RunOne(ASomnusCharacter* Character, USomnusEquipmentSlotComponent* Slot)
	{
		UE_LOG(LogSomnusInventory, Warning, TEXT("  --- %s ---"), *GetNameSafe(Slot));

		if (Slot->GetAllItems().Num() > 0)
		{
			UE_LOG(LogSomnusInventory, Warning,
				TEXT("    skipped: not empty. A slot outside GetActiveContainers() has no other way out - move its contents by hand."));
			return;
		}

		TArray<FSlotProofCandidate> Candidates;
		int32 GridCount = 0;
		SlotProof_GatherCandidates(Character, Candidates, GridCount);

		if (Candidates.Num() == 0)
		{
			UE_LOG(LogSomnusInventory, Warning,
				TEXT("    skipped: %d reachable grid(s), all empty. Put an item bigger than 1x1 in the rig or backpack."),
				GridCount);
			return;
		}

		USomnusInventoryComponent* Source = Candidates[0].Container;
		const FSomnusItemInstance Subject = Candidates[0].Item;
		const FIntPoint SubjectSize = Subject.ItemData->GetEffectiveSize(Subject.bRotated);

		UE_LOG(LogSomnusInventory, Warning,
			TEXT("    subject: %s  %dx%d  stackable=%s  container=%s  from %s  (%d grid(s), %d item(s))"),
			*GetNameSafe(Subject.ItemData), SubjectSize.X, SubjectSize.Y,
			Subject.ItemData->MaxStackCount > 1 ? TEXT("yes") : TEXT("no"),
			Subject.ContainerActor ? TEXT("yes") : TEXT("no"),
			*GetNameSafe(Source), GridCount, Candidates.Num());

		if (SubjectSize.X * SubjectSize.Y <= 1)
		{
			UE_LOG(LogSomnusInventory, Warning,
				TEXT("    WARNING: the largest item available is 1x1, so nothing below tests size blindness."));
		}

		// --- empty slot -------------------------------------------------------------------------
		SlotProof_Check(TEXT("empty slot admits the item at (0,0)"),
			Slot->CanFitAt(Subject.ItemData, 0, 0, false));

		SlotProof_Check(TEXT("empty slot refuses a cell other than (0,0)"),
			!Slot->CanFitAt(Subject.ItemData, 1, 0, false));

		{
			int32 X = -1, Y = -1;
			bool bRotated = true;
			const bool bFound = Slot->FindFirstFit(Subject.ItemData, X, Y, bRotated);
			SlotProof_Check(TEXT("empty slot reports its one free cell"),
				bFound && X == 0 && Y == 0 && !bRotated,
				FString::Printf(TEXT("-> %s (%d,%d) rotated=%d"), bFound ? TEXT("true") : TEXT("false"), X, Y, bRotated ? 1 : 0));
		}

		// --- the move itself --------------------------------------------------------------------
		SlotProof_Check(TEXT("a larger-than-slot item moves in"),
			Slot->MoveItemFrom(Source, Subject.InstanceID, 0, 0, false));

		SlotProof_Check(TEXT("slot holds exactly one item"),
			Slot->GetAllItems().Num() == 1,
			FString::Printf(TEXT("-> %d"), Slot->GetAllItems().Num()));

		{
			FSomnusItemInstance Leftover;
			SlotProof_Check(TEXT("the source no longer holds it"),
				!Source->FindItemByID(Subject.InstanceID, Leftover));
		}

		// --- full slot --------------------------------------------------------------------------
		SlotProof_Check(TEXT("full slot refuses a new item"),
			!Slot->CanFitAt(Subject.ItemData, 0, 0, false));

		// The occupant must not block its own re-placement, which is what TryMoveItem asks for on
		// every drag that ends where it started.
		SlotProof_Check(TEXT("full slot admits the item it already holds (IgnoreItemID)"),
			Slot->CanFitAt(Subject.ItemData, 0, 0, false, Subject.InstanceID));

		{
			int32 X = 0, Y = 0;
			bool bRotated = false;
			SlotProof_Check(TEXT("full slot reports no free cell"),
				!Slot->FindFirstFit(Subject.ItemData, X, Y, bRotated));
		}

		if (Candidates.Num() >= 2)
		{
			const FSlotProofCandidate& Second = Candidates[1];
			const bool bMoved = Slot->MoveItemFrom(Second.Container, Second.Item.InstanceID, 0, 0, false);
			SlotProof_Check(TEXT("a second item is refused"),
				!bMoved && Slot->GetAllItems().Num() == 1,
				FString::Printf(TEXT("tried %s"), *GetNameSafe(Second.Item.ItemData)));
		}
		else
		{
			UE_LOG(LogSomnusInventory, Warning,
				TEXT("    [SKIP] a second item is refused   (only one item was reachable)"));
		}

		// --- round trip -------------------------------------------------------------------------
		{
			int32 X = 0, Y = 0;
			bool bRotated = false;
			if (Source->FindFirstFit(Subject.ItemData, X, Y, bRotated))
			{
				const bool bMovedBack = Source->MoveItemFrom(Slot, Subject.InstanceID, X, Y, bRotated);
				SlotProof_Check(TEXT("the item moves back out and the slot empties"),
					bMovedBack && Slot->GetAllItems().Num() == 0,
					FString::Printf(TEXT("-> %s (%d,%d)"), *GetNameSafe(Source), X, Y));
			}
			else
			{
				UE_LOG(LogSomnusInventory, Warning,
					TEXT("    [SKIP] round trip   (the source has no room left to take it back)"));
			}
		}
	}
}

void ASomnusCharacter::SomnusSlotProof()
{
	UE_LOG(LogSomnusInventory, Warning, TEXT("===== SlotProof ====="));

	if (!HasAuthority())
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("  aborted: no authority. This proof is about slot semantics, not routing - run it in the listen server window."));
		return;
	}

	// Every slot on the character, so adding one never quietly narrows what this covers. Naming
	// each run matters as much as the count: FindComponentByClass would have picked one of them
	// arbitrarily and reported a pass without saying which slot earned it.
	TArray<USomnusEquipmentSlotComponent*> Slots;
	GetComponents<USomnusEquipmentSlotComponent>(Slots);

	if (Slots.Num() == 0)
	{
		USomnusEquipmentSlotComponent* Scratch = NewObject<USomnusEquipmentSlotComponent>(this, TEXT("SlotProof_Scratch"));
		Scratch->RegisterComponent();
		Slots.Add(Scratch);
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("  %s has no slot component - created a scratch one to test against."), *GetName());
	}

	GSlotProofTotal = 0;
	GSlotProofFailed = 0;

	FSlotProofErrorSink Sink;
	GLog->AddOutputDevice(&Sink);

	for (USomnusEquipmentSlotComponent* Slot : Slots)
	{
		SlotProof_RunOne(this, Slot);
	}

	GLog->Flush();
	GLog->RemoveOutputDevice(&Sink);

	// The assertion none of the others can make. Every functional check above passes with
	// RebuildOccupationGrid left to the base class; only this line notices.
	UE_LOG(LogSomnusInventory, Warning, TEXT("  --- across all slots ---"));
	SlotProof_Check(TEXT("no error log lines were produced"),
		Sink.ErrorCount == 0,
		Sink.ErrorCount == 0 ? FString() : FString::Printf(TEXT("-> %d, first: %s"), Sink.ErrorCount, *Sink.FirstError));

	UE_LOG(LogSomnusInventory, Warning, TEXT("===== SlotProof: %d slot(s), %d/%d passed ====="),
		Slots.Num(), GSlotProofTotal - GSlotProofFailed, GSlotProofTotal);
}

