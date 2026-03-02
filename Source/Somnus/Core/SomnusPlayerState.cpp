// Fill out your copyright notice in the Description page of Project Settings.


#include "SomnusPlayerState.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "AbilitySystemComponent.h"

ASomnusPlayerState::ASomnusPlayerState()
{
	// Create the Ability System Component
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    
	// Enable replication for multiplayer support
	AbilitySystemComponent->SetIsReplicated(true);
    
	// Mixed mode is the Best Practice for multiplayer action games
	// It replicates Gameplay Effects to the owning client, but not to others
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Increase the update frequency for smoother networking in fast-paced games
	SetNetUpdateFrequency(100.0f);
	
	AttributeSet = CreateDefaultSubobject<USomnusAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ASomnusPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
