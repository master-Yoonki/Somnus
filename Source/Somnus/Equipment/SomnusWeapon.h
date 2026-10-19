// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Actor.h"
#include "SomnusWeapon.generated.h"

class USomnusItemAnimLayers;

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

	TSubclassOf<USomnusItemAnimLayers> GetAnimLayerClass() const { return AnimLayerClass; }
	float GetAimStanceYaw() const { return AimStanceYaw; }

protected:
	/** Linked into the holder's animation while this weapon is drawn. The weapon only names it -
	 *  the character owns the mesh, so linking and unlinking are the character's to do. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TSubclassOf<USomnusItemAnimLayers> AnimLayerClass;

	/** Degrees the stance clips turn the pelvis away from where the weapon points. The body is
	 *  turned this far while aiming, so the stance's upper body lands on hips that already face
	 *  its way instead of twisting at the waist over hips that face forward. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation", meta = (Units = "Degrees"))
	float AimStanceYaw = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	class UStaticMeshComponent* WeaponMesh;

	// Socket name for attachment (e.g., "hand_rSocket")
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Setup")
	FName AttachSocketName;

	// Abilities to grant (Aim, Swing, etc.)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|GAS")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

	// Tags to apply to the owner when equipped (e.g., Weapon.Equipped.Bat)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weapon|GAS")
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
