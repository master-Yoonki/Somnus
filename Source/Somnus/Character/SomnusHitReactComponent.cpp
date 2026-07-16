// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SomnusHitReactComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

USomnusHitReactComponent::USomnusHitReactComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Enabled only while a reaction is recovering; see TickComponent.
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Without this the actor never adds the component to its replicated list and the
	// multicast below is dropped on the server with no error.
	SetIsReplicatedByDefault(true);

	// Local simulation drives bodies toward their parent-relative animated transform, which is what
    // a flinch wants: the limb returns to the pose regardless of where the character has walked to.
	PhysicalAnimationSettings.bIsLocalSimulation = true;
	PhysicalAnimationSettings.OrientationStrength = 1000.0f;
	PhysicalAnimationSettings.AngularVelocityStrength = 100.0f;
	PhysicalAnimationSettings.MaxAngularForce = 0.0f;
}

void USomnusHitReactComponent::BeginPlay()
{
	Super::BeginPlay();

	// ACharacter::GetMesh() rather than FindComponentByClass: the owner may carry other
	// skeletal meshes (weapons, attachments) and component order is not guaranteed.
	if (const ACharacter* OwningCharacter = Cast<ACharacter>(GetOwner()))
	{
		CachedSkeletalMeshComponent = OwningCharacter->GetMesh();
	}

	if (!ensureMsgf(CachedSkeletalMeshComponent != nullptr,
		TEXT("%s requires an ACharacter owner with a mesh. Hit reactions are disabled on %s."),
		*GetClass()->GetName(), *GetNameSafe(GetOwner())))
	{
		return;
	}
	
	if (!ensureMsgf(
		CachedSkeletalMeshComponent->GetBoneIndex(SimulationRootBone) != INDEX_NONE, 
		TEXT("%s bone does not exist. Hit reactions are disabled on %s."), 
		*SimulationRootBone.ToString(),  *GetNameSafe(GetOwner())))
	{
		return;
	}

	// Spawned here rather than as a default subobject: a UActorComponent cannot register another
	// actor component from its own constructor, and both characters would otherwise have to
	// declare this one themselves.
	PhysicalAnimation = NewObject<UPhysicalAnimationComponent>(GetOwner());
	PhysicalAnimation->RegisterComponent();
	PhysicalAnimation->SetSkeletalMeshComponent(CachedSkeletalMeshComponent);
}

void USomnusHitReactComponent::TriggerHitReact(const FHitResult& HitResult, const FVector& Impulse)
{
	// The driving ability is ServerInitiated, so reaching here as a client is a wiring bug:
	// the multicast would run locally only and silently desync.
	if (!ensure(GetOwnerRole() == ROLE_Authority))
	{
		return;
	}

	MulticastHitReact(HitResult.BoneName, HitResult.ImpactPoint, Impulse);
}

void USomnusHitReactComponent::MulticastHitReact_Implementation(FName HitBone, FVector_NetQuantize ImpactPoint, FVector_NetQuantize Impulse)
{
	if (!CachedSkeletalMeshComponent || !PhysicalAnimation)
	{
		return;
	}

	// A hit outside the simulated subtree (legs, mostly) cannot flinch anything, so it must not
	// start or extend a reaction either. Tested against the reference skeleton, not the bodies'
	// simulate flags: those read false until BeginReaction runs, which would reject the first hit.
	// The two terms mirror UPhysicsAsset::GetBodyIndicesBelow, so this matches SetAllBodiesBelow.
	if (HitBone != SimulationRootBone && !CachedSkeletalMeshComponent->BoneIsChildOf(HitBone, SimulationRootBone))
	{
		return;
	}

	if (!bIsReacting)
	{
		BeginReaction();
	}

	CachedSkeletalMeshComponent->AddImpulseAtLocation(Impulse, ImpactPoint, HitBone);

	// Repeat hits extend the reaction rather than starting a competing one. The impulses simply sum
	// in the solver, so there is no per-hit state to reconcile.
	RecoveryElapsed = 0.0f;
}

void USomnusHitReactComponent::BeginReaction()
{
	CachedSkeletalMeshComponent->SetAllBodiesBelowSimulatePhysics(SimulationRootBone, true, /*bIncludeSelf=*/true);
	CachedSkeletalMeshComponent->SetAllBodiesBelowPhysicsBlendWeight(SimulationRootBone, MaxBlendWeight);
	PhysicalAnimation->ApplyPhysicalAnimationSettingsBelow(SimulationRootBone, PhysicalAnimationSettings, /*bIncludeSelf=*/true);

	bIsReacting = true;
	SetComponentTickEnabled(true);
}

void USomnusHitReactComponent::StopHitReact()
{
	if (!bIsReacting)
	{
		return;
	}

	bIsReacting = false;
	SetComponentTickEnabled(false);
	RecoveryElapsed = 0.0f;

	if (CachedSkeletalMeshComponent)
	{
		CachedSkeletalMeshComponent->SetAllBodiesBelowPhysicsBlendWeight(SimulationRootBone, 0.0f);
		CachedSkeletalMeshComponent->SetAllBodiesBelowSimulatePhysics(SimulationRootBone, false, /*bIncludeSelf=*/true);
	}
}

void USomnusHitReactComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RecoveryElapsed += DeltaTime;

	if (RecoveryElapsed >= RecoveryTime)
	{
		StopHitReact();
		return;
	}

	const float BlendOutStart = RecoveryTime - BlendOutTime;
	if (RecoveryElapsed > BlendOutStart && BlendOutTime > 0.0f)
	{
		const float Alpha = (RecoveryElapsed - BlendOutStart) / BlendOutTime;
		CachedSkeletalMeshComponent->SetAllBodiesBelowPhysicsBlendWeight(
			SimulationRootBone, FMath::Lerp(MaxBlendWeight, 0.0f, Alpha));
	}
}
