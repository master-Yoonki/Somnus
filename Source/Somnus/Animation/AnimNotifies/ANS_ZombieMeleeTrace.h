// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ZombieMeleeTrace.generated.h"

USTRUCT(BlueprintType)
struct FSomnusZombieTraceConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FName SocketName;
	UPROPERTY(EditAnywhere)
	float RaceRadius = 10.f;
};
/**
 * 
 */
UCLASS()
class SOMNUS_API UANS_ZombieMeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Trace")
	TArray<FSomnusZombieTraceConfig> TraceConfigs;
	
private:
	// Track actors already hit during this swing to prevent duplicate hits
	UPROPERTY()
	TArray<TObjectPtr<AActor>> AlreadyHitActors;
};
