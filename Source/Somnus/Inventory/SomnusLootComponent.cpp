// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusLootComponent.h"

#include "SomnusContainerActor.h"
#include "SomnusInventoryComponent.h"
#include "Net/UnrealNetwork.h"

USomnusLootComponent::USomnusLootComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// LootTarget is the whole point of this component and it lives on the client's screen, so
	// without this nothing downstream of it exists on the client at all.
	SetIsReplicatedByDefault(true);
}

void USomnusLootComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(USomnusLootComponent, LootTarget, COND_OwnerOnly);
}

void USomnusLootComponent::OpenLoot(AActor* Body)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!Body) return;
	
	FVector RootLocation = GetOwner()->GetActorLocation();
	FVector BodyLocation = Body->GetActorLocation();
	
	if (FVector::DistSquared(RootLocation, BodyLocation) < FMath::Square(LootRange))
	{
		LootTarget = Body;
		OnRep_LootTarget();
	}
}

bool USomnusLootComponent::CanAccessContainer(const class USomnusInventoryComponent* Container) const
{
	if (!Container) return false;
	AActor* RootHolder = ASomnusContainerActor::ResolveRootHolder(Container->GetOwner());
	if (!RootHolder) return false;
	
	if (RootHolder == GetOwner())
	{
		return true;
	}
	if (RootHolder == LootTarget)
	{
		FVector RootLocation = GetOwner()->GetActorLocation();
		FVector BodyLocation = RootHolder->GetActorLocation();
	
		if (FVector::DistSquared(RootLocation, BodyLocation) < FMath::Square(LootRange))
		{
			return true;
		}
	}
	return false;
}

void USomnusLootComponent::Server_CloseLoot_Implementation()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	LootTarget = nullptr;
	OnRep_LootTarget();
}

void USomnusLootComponent::OnRep_LootTarget()
{
	// Called by hand from the two server-side writers as well, because OnRep never fires on the
	// authority - a listen server host would otherwise be the one player whose panel never opens.
	OnLootTargetChanged.Broadcast();
}


