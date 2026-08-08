// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/SomnusHitReactTestActor.h"
#include "Character/SomnusHitReactComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SkeletalMeshComponent.h"

ASomnusHitReactTestActor::ASomnusHitReactTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	SetRootComponent(Billboard);
}

void ASomnusHitReactTestActor::TriggerHit()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: TriggerHit only works on the server (MulticastHitReaction requires authority)."), *GetName());
		return;
	}

	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: TargetActor is not set."), *GetName());
		return;
	}

	USomnusHitReactComponent* HitReactComponent = TargetActor->FindComponentByClass<USomnusHitReactComponent>();
	if (!HitReactComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: TargetActor '%s' has no USomnusHitReactComponent."), *GetName(), *TargetActor->GetName());
		return;
	}

	FVector Location = TargetActor->GetActorLocation();
	if (USkeletalMeshComponent* Mesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(BoneName))
		{
			Location = Mesh->GetSocketLocation(BoneName);
		}
	}

	const FVector Impulse = ImpulseDirection.GetSafeNormal() * ImpulseStrength;
	HitReactComponent->HitReaction(BoneName, Location, Impulse);
}
