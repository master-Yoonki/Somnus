// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SomnusAnimNotifyState_MovementCancellable.generated.h"

/**
 * While this notify state is active, adds State.MovementCancellable to the
 * character's ASC. Movement input during this window will cancel active
 * melee abilities, allowing the player to "recovery cancel" out of attacks.
 */
UCLASS(DisplayName = "Movement Cancellable Window")
class SOMNUS_API USomnusAnimNotifyState_MovementCancellable : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("MovementCancellable"); }
};
