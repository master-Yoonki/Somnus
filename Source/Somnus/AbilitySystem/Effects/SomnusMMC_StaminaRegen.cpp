// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effects/SomnusMMC_StaminaRegen.h"
#include "AbilitySystem/Attributes/SomnusAttributeSet.h"

USomnusMMC_StaminaRegen::USomnusMMC_StaminaRegen()
{
	StaminaRegenRateDef.AttributeToCapture = USomnusAttributeSet::GetStaminaRegenRateAttribute();
	StaminaRegenRateDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
	StaminaRegenRateDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(StaminaRegenRateDef);
}

float USomnusMMC_StaminaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;

	float RegenRate = 0.0f;
	GetCapturedAttributeMagnitude(StaminaRegenRateDef, Spec, EvalParams, RegenRate);

	return RegenRate;
}
