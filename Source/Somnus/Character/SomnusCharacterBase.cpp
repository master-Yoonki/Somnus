// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/SomnusCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "PhysicsControlComponent.h"
#include "Character/SomnusHitReactComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/SomnusGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

ASomnusCharacterBase::ASomnusCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Named exactly as each subclass named them before the base existed. A blueprint stores its
	// overrides on a component against the component's name, and a rename would drop them quietly.
	HitReact = CreateDefaultSubobject<USomnusHitReactComponent>(TEXT("HitReact"));
	PhysicsControl = CreateDefaultSubobject<UPhysicsControlComponent>(TEXT("PhysicsControl"));

	// The capsule owns where the body is. Simulated bodies that also wrote it back would fight the
	// capsule for the transform every time a hit reaction woke them.
	GetMesh()->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;
}

void ASomnusCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASomnusCharacterBase, bDead);
}

void ASomnusCharacterBase::GrantDefaults(UAbilitySystemComponent* ASC)
{
	if (!ASC || !HasAuthority() || bDefaultsGranted)
	{
		return;
	}
	bDefaultsGranted = true;

	for (const TSubclassOf<UGameplayEffect>& EffectClass : DefaultGameplayEffects)
	{
		if (!EffectClass) continue;

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass) continue;
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
	}
}

void ASomnusCharacterBase::Die(const FVector& HitDirection)
{
	// Death is a server decision, and bDead is replicated - a client must never set it locally.
	if (!HasAuthority() || IsDead()) return;

	// Set before anything that could bail out. Whether this body is a corpse must not depend on
	// whether there happened to be an ability system left to clean up.
	bDead = true;

	HandleServerDeath();

	// Only an ability system that exists can be cleaned up - a player's lives on a PlayerState,
	// which a character placed in the level never had and an unpossessed corpse no longer has.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();

		// Regen, buffs and anything else that has no business ticking on a body.
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(SomnusTags::Effect_RemoveOnDeath));

		// Replicated, which a loose tag is not by default, so a client asking the ability system
		// hears what the server does. Kept alongside bDead because it is what blocks abilities.
		ASC->AddLooseGameplayTag(SomnusTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
	}

	OnDied.Broadcast(this);

	// No hand call to ApplyDeathState here: a multicast runs locally on the authority too
	// (Actor.cpp:5500-5519), so the server gets its half from this line.
	MulticastDeath(HitDirection);
}

bool ASomnusCharacterBase::IsDead() const
{
	if (bDead) return true;

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(SomnusTags::State_Dead);
}

void ASomnusCharacterBase::MulticastDeath_Implementation(const FVector& HitDirection)
{
	// Called here as well as from OnRep_Dead because the two have no ordering guarantee between
	// them. Arriving first, this is what has the mesh simulating in time for the impulse below;
	// arriving second, it costs nothing.
	ApplyDeathState();

	// An event rather than part of the state above, because it only means anything at the instant
	// it happens. A machine that missed it wants the body, not a shove five seconds late.
	if (!HitDirection.IsNearlyZero())
	{
		GetMesh()->AddImpulse(HitDirection * DeathImpulseStrength, NAME_None, true);
	}

	HandleDeathEvent(HitDirection);
}

void ASomnusCharacterBase::OnRep_Dead()
{
	// The half of dying that has to survive being missed. A machine that was not watching - out of
	// relevancy, or not yet connected when the body fell - never hears the multicast, and would
	// otherwise be left with a corpse standing up and playing its idle for as long as it lies there.
	if (bDead)
	{
		ApplyDeathState();
	}
}

void ASomnusCharacterBase::ApplyDeathState()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// Going slack goes through the hit react component rather than the mesh directly: it owns the
	// physics control body modifiers, and those reassert the movement type. Setting bodies to
	// simulate behind its back leaves them kinematic, which reads on screen as a frozen pose.
	if (HitReact)
	{
		HitReact->SetPhysicsPose(ESomnusPhysicsPose::Limp);
	}
}
