// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/SomnusWeapon.h"
#include "Character/SomnusCharacter.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASomnusWeapon::ASomnusWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// Must replicate for multiplayer
	bReplicates = true;
	SetReplicateMovement(true);

	// Component setup
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = WeaponMesh;

	// Default values
	AttachSocketName = FName("HandGrip_R");
}

void ASomnusWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Replicate the owner to all clients
	DOREPLIFETIME(ASomnusWeapon, OwningCharacter);
}

void ASomnusWeapon::Equip(ASomnusCharacter* TargetCharacter)
{
	// Equip logic must only run on the Server
	if (!HasAuthority() || !TargetCharacter) return;
	
	OwningCharacter = TargetCharacter;
	SetOwner(TargetCharacter);

	// 1. Physical Attachment (Server side)
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(OwningCharacter->GetMesh(), AttachRules, AttachSocketName);

	// 2. GAS Grants (Tags and Abilities)
	if (UAbilitySystemComponent* ASC = OwningCharacter->GetAbilitySystemComponent())
	{
		// Grant permissions tags (e.g., Ability.Enable.Swing)
		ASC->AddLooseGameplayTags(WeaponTags);

		// Grant gameplay abilities
		for (TSubclassOf<UGameplayAbility> AbilityClass : AbilitiesToGrant)
		{
			if (AbilityClass)
			{
				// Create a spec and grant the ability, storing the receipt (Handle)
				FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
				FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
				GrantedAbilityHandles.Add(Handle);
			}
		}
	}
}

void ASomnusWeapon::Unequip()
{
	// Unequip logic must only run on the Server
	if (!HasAuthority() || !OwningCharacter) return;

	// 1. GAS Revokes (Tags and Abilities)
	if (UAbilitySystemComponent* ASC = OwningCharacter->GetAbilitySystemComponent())
	{
		// Remove permission tags
		ASC->RemoveLooseGameplayTags(WeaponTags);

		// Remove granted abilities using stored receipts
		for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
		{
			ASC->ClearAbility(Handle);
		}
		GrantedAbilityHandles.Empty();
	}

	// 2. Physical Detachment (Server side)
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	OwningCharacter = nullptr;
	SetOwner(nullptr);
}

void ASomnusWeapon::OnRep_OwningCharacter()
{
	// Client-side physical attachment/detachment only
	if (OwningCharacter)
	{
		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		AttachToComponent(OwningCharacter->GetMesh(), AttachRules, AttachSocketName);
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void ASomnusWeapon::LinkAnimLayers(USkeletalMeshComponent* Mesh)
{
	if (!Mesh) return;

	if (FullBodyLocomotionLayerClass)
	{
		Mesh->LinkAnimClassLayers(FullBodyLocomotionLayerClass);
	}
	if (UpperBodyAnimLayerClass)
	{
		Mesh->LinkAnimClassLayers(UpperBodyAnimLayerClass);
	}
}

void ASomnusWeapon::UnlinkAnimLayers(USkeletalMeshComponent* Mesh)
{
	if (!Mesh) return;

	if (FullBodyLocomotionLayerClass)
	{
		Mesh->UnlinkAnimClassLayers(FullBodyLocomotionLayerClass);
	}
	if (UpperBodyAnimLayerClass)
	{
		Mesh->UnlinkAnimClassLayers(UpperBodyAnimLayerClass);
	}
}
