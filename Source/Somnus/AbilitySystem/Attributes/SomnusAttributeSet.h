// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "SomnusAttributeSet.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangedDelegate, float /*CurrentValue*/, float /*MaxValue*/);

// Macro for automatically generating getters, setters, and initters for attributes
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Core attributes for Project Somnus (Health, Stamina, etc.)
 */
UCLASS()
class SOMNUS_API USomnusAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    USomnusAttributeSet();
    
    FOnAttributeChangedDelegate OnHealthChanged;
    FOnAttributeChangedDelegate OnStaminaChanged;

    // Required for network replication (Multiplayer support)
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    // Clamp attribute values before they are applied
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

    // Process damage after GE execution: reads IncomingDamage, subtracts from Health, then zeroes it out
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
    
public:
    // Current Health
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(USomnusAttributeSet, Health)

    // Maximum Health
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(USomnusAttributeSet, MaxHealth)

    // Current Stamina
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina", ReplicatedUsing = OnRep_Stamina)
    FGameplayAttributeData Stamina;
    ATTRIBUTE_ACCESSORS(USomnusAttributeSet, Stamina)

    // Maximum Stamina
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina", ReplicatedUsing = OnRep_MaxStamina)
    FGameplayAttributeData MaxStamina;
    ATTRIBUTE_ACCESSORS(USomnusAttributeSet, MaxStamina)

    // Stamina regenerated per tick (used as attribute-based magnitude by the regen GE)
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina", ReplicatedUsing = OnRep_StaminaRegenRate)
    FGameplayAttributeData StaminaRegenRate;
    ATTRIBUTE_ACCESSORS(USomnusAttributeSet, StaminaRegenRate)

    // Meta attribute — temporary damage bucket, not replicated.
    // GE sets this value; PostGameplayEffectExecute subtracts it from Health.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Damage")
    FGameplayAttributeData IncomingDamage;
    ATTRIBUTE_ACCESSORS(USomnusAttributeSet, IncomingDamage)

protected:
    // Replication functions (Called on clients when values change on the server)
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

    UFUNCTION()
    virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

    UFUNCTION()
    virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

    UFUNCTION()
    virtual void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate);
};