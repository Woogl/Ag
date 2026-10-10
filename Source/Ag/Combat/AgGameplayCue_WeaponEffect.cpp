// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCue_WeaponEffect.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

AAgGameplayCue_WeaponEffect::AAgGameplayCue_WeaponEffect()
{
	bAutoDestroyOnRemove = true;
}

bool AAgGameplayCue_WeaponEffect::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	USceneComponent* AttachTo = Parameters.TargetAttachComponent.Get();
	if (!AttachTo && MyTarget)
	{
		AttachTo = MyTarget->GetRootComponent();
	}
	if (Effect && AttachTo)
	{
		SpawnedEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(Effect, AttachTo, AttachSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, /*bAutoDestroy*/ false);
	}
	return true;
}

bool AAgGameplayCue_WeaponEffect::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (SpawnedEffect)
	{
		SpawnedEffect->DestroyComponent();
		SpawnedEffect = nullptr;
	}
	return true;
}
