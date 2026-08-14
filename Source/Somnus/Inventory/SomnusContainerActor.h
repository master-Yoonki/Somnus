// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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

	/** Collapses a chain of container actors down to whatever is holding it - a character, a
	 *  pickup. Containers nest (a backpack inside a rig), and each link owns the next, so the
	 *  question "whose is this" only has an answer at the root. Stops at the first link with no
	 *  owner, which is still correct: that link becomes the root and the chain repairs itself
	 *  once it gets one. */
	static AActor* ResolveRootHolder(AActor* Actor);
	
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
	
	/** Copied from the data asset at Initialize and handed to every compartment. Not editable:
	 *  these actors are only ever spawned for an item instance, so a value set in the editor
	 *  would never be the one in use. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tag")
	FGameplayTagContainer AcceptedTags;

	UPROPERTY(BlueprintReadOnly, Replicated)
	TArray<TObjectPtr<USomnusInventoryComponent>> Compartments;
};
