// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Actor.h"
#include "SomnusWeapon.generated.h"

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

protected:
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
