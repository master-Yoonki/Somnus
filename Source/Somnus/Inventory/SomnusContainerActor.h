// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SomnusContainerActor.generated.h"

class USomnusInventoryComponent;

UCLASS()
class SOMNUS_API ASomnusContainerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASomnusContainerActor();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Container")
	void Initialize(class USomnusContainerDataAsset* ContainerDataAsset);
	
	UFUNCTION(BlueprintPure, Category = "Container")
	TArray<USomnusInventoryComponent*> GetCompartments() const
	{
		TArray<USomnusInventoryComponent*> Result;
		Result.Reserve(Compartments.Num());
		for (const TObjectPtr<USomnusInventoryComponent>& Compartment : Compartments)
		{
			Result.Add(Compartment);
		}
		return Result;
	}
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly, Replicated)
	TArray<TObjectPtr<USomnusInventoryComponent>> Compartments;
};
