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
#include "Inventory/SomnusEquipmentComponent.h"
#include "Inventory/SomnusLootComponent.h"
#include "EngineUtils.h"
#include "Core/SomnusInteractable.h"
#include "Core/SomnusCollisionChannels.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/SomnusHitReactComponent.h"
#include "Equipment/SomnusMeleeWeapon.h"
#include "PhysicsControlComponent.h"
#include "Core/SomnusInteractorComponent.h"

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

	EquipmentComponent = CreateDefaultSubobject<USomnusEquipmentComponent>(TEXT("Equipment"));

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

	UpdateWeaponAnimLayers(OldWeapon, NewWeapon);
}

void ASomnusCharacter::NotifyCarriedWeaponRetiring(ASomnusWeapon* Weapon)
{
	if (!Weapon || EquippedWeapon != Weapon)
	{
		return;
	}

	// Null first, then relink. UpdateWeaponAnimLayers never reads the old weapon, so passing one
	// that is a moment from being destroyed is safe, and naming it keeps the two calls that swap
	// weapons and the one that loses one reading the same way.
	EquippedWeapon = nullptr;
	UpdateWeaponAnimLayers(Weapon, nullptr);
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

	// Cleared as well as unequipped. A body that still names a weapon is a body that answers
	// GetEquippedWeapon with something it is no longer holding - and every melee trace and anim
	// notify asks exactly that question.
	if (EquippedWeapon)
	{
		EquippedWeapon->Unequip();
		EquippedWeapon = nullptr;
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
