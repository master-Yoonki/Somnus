// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Core/SomnusStrikeSource.h"
#include "SomnusAT_TrackStrikeVelocity.generated.h"


UCLASS()
class SOMNUS_API USomnusAT_TrackStrikeVelocity : public UAbilityTask
{
	GENERATED_BODY()
public:
	USomnusAT_TrackStrikeVelocity(const FObjectInitializer& ObjectInitializer);
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	
	// Resolves the strike source nearest the contact point, returning its interpolated
	// contact velocity and mass. Both are zeroed when no usable source is found.
	void GetVelocityAtContactPoint(FVector ImpactPoint, FVector& OutVelocity, float& OutWeight) const;
	
	// Velocity index is same as StrikeSourceInfo
	const TArray<TArray<FVector>>& GetVelocities() const { return Velocities; }
	
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
	meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static USomnusAT_TrackStrikeVelocity* TrackStrikeVelocity(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		const TArray<FSomnusStrikeSourceInfo>& StrikeSourceInfos );
	
protected:
	UPROPERTY()
	TArray<FSomnusStrikeSourceInfo> CachedStrikeSourceInfos;
	
private:
	TArray<TArray<FVector>> PreviousLocations;
	TArray<TArray<FVector>> Velocities;
};
