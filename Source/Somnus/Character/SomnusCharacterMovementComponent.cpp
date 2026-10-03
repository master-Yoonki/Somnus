// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/SomnusCharacterMovementComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/SomnusCharacter.h"
#include "Character/Zombie/SomnusZombieCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Core/SomnusCollisionChannels.h"
#include "Core/SomnusGameplayTags.h"
#include "Engine/OverlapResult.h"
#include "Equipment/SomnusWeapon.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

namespace SomnusMoveFlags
{
	// FLAG_Custom_0..3 are the four bits the engine leaves free in every move it sends.
	constexpr uint8 WantsToWalk = FSavedMove_Character::FLAG_Custom_0;
	constexpr uint8 WantsToSprint = FSavedMove_Character::FLAG_Custom_1;
	constexpr uint8 WantsToStrafe = FSavedMove_Character::FLAG_Custom_2;
}

namespace SomnusGaitAngles
{
	// Where the diagonal clips sit. They were captured at the forward and backward speeds, so the
	// table holds flat out to these and only blends through the quarters either side of strafe.
	constexpr float ForwardDiagonal = 45.f;
	constexpr float Strafe = 90.f;
	constexpr float BackwardDiagonal = 135.f;
}

namespace
{
	FSomnusGaitSettings MakeGaitSettings(float Forward, float Strafe, float Backward)
	{
		FSomnusGaitSettings Settings;
		Settings.ForwardSpeed = Forward;
		Settings.StrafeSpeed = Strafe;
		Settings.BackwardSpeed = Backward;
		return Settings;
	}
}

USomnusCharacterMovementComponent::USomnusCharacterMovementComponent()
	// Walking is where a player starts: careful movement is the default in the mall, and running
	// is the choice to make noise.
	: bWantsToWalk(true)
	, bWantsToSprint(false)
	, bWantsToStrafe(false)
{
	// Speeds measured by Content/Python/measure_locomotion_speeds.py off the loop clips. Sprint was
	// captured forward only (straight and up to 45 degrees off), so its sideways and backward
	// entries are run's - sprinting anywhere but ahead falls back to a run on its own.
	WalkSettings = MakeGaitSettings(206.f, 185.f, 154.f);
	RunSettings = MakeGaitSettings(515.f, 360.f, 309.f);
	SprintSettings = MakeGaitSettings(700.f, 360.f, 309.f);

	// Walk and run keep the struct's flat acceleration and friction. Sprint takes the Game Animation
	// Sample's ramp: quick off the mark, a long build to top speed, and wider turns once it is there.
	SprintSettings.RampStartSpeed = 300.f;
	SprintSettings.AccelerationAtTopSpeed = 300.f;
	SprintSettings.GroundFrictionAtTopSpeed = 3.f;
}

void USomnusCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToWalk = (Flags & SomnusMoveFlags::WantsToWalk) != 0;
	bWantsToSprint = (Flags & SomnusMoveFlags::WantsToSprint) != 0;
	bWantsToStrafe = (Flags & SomnusMoveFlags::WantsToStrafe) != 0;
}

FNetworkPredictionData_Client* USomnusCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (!ClientPredictionData)
	{
		// The engine does the same lazy allocation through a const getter (CharacterMovementComponent.cpp).
		USomnusCharacterMovementComponent* MutableThis = const_cast<USomnusCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Somnus(*this);
	}

	return ClientPredictionData;
}

bool USomnusCharacterMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
	// Replaying rewrites the wishes move by move, so a key pressed this frame, before the replay,
	// would be lost to the last replayed move's value. The engine guards bWantsToCrouch the same way.
	const bool bRealWantsToWalk = bWantsToWalk;
	const bool bRealWantsToSprint = bWantsToSprint;
	const bool bRealWantsToStrafe = bWantsToStrafe;

	const bool bResult = Super::ClientUpdatePositionAfterServerUpdate();

	bWantsToWalk = bRealWantsToWalk;
	bWantsToSprint = bRealWantsToSprint;
	bWantsToStrafe = bRealWantsToStrafe;
	return bResult;
}

float USomnusCharacterMovementComponent::GetMaxSpeed() const
{
	// Only ground movement has clips to match. Air and swimming keep the engine's speeds, which
	// jump distance and air control are tuned against.
	if (!IsMovingOnGround())
	{
		return Super::GetMaxSpeed();
	}

	return GetSpeedAtAngle(GetGaitSettings(GetRequestedGait()), GetMoveAngle());
}

float USomnusCharacterMovementComponent::GetMaxAcceleration() const
{
	// Air control scales off this too, and is tuned against the flat value.
	if (!IsMovingOnGround())
	{
		return Super::GetMaxAcceleration();
	}

	const FSomnusGaitSettings& Settings = GetGaitSettings(GetRequestedGait());
	return FMath::Lerp(Settings.Acceleration, Settings.AccelerationAtTopSpeed, GetRampAlpha(Settings));
}

float USomnusCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	if (!IsMovingOnGround())
	{
		return Super::GetMaxBrakingDeceleration();
	}

	// The push this move carries, not the character's pending input: the server only has the former.
	return Acceleration.IsNearlyZero() ? BrakingDecelerationWithoutInput : BrakingDecelerationWithInput;
}

void USomnusCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	if (IsMovingOnGround())
	{
		const FSomnusGaitSettings& Settings = GetGaitSettings(GetRequestedGait());
		Friction = FMath::Lerp(Settings.GroundFriction, Settings.GroundFrictionAtTopSpeed, GetRampAlpha(Settings));
	}

	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);

	// After the engine has built this move's velocity, so the crowd gets the last word on how much
	// of it may head into a zombie. In the air as well as on the ground - falling builds its sideways
	// velocity through here too, and a zombie is no easier to pass for jumping at it.
	ApplyCrowdContactResistance(DeltaTime);
}

ESomnusGait USomnusCharacterMovementComponent::GetRequestedGait() const
{
	// Aiming outranks everything, the sprint wish included: the stance clips only exist at a walk.
	if (IsAiming())
	{
		return ESomnusGait::Walk;
	}

	// A sprint wish beats the walk toggle, and letting go of it drops back to whatever the toggle
	// says rather than clearing it.
	if (bWantsToSprint)
	{
		return ESomnusGait::Sprint;
	}

	return bWantsToWalk ? ESomnusGait::Walk : ESomnusGait::Run;
}

float USomnusCharacterMovementComponent::GetMoveAngle() const
{
	if (Acceleration.IsNearlyZero())
	{
		return 0.f;
	}

	// Rotation() takes the yaw off the horizontal heading, so a push up a slope reads the same as
	// one across flat ground.
	return FMath::FindDeltaAngleDegrees(GetBodyFacingYaw(), Acceleration.Rotation().Yaw);
}

ESomnusGait USomnusCharacterMovementComponent::ResolveDisplayedGait(ESomnusGait PreviousDisplayedGait) const
{
	const ESomnusGait Requested = GetRequestedGait();
	if (Requested != ESomnusGait::Sprint)
	{
		return Requested;
	}

	const float ArcLimit = PreviousDisplayedGait == ESomnusGait::Sprint ? SprintDisplayExitAngle : SprintDisplayEnterAngle;
	return FMath::Abs(GetMoveAngle()) <= ArcLimit ? ESomnusGait::Sprint : ESomnusGait::Run;
}

void USomnusCharacterMovementComponent::ApplyRotationMode(bool bStrafing)
{
	// Strafing keeps the body square to the camera; otherwise it turns to face where it moves.
	bUseControllerDesiredRotation = bStrafing;
	bOrientRotationToMovement = !bStrafing;

	float YawRate = bStrafing ? StrafeRotationRate : OrientRotationRate;
	if (IsFalling())
	{
		YawRate = FallingRotationRate;
	}
	RotationRate = FRotator(0.f, YawRate, 0.f);
}

void USomnusCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	ApplyRotationMode(bWantsToStrafe);
	UpdateCrowd();
}

const FSomnusGaitSettings& USomnusCharacterMovementComponent::GetGaitSettings(ESomnusGait Gait) const
{
	switch (Gait)
	{
	case ESomnusGait::Walk:		return WalkSettings;
	case ESomnusGait::Sprint:	return SprintSettings;
	case ESomnusGait::Run:
	default:					return RunSettings;
	}
}

float USomnusCharacterMovementComponent::GetSpeedAtAngle(const FSomnusGaitSettings& Settings, float MoveAngle)
{
	// Left and right were captured at the same speeds, so only how far off the facing matters.
	const float OffFacing = FMath::Abs(MoveAngle);

	if (OffFacing <= SomnusGaitAngles::Strafe)
	{
		return FMath::GetMappedRangeValueClamped(
			FVector2f(SomnusGaitAngles::ForwardDiagonal, SomnusGaitAngles::Strafe),
			FVector2f(Settings.ForwardSpeed, Settings.StrafeSpeed), OffFacing);
	}

	return FMath::GetMappedRangeValueClamped(
		FVector2f(SomnusGaitAngles::Strafe, SomnusGaitAngles::BackwardDiagonal),
		FVector2f(Settings.StrafeSpeed, Settings.BackwardSpeed), OffFacing);
}

float USomnusCharacterMovementComponent::GetRampAlpha(const FSomnusGaitSettings& Settings) const
{
	// A ramp that ends where it starts has no length to be part way through; the gait holds its
	// low-speed values.
	const float RampLength = Settings.ForwardSpeed - Settings.RampStartSpeed;
	if (RampLength <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	return FMath::Clamp((Velocity.Size2D() - Settings.RampStartSpeed) / RampLength, 0.f, 1.f);
}

bool USomnusCharacterMovementComponent::IsAiming() const
{
	// Straight from the ability system rather than the character's per-tick copy: a server runs a
	// move whenever its packet lands, not after the character has ticked, so a cached flag could be
	// a frame stale on one machine and fresh on the other. The library call finds the player's ASC
	// on its PlayerState through the character's interface. Missing until a PlayerState arrives.
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CharacterOwner);
	return ASC && ASC->HasMatchingGameplayTag(SomnusTags::State_Aiming);
}

float USomnusCharacterMovementComponent::GetBodyFacingYaw() const
{
	const float CapsuleYaw = UpdatedComponent->GetComponentRotation().Yaw;

	// Asked of this component rather than read off the character's cached flag, so the stance turn
	// and the forced walk it comes with are decided from the same answer within one move.
	if (!IsAiming())
	{
		return CapsuleYaw;
	}

	const ASomnusCharacter* SomnusCharacter = Cast<ASomnusCharacter>(CharacterOwner);
	const ASomnusWeapon* Weapon = SomnusCharacter ? SomnusCharacter->GetEquippedWeapon() : nullptr;
	return Weapon ? CapsuleYaw + Weapon->GetAimStanceYaw() : CapsuleYaw;
}

void USomnusCharacterMovementComponent::FindOverlappingZombies(TArray<AActor*>& OutZombies) const
{
	if (!CharacterOwner) return;
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	if (!CapsuleComponent) return;
	FCollisionShape CapsuleCollisionShape = FCollisionShape::MakeCapsule(
		CapsuleComponent->GetScaledCapsuleRadius() + CrowdQueryInflation, 
		CapsuleComponent->GetScaledCapsuleHalfHeight());
	
	TArray<FOverlapResult> OverlapResults; 
	
	GetWorld()->OverlapMultiByObjectType(OverlapResults, CharacterOwner->GetActorLocation(), 
		FQuat::Identity, FCollisionObjectQueryParams(SomnusCollision::Zombie),
		CapsuleCollisionShape, 
		FCollisionQueryParams(SCENE_QUERY_STAT(CrowdQuery), false, CharacterOwner));
	
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		OutZombies.AddUnique(OverlapResult.GetActor());
	}
}

void USomnusCharacterMovementComponent::UpdateCrowd()
{
	TArray<AActor*> OverlappingZombies;
	FindOverlappingZombies(OverlappingZombies);

	CrowdThisMove.Reset(OverlappingZombies.Num());
	for (AActor* Zombie : OverlappingZombies)
	{
		CrowdThisMove.Add(Zombie);
	}
}

void USomnusCharacterMovementComponent::ApplyCrowdContactResistance(float DeltaTime)
{
	if (!CharacterOwner) return;

	const FVector OwnLocation = CharacterOwner->GetActorLocation();
	const float OwnRadius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();

	for (const TWeakObjectPtr<AActor>& CrowdMember : CrowdThisMove)
	{
		const ASomnusZombieCharacter* Zombie = Cast<ASomnusZombieCharacter>(CrowdMember.Get());
		if (!Zombie) continue;

		// Flat, for the same reason the shove is: height in it would press the player into or off
		// the floor whenever the two stand on different steps.
		const FVector ToZombie = FVector(Zombie->GetActorLocation() - OwnLocation) * FVector(1.f, 1.f, 0.f);
		const double Distance = ToZombie.Size();

		// Standing exactly on top of each other gives no line to resist along.
		if (FMath::IsNearlyZero(Distance)) continue;

		const FVector TowardZombie = ToZombie / Distance;
		const double SpeedIntoZombie = FVector::DotProduct(Velocity, TowardZombie);

		// Already moving away, or only past it; nothing to resist.
		if (SpeedIntoZombie <= 0.0) continue;

		// Negative while the zombie is inside the query's inflation but not yet touching, which
		// starts the drag a little before contact - a brush costs something even when it is close.
		const double Penetration = OwnRadius + Zombie->GetCapsuleComponent()->GetScaledCapsuleRadius() - Distance;

		// Deep enough and no further in at all: without this a zombie that cannot give way is walked
		// through, since the shove flips to the far side once the player passes its centre.
		if (Penetration >= CrowdMaxPenetration)
		{
			Velocity -= TowardZombie * SpeedIntoZombie;
			continue;
		}

		// A cap rather than a share taken off: taking a share every move would compound against the
		// engine re-accelerating in between, settling on a crawl that depends on the frame rate. As a
		// cap, the pair always moves at what the zombie's resistance leaves of this move's top speed,
		// and the server carries the zombie along at exactly that.
		const double CarrySpeed = GetMaxSpeed() * (1.f - Zombie->GetCrowdResistance());
		if (SpeedIntoZombie > CarrySpeed)
		{
			Velocity -= TowardZombie * (SpeedIntoZombie - CarrySpeed);
		}
	}
}

void FSavedMove_Somnus::Clear()
{
	Super::Clear();

	bSavedWantsToWalk = false;
	bSavedWantsToSprint = false;
	bSavedWantsToStrafe = false;
}

uint8 FSavedMove_Somnus::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();

	if (bSavedWantsToWalk)
	{
		Flags |= SomnusMoveFlags::WantsToWalk;
	}
	if (bSavedWantsToSprint)
	{
		Flags |= SomnusMoveFlags::WantsToSprint;
	}
	if (bSavedWantsToStrafe)
	{
		Flags |= SomnusMoveFlags::WantsToStrafe;
	}
	return Flags;
}

bool FSavedMove_Somnus::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	// Two moves are only sent as one if the server would run them the same way, and a change of
	// wish between them would not be.
	const FSavedMove_Somnus* Other = static_cast<const FSavedMove_Somnus*>(NewMove.Get());
	if (bSavedWantsToWalk != Other->bSavedWantsToWalk
		|| bSavedWantsToSprint != Other->bSavedWantsToSprint
		|| bSavedWantsToStrafe != Other->bSavedWantsToStrafe)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Somnus::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel,
                                   FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const USomnusCharacterMovementComponent* Movement = Cast<USomnusCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsToWalk = Movement->bWantsToWalk;
		bSavedWantsToSprint = Movement->bWantsToSprint;
		bSavedWantsToStrafe = Movement->bWantsToStrafe;
	}
}

void FSavedMove_Somnus::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	// Replaying after a correction runs old moves again, and each must run under the wishes it was
	// first made with rather than whatever the player is holding now.
	if (USomnusCharacterMovementComponent* Movement = Cast<USomnusCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		Movement->bWantsToWalk = bSavedWantsToWalk;
		Movement->bWantsToSprint = bSavedWantsToSprint;
		Movement->bWantsToStrafe = bSavedWantsToStrafe;
	}
}

FNetworkPredictionData_Client_Somnus::FNetworkPredictionData_Client_Somnus(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Somnus::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Somnus());
}
