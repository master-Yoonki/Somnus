// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/SomnusHitTraceTestCharacter.h"
#include "Camera/CameraComponent.h"
#include "Character/SomnusHitReactComponent.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Core/SomnusCollisionChannels.h"
#include "GameFramework/CharacterMovementComponent.h"

ASomnusHitTraceTestCharacter::ASomnusHitTraceTestCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	// Body yaws to match the look direction (so movement is view-relative); pitch
	// stays off the capsule and lives on the camera alone, standard FPS convention.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	Camera->bUsePawnControlRotation = true;
}

void ASomnusHitTraceTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxisKey(EKeys::W, this, &ASomnusHitTraceTestCharacter::MoveForwardPositive);
	PlayerInputComponent->BindAxisKey(EKeys::S, this, &ASomnusHitTraceTestCharacter::MoveForwardNegative);
	PlayerInputComponent->BindAxisKey(EKeys::D, this, &ASomnusHitTraceTestCharacter::MoveRightPositive);
	PlayerInputComponent->BindAxisKey(EKeys::A, this, &ASomnusHitTraceTestCharacter::MoveRightNegative);

	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &ASomnusHitTraceTestCharacter::AddLookPitchInput);

	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ASomnusHitTraceTestCharacter::Fire);
}

void ASomnusHitTraceTestCharacter::MoveForwardPositive(float Value)
{
	AddMovementInput(GetActorForwardVector(), Value);
}

void ASomnusHitTraceTestCharacter::MoveForwardNegative(float Value)
{
	AddMovementInput(GetActorForwardVector(), -Value);
}

void ASomnusHitTraceTestCharacter::MoveRightPositive(float Value)
{
	AddMovementInput(GetActorRightVector(), Value);
}

void ASomnusHitTraceTestCharacter::MoveRightNegative(float Value)
{
	AddMovementInput(GetActorRightVector(), -Value);
}

void ASomnusHitTraceTestCharacter::AddLookPitchInput(float Value)
{
	// Raw MouseY needs inverting - moving the mouse up should look up.
	AddControllerPitchInput(-Value);
}

void ASomnusHitTraceTestCharacter::ResetFireGate()
{
	bCanFire = true;
}

void ASomnusHitTraceTestCharacter::Fire()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Fire only works on the server."), *GetName());
		return;
	}

	if (!bCanFire)
	{
		return;
	}

	bCanFire = false;
	GetWorldTimerManager().SetTimer(FireCooldownTimer, this, &ASomnusHitTraceTestCharacter::ResetFireGate, FireCooldown, false);

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * TraceRange;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity, SomnusCollision::Weapon,
		FCollisionShape::MakeSphere(TraceRadius), Params);

	if (!bHit || !Hit.GetActor())
	{
		return;
	}

	USomnusHitReactComponent* HitReactComponent = Hit.GetActor()->FindComponentByClass<USomnusHitReactComponent>();
	if (!HitReactComponent)
	{
		return;
	}

	const FVector Impulse = (Hit.ImpactPoint - Start).GetSafeNormal() * ImpulseStrength;
	HitReactComponent->HitReaction(Hit.BoneName, Hit.ImpactPoint, Impulse);
}
