// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Zombie/SomnusZombieCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Core/SomnusCollisionChannels.h"

ASomnusZombieCharacter::ASomnusZombieCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<USomnusAttributeSet>("AttributeSet");

	GetCapsuleComponent()->SetCollisionProfileName(SomnusCollision::ZombieProfile);
}

void ASomnusZombieCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	GrantDefaults(AbilitySystemComponent);
	RefreshInAirTag();
}

TArray<FSomnusStrikeSourceInfo> ASomnusZombieCharacter::GetStrikeSources() const
{
	TArray<FSomnusStrikeSourceInfo> StrikeSources;
	// TODO: Values are hard coded here, in the furutre, we need more complicated system and matching property filed for this
	FSomnusStrikeSourceInfo LeftHandStrikeSource;
	LeftHandStrikeSource.SocketNames.Add(FName("HandGrip_L"));
	LeftHandStrikeSource.ReferenceMeshComponent = GetMesh();
	LeftHandStrikeSource.Weight = 2.f;
	LeftHandStrikeSource.TraceRadius = 10.f;

	FSomnusStrikeSourceInfo RightHandStrikeSource;
	RightHandStrikeSource.SocketNames.Add(FName("HandGrip_R"));
	RightHandStrikeSource.ReferenceMeshComponent = GetMesh();
	RightHandStrikeSource.Weight = 2.f;
	RightHandStrikeSource.TraceRadius = 10.f;
	StrikeSources.Add(LeftHandStrikeSource);
	StrikeSources.Add(RightHandStrikeSource);

	return StrikeSources;
}

void ASomnusZombieCharacter::HandleServerDeath()
{
	DetachFromControllerPendingDestroy();
	SetLifeSpan(CorpseLifeSpan);
}
