// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayCues/SomnusGCN_MeleeHit.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Character/SomnusCharacterBase.h"
#include "Kismet/GameplayStatics.h"

USomnusGCN_MeleeHit::USomnusGCN_MeleeHit()
{
	// GameplayCueTag is deliberately left for the blueprint to set. The cue manager finds notifies
	// by the GameplayCueName saved in each asset, and the editor rewrites that name on save: when a
	// blueprint's tag merely matches its parent's, it clears the tag, tries to read one off the
	// asset name, and on failure restores the tag but leaves the saved name empty - so a tag
	// inherited from here would vanish from the manager on every reload.
}

bool USomnusGCN_MeleeHit::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AActor* Causer = Parameters.EffectContext.GetEffectCauser();
	if (const FHitResult* HitResult = Parameters.EffectContext.GetHitResult())
	{	
		const FRotator BloodRotation = HitResult->ImpactNormal.Rotation();
		if (UNiagaraSystem* NiagaraBlood = Cast<UNiagaraSystem>(BloodEffect))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				MyTarget, NiagaraBlood, HitResult->ImpactPoint, BloodRotation);
		}
		else if (UParticleSystem* CascadeBlood = Cast<UParticleSystem>(BloodEffect))
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				MyTarget, CascadeBlood, HitResult->ImpactPoint, BloodRotation);
		}
	
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(MyTarget, HitSound, HitResult->ImpactPoint);
		}
	}
	ASomnusCharacterBase* CauserCharacter = Cast<ASomnusCharacterBase>(Causer);
	ASomnusCharacterBase* TargetCharacter = Cast<ASomnusCharacterBase>(MyTarget);
	if (CauserCharacter && TargetCharacter)
	{
		CauserCharacter->ApplyHitStop(HitStopDuration, HitStopRateScale);
		TargetCharacter->ApplyHitStop(HitStopDuration, HitStopRateScale);
	}
	
	if (CameraShake)
	{
		if (CauserCharacter && CauserCharacter->IsLocallyControlled())
		{
			if (APlayerController* PC = Cast<APlayerController>(CauserCharacter->GetController()))
			{
				PC->PlayerCameraManager->StartCameraShake(CameraShake, CameraShakeScale);
			}
		}
	}
	
	return true;
}
