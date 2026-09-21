// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "SomnusCharacterBase.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UPhysicsControlComponent;
class USomnusHitReactComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSomnusCharacterDiedSignature, ASomnusCharacterBase*, DeadCharacter);

/**
 * What every body in the game has in common, whoever is driving it: an ability system, a physical
 * reaction to being hit, and a way of dying that every machine agrees on.
 *
 * Where the ability system lives is left to each subclass on purpose. A player's lives on the
 * PlayerState so it outlives the pawn; a zombie's lives on the zombie because nothing else would
 * hold it. The base asks for the component rather than owning one so that difference stays in
 * sight instead of being flattened into a shared member that is only right for one of them.
 */
UCLASS(Abstract)
class SOMNUS_API ASomnusCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASomnusCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
		PURE_VIRTUAL(ASomnusCharacterBase::GetAbilitySystemComponent, return nullptr;);

	/** Server only, and only once - a body that is already dead ignores it. HitDirection is the
	 *  direction of the killing blow, used to shove the ragdoll; zero means no shove. */
	UFUNCTION(BlueprintCallable, Category = "GAS")
	virtual void Die(const FVector& HitDirection = FVector::ZeroVector);

	UFUNCTION(BlueprintPure, Category = "GAS")
	bool IsDead() const;

	/** Fires on the server the moment this character dies, before the ragdoll goes out. Anything
	 *  counting the living should listen here rather than for destruction, which a corpse can put
	 *  off for as long as it lies there. */
	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FSomnusCharacterDiedSignature OnDied;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Applies DefaultGameplayEffects and grants DefaultAbilities, once per body however many times
	 *  it is possessed. Server only. */
	void GrantDefaults(UAbilitySystemComponent* ASC);

	/** What this kind of character does on dying beyond what every character does. Runs on the
	 *  server once bDead is set, before its abilities are cancelled. */
	virtual void HandleServerDeath() {}

	/** Per-machine reaction once the body has gone limp - a death screen, say. The state of the
	 *  body itself belongs in ApplyDeathState, which also reaches a machine that missed this. */
	virtual void HandleDeathEvent(const FVector& HitDirection) {}

	/** Everything about being a corpse that has to hold on every machine for as long as the body
	 *  lasts. Idempotent on purpose: it arrives by replication, by multicast, or by both in either
	 *  order, and none of those know about the others. */
	virtual void ApplyDeathState();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	/** Physics-based flinch on non-lethal hits, and the one owner of the body's physics pose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USomnusHitReactComponent> HitReact;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitReact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicsControlComponent> PhysicsControl;

	/** How hard the killing blow shoves the ragdoll. */
	UPROPERTY(EditDefaultsOnly, Category = "GAS", meta = (ClampMin = "0"))
	float DeathImpulseStrength = 1500.f;

	/** Set on death and never cleared. Asked ahead of the ability system because a body can outlive
	 *  its access to one - an unpossessed player corpse has no PlayerState left to ask. */
	UPROPERTY(ReplicatedUsing = OnRep_Dead, VisibleInstanceOnly, BlueprintReadOnly, Category = "GAS")
	bool bDead = false;

	UFUNCTION()
	void OnRep_Dead();

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeath(const FVector& HitDirection);

	bool bDefaultsGranted = false;
};
