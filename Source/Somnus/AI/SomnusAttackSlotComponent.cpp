// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/SomnusAttackSlotComponent.h"

#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "NavigationSystem.h"
#include "Character/SomnusCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/SomnusGameplayTags.h"

USomnusAttackSlotComponent::USomnusAttackSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	// Follows the owner's position but never its turning, so offsets in this component's space are
	// fixed against the world: a body looking around does not swing the ring with it.
	SetUsingAbsoluteRotation(true);

	SetIsReplicatedByDefault(true);
}

void USomnusAttackSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USomnusAttackSlotComponent, ActiveSlots);
}

void USomnusAttackSlotComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->HasAuthority())
	{
		UpdateSlots();
	}
}

void USomnusAttackSlotComponent::BuildSlots()
{
	ActiveSlots.Reset();
	ActiveSlots.AddDefaulted(NumSlots);

	const float StepAngleInRadian = TWO_PI / NumSlots;
	for (int i = 0; i < NumSlots; i++)
	{
		const float AngleInRadian = i * StepAngleInRadian;
		ActiveSlots[i].LocationOffset =
			FVector(FMath::Cos(AngleInRadian) * Range, FMath::Sin(AngleInRadian) * Range, 0.f);
	}
}

void USomnusAttackSlotComponent::UpdateSlots()
{
	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return;

	// The ray has to start on the navmesh, and the actor's location is the capsule centre, well above it.
	// No navmesh under the player (mid-jump, off the mesh) means no ring this update; the last one stands.
	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	FNavLocation PlayerNav;
	if (!NavSys->ProjectPointToNavigation(OwnerLocation, PlayerNav, ProjectionExtent)) return;

	// The navmesh stops short of walls by the agent radius, so a player hugging one stands off it and
	// projects onto its very edge. A navmesh ray starting on that edge can read as blocked at once in
	// every direction, so step the start a little further in, the way the projection already moved it.
	const FVector ToMesh = PlayerNav.Location - OwnerLocation;
	if (ToMesh.SizeSquared2D() > FMath::Square(1.f))
	{
		const FVector Inset = PlayerNav.Location + ToMesh.GetSafeNormal2D() * 10.f;
		NavSys->ProjectPointToNavigation(Inset, PlayerNav, ProjectionExtent);
	}

	for (FAttackSlot& Slot : ActiveSlots)
	{
		AActor* AssignedActor = Slot.AssignedActor.Get();
		if (ASomnusCharacterBase* AssignedCharacter = Cast<ASomnusCharacterBase>(AssignedActor))
		{
			if (AssignedCharacter->IsDead())
			{
				Slot.AssignedActor.Reset();
			}
		}
		Slot.bIsValid = ResolveSlotLocation(*NavSys, PlayerNav.Location, Slot.LocationOffset, Slot.ProjectedLocation);
	}
}

bool USomnusAttackSlotComponent::ResolveSlotLocation(const UNavigationSystemV1& NavSys,
	const FVector& PlayerNavLocation, const FVector& Offset, FVector& OutLocation) const
{
	const FVector Desired = PlayerNavLocation + Offset;

	// A wall, pillar or ledge in the way pulls the slot in to where the navmesh ends. The raycast
	// leaves the end point in place when nothing is hit.
	FVector Candidate = Desired;
	UNavigationSystemV1::NavigationRaycast(GetWorld(), PlayerNavLocation, Desired, Candidate);

	// Pulled in so far that a zombie would not fit between the player and the obstacle.
	if (FVector::Dist2D(PlayerNavLocation, Candidate) < MinSlotRange) return false;

	// The candidate still sits at the player's height; settle it onto the floor it is over, which
	// differs on a slope.
	FNavLocation Projected;
	if (!NavSys.ProjectPointToNavigation(Candidate, Projected, ProjectionExtent)) return false;

	OutLocation = Projected.Location;
	return true;
}

int32 USomnusAttackSlotComponent::RequestSlot(AActor* Zombie)
{
	if (!Zombie) return INDEX_NONE;

	const FVector ZombieLocation = Zombie->GetActorLocation();

	// One pass records both what the zombie holds and the nearest slot it could take instead; every
	// decision waits until the whole ring has been seen.
	int32 HeldIndex = INDEX_NONE;
	int32 NearestFreeIndex = INDEX_NONE;
	float NearestFreeDistSq = TNumericLimits<float>::Max();

	for (int32 i = 0; i < ActiveSlots.Num(); i++)
	{
		const FAttackSlot& Slot = ActiveSlots[i];

		if (Slot.AssignedActor.Get() == Zombie)
		{
			HeldIndex = i;
			continue;
		}

		if (Slot.bIsValid && !Slot.AssignedActor.IsValid())
		{
			// Squared is enough to rank by, and skips the square root.
			const float DistSq = FVector::DistSquared2D(ZombieLocation, Slot.ProjectedLocation);
			if (DistSq < NearestFreeDistSq)
			{
				NearestFreeDistSq = DistSq;
				NearestFreeIndex = i;
			}
		}
	}

	// A held slot that has gone invalid is dropped, and the zombie falls through to taking a fresh
	// one in the same call rather than going a whole update with nowhere to stand.
	if (HeldIndex != INDEX_NONE && !ActiveSlots[HeldIndex].bIsValid)
	{
		ActiveSlots[HeldIndex].AssignedActor.Reset();
		HeldIndex = INDEX_NONE;
	}

	if (HeldIndex != INDEX_NONE)
	{
		// Mid-swing the slot is locked: moving it now would drag the body off the strike it is
		// already committed to.
		if (IsAttacking(Zombie))
		{
			return HeldIndex;
		}

		// Only move over for a slot clearly nearer than the one held. The margin has to come off a
		// real distance, so this is where the square roots are taken.
		const float HeldDist = FVector::Dist2D(ZombieLocation, ActiveSlots[HeldIndex].ProjectedLocation);
		if (NearestFreeIndex == INDEX_NONE || FMath::Sqrt(NearestFreeDistSq) >= HeldDist - SwitchMargin)
		{
			return HeldIndex;
		}
		ActiveSlots[HeldIndex].AssignedActor.Reset();
	}

	if (NearestFreeIndex != INDEX_NONE)
	{
		ActiveSlots[NearestFreeIndex].AssignedActor = Zombie;
	}
	return NearestFreeIndex;
}

bool USomnusAttackSlotComponent::IsAttacking(const AActor* Zombie)
{
	const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(Zombie);
	const UAbilitySystemComponent* ASC = AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
	return ASC && ASC->HasMatchingGameplayTag(SomnusTags::State_Attacking);
}

int32 USomnusAttackSlotComponent::FindSlotOf(const AActor* Zombie) const
{
	if (Zombie == nullptr) return INDEX_NONE;
	for (int i = 0; i < ActiveSlots.Num(); i++)
	{
		const FAttackSlot& Slot = ActiveSlots[i];
		
		if (Slot.AssignedActor.Get() == Zombie)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void USomnusAttackSlotComponent::ReleaseSlot(AActor* Zombie)
{
	for (FAttackSlot& Slot : ActiveSlots)
	{
		if (Slot.AssignedActor.Get() == Zombie)
		{
			Slot.AssignedActor.Reset();
			return;
		}
	}
}

bool USomnusAttackSlotComponent::GetSlotLocation(int32 SlotIndex, FVector& OutLocation) const
{
	if (ActiveSlots.IsValidIndex(SlotIndex))
	{
		if (ActiveSlots[SlotIndex].bIsValid)
		{
			OutLocation = ActiveSlots[SlotIndex].ProjectedLocation;
			return true;
		}
	}
	return false;
}

bool USomnusAttackSlotComponent::IsValidKey(int32 SlotIndex) const
{
	return ActiveSlots.IsValidIndex(SlotIndex);
}

void USomnusAttackSlotComponent::DebugDraw()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;
	const FVector OwnerLocation = Owner->GetActorLocation();
	// Bounded by the array rather than NumSlots, so a slot count edited after the slots were built
	// cannot read past the end.
	for (int i = 0; i < ActiveSlots.Num(); i++)
	{
		FAttackSlot& Slot = ActiveSlots[i];
		// A valid slot shows where it landed on the navmesh; an invalid one has no such point, so it
		// stays where the ring wants it.
		const FVector SlotLocation = Slot.bIsValid ? Slot.ProjectedLocation : OwnerLocation + Slot.LocationOffset;

		const AActor* Occupant = Slot.AssignedActor.Get();
		const FColor SlotColor = !Slot.bIsValid ? FColor::Red : Occupant ? FColor::Green : FColor::Yellow;
		// One-frame lifetime: any positive lifetime goes to the persistent batcher, which ticks after us
		// and would expire a DeltaTime-long line before it is ever rendered.
		DrawDebugSphere(GetWorld(), SlotLocation, 20.f, 8, SlotColor, false, -1.f);

		if (Occupant)
		{
			// Zero duration still shows for one frame: the HUD draws text before it counts the time down.
			// Here -1 would mean forever, the opposite of the lines above.
			DrawDebugString(GetWorld(), SlotLocation + FVector(0.f, 0.f, 30.f), Occupant->GetName(),
				nullptr, SlotColor, 0.f, false, 0.8f);
		}
	}
}

void USomnusAttackSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	// The server owns the ring; clients receive it through replication.
	if (GetOwner()->HasAuthority())
	{
		BuildSlots();
	}
}
