// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/SomnusInteractorComponent.h"

#include "SomnusCollisionChannels.h"
#include "SomnusInteractable.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

USomnusInteractorComponent::USomnusInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool USomnusInteractorComponent::TraceForInteractable(FHitResult& OutHit) const
{
	AActor* OwnerActor = GetOwner();
	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn)
	{
		return false;
	}

	// Control rotation replicates, so the server can aim this itself and owes the client no
	// trust at all. The few frames it lags behind cost nothing at this range.
	const FVector TraceStart = OwnerPawn->GetPawnViewLocation();
	const FVector TraceEnd = TraceStart + OwnerPawn->GetControlRotation().Vector() * TraceLength;

	// Its own body joins the interaction channel the moment it dies, and a corpse cannot search
	// itself: Interact refuses an Interactor that is the target. Highlighting one from inside it
	// would promise something the button then declines to do.
	const TArray<AActor*> ActorsToIgnore{ OwnerActor };

	return UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(), TraceStart, TraceEnd, TraceRadius,
		UEngineTypes::ConvertToTraceType(SomnusCollision::Interaction),
		false, ActorsToIgnore, EDrawDebugTrace::None,
		OutHit, true);
}

void USomnusInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// What one player is looking at is not a fact the world needs to agree on, so this never
	// leaves the machine it runs on. Checked per tick rather than latched in BeginPlay because a
	// client learns its controller by replication, which lands after the pawn has begun play.
	//
	// The cast is what actually asks the question: AController::IsLocalController() is
	// unconditionally true in standalone (Controller.cpp:90-94), so an AI-possessed body would
	// otherwise light up loot for whoever happens to share the process.
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	AActor* NewFocus = nullptr;
	if (FHitResult Hit; TraceForInteractable(Hit))
	{
		// The channel being interaction-only makes a hit on something else unlikely rather than
		// impossible, and Execute_SetHighlighted asserts rather than asks.
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->Implements<USomnusInteractable>())
		{
			NewFocus = HitActor;
		}
	}

	SetFocus(NewFocus);
}

void USomnusInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The highlight lives on the other actor, which outlives this component - nothing else would
	// ever turn it off.
	SetFocus(nullptr);

	Super::EndPlay(EndPlayReason);
}

void USomnusInteractorComponent::SetFocus(AActor* NewFocus)
{
	AActor* OldFocus = FocusedActor.Get();
	if (OldFocus == NewFocus)
	{
		return;
	}

	// Off before on. They are different actors here by definition, so the order only matters to
	// whatever counts highlights - but it costs nothing to be the obvious way round.
	if (OldFocus)
	{
		ISomnusInteractable::Execute_SetHighlighted(OldFocus, false);
	}
	if (NewFocus)
	{
		ISomnusInteractable::Execute_SetHighlighted(NewFocus, true);
	}

	FocusedActor = NewFocus;
}
