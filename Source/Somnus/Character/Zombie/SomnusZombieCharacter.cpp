// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Zombie/SomnusZombieCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "NavigationSystemTypes.h"
#include "AbilitySystem/Abilities/Zombie/SomnusGA_ZombieMeleeAttack.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ASomnusZombieCharacter::ASomnusZombieCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	AttributeSet = CreateDefaultSubobject<USomnusAttributeSet>("AttributeSet");
}

// Called when the game starts or when spawned
void ASomnusZombieCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASomnusZombieCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASomnusZombieCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ASomnusZombieCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	if (AttributeSet)
	{
		AttributeSet->OnHealthChanged.AddUObject(this, &ASomnusZombieCharacter::OnHealthChanged);
	}
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	
	for (const TSubclassOf<UGameplayEffect>& GEClass : DefaultGameplayEffects)
	{
		if (!GEClass) continue;
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GEClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			UE_LOG(LogTemp, Warning, TEXT("[Somnus][Possess]   Applied %s"), *GetNameSafe(GEClass));
		}
	}
	

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass) continue;
		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
		ASC->GiveAbility(Spec);
	}
}

void ASomnusZombieCharacter::OnHealthChanged(float CurrentValue, float MaxValue)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
			FString::Printf(TEXT("Zombie Health: %f / %f"), CurrentValue, MaxValue));
	}
	
	if (CurrentValue <= 0.f)
	{
		Die();
		// Sprint 1 Goal: Disable collision, unpossess, and DestroyActor
	}
}

void ASomnusZombieCharacter::Die()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetachFromControllerPendingDestroy();
	SetLifeSpan(5.f);
}
