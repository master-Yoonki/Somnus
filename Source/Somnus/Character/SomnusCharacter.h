// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Core/SomnusStrikeSource.h"
#include "Core/SomnusInteractable.h"
#include "SomnusCharacter.generated.h"

class ASomnusWeapon;
class UGameplayAbility;
class UGameplayEffect;
class USomnusInputConfig;
class USomnusInventoryComponent;
enum class ESomnusGait : uint8;

/**
 * Base character class for Project Somnus.
 * Acts as the physical avatar for the GAS component stored in the PlayerState.
 */
UCLASS()
class SOMNUS_API ASomnusCharacter : public ACharacter, public IAbilitySystemInterface, public ISomnusStrikeSource, public ISomnusInteractable
{
	GENERATED_BODY()

public:
	ASomnusCharacter();

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
	
	// Physics-based flinch on non-lethal hits
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USomnusHitReactComponent> HitReact;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPhysicsControlComponent> PhysicsControl;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void InitHUD();

	UFUNCTION(BlueprintCallable, Category = "GAS|UI")
	void BindAttributeCallbacks();

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|UI")
	void UpdateHealthUI(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|UI")
	void UpdateStaminaUI(float CurrentStamina, float MaxStamina);

	// Called when Health reaches zero. Cancels abilities, ragdolls.
	// HitDirection is the direction of the killing blow (for ragdoll impulse). Zero = no impulse.
	UFUNCTION(BlueprintCallable, Category = "GAS")
	virtual void Die(const FVector& HitDirection = FVector::ZeroVector);

	UFUNCTION(BlueprintPure, Category = "GAS")
	bool IsDead() const;

protected:
	/** Set on death and never cleared. The ability system lives on the PlayerState, which
	 *  unpossessing hands to the next pawn - a corpse asked through the ASC alone would answer
	 *  that it is alive, and so would a character placed in the level that never had one. */
	UPROPERTY(ReplicatedUsing = OnRep_Dead, VisibleInstanceOnly, BlueprintReadOnly, Category = "GAS")
	bool bDead = false;

	UFUNCTION()
	void OnRep_Dead();

	/** Everything about being a corpse that has to hold on every machine for as long as the body
	 *  lasts. Idempotent on purpose: it arrives by replication, by multicast, or by both in
	 *  either order, and none of those know about the others. */
	void ApplyDeathState();

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

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<struct FSomnusActiveContainerInfo> GetActiveContainers() const;
	
	virtual void SetHighlighted_Implementation(bool bHighlighted) override;

	/** ISomnusInteractable. A body hands its searcher over to their own loot component; a living
	 *  character offers nothing. Whether the searcher may then reach any particular grid is that
	 *  component's question, not this one's. */
	virtual void Interact_Implementation(AActor* Interactor) override;

	// Switch weapon slot: 0 = unarmed, 1+ = weapon index in WeaponClasses.
	// Safe to call from client — routes to ServerSwitchWeapon.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeapon(int32 SlotIndex);

	// Server-authoritative switch. Do not call directly from BP — use SwitchWeapon.
	UFUNCTION(Server, Reliable)
	void ServerSwitchWeapon(int32 SlotIndex);

	UFUNCTION()
	ESomnusGait GetCurrentGait() const { return CurrentGait; }

	// Console: dumps this machine's view of every character's containers - each compartment's
	// grid, its contents, and the ASomnusContainerActor behind any container item, recursing
	// into nested ones. Run it in both windows and diff: items carry their replicated
	// InstanceID, so the same item is identifiable on either side even though actor and
	// component names are not.
	UFUNCTION(Exec)
	void SomnusDumpContainers();

	// TEMPORARY - equip slot isolation proof. Runs the whole risk check server-side and prints
	// PASS/FAIL per assertion, including a count of error log lines, which is the only way the
	// missing RebuildOccupationGrid override announces itself. Delete once the result is recorded.
	UFUNCTION(Exec)
	void SomnusSlotProof();

protected:
	// GEs applied to the ASC at possession (e.g., stamina regen, passive buffs)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

	// Abilities granted at possession time (innate abilities like Jump)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

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

	// Guards against double-granting on repossession/respawn
	bool bDefaultAbilitiesGiven = false;
	bool bDefaultEffectsApplied = false;

private:
	// Multicast: runs ragdoll and visual death effects on all machines
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeath(const FVector& HitDirection);
};
