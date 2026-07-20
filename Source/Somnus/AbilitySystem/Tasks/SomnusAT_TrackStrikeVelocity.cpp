// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Tasks/SomnusAT_TrackStrikeVelocity.h"

#include "Core/SomnusStrikeSource.h"

USomnusAT_TrackStrikeVelocity::USomnusAT_TrackStrikeVelocity
(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bTickingTask = true;
}

void USomnusAT_TrackStrikeVelocity::Activate()
{
	Super::Activate();
	for (int32 i = 0; i < CachedStrikeSourceInfos.Num(); i++)
	{
		const FSomnusStrikeSourceInfo& SourceInfo = CachedStrikeSourceInfos[i];
		const int32 NumSocketNames = SourceInfo.SocketNames.Num();

		// Add the outer entries first so PreviousLocations/Velocities stay index-aligned
		// with CachedStrikeSourceInfos even when a source can't be tracked below.
		PreviousLocations.Add({});
		Velocities.Add({});

		if (!ensureMsgf(SourceInfo.ReferenceMeshComponent,
			TEXT("StrikeSource %d has no ReferenceMeshComponent; it will not be tracked."), i))
		{
			continue;
		}

		for (int32 j = 0; j < NumSocketNames; j++)
		{
			const FName& SocketName = SourceInfo.SocketNames[j];
			FVector Location = SourceInfo.ReferenceMeshComponent->GetSocketLocation(SocketName);
			PreviousLocations[i].Add(Location);
			Velocities[i].Add(FVector{});
		}
	}
}

void USomnusAT_TrackStrikeVelocity::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	for (int32 i = 0; i < CachedStrikeSourceInfos.Num(); i++)
	{
		const FSomnusStrikeSourceInfo& SourceInfo = CachedStrikeSourceInfos[i];
		const int32 NumSocketNames = SourceInfo.SocketNames.Num();

		// Skip sources that failed to initialize in Activate (null mesh leaves empty arrays).
		if (!SourceInfo.ReferenceMeshComponent || PreviousLocations[i].Num() != NumSocketNames)
		{
			continue;
		}

		for (int32 j = 0; j < NumSocketNames; j++)
		{
			const FName& SocketName = SourceInfo.SocketNames[j];
			FVector CurrentLocation = SourceInfo.ReferenceMeshComponent->GetSocketLocation(SocketName);
			FVector Velocity = (CurrentLocation - PreviousLocations[i][j]) / FMath::Max(DeltaTime, 0.01f);
			PreviousLocations[i][j] = CurrentLocation;
			Velocities[i][j] = Velocity;
		}
	}
}

void USomnusAT_TrackStrikeVelocity::GetVelocityAtContactPoint(FVector ImpactPoint, FVector& OutVelocity, float& OutWeight) const
{
	OutVelocity = FVector::ZeroVector;
	OutWeight = 0.f;
	float MinDistanceToSurface = FLT_MAX;

	for (int32 i = 0; i < CachedStrikeSourceInfos.Num(); i++)
	{
		const FSomnusStrikeSourceInfo& SourceInfo = CachedStrikeSourceInfos[i];
		const int32 NumSockets = SourceInfo.SocketNames.Num();

		// A source that failed to initialize has no tracked samples — skip it.
		if (Velocities[i].Num() != NumSockets)
		{
			continue;
		}

		if (NumSockets == 1)
		{
			const float DistanceToSurface =
				FMath::Abs(FVector::Dist(PreviousLocations[i][0], ImpactPoint) - SourceInfo.TraceRadius);

			if (DistanceToSurface < MinDistanceToSurface)
			{
				MinDistanceToSurface = DistanceToSurface;
				OutVelocity = Velocities[i][0];
				OutWeight = SourceInfo.Weight;
			}
		}
		else if (NumSockets == 2)
		{
			const FVector BaseVelocity = Velocities[i][0];
			const FVector TipVelocity  = Velocities[i][1];

			const FVector BaseLocation = PreviousLocations[i][0];
			const FVector TipLocation  = PreviousLocations[i][1];

			const FVector BaseToTip     = TipLocation - BaseLocation;
			const FVector BaseToContact = ImpactPoint - BaseLocation;

			const float SegmentLength = BaseToTip.Length();
			const FVector SegmentDir  = BaseToTip.GetSafeNormal();

			// Project the contact point onto the base->tip line.
			const float ProjectedDist   = FVector::DotProduct(SegmentDir, BaseToContact);
			const FVector ProjectedFoot = SegmentDir * ProjectedDist;

			const float PerpendicularDist = (BaseToContact - ProjectedFoot).Length();
			const float DistanceToSurface = FMath::Abs(PerpendicularDist - SourceInfo.TraceRadius);

			if (DistanceToSurface < MinDistanceToSurface)
			{
				const float Alpha = (SegmentLength > KINDA_SMALL_NUMBER)
					? FMath::Clamp(ProjectedDist / SegmentLength, 0.f, 1.f)
					: 0.f;
				MinDistanceToSurface = DistanceToSurface;
				OutVelocity = FMath::Lerp(BaseVelocity, TipVelocity, Alpha);
				OutWeight = SourceInfo.Weight;
			}
		}
		else
		{
			ensureMsgf(false,
				TEXT("StrikeSource %d has %d sockets; expected 1 (sphere) or 2 (capsule)."), i, NumSockets);
		}
	}

	ensureMsgf(MinDistanceToSurface < FLT_MAX,
		TEXT("GetVelocityAtContactPoint found no usable strike source; returning zero velocity."));
}

USomnusAT_TrackStrikeVelocity* USomnusAT_TrackStrikeVelocity::TrackStrikeVelocity(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName, const TArray<FSomnusStrikeSourceInfo>& StrikeSourceInfos)
{
	USomnusAT_TrackStrikeVelocity* Task =
		NewAbilityTask<USomnusAT_TrackStrikeVelocity>(OwningAbility, TaskInstanceName);
	Task->CachedStrikeSourceInfos = StrikeSourceInfos;

	return Task;
}
