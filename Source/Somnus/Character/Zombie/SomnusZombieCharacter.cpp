// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Zombie/SomnusZombieCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "NavigationSystemTypes.h"
#include "AbilitySystem/Abilities/Zombie/SomnusGA_ZombieMeleeAttack.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Character/SomnusHitReactComponent.h"
#include "Components/CapsuleComponent.h"
#include "PhysicsControlComponent.h"
#include "Core/SomnusGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASomnusZombieCharacter::ASomnusZombieCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	AttributeSet = CreateDefaultSubobject<USomnusAttributeSet>("AttributeSet");

	HitReact = CreateDefaultSubobject<USomnusHitReactComponent>("HitReact");

	PhysicsControl = CreateDefaultSubobject<UPhysicsControlComponent>("PhysicsControl");

	GetMesh()->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;
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
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	check(ASC);
	if (ASC->HasMatchingGameplayTag(SomnusTags::State_Dead))
	{
		return;
	}
	{
		// Server only logic
		bDead = true;
		ASC->AddLooseGameplayTag(SomnusTags::State_Dead);
		ASC->CancelAllAbilities();
		DetachFromControllerPendingDestroy();
		SetLifeSpan(5.f);
	}

	// A multicast runs locally on the authority too (Actor.cpp:5500-5519), so the server picks up
	// its own death state from here rather than needing the hand call OnRep cannot give it.
	MulticastZombieDeath();
}

void ASomnusZombieCharacter::MulticastZombieDeath_Implementation()
{
	ApplyDeathState();
}

void ASomnusZombieCharacter::ApplyDeathState()
{
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// Same reason as the player: the hit react component owns the physics control body modifiers
	// and reasserts the movement type, so the mesh cannot be made to simulate behind its back.
	if (HitReact)
	{
		HitReact->SetPhysicsPose(ESomnusPhysicsPose::Limp);
	}
}

void ASomnusZombieCharacter::OnRep_Dead()
{
	// The half of dying that survives being missed. The multicast reaches only whoever was
	// relevant at that instant; this reaches everyone else the moment they arrive.
	if (bDead)
	{
		ApplyDeathState();
	}
}

void ASomnusZombieCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASomnusZombieCharacter, bDead);
}
