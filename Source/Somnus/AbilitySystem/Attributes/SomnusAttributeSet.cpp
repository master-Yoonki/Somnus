// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/SomnusAttributeSet.h"

#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "Net/UnrealNetwork.h"

USomnusAttributeSet::USomnusAttributeSet()
{
	// Initialize default values (Can be overridden later by Gameplay Effects)
	InitHealth(500.0f);
	InitMaxHealth(500.0f);
	InitStamina(100.0f);
	InitMaxStamina(100.0f);
	InitStaminaRegenRate(1.0f);
}

void USomnusAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Register attributes for replication to all clients
	DOREPLIFETIME_CONDITION_NOTIFY(USomnusAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USomnusAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USomnusAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USomnusAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USomnusAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
}

void USomnusAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
		AdjustAttributeForMaxChange(Stamina, MaxStamina, NewValue, GetStaminaAttribute());
	}
	else if (Attribute == GetStaminaRegenRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void USomnusAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float DamageDone = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		if (DamageDone > 0.0f)
		{
			const float NewHealth = FMath::Clamp(GetHealth() - DamageDone, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);

			if (NewHealth <= 0.0f)
			{
				if (ASomnusCharacter* Character = Cast<ASomnusCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor()))
				{
					// Extract hit direction from the damage effect context
					FVector HitDirection = FVector::ZeroVector;
					if (const FHitResult* HitResult = Data.EffectSpec.GetEffectContext().GetHitResult())
					{
						// TraceEnd - TraceStart gives the direction the trace was traveling
						HitDirection = (HitResult->TraceEnd - HitResult->TraceStart).GetSafeNormal();
					}
					Character->Die(HitDirection);
				}
			}
			else
			{
				// Only physical hits flinch — periodic/environmental damage carries no HitResult and shouldn't stagger
				const FGameplayEffectContextHandle Context = Data.EffectSpec.GetEffectContext();

				
				if (Context.GetHitResult())
				{
					FGameplayEventData GameplayEventData;
					GameplayEventData.EventTag = SomnusTags::Event_HitReact;
					GameplayEventData.Instigator = Context.GetInstigator();
					GameplayEventData.ContextHandle = Context;
					GameplayEventData.EventMagnitude = DamageDone;
					UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
					ASC->HandleGameplayEvent(SomnusTags::Event_HitReact, &GameplayEventData);
				}
			}
		}
	}
}

void USomnusAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// Periodic effects modify the base value directly, so clamp here too or they can
	// accumulate past the cap.
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
}

void USomnusAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		OnHealthChanged.Broadcast(NewValue, GetMaxHealth());
	}
    
	if (Attribute == GetStaminaAttribute())
	{
		OnStaminaChanged.Broadcast(NewValue, GetMaxStamina());
	}
}

void USomnusAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USomnusAttributeSet, Health, OldHealth);
	OnHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
}

void USomnusAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USomnusAttributeSet, MaxHealth, OldMaxHealth);
}

void USomnusAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USomnusAttributeSet, Stamina, OldStamina);
	OnStaminaChanged.Broadcast(GetStamina(), GetMaxStamina());
}

void USomnusAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USomnusAttributeSet, MaxStamina, OldMaxStamina);
}

void USomnusAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USomnusAttributeSet, StaminaRegenRate, OldStaminaRegenRate);
}

void USomnusAttributeSet::AdjustAttributeForMaxChange(const FGameplayAttributeData& AffectedAttribute,
	const FGameplayAttributeData& MaxAttribute, float NewMaxValue,
	const FGameplayAttribute& AffectedAttributeProperty) const
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && ASC)
	{
		// Scale the current value proportionally to the new max.
		// e.g., 50/100 HP (50%) -> 75/150 HP (still 50%) when MaxHealth goes 100->150.
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		const float NewDelta = (CurrentMaxValue > 0.0f)
			? (NewMaxValue - CurrentMaxValue) * (CurrentValue / CurrentMaxValue)
			: NewMaxValue;
		ASC->ApplyModToAttribute(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}		