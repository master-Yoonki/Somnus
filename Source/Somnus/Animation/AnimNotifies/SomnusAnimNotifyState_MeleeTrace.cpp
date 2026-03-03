// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotifies/SomnusAnimNotifyState_MeleeTrace.h"
#include "Character/SomnusCharacter.h"
#include "Equipment/SomnusMeleeWeapon.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Core/SomnusGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/StaticMeshComponent.h"

void USomnusAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (!MeshComp || !MeshComp->GetOwner()) return;

    // Get the character owning this animation
    ASomnusCharacter* OwnerChar = Cast<ASomnusCharacter>(MeshComp->GetOwner());
    if (!OwnerChar) return;

    // Get the equipped melee weapon data container
    ASomnusMeleeWeapon* Weapon = Cast<ASomnusMeleeWeapon>(OwnerChar->GetEquippedWeapon());
    if (!Weapon || !Weapon->GetWeaponMesh()) return;

    // 1. Read the trace data from the weapon
    FVector BaseLocation = Weapon->GetWeaponMesh()->GetSocketLocation(Weapon->GetBaseSocketName());
    FVector TipLocation = Weapon->GetWeaponMesh()->GetSocketLocation(Weapon->GetTipSocketName());
    float Radius = Weapon->GetTraceRadius();

    TArray<FHitResult> HitResults;
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(OwnerChar); // Never hit ourselves

    // 2. Perform the Sphere Trace
    UKismetSystemLibrary::SphereTraceMulti(
        OwnerChar,
        BaseLocation,
        TipLocation,
        Radius,
        UEngineTypes::ConvertToTraceType(ECC_Pawn),
        false,
        ActorsToIgnore,
        EDrawDebugTrace::ForOneFrame, // Draw red/green lines for visual debugging
        HitResults,
        true,
        FLinearColor::Red,
        FLinearColor::Green,
        0.0f
    );

    // 3. Process Hits and send Gameplay Events to the Ability!
    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor != OwnerChar)
        {
            // Pack the hit information into a Payload
            FGameplayEventData Payload;
            Payload.Instigator = OwnerChar;
            Payload.Target = HitActor;
            
            // You can also add the exact HitResult to TargetData here for precise blood splatters later!

            // Send the "I hit someone!" event to the character's Ability System Component
            UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerChar, SomnusTags::Event_Melee_Hit, Payload);
        }
    }
}
