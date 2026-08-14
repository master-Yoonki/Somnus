// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SomnusStrikeSource.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USomnusStrikeSource : public UInterface
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FSomnusStrikeSourceInfo
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UMeshComponent> ReferenceMeshComponent;
	
	// Index: 0 = base (or point), Index = 1 = tip (optional)
	UPROPERTY(EditDefaultsOnly)
	TArray<FName> SocketNames;
	
	// Weight of the mesh, used for calculating impulse when it hit
	UPROPERTY(EditDefaultsOnly)
	float Weight = 0.f;
	
	UPROPERTY(EditDefaultsOnly)
	float TraceRadius = 10.f;
};
/**
 * 
 */
class SOMNUS_API ISomnusStrikeSource
{
	GENERATED_BODY()
	
public:
	virtual TArray<FSomnusStrikeSourceInfo> GetStrikeSources() const = 0;
};
