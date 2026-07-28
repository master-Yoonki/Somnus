// Fill out your copyright notice in the Description page of Project Settings.


#include "SomnusCharacter.h"
#include "Core/SomnusPlayerState.h"
#include "AbilitySystemComponent.h"
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
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "Core/SomnusGameMode.h"
#include "Inventory/SomnusInventoryComponent.h"
#include "Inventory/SomnusContainerActor.h"
#include "Inventory/SomnusContainerDataAsset.h"
#include "Inventory/SomnusItemTypes.h"
#include "Inventory/SomnusItemDataAsset.h"
#include "Inventory/SomnusContainerEquipComponent.h"
#include "EngineUtils.h"

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

	CurrentGait = ESomnusGait::None;
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
		const USomnusAttributeSet* AS = PS->GetAttributeSet();
		UE_LOG(LogTemp, Warning, TEXT("[Somnus][Possess] %s  bDefaultEffectsApplied=%d  Health BEFORE=%.1f/%.1f  DefaultGE count=%d"),
			*GetNameSafe(this), bDefaultEffectsApplied ? 1 : 0,
			AS ? AS->GetHealth() : -1.f, AS ? AS->GetMaxHealth() : -1.f,
			DefaultGameplayEffects.Num());

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
					UE_LOG(LogTemp, Warning, TEXT("[Somnus][Possess]   Applied %s"), *GetNameSafe(GEClass));
				}
			}
			bDefaultEffectsApplied = true;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Somnus][Possess] %s  Health AFTER =%.1f/%.1f"),
			*GetNameSafe(this), AS ? AS->GetHealth() : -1.f, AS ? AS->GetMaxHealth() : -1.f);

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
	// Ability actions (Jump is routed here too — see GA_Jump for InputReleased handling)
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
	if (IsDead()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	// --- Server-only GAS logic ---
	// 1. Cancel all active abilities
	ASC->CancelAllAbilities();

	// 2. Remove effects tagged with Effect.RemoveOnDeath (regen, buffs, etc.)
	FGameplayTagContainer EffectTagsToRemove;
	EffectTagsToRemove.AddTag(SomnusTags::Effect_RemoveOnDeath);
	ASC->RemoveActiveEffectsWithGrantedTags(EffectTagsToRemove);

	// 3. Add the dead state tag
	ASC->AddLooseGameplayTag(SomnusTags::State_Dead);

	// 4. Multicast visual death (ragdoll, impulse, UI) to all machines
	MulticastDeath(HitDirection);
}

void ASomnusCharacter::MulticastDeath_Implementation(const FVector& HitDirection)
{
	// Disable capsule collision and stop movement
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	// Ragdoll the skeletal mesh
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();

	// Apply impulse in the hit direction
	if (!HitDirection.IsNearlyZero())
	{
		GetMesh()->AddImpulse(HitDirection * 1500.0f, NAME_None, true);
	}

	// Notify Blueprint for death UI (owning client only)
	if (IsLocallyControlled())
	{
		OnDeath();
	}
}

bool ASomnusCharacter::IsDead() const
{
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

// ===== [DEBUG SPIKE] Dynamic component replication verification. Delete once verified. =====

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

// Lists the items of one grid. Container items are labelled with their InstanceID, which is
// server-generated and replicated, so the same item can be matched across machines even
// though actor and component names differ.
static void SomnusDebug_DumpGridContents(USomnusInventoryComponent* Grid, const TCHAR* Indent)
{
	for (const FSomnusItemInstance& Item : Grid->GetAllItems())
	{
		const FString ItemName = Item.ItemData ? Item.ItemData->GetName() : TEXT("<null ItemData>");

		const FString InstanceLabel = Item.InstanceID.ToString(EGuidFormats::DigitsWithHyphens).Left(8);

		if (!Cast<USomnusContainerDataAsset>(Item.ItemData))
		{
			// Plain items carry their id and cell too, so stacks split off the same incoming
			// instance can be told apart - two entries sharing an id is the bug to watch for.
			UE_LOG(LogSomnusInventory, Warning, TEXT("%s%s x%d  [id %s]  at (%d,%d)%s"),
				Indent, *ItemName, Item.StackCount, *InstanceLabel,
				Item.GridPosition.X, Item.GridPosition.Y,
				Item.bRotated ? TEXT("  rotated") : TEXT(""));
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
			TEXT("%s%s [id %s]  ContainerActor=%s, Compartments=%d"),
			Indent, *ItemName, *InstanceLabel,
			*Item.ContainerActor->GetName(), Item.ContainerActor->GetCompartments().Num());
	}
}

// Picks a filler out of whatever the character is already carrying, so no extra asset setup is
// needed. Prefers a stackable one — a stack test against MaxStackCount 1 proves nothing.
static USomnusItemDataAsset* SomnusDebug_FindFiller(ASomnusCharacter* Character)
{
	USomnusItemDataAsset* Fallback = nullptr;
	for (const FSomnusActiveContainerInfo& Info : Character->GetActiveContainers())
	{
		if (!Info.Container)
		{
			continue;
		}
		for (const FSomnusItemInstance& Item : Info.Container->GetAllItems())
		{
			if (!Item.ItemData || Cast<USomnusContainerDataAsset>(Item.ItemData))
			{
				continue;
			}
			if (Item.ItemData->MaxStackCount > 1)
			{
				return Item.ItemData;
			}
			if (!Fallback)
			{
				Fallback = Item.ItemData;
			}
		}
	}
	return Fallback;
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

		// The leading number is the flat index into GetActiveContainers(), which is what
		// SomnusSeed takes to target one grid.
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

void ASomnusCharacter::SomnusFillActive()
{
	if (!HasAuthority())
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("SomnusFillActive: this is a client. Run it in the server window instead."));
		return;
	}

	USomnusContainerEquipComponent* Equip = FindComponentByClass<USomnusContainerEquipComponent>();
	if (!Equip)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("SomnusFillActive: no equip component on %s"), *GetName());
		return;
	}

	USomnusItemDataAsset* Filler = SomnusDebug_FindFiller(this);
	if (!Filler)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("SomnusFillActive: no plain item to use as filler"));
		return;
	}

	const TArray<FSomnusActiveContainerInfo> Active = Equip->GetActiveContainers();
	UE_LOG(LogSomnusInventory, Warning, TEXT("SomnusFillActive: character=%s, filler=%s, containers=%d"),
		*GetName(), *Filler->GetName(), Active.Num());

	for (const FSomnusActiveContainerInfo& Info : Active)
	{
		if (!Info.Container)
		{
			continue;
		}

		const int32 Leftover = Info.Container->AddItemAnywhere(Filler, 1);
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("   [%s %d]  leftover=%d  items=%d%s"),
			*SomnusDebug_SlotLabel(Info.SlotType), Info.SlotIndex,
			Leftover, Info.Container->GetAllItems().Num(),
			Leftover > 0 ? TEXT("   <-- did not fit") : TEXT(""));
	}
}

void ASomnusCharacter::SomnusSeed(int32 ContainerIndex, int32 Quantity)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("SomnusSeed: this is a client. Run it in the server window instead."));
		return;
	}

	USomnusContainerEquipComponent* Equip = FindComponentByClass<USomnusContainerEquipComponent>();
	if (!Equip)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("SomnusSeed: no equip component on %s"), *GetName());
		return;
	}

	const TArray<FSomnusActiveContainerInfo> Active = Equip->GetActiveContainers();
	if (!Active.IsValidIndex(ContainerIndex))
	{
		UE_LOG(LogSomnusInventory, Error,
			TEXT("SomnusSeed: index %d is out of range - GetActiveContainers() has %d entries. Run SomnusDumpContainers for the list."),
			ContainerIndex, Active.Num());
		return;
	}

	USomnusItemDataAsset* Filler = SomnusDebug_FindFiller(this);
	if (!Filler)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("SomnusSeed: no plain item to use as filler"));
		return;
	}

	const int32 Amount = FMath::Max(1, Quantity);
	const FSomnusActiveContainerInfo& Info = Active[ContainerIndex];

	// Straight into one grid, deliberately bypassing the aggregator - this builds the starting
	// state (a half-filled stack somewhere specific) that SomnusGive is then measured against.
	const int32 Leftover = Info.Container->AddItemAnywhere(Filler, Amount);

	UE_LOG(LogSomnusInventory, Warning,
		TEXT("SomnusSeed: #%d [%s %d] <- %s x%d (MaxStack=%d), leftover=%d"),
		ContainerIndex, *SomnusDebug_SlotLabel(Info.SlotType), Info.SlotIndex,
		*Filler->GetName(), Amount, Filler->MaxStackCount, Leftover);

	SomnusDebug_DumpGridContents(Info.Container, TEXT("     "));
}

void ASomnusCharacter::SomnusGive(int32 Quantity)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSomnusInventory, Warning,
			TEXT("SomnusGive: this is a client. Run it in the server window instead."));
		return;
	}

	USomnusContainerEquipComponent* Equip = FindComponentByClass<USomnusContainerEquipComponent>();
	if (!Equip)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("SomnusGive: no equip component on %s"), *GetName());
		return;
	}

	USomnusItemDataAsset* Filler = SomnusDebug_FindFiller(this);
	if (!Filler)
	{
		UE_LOG(LogSomnusInventory, Error, TEXT("SomnusGive: no plain item to use as filler"));
		return;
	}

	const int32 Amount = FMath::Max(1, Quantity);

	// Unlike SomnusFillActive, this goes through the aggregator, so it exercises the merge
	// pass, the placement pass and the priority order in one call - the thing under test.
	const int32 Leftover = Equip->TryAddItemAnywhere(Filler, Amount);

	UE_LOG(LogSomnusInventory, Warning,
		TEXT("SomnusGive: character=%s, filler=%s (MaxStack=%d), asked=%d, leftover=%d"),
		*GetName(), *Filler->GetName(), Filler->MaxStackCount, Amount, Leftover);

	SomnusDebug_DumpCharacter(this);
}

// ===== [/DEBUG SPIKE] =====
