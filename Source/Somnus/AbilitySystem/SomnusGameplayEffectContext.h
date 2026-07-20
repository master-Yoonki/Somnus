// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "SomnusGameplayEffectContext.generated.h"

/**
 * Project effect context. Carries the strike impulse from the attacker's melee ability
 * to the victim's hit-react ability, since the base FGameplayEffectContext has no vector slot.
 * Requires USomnusAbilitySystemGlobals to be registered so this type is allocated.
 */
USTRUCT()
struct SOMNUS_API FSomnusGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	FVector GetHitImpulse() const { return HitImpulse; }
	void SetHitImpulse(const FVector& InImpulse) { HitImpulse = InImpulse; }

	/** Must return this struct's type so the network layer serializes the right thing. */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FSomnusGameplayEffectContext::StaticStruct();
	}

	/** Deep-copies this context (the ASC duplicates contexts before applying effects). */
	virtual FGameplayEffectContext* Duplicate() const override
	{
		FSomnusGameplayEffectContext* NewContext = new FSomnusGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Deep copy the hit result rather than sharing the pointer.
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

protected:
	UPROPERTY()
	FVector_NetQuantize HitImpulse = FVector::ZeroVector;
};

template<>
struct TStructOpsTypeTraits<FSomnusGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FSomnusGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
