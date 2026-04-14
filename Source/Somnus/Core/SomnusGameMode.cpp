// Fill out your copyright notice in the Description page of Project Settings.


#include "SomnusGameMode.h"

#include "Core/SomnusPlayerState.h"
#include "Character/SomnusCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Core/SomnusGameplayTags.h"

ASomnusGameMode::ASomnusGameMode()
{
	DefaultPawnClass = ASomnusCharacter::StaticClass();
	PlayerStateClass = ASomnusPlayerState::StaticClass();
}

void ASomnusGameMode::RequestRespawn(AController* Controller)
{
	if (!Controller) return;

	// Clean up ASC state on PlayerState before spawning new pawn
	if (ASomnusPlayerState* PS = Controller->GetPlayerState<ASomnusPlayerState>())
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			// Remove death tag so the new pawn starts alive
			ASC->RemoveLooseGameplayTag(SomnusTags::State_Dead);

			// Wipe granted abilities — PossessedBy on the new pawn will re-grant them
			ASC->ClearAllAbilities();

			// Remove all active effects — PossessedBy will re-apply DefaultGameplayEffects
			FGameplayEffectQuery RemoveAllQuery;
			ASC->RemoveActiveEffects(RemoveAllQuery);
		}
	}

	// Destroy the old (ragdolled) pawn
	if (APawn* OldPawn = Controller->GetPawn())
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}

	// Spawn a new pawn and possess it — triggers PossessedBy which re-initializes everything
	RestartPlayer(Controller);
}
