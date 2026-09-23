// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/SomnusMovementTypes.h"
#include "SomnusCharacterMovementComponent.generated.h"

/**
 * How one gait moves on the ground.
 *
 * Speeds are measured off the root motion of that gait's motion matching loops - a capsule that
 * outruns its clips slides the feet, and one that lags them makes the search reach for the wrong
 * clip. Acceleration and friction each hold one value at low speed and one at the gait's forward
 * speed, blended by how fast the character is going; a gait that should feel the same throughout
 * sets both ends equal.
 */
USTRUCT(BlueprintType)
struct FSomnusGaitSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Speed", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float ForwardSpeed = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float StrafeSpeed = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float BackwardSpeed = 0.f;

	/** Speed at which acceleration and friction start leaving their low-speed values. They reach
	 *  their top-speed values at ForwardSpeed. */
	UPROPERTY(EditDefaultsOnly, Category = "Ramp", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float RampStartSpeed = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ramp", meta = (ClampMin = "0.0"))
	float Acceleration = 800.f;

	/** Lower than Acceleration makes the last stretch to top speed build slowly. */
	UPROPERTY(EditDefaultsOnly, Category = "Ramp", meta = (ClampMin = "0.0"))
	float AccelerationAtTopSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ramp", meta = (ClampMin = "0.0"))
	float GroundFriction = 5.f;

	/** Lower than GroundFriction lets the body slide wider through turns at speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Ramp", meta = (ClampMin = "0.0"))
	float GroundFrictionAtTopSpeed = 5.f;
};

/**
 * Carries the player's movement wishes - walk, sprint, strafe - inside the movement it predicts, so
 * the server runs every move with the same wishes the client ran it with.
 *
 * Wishes rather than outcomes on purpose. What the character actually does - walking because it is
 * aiming, running because it is sprinting sideways - is decided again from these on every move, on
 * both machines, from state both of them have. Sending a decided gait instead would let a client
 * tell the server how fast it is allowed to go.
 *
 * Everything that shapes a move on the ground is decided here for the same reason: speed,
 * acceleration, braking, friction and how the body turns. A value set from the character's tick
 * instead would be set at a different moment on each machine, and the server would replay the
 * client's moves under numbers the client never used.
 */
UCLASS()
class SOMNUS_API USomnusCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	USomnusCharacterMovementComponent();

	/** Call on the controlling client. The flag reaches the server with the next move. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Gait")
	void SetWantsToWalk(bool bWants) { bWantsToWalk = bWants; }

	/** Call on the controlling client. The flag reaches the server with the next move. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Gait")
	void SetWantsToSprint(bool bWants) { bWantsToSprint = bWants; }

	/** Call on the controlling client. The flag reaches the server with the next move. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Rotation")
	void SetWantsToStrafe(bool bWants) { bWantsToStrafe = bWants; }

	UFUNCTION(BlueprintPure, Category = "Movement|Gait")
	bool WantsToWalk() const { return bWantsToWalk; }

	UFUNCTION(BlueprintPure, Category = "Movement|Gait")
	bool WantsToSprint() const { return bWantsToSprint; }

	UFUNCTION(BlueprintPure, Category = "Movement|Rotation")
	bool WantsToStrafe() const { return bWantsToStrafe; }

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual bool ClientUpdatePositionAfterServerUpdate() override;

	/** Top speed for this move on the ground: the requested gait's speeds, read at the angle the
	 *  character is moving at. Anything off the ground keeps the engine's answer. */
	virtual float GetMaxSpeed() const override;

	/** The requested gait's acceleration at the current speed, on the ground. */
	virtual float GetMaxAcceleration() const override;

	/** On the ground, gentle while still pushing and hard once the push lets go: easing down from
	 *  a sprint to a run reads as a stride shortening, and stopping reads as planting the feet. */
	virtual float GetMaxBrakingDeceleration() const override;

	/** Swaps in the requested gait's friction on the ground. The engine reads GroundFriction
	 *  straight off the member with no getter to override, so the value is replaced where it
	 *  arrives instead. */
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	/** The gait the wishes and the character's state ask for - aiming forces a walk, a sprint wish
	 *  beats the walk toggle. Picks which settings are read, not which animation plays: a sprint
	 *  sideways is still requested as a sprint and slows to a run through the speeds themselves. */
	UFUNCTION(BlueprintPure, Category = "Movement|Gait")
	ESomnusGait GetRequestedGait() const;

	/** Degrees from where the body faces to where the character is pushing, -180 to 180, positive
	 *  to the right (yaw grows clockwise seen from above). 0 when there is no push to measure. */
	float GetMoveAngle() const;

	/** The gait animation should show, given the one it showed last. Differs from the requested
	 *  gait only for a sprint off to the side: the sprint clips cover the forward arc alone, so
	 *  outside it the body is shown running, which is also what its speed has fallen to. The arc
	 *  is wider to stay in than to enter, so a heading on its edge does not flip the pick every
	 *  frame. Animation only - movement never reads it, so the state it keeps is harmless. */
	ESomnusGait ResolveDisplayedGait(ESomnusGait PreviousDisplayedGait) const;

	/** Sets the turn flags and rate for strafing or facing the move. Moves call it from their own
	 *  wish; a simulated proxy calls it with the replicated state, since it runs no moves but its
	 *  animation still reads these flags to predict where the body will face. */
	void ApplyRotationMode(bool bStrafing);

protected:
	/** Sets how the body turns for this move from the strafe wish and whether it is in the air.
	 *  Runs inside every move - the client's, the server's replay of it, and a correction replay -
	 *  so each move turns under the wish it was made with. */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	const FSomnusGaitSettings& GetGaitSettings(ESomnusGait Gait) const;

	/** One gait's speed at a move angle from GetMoveAngle; either sign, left and right are the same.
	 *  Flat across the diagonals, because the diagonal clips were captured at the forward and
	 *  backward speeds: 0-45 forward, 45-90 forward to strafe, 90-135 strafe to backward, 135-180
	 *  backward. */
	static float GetSpeedAtAngle(const FSomnusGaitSettings& Settings, float MoveAngle);

	/** How far through a gait's ramp the current horizontal speed is, 0 to 1. */
	float GetRampAlpha(const FSomnusGaitSettings& Settings) const;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Gait")
	FSomnusGaitSettings WalkSettings;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Gait")
	FSomnusGaitSettings RunSettings;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Gait")
	FSomnusGaitSettings SprintSettings;

	/** Braking on the ground while there is still a push, as when a sprint eases down to a run. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Gait", meta = (ClampMin = "0.0"))
	float BrakingDecelerationWithInput = 500.f;

	/** Braking on the ground once the push lets go. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Gait", meta = (ClampMin = "0.0"))
	float BrakingDecelerationWithoutInput = 2000.f;

	/** Yaw turn rate while strafing. Negative turns instantly, so the body stays square to the
	 *  camera. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Rotation")
	float StrafeRotationRate = -1.f;

	/** Yaw turn rate while the body turns to face where it moves. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Rotation")
	float OrientRotationRate = 300.f;

	/** Yaw turn rate in the air, in either mode. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Rotation")
	float FallingRotationRate = 200.f;

	/** How far off the body's facing a sprint may head and still be shown as one. Matches the
	 *  widest sprint loops, captured at 45 degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Animation", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
	float SprintDisplayEnterAngle = 45.f;

	/** How far off the facing a sprint already shown may drift before it is shown as a run. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Animation", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
	float SprintDisplayExitAngle = 60.f;

private:
	/** Whether the owner's ability system carries State.Aiming. */
	bool IsAiming() const;

	/** World yaw the body faces: the capsule's, plus the equipped weapon's stance turn while aiming.
	 *  The weapon's target turn rather than the mesh's smoothed one, because both are replicated
	 *  and so agree on server and client for the same move; the mesh only lags it for a moment. */
	float GetBodyFacingYaw() const;

	uint8 bWantsToWalk : 1;
	uint8 bWantsToSprint : 1;
	uint8 bWantsToStrafe : 1;

	friend class FSavedMove_Somnus;
};

/** One recorded move, with the wishes it was made under. */
class FSavedMove_Somnus : public FSavedMove_Character
{
public:
	using Super = FSavedMove_Character;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel,
	                        FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;

private:
	uint8 bSavedWantsToWalk : 1 = false;
	uint8 bSavedWantsToSprint : 1 = false;
	uint8 bSavedWantsToStrafe : 1 = false;
};

class FNetworkPredictionData_Client_Somnus : public FNetworkPredictionData_Client_Character
{
public:
	using Super = FNetworkPredictionData_Client_Character;

	explicit FNetworkPredictionData_Client_Somnus(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};
