// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SomnusHitReactComponent.h"

#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"

USomnusHitReactComponent::USomnusHitReactComponent()
{
	SetIsReplicated( true );
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	auto AddLimbSetup = [&](FName LimbName, FName StartBone, bool bIncludeParentBone)
	{
		FPhysicsControlLimbSetupData SetupData;
		SetupData.LimbName = LimbName;
		SetupData.StartBone = StartBone;
		SetupData.bIncludeParentBone = bIncludeParentBone;
    
		// These three boolean values are checked (true) for all elements in the provided images
		SetupData.bCreateWorldSpaceControls = true;
		SetupData.bCreateParentSpaceControls = true;
		SetupData.bCreateBodyModifiers = true;
    
		LimbSetupData.Add(SetupData);
	};

	AddLimbSetup(FName(TEXT("Head")), FName(TEXT("neck_01")), false);
	AddLimbSetup(FName(TEXT("ArmLeft")), FName(TEXT("clavicle_l")), false);
	AddLimbSetup(FName(TEXT("ArmRight")), FName(TEXT("clavicle_r")), false);
	AddLimbSetup(FName(TEXT("LegLeft")), FName(TEXT("thigh_l")), false);
	AddLimbSetup(FName(TEXT("LegRight")), FName(TEXT("thigh_r")), false);
	AddLimbSetup(FName(TEXT("Spine")), FName(TEXT("spine_02")), true);
	
	// 1. Initialize World Space Control Data (Spring and Damping parameters)
	WorldSpaceControlData.bEnabled = false;
	
	WorldSpaceControlData.LinearStrength = 3.0f;
	WorldSpaceControlData.LinearDampingRatio = 1.0f;
	WorldSpaceControlData.LinearExtraDamping = 0.0f;
	WorldSpaceControlData.MaxForce = 0.0f;

	WorldSpaceControlData.AngularStrength = 3.0f;
	WorldSpaceControlData.AngularDampingRatio = 1.0f;
	WorldSpaceControlData.AngularExtraDamping = 0.0f;
	WorldSpaceControlData.MaxTorque = 0.0f;

	WorldSpaceControlData.LinearTargetVelocityMultiplier = 1.0f;
	WorldSpaceControlData.AngularTargetVelocityMultiplier = 1.0f;

	WorldSpaceControlData.CustomControlPoint = FVector::ZeroVector; // Same as (0.0f, 0.0f, 0.0f)
	WorldSpaceControlData.bUseCustomControlPoint = false;
	
	WorldSpaceControlData.bUseCustomControlPoint = false;
	WorldSpaceControlData.bUseSkeletalAnimation = true;
	WorldSpaceControlData.bDisableCollision = false;         // Disable Collision: Unchecked
	WorldSpaceControlData.bOnlyControlChildObject = true;    // Only Control Child Object: Checked
	
	/////////////////
	
	ParentSpaceControlData.bEnabled = false; // Enabled: Unchecked

	ParentSpaceControlData.LinearStrength = 0.0f;
	ParentSpaceControlData.LinearDampingRatio = 1.0f;
	ParentSpaceControlData.LinearExtraDamping = 0.0f;
	ParentSpaceControlData.MaxForce = 0.0f;

	ParentSpaceControlData.AngularStrength = 10.0f;
	ParentSpaceControlData.AngularDampingRatio = 1.0f;
	ParentSpaceControlData.AngularExtraDamping = 0.0f;
	ParentSpaceControlData.MaxTorque = 0.0f;

	ParentSpaceControlData.LinearTargetVelocityMultiplier = 1.0f;
	ParentSpaceControlData.AngularTargetVelocityMultiplier = 1.0f;

	ParentSpaceControlData.CustomControlPoint = FVector::ZeroVector; 
	ParentSpaceControlData.bUseCustomControlPoint = false; // Unchecked

	ParentSpaceControlData.bUseSkeletalAnimation = true;      // Checked
	ParentSpaceControlData.bDisableCollision = false;         // Unchecked
	ParentSpaceControlData.bOnlyControlChildObject = false;   // Unchecked
	
	////////////////
	
	BodyModifierData.MovementType = EPhysicsMovementType::Kinematic;
	BodyModifierData.CollisionType = ECollisionEnabled::QueryAndPhysics;
	BodyModifierData.KinematicTargetSpace = EPhysicsControlKinematicTargetSpace::OffsetInBoneSpace;
	BodyModifierData.GravityMultiplier = 1.f;
	BodyModifierData.PhysicsBlendWeight = 1.f;
	BodyModifierData.bUpdateKinematicFromSimulation = true;
}

void USomnusHitReactComponent::BeginPlay()
{
	Super::BeginPlay();
	ACharacter* OwningCharacter = Cast<ACharacter>(GetOwner());
	if (OwningCharacter)
	{
		SkeletalMeshComponent = OwningCharacter->GetMesh();
		PhysicsControlComponent = OwningCharacter->GetComponentByClass<UPhysicsControlComponent>();
	}
	
	if (!ensureMsgf(SkeletalMeshComponent && PhysicsControlComponent,
		TEXT("USomnusHitReactComponent requires an ACharacter owner with a mesh and a UPhysicsControlComponent.")))
	{
		return;
	}

	// TODO : Consider apply this line when actually hit happen
	SkeletalMeshComponent->SetConstraintProfileForAll(ConstraintProfile);
	
	PhysicsControlComponent->CreateControlsAndBodyModifiersFromLimbBones(
		AllWorldSpaceControls,
		LimbWorldSpaceControls,
		AllParentSpaceControls,
		LimbParentSpaceControls,
		AllBodyModifiers,
		LimbBodyModifiers,
		SkeletalMeshComponent,
		LimbSetupData,
		WorldSpaceControlData,
		ParentSpaceControlData,
		BodyModifierData,
		nullptr,
		NAME_None);
	
	// Just to call AddBodyModifierToSet, but we are not gonna use output data, let it just set it internally
	FPhysicsControlNames DummyNewSet;
	// Make a Body Modifier set for the feet
	PhysicsControlComponent->AddBodyModifierToSet(DummyNewSet, FName("foot_l"), FName("Feet"));
	PhysicsControlComponent->AddBodyModifierToSet(DummyNewSet, FName("foot_r"), FName("Feet"));
	
	// Make a Body Modifier set for the Hands
	PhysicsControlComponent->AddBodyModifierToSet(DummyNewSet, FName("hand_l"), FName("Hand"));
	PhysicsControlComponent->AddBodyModifierToSet(DummyNewSet, FName("hand_r"), FName("Hand"));
	
	// Make a set of ParentSpace Controls for the arms
	PhysicsControlComponent->AddControlsToSet(DummyNewSet, 
		PhysicsControlComponent->GetControlNamesInSet(FName("ParentSpace_ArmLeft")), 
		FName("ParentSpace_Arms"));
	PhysicsControlComponent->AddControlsToSet(DummyNewSet, 
	PhysicsControlComponent->GetControlNamesInSet(FName("ParentSpace_ArmRight")), 
	FName("ParentSpace_Arms"));
	
	
	// Aggregate the individual upper-body WorldSpace controls into one set
	PhysicsControlComponent->AddControlsToSet(DummyNewSet,
		PhysicsControlComponent->GetControlNamesInSet(FName("WorldSpace_Spine")),
		FName("WorldSpace_UpperBody"));
	PhysicsControlComponent->AddControlsToSet(DummyNewSet,
		PhysicsControlComponent->GetControlNamesInSet(FName("WorldSpace_Head")),
		FName("WorldSpace_UpperBody"));
	PhysicsControlComponent->AddControlsToSet(DummyNewSet,
		PhysicsControlComponent->GetControlNamesInSet(FName("WorldSpace_ArmLeft")),
		FName("WorldSpace_UpperBody"));
	PhysicsControlComponent->AddControlsToSet(DummyNewSet,
		PhysicsControlComponent->GetControlNamesInSet(FName("WorldSpace_ArmRight")),
		FName("WorldSpace_UpperBody"));
	
	// WorldSpace controls for the feet
	PhysicsControlComponent->AddControlToSet(DummyNewSet, FName("foot_l"), FName("WorldSpace_Feet"));
	PhysicsControlComponent->AddControlToSet(DummyNewSet, FName("foot_r"), FName("WorldSpace_Feet"));
}

void USomnusHitReactComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TArray<FName> HitReactBoneToDelete;
	HitReactBoneToDelete.Reserve(BoneHitReactProcessingData.Num());

	// Track the freshest active hit (smallest elapsed time) across every bone and every
	// in-flight record on that bone; it drives the whole-body recovery.
	float MinProcessingTime = TNumericLimits<float>::Max();

	for (auto& Data : BoneHitReactProcessingData)
	{
		TArray<FBoneHitReactProcessingData>& Records = Data.Value;
		for (int32 Index = Records.Num() - 1; Index >= 0; --Index)
		{
			FBoneHitReactProcessingData& HitReactProcessingData = Records[Index];
			HitReactProcessingData.ProcessingTime += DeltaTime;
			if (HitReactProcessingData.ProcessingTime > RecoveryTime)
			{
				GetWorld()->GetTimerManager().ClearTimer(HitReactProcessingData.DelayTimer);
				Records.RemoveAt(Index);
			}
			else
			{
				MinProcessingTime = FMath::Min(MinProcessingTime, HitReactProcessingData.ProcessingTime);
			}
		}

		// Every in-flight hit on this bone has recovered - the bone itself is done.
		if (Records.Num() == 0)
		{
			HitReactBoneToDelete.Add(Data.Key);
		}
	}

	// Drive recovery once, from the freshest hit, across the global control sets.
	if (!BoneHitReactProcessingData.IsEmpty() && SkeletalKinematicCurveFloat && AnimToAnimCurveFloat)
	{
		const float Alpha = FMath::Clamp(MinProcessingTime / FMath::Max(RecoveryTime, KINDA_SMALL_NUMBER), 0.f, 1.f);
		const float SKStrengthMultiplier = SkeletalKinematicCurveFloat->GetFloatValue(Alpha);
		const float AAStrengthMultiplier = AnimToAnimCurveFloat->GetFloatValue(Alpha);

		FPhysicsControlMultiplier WorldSpacePhysicsControlMultiplier;
		WorldSpacePhysicsControlMultiplier.LinearStrengthMultiplier = FVector(SKStrengthMultiplier);
		WorldSpacePhysicsControlMultiplier.AngularStrengthMultiplier = SKStrengthMultiplier;
		// bEnableControl is set to false, it updates control status only if it is true, 
		// true => false not happening here, just updating values not the status
		PhysicsControlComponent->SetControlMultipliersInSet(FName("WorldSpace"), WorldSpacePhysicsControlMultiplier, false);

		FPhysicsControlMultiplier ParentSpacePhysicsControlMultiplier;
		ParentSpacePhysicsControlMultiplier.AngularStrengthMultiplier = AAStrengthMultiplier;
		PhysicsControlComponent->SetControlMultipliersInSet(FName("ParentSpace"), ParentSpacePhysicsControlMultiplier, false);
	}
	
	for (const FName& BoneToDelete : HitReactBoneToDelete)
	{
		// Only this bone's bodies revert to kinematic - other bones may still have
		// hits in flight, and StopPhysicalAnimProcessing() (below) handles the
		// whole-body revert once every bone has finished.
		bool bIsRagdoll = false;
		if (!bIsRagdoll)
		{
			PhysicsControlComponent->SetBodyModifiersMovementType({BoneToDelete}, EPhysicsMovementType::Kinematic);
		}

		BoneHitReactProcessingData.Remove(BoneToDelete);
	}

	IsAnyBoneProcessing = (BoneHitReactProcessingData.Num() != 0);
	if (!IsAnyBoneProcessing)
	{
		// No bone is reacting anymore: hand the skeleton back to pure animation, then stop ticking.
		StopPhysicalAnimProcessing();
		SetComponentTickEnabled(false);
	}
}

void USomnusHitReactComponent::HitReaction(FName BoneName, const FVector& Location, const FVector& Impulse)
{
	if (GetOwner()->HasAuthority() && SkeletalMeshComponent && PhysicsControlComponent)
	{
		MulticastHitReaction(BoneName, Location, Impulse);
	}
}

void USomnusHitReactComponent::SetupPhysicsForHit(bool bIsRagdoll)
{
	SetAnimation(bIsRagdoll);
	PhysicsControlComponent->SetBodyModifiersInSetMovementType(FName("All"), EPhysicsMovementType::Simulated);
	if (bIsRagdoll)
	{
		SkeletalMeshComponent->SetConstraintProfileForAll(NAME_None, true);
		PhysicsControlComponent->SetBodyModifiersInSetGravityMultiplier(FName("All"), 1.f);
		PhysicsControlComponent->SetControlsInSetEnabled(FName("WorldSpace"), false);
		PhysicsControlComponent->SetControlsInSetEnabled(FName("ParentSpace"), true);
	}
	else
	{
		SkeletalMeshComponent->SetConstraintProfileForAll(FName("RagdollNoDrives"), false);
		PhysicsControlComponent->SetBodyModifiersInSetGravityMultiplier(FName("All"), 0.f);
		PhysicsControlComponent->SetBodyModifiersInSetMovementType(FName("Feet"), EPhysicsMovementType::Kinematic);
		PhysicsControlComponent->SetControlsInSetEnabled(FName("WorldSpace"), true);
		PhysicsControlComponent->SetControlsInSetEnabled(FName("ParentSpace"), false);
	}
}

void USomnusHitReactComponent::SetAnimation(bool bIsRagdoll)
{
	// TODO: Just send abp use ragdoll
	// ABP = Cast<>SkeletalMeshComponent;
	// ABP->HitReaction = bIsRagdoll;
}

void USomnusHitReactComponent::StopPhysicalAnimProcessing()
{
	// Inverse of SetupPhysicsForHit: drop all physical drive and return to animation.
	SetAnimation(false);
	PhysicsControlComponent->SetControlsInSetEnabled(FName("WorldSpace"), false);
	PhysicsControlComponent->SetControlsInSetEnabled(FName("ParentSpace"), false);
	PhysicsControlComponent->SetBodyModifiersInSetMovementType(FName("All"), EPhysicsMovementType::Kinematic);
}

void USomnusHitReactComponent::MulticastHitReaction_Implementation(FName BoneName, const FVector& Location,
                                                                    const FVector& Impulse)
{
	if (BoneHitReactProcessingData.IsEmpty())
	{
		SetComponentTickEnabled(true);
	}
	
	// Every hit gets its own record and its own delay timer, independent of whatever
	// else is already in flight on this bone - a fast re-hit must not cancel an
	// earlier hit's still-pending delayed impulse.
	TArray<FBoneHitReactProcessingData>& Records = BoneHitReactProcessingData.FindOrAdd(BoneName);
	FBoneHitReactProcessingData& NewRecord = Records.AddDefaulted_GetRef();

	FTimerDelegate DelayDelegate = FTimerDelegate::CreateUObject(
		this, &USomnusHitReactComponent::ApplyDelayedImpulse, BoneName, Location, Impulse);

	GetWorld()->GetTimerManager().SetTimer(
		NewRecord.DelayTimer, DelayDelegate, ImpuseApplyDelay, /*bLoop=*/false);

	bool bIsRagdoll = false;
	SetupPhysicsForHit(bIsRagdoll);
}

void USomnusHitReactComponent::ApplyDelayedImpulse(FName BoneName, FVector Location, FVector Impulse)
{
	if (!SkeletalMeshComponent->IsSimulatingPhysics(BoneName))
	{
		SkeletalMeshComponent->SetBodySimulatePhysics(BoneName, true);
	}
	// Apply at the exact impact point so an off-centre hit produces torque (mass-aware, no bVelChange).
	SkeletalMeshComponent->AddImpulseAtLocation(Impulse, Location, BoneName);
}
