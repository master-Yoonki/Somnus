// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Actor.h"
#include "SomnusWeapon.generated.h"

UENUM(BlueprintType)
enum class ESomnusWeaponType : uint8
{
	None    UMETA(DisplayName = "None"),
	Bat     UMETA(DisplayName = "Bat"),
	Sword   UMETA(DisplayName = "Sword")
};

UCLASS()
class SOMNUS_API ASomnusWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASomnusWeapon();

	// Replicate this actor for multiplayer
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called by the server to equip this weapon to a character
	virtual void Equip(class ASomnusCharacter* TargetCharacter);
    
	// Called by the server to unequip
	virtual void Unequip();

	ESomnusWeaponType GetWeaponType() const { return WeaponType; }
	bool HasUpperBodyLayer() const { return UpperBodyAnimLayerClass != nullptr; }

	void LinkAnimLayers(USkeletalMeshComponent* Mesh);
	void UnlinkAnimLayers(USkeletalMeshComponent* Mesh);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Setup")
	ESomnusWeaponType WeaponType = ESomnusWeaponType::None;

	// Replaces full body locomotion (e.g., rifle has its own walk/run/idle set)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TSubclassOf<UAnimInstance> FullBodyLocomotionLayerClass;

	// Upper body overlay (e.g., bat/sword idle pose over unarmed locomotion)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TSubclassOf<UAnimInstance> UpperBodyAnimLayerClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	class UStaticMeshComponent* WeaponMesh;

	// Socket name for attachment (e.g., "hand_rSocket")
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Setup")
	FName AttachSocketName;

	// Abilities to grant (Aim, Swing, etc.)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|GAS")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

	// Tags to apply to the owner when equipped (e.g., Weapon.Equipped.Bat)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|GAS")
	FGameplayTagContainer WeaponTags;

	// Internal state tracking
	UPROPERTY(Transient, ReplicatedUsing = OnRep_OwningCharacter)
	class ASomnusCharacter* OwningCharacter;

	// Store handles (receipts) to remove abilities later
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	
	// Called on clients when OwningCharacter is replicated
	UFUNCTION()
	virtual void OnRep_OwningCharacter();

};
