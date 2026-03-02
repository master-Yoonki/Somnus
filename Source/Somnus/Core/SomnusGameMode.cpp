// Fill out your copyright notice in the Description page of Project Settings.


#include "SomnusGameMode.h"

#include "Core/SomnusPlayerState.h"
#include "Character/SomnusCharacter.h"

ASomnusGameMode::ASomnusGameMode()

{// Set the default pawn to our custom GAS character
	DefaultPawnClass = ASomnusCharacter::StaticClass();

	// Set the default PlayerState to our custom GAS PlayerState
	PlayerStateClass = ASomnusPlayerState::StaticClass();
}
