// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/SomnusAttributeSet.h"
#include "Net/UnrealNetwork.h"

USomnusAttributeSet::USomnusAttributeSet()
{
	// Initialize default values (Can be overridden later by Gameplay Effects)
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
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
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
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
			OnHealthChanged.Broadcast(NewHealth, GetMaxHealth());
		}
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