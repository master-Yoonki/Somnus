// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SomnusCameraMode.generated.h"

/**
 * The framings the camera can hold. These name how the shot is composed, not how the character
 * turns or what it is holding - the character works out which framing its rotation mode, aim state
 * and weapon add up to, and the camera is told only the answer.
 */
UENUM(BlueprintType)
enum class ESomnusCameraFraming : uint8
{
	/** Facing follows movement, so the character crosses the frame and has to stay readable. */
	Explore		UMETA(DisplayName = "Explore"),
	/** Facing follows the camera, so the character's back is a predictable silhouette. */
	Strafe		UMETA(DisplayName = "Strafe"),
	/** Close enough to read the swing, but never narrower - this is when the horde is nearest. */
	MeleeAim	UMETA(DisplayName = "Melee Aim"),
	/** The only framing that trades field of view for reach, because only a gun has the reach. */
	GunAim		UMETA(DisplayName = "Gun Aim"),
};

/**
 * One framing the camera can hold - how far back it sits, how far off the shoulder, and how wide
 * it sees. Every mode is the same rig on the capsule, so a mode is a set of numbers rather than a
 * camera of its own, and moving between two of them is interpolation rather than a view change.
 * A framing that is not this rig - a death camera orbiting the body, two players held in one shot -
 * has no matching numbers and wants its own camera blended by view target instead.
 */
USTRUCT(BlueprintType)
struct FSomnusCameraMode
{
	GENERATED_BODY()

	/** Spring arm length. Pick it from the narrowest corridor the player has to walk down, not by
	 *  eye: a camera that cannot fit through the level breaks line of sight before it looks wrong. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0"))
	float TargetArmLength = 300.f;

	/** Offset from the arm's end. Y moves the camera off the shoulder, Z up or down from it. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector SocketOffset = FVector::ZeroVector;

	/** Horizontal, at 16:9. Derived from the same blend as the arm length rather than animated on
	 *  its own - distance and field of view moving apart is what reads as a laggy camera. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (Units = "Degrees", ClampMin = "5", ClampMax = "170"))
	float FieldOfView = 88.f;

	/** How long entering this mode takes. It belongs to the mode being entered, so a slow settle
	 *  into exploration can still be left in a hurry when the weapon comes up. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (Units = "Seconds", ClampMin = "0"))
	float BlendTime = 0.25f;
};
