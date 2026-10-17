// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCue_AttackTelegraph.h"
#include "Character/AgCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

AAgGameplayCue_AttackTelegraph::AAgGameplayCue_AttackTelegraph()
{
	bAutoDestroyOnRemove = true;
}

bool AAgGameplayCue_AttackTelegraph::OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	ClearVFX();
	// 몽타주 노티파이는 몸 메시를 넘기므로 캐릭터의 무기를 먼저 씁니다. 캐릭터가 없는 미리보기에서는
	// 넘어온 컴포넌트에 붙입니다.
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(Target);
	USceneComponent* AttachTo = Character ? Character->GetWeaponComponent() : nullptr;
	if (!AttachTo)
	{
		AttachTo = Parameters.TargetAttachComponent.Get();
	}
	if (!AttachTo && Target)
	{
		AttachTo = Target->GetRootComponent();
	}
	if (!AttachTo)
	{
		return true;
	}
	if (VFX)
	{
		SpawnedVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
		    VFX, AttachTo, AttachSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
		if (SpawnedVFX)
		{
			SpawnedVFX->SetVariableLinearColor(ColorParameter, Color);
		}
	}
	if (Sound)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, AttachSocket);
	}
	return true;
}

void AAgGameplayCue_AttackTelegraph::ClearVFX()
{
	if (SpawnedVFX)
	{
		SpawnedVFX->DestroyComponent();
		SpawnedVFX = nullptr;
	}
}

bool AAgGameplayCue_AttackTelegraph::Recycle()
{
	ClearVFX();
	return Super::Recycle();
}

bool AAgGameplayCue_AttackTelegraph::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	ClearVFX();
	return true;
}

void AAgGameplayCue_AttackTelegraph::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearVFX();
	Super::EndPlay(EndPlayReason);
}
