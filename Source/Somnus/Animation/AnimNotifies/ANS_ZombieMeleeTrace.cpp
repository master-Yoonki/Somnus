// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotifies/ANS_ZombieMeleeTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Core/SomnusCollisionChannels.h"

void UANS_ZombieMeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                        float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	AlreadyHitActors.Empty();
}

void UANS_ZombieMeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (MeshComp)
	{
		AActor* OwnerActor = MeshComp->GetOwner();
		if (OwnerActor && OwnerActor->HasAuthority())
		{
			for (const FSomnusZombieTraceConfig& TraceConfig : TraceConfigs)
			{
				const FName& SocketName = TraceConfig.SocketName;
				const float TraceRadius = TraceConfig.RaceRadius;
				
				TArray<AActor*> ActorsToIgnore = AlreadyHitActors;
				ActorsToIgnore.Add(OwnerActor);
				
				TArray<FHitResult> HitResults;
				if (MeshComp->DoesSocketExist(SocketName))
				{
					FVector SocketWorldLocation = MeshComp->GetSocketLocation(SocketName);
					UKismetSystemLibrary::SphereTraceMulti(
						OwnerActor->GetWorld(),	
						SocketWorldLocation, 
						SocketWorldLocation, 
						TraceRadius,
						UEngineTypes::ConvertToTraceType(Somnus_TraceChannel_Weapon),
						false,
						ActorsToIgnore,
						EDrawDebugTrace::ForOneFrame,
						HitResults,
						true,
						FLinearColor::Red,
			FLinearColor::Green,
				0.0f
						);
				}
				for (FHitResult HitResult : HitResults)
				{
					AActor* TargetActor = HitResult.GetActor();
					if (TargetActor)
					{
						if (Cast<ASomnusCharacter>(TargetActor))
						{
							AlreadyHitActors.Add(TargetActor);
							
							FGameplayEventData Payload;
							Payload.Instigator = OwnerActor;
							Payload.Target = TargetActor;
							
							Payload.TargetData = 
								UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);
							
							UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
								OwnerActor, 
								SomnusTags::Event_Melee_Hit,
								Payload
								);
						}
					}
				}
			}
		}
	}
}

void UANS_ZombieMeleeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	AlreadyHitActors.Empty();
}
