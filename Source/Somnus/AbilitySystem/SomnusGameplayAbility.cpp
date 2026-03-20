// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SomnusGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "AbilitySystem/Effects/SomnusGE_StaminaCost.h"
#include "Core/SomnusGameplayTags.h"

USomnusGameplayAbility::USomnusGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Default cost GE — abilities with StaminaCost > 0 will use this.
	// Can be swapped in Blueprint for abilities that need different cost logic.
	CostGameplayEffectClass = USomnusGE_StaminaCost::StaticClass();
}

bool USomnusGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// Skip the default Super::CheckCost — it uses CanApplyAttributeModifiers which
	// doesn't resolve SetByCaller magnitudes. We handle cost checking manually.

	if (StaminaCost > 0.0f && GetCostGameplayEffect())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (!ASC)
		{
			return false;
		}

		bool bFound = false;
		float CurrentStamina = ASC->GetGameplayAttributeValue(USomnusAttributeSet::GetStaminaAttribute(), bFound);
		if (!bFound || CurrentStamina < StaminaCost)
		{
			return false;
		}
	}

	return true;
}

void USomnusGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// Don't call Super — it would apply the cost GE without SetByCaller magnitudes.
	// We apply it ourselves with the correct magnitude.

	UGameplayEffect* CostGE = GetCostGameplayEffect();
	if (StaminaCost > 0.0f && CostGE)
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (!ASC)
		{
			return;
		}

		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CostGE->GetClass(), GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(SomnusTags::Data_StaminaCost, -StaminaCost);
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}
