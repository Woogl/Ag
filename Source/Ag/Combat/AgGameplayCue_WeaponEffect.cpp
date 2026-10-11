// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCue_WeaponEffect.h"

#include "Character/AgCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"

AAgGameplayCue_WeaponEffect::AAgGameplayCue_WeaponEffect()
{
	bAutoDestroyOnRemove = true;
}

bool AAgGameplayCue_WeaponEffect::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 대상 캐릭터의 무기에 붙입니다. 몽타주 노티파이는 몸 메시를 넘기므로 캐릭터의 무기를 먼저 쓰고, 애니메이션 에디터
	// 미리보기처럼 캐릭터가 없으면 넘어온 컴포넌트에 붙입니다.
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(MyTarget);
	USceneComponent* AttachTo = Character ? Character->GetWeaponComponent() : nullptr;
	if (!AttachTo)
	{
		AttachTo = Parameters.TargetAttachComponent.Get();
	}
	if (!AttachTo && MyTarget)
	{
		AttachTo = MyTarget->GetRootComponent();
	}
	if (Effect && AttachTo)
	{
		SpawnedEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(Effect, AttachTo, AttachSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, /*bAutoDestroy*/ false);
		if (SpawnedEffect)
		{
			SpawnedEffect->SetVariableLinearColor(ColorParameter, Color);
		}
	}
	if (Sound && AttachTo)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, AttachSocket);
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
