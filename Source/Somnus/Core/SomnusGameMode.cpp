// Fill out your copyright notice in the Description page of Project Settings.


#include "SomnusGameMode.h"

#include "Core/SomnusPlayerState.h"
#include "Character/SomnusCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Core/SomnusGameplayTags.h"
#include "Inventory/SomnusContainerEquipComponent.h"

ASomnusGameMode::ASomnusGameMode()
{
	DefaultPawnClass = ASomnusCharacter::StaticClass();
	PlayerStateClass = ASomnusPlayerState::StaticClass();
}

void ASomnusGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (!NewPlayer) return;

	// After Super, because the pawn is spawned and possessed inside it - and its components have
	// begun play, so the slots the kit needs to land in already exist and pockets are already worn.
	//
	// Whether this is a first life is a fact about the player, not about the path taken to get a
	// body: PostLogin restarts the first one, a starting match restarts whoever was waiting, and
	// RequestRespawn restarts the dead. The player state is what survives all three to be asked.
	ASomnusPlayerState* PS = NewPlayer->GetPlayerState<ASomnusPlayerState>();
	if (!PS || PS->bHasReceivedStartingKit) return;

	APawn* Pawn = NewPlayer->GetPawn();
	if (!Pawn) return;

	if (USomnusContainerEquipComponent* Storage = Pawn->FindComponentByClass<USomnusContainerEquipComponent>())
	{
		Storage->GrantStartingKit();
		PS->bHasReceivedStartingKit = true;
	}
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

	// Leave the old pawn standing as a lootable body. Unpossessing clears its PlayerState
	// (Pawn.cpp:698), which is why the corpse tracks its own death rather than asking the ASC.
	if (APawn* OldPawn = Controller->GetPawn())
	{
		Controller->UnPossess();
		RegisterCorpse(OldPawn);
	}

	// Spawn a new pawn and possess it — triggers PossessedBy which re-initializes everything
	RestartPlayer(Controller);
}

void ASomnusGameMode::RegisterCorpse(APawn* Corpse)
{
	if (!IsValid(Corpse)) return;

	// Bodies can be destroyed by other means (level streaming, cheats), and a budget filled with
	// dead entries would cull live corpses early.
	Corpses.RemoveAll([](const APawn* Existing) { return !IsValid(Existing); });

	Corpses.Add(Corpse);

	while (Corpses.Num() > MaxCorpses)
	{
		if (APawn* Oldest = Corpses[0])
		{
			Oldest->Destroy();
		}
		Corpses.RemoveAt(0);
	}
}
