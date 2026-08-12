// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusLootComponent.h"

#include "SomnusContainerActor.h"
#include "SomnusContainerEquipComponent.h"
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
	if (!IsWithinReach(Body)) return;

	LootTarget = Body;
	OnRep_LootTarget();
}

bool USomnusLootComponent::IsWithinReach(const AActor* Body) const
{
	if (!Body || !GetOwner()) return false;

	return FVector::DistSquared(GetOwner()->GetActorLocation(), Body->GetActorLocation())
		< FMath::Square(LootRange);
}

bool USomnusLootComponent::CanAccessHolder(const AActor* Holder) const
{
	if (!Holder || !GetOwner()) return false;

	if (Holder == GetOwner()) return true;
	if (Holder == LootTarget) return IsWithinReach(Holder);

	return false;
}

bool USomnusLootComponent::CanAccessContainer(const class USomnusInventoryComponent* Container) const
{
	// A grid answers for whoever is ultimately holding it, which is the only level the question
	// has an answer at - containers nest, and each link owns the next.
	return Container && CanAccessHolder(ASomnusContainerActor::ResolveRootHolder(Container->GetOwner()));
}

void USomnusLootComponent::Server_MoveItem_Implementation(USomnusInventoryComponent* Source,
	USomnusInventoryComponent* Dest, FGuid InstanceID, int32 TopLeftX, int32 TopLeftY, bool bRotated)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!Source || !Dest) return;
	
	if (!CanAccessContainer(Source)) return;
	if (!CanAccessContainer(Dest)) return;
	
	Dest->MoveItemFrom(Source, InstanceID, TopLeftX, TopLeftY, bRotated);
}

void USomnusLootComponent::Server_CloseLoot_Implementation()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	LootTarget = nullptr;
	OnRep_LootTarget();
}

void USomnusLootComponent::Server_DropFrom_Implementation(USomnusContainerEquipComponent* Source,
	FGuid InstanceID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!Source || !Source->GetOwner()) return;
	if (!CanAccessHolder(Source->GetOwner())) return;
	
	Source->DropItem(InstanceID);
}

void USomnusLootComponent::OnRep_LootTarget()
{
	// Called by hand from the two server-side writers as well, because OnRep never fires on the
	// authority - a listen server host would otherwise be the one player whose panel never opens.
	OnLootTargetChanged.Broadcast();
}


