// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/SomnusZombieAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "Character/Zombie/SomnusZombieCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

void USomnusZombieAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CachedCharacter = Cast<ASomnusZombieCharacter>(TryGetPawnOwner());
	if (CachedCharacter)
	{
		MovementComponent = CachedCharacter->GetCharacterMovement();
	}
}

void USomnusZombieAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedCharacter || !MovementComponent)
	{
		return;
	}

	GroundSpeed = MovementComponent->Velocity.Size2D();
	bIsMoving = GroundSpeed > 3.0f;

	if (const UAbilitySystemComponent* ASC = CachedCharacter->GetAbilitySystemComponent())
	{
		bIsDead = ASC->HasMatchingGameplayTag(SomnusTags::State_Dead);
	}
}
