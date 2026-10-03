// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SomnusCharacterBase.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Core/SomnusStrikeSource.h"
#include "Core/SomnusInteractable.h"
#include "Character/SomnusMovementTypes.h"
#include "Camera/SomnusCameraMode.h"
#include "SomnusCharacter.generated.h"

class ASomnusWeapon;
class UGameplayAbility;
class UGameplayEffect;
class USomnusInputConfig;
class USomnusInventoryComponent;
class USomnusItemAnimLayers;
class USomnusCharacterMovementComponent;

/**
 * Base character class for Project Somnus.
 * Acts as the physical avatar for the GAS component stored in the PlayerState.
 */
UCLASS()
class SOMNUS_API ASomnusCharacter : public ASomnusCharacterBase, public ISomnusStrikeSource, public ISomnusInteractable
{
	GENERATED_BODY()

public:
	ASomnusCharacter(const FObjectInitializer& ObjectInitializer);

	// Implement IAbilitySystemInterface to fetch ASC from PlayerState
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void Tick(float DeltaTime) override;

	// Called when the server assigns a controller to this character
	virtual void PossessedBy(AController* NewController) override;

	// Replicate this actor for multiplayer
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual TArray<FSomnusStrikeSourceInfo> GetStrikeSources() const override;
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

	void Interact(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void Server_Interact();
	
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

	// Owns every grid this character can reach: pockets, plus any worn rig and backpack.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EquipmentComponent", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USomnusContainerEquipComponent> ContainerEquipmentComponent;

	// The worn slots that are not storage - weapons now, armour later.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EquipmentComponent", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USomnusEquipmentComponent> EquipmentComponent;

	// Which grids this character may reach into, its own included - i.e. the body it has open.
	// A C++ subobject rather than a Blueprint-added component because every client entry point
	// into storage now routes through it: a character missing it could not move items at all.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USomnusLootComponent> LootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USomnusInteractorComponent> InteractorComponent;


public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void InitHUD();

	UFUNCTION(BlueprintCallable, Category = "GAS|UI")
	void BindAttributeCallbacks();

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|UI")
	void UpdateHealthUI(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|UI")
	void UpdateStaminaUI(float CurrentStamina, float MaxStamina);

protected:
	/** A looter who dies stops looting, and a body stops holding its weapon. */
	virtual void HandleServerDeath() override;

	/** The death screen, on the machine of the player who died. */
	virtual void HandleDeathEvent(const FVector& HitDirection) override;

public:

	// Blueprint event fired on the owning client when this character dies (for death UI)
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS")
	void OnDeath();

	// Client requests respawn — server validates and tells GameMode to spawn a new pawn
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "GAS")
	void ServerRequestRespawn();

	// Getter function for AnimNotify and Abilities to read the weapon data
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ASomnusWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

	/** How far the mesh is turned for the aim stance right now, mid-blend included. Animation
	 *  counter-rotates the stance clips by exactly this so the weapon keeps pointing at the aim. */
	float GetAimStanceYaw() const { return AimStanceYaw; }

	/** Whether the aim ability is holding State.Aiming. The tag is asked once a frame and kept
	 *  here so the aim stance, the camera and animation all answer from the same value, and so
	 *  that animation never reaches into the ability system from a worker thread. */
	UFUNCTION(BlueprintPure, Category = "GAS")
	bool IsAiming() const { return bIsAiming; }

	/** Called by the equipment component when a carried weapon is about to be destroyed - most
	 *  often because it was dragged out of its slot while it was in a hand. Does nothing unless
	 *  that weapon is the one currently drawn.
	 *
	 *  Not a request to unequip: the caller already did that while the actor could still hand its
	 *  abilities back. This is only the character stopping pointing at it. */
	void NotifyCarriedWeaponRetiring(ASomnusWeapon* Weapon);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<struct FSomnusActiveContainerInfo> GetActiveContainers() const;
	
	virtual void SetHighlighted_Implementation(bool bHighlighted) override;

	/** ISomnusInteractable. A body hands its searcher over to their own loot component; a living
	 *  character offers nothing. Whether the searcher may then reach any particular grid is that
	 *  component's question, not this one's. */
	virtual void Interact_Implementation(AActor* Interactor) override;

	// Which worn slot to draw from, in the indices USomnusEquipmentComponent::GetWeaponSlot
	// defines: 0 is primary, 1 is secondary. An index no slot answers to holsters, so there is no
	// separate "unarmed" index that has to be kept in step with how many slots exist.
	// Safe to call from client — routes to ServerSwitchWeapon.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeapon(int32 SlotIndex);

	// Server-authoritative switch. Do not call directly from BP — use SwitchWeapon.
	UFUNCTION(Server, Reliable)
	void ServerSwitchWeapon(int32 SlotIndex);

	/** The gait animation shows. See Gait. */
	UFUNCTION(BlueprintPure, Category = "Movement")
	ESomnusGait GetGait() const { return Gait; }

	/** Whether the body is held square to the camera. See bIsStrafing. */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsStrafing() const { return bIsStrafing; }

	/** The movement component as the class it is always created as (see the constructor). */
	UFUNCTION(BlueprintPure, Category = "Movement")
	USomnusCharacterMovementComponent* GetSomnusMovement() const;

	// Console: dumps this machine's view of every character's containers - each compartment's
	// grid, its contents, and the ASomnusContainerActor behind any container item, recursing
	// into nested ones. Run it in both windows and diff: items carry their replicated
	// InstanceID, so the same item is identifiable on either side even though actor and
	// component names are not.
	UFUNCTION(Exec)
	void SomnusDumpContainers();
	
	UFUNCTION(BlueprintPure, Category = "Movement")
	ESomnusMovementMode GetMovementMode() const;
	
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsMoving() const;
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool JustLanded() const { return bJustLanded; }
	UFUNCTION(BlueprintPure, Category = "Movement")
	FVector GetLandVelocity() const { return LandVelocity; }
protected:
	// Currently equipped weapon (null = unarmed)
	UPROPERTY(Transient, ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<ASomnusWeapon> EquippedWeapon;

	/** The item layer this machine last linked into the mesh. Unlinking goes by this rather than by
	 *  the previous weapon, which may already be destroyed by the time the swap is seen here. */
	UPROPERTY(Transient)
	TSubclassOf<USomnusItemAnimLayers> LinkedItemLayerClass;

	/** Time the body takes to settle into, or back out of, the aim stance turn. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation", meta = (ClampMin = "0", Units = "Seconds"))
	float AimStanceTurnSmoothingTime = 0.1f;

	/** The mesh's authored relative rotation, which the aim stance turn is added on top of. */
	FQuat BaseMeshRelativeRotation = FQuat::Identity;

	float AimStanceYaw = 0.f;

	bool bIsAiming = false;

	/** Turns the mesh rather than the capsule: movement, aim and camera stay on the capsule, while
	 *  motion matching reads the mesh's rotation as the body's facing and picks steps to suit. */
	void UpdateAimStanceTurn(float DeltaTime);

	/** The framing the camera is moving toward. Set this from whatever decides the character is
	 *  exploring, strafing or aiming; the camera does not read that state itself. */
	UPROPERTY(BlueprintReadWrite, Category = "Camera")
	ESomnusCameraFraming CameraFraming = ESomnusCameraFraming::Explore;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Framing")
	FSomnusCameraMode ExploreCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Framing")
	FSomnusCameraMode StrafeCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Framing")
	FSomnusCameraMode MeleeAimCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Framing")
	FSomnusCameraMode GunAimCamera;

	const FSomnusCameraMode& GetCameraMode(ESomnusCameraFraming Framing) const;

	/** Moves the camera toward the framing it was given. Arm length, offset and field of view all
	 *  come off one blend: moved apart they read as a camera lagging its own decision. */
	void UpdateCamera(float DeltaTime);

	/** The one place the camera's numbers are written. Blending the result of two framings rather
	 *  than their inputs would start here too, so the rest of the class need not know which it is. */
	void ApplyCameraMode(const FSomnusCameraMode& From, const FSomnusCameraMode& To, float Alpha);

	/** Where the camera stood when the framing last changed. Read off the components rather than
	 *  off the framing it was leaving, so a blend interrupted halfway does not jump backwards. */
	FSomnusCameraMode CameraBlendStart;
	ESomnusCameraFraming CameraFraming_LastFrame = ESomnusCameraFraming::Explore;
	float CameraBlendElapsed = 0.f;
	
	UFUNCTION()
	void OnRep_EquippedWeapon(ASomnusWeapon* OldWeapon);

	// Handles anim layer swap — called on both server and client
	void UpdateWeaponAnimLayers(const ASomnusWeapon* NewWeapon);
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement")
	FVector LastUpdateVelocity;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Movement")
	FVector LandVelocity;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Movement")
	bool bJustLanded;

	/** The gait animation shows. Worked out every tick from the movement component by the machines
	 *  that run this character's moves - its owner, straight from its own input, and the server -
	 *  and sent from the server to everyone else, who have no wishes to work it out from. */
	UPROPERTY(Transient, Replicated, BlueprintReadOnly, VisibleAnywhere, Category = "Movement")
	ESomnusGait Gait;

	/** Kept, and sent to the machines that only watch, for the same reason as Gait. */
	UPROPERTY(Transient, Replicated, BlueprintReadOnly, VisibleAnywhere, Category = "Movement")
	bool bIsStrafing = false;

	/** Refreshes Gait and bIsStrafing on the machines that run this character's moves, and on the
	 *  ones that only watch, hands the replicated strafe state to the movement component. */
	void UpdateLocomotionState();

	/** How hard an overlap that nobody is pushing into gets eased apart, as the share of it closed
	 *  per second - a zombie that walked into a standing player, say. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Crowd", meta = (ClampMin = "0.0"))
	float CrowdPushStrength = 8.f;

	/** Moves each overlapping zombie by ComputeZombieShove, swept. Server only - zombies are the
	 *  server's to move, and their new places replicate. */
	void PushOverlappingZombies(float DeltaTime);

	/** Where one zombie should go this tick, made of three parts along the line from this body to it:
	 *  carried at the speed this body is moving into it, which the movement component has already
	 *  cut to what the zombie's resistance leaves; eased out by part of whatever overlap is left;
	 *  and pushed straight back out of anything past the movement component's penetration limit,
	 *  so a zombie walking into the player cannot reach its centre either. */
	FVector ComputeZombieShove(const ACharacter* Zombie, float DeltaTime) const;
};
