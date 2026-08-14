// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusContainerActor.h"

#include "SomnusInventoryComponent.h"
#include "Inventory/SomnusContainerDataAsset.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASomnusContainerActor::ASomnusContainerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	bAlwaysRelevant = true;
}

void ASomnusContainerActor::Initialize(USomnusContainerDataAsset* ContainerDataAsset)
{
	if (ContainerDataAsset == nullptr)
	{
		return;
	}
	
	AcceptedTags = ContainerDataAsset->AcceptedTags;
	
	const int32 NumCompartments = ContainerDataAsset->CompartmentSizes.Num();
	Compartments.SetNum(NumCompartments);
	for (int32 i = 0; i < NumCompartments; i++)
	{
		const FIntPoint CompartmentSize = ContainerDataAsset->CompartmentSizes[i];
		USomnusInventoryComponent* InventoryComponent 
			= NewObject<USomnusInventoryComponent>
				(this, *FString::Printf(TEXT("Compartments%d"), i));
		
		InventoryComponent->InitializeGridSize(CompartmentSize.X, CompartmentSize.Y);
		InventoryComponent->InitializeAcceptedItemTags(AcceptedTags);
		InventoryComponent->RegisterComponent();
		Compartments[i] = InventoryComponent;
	}
}

void ASomnusContainerActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASomnusContainerActor, Compartments);
}

AActor* ASomnusContainerActor::ResolveRootHolder(AActor* Actor)
{
	while (ASomnusContainerActor* AsContainer = Cast<ASomnusContainerActor>(Actor))
	{
		AActor* NextOwner = AsContainer->GetOwner();
		if (!NextOwner)
		{
			break;
		}
		Actor = NextOwner;
	}
	return Actor;
}

// Called when the game starts or when spawned
void ASomnusContainerActor::BeginPlay()
{
	Super::BeginPlay();
}

