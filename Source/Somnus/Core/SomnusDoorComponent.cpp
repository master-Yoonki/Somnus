// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SomnusDoorComponent.h"

#include "Net/UnrealNetwork.h"

USomnusDoorComponent::USomnusDoorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USomnusDoorComponent::BeginPlay()
{
	Super::BeginPlay();

	// A component only replicates through an actor that does, and the doors this is dropped onto
	// were authored as scenery. Silently failing would look like a one-sided door in a playtest
	// and cost an hour, so it is said here instead, once, by name.
	const AActor* Owner = GetOwner();
	if (Owner && !Owner->GetIsReplicated())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s has a door component but does not replicate - it will only open on the machine "
			     "that interacted with it. Tick Replicates in its class defaults."),
			*Owner->GetName());
	}
}

void USomnusDoorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USomnusDoorComponent, bOpen);
}

void USomnusDoorComponent::Toggle()
{
	SetOpen(!bOpen);
}

void USomnusDoorComponent::SetOpen(bool bNewOpen)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bOpen == bNewOpen)
	{
		return;
	}

	bOpen = bNewOpen;

	// Replication notifies the other machines; the server is not one of them and has to be told
	// separately, or the door opens everywhere except where it was opened.
	OnRep_Open();
}

void USomnusDoorComponent::OnRep_Open()
{
	bHasEverChanged = true;
	OnDoorStateChanged.Broadcast(bOpen);
}
