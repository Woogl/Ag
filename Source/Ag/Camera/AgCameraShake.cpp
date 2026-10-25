// Copyright Woogle. All Rights Reserved.

#include "Camera/AgCameraShake.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Core/AgGameplayTags.h"

namespace
{
	FGameplayTag GetCueTag(EAgCameraShake Shake)
	{
		switch (Shake)
		{
			case EAgCameraShake::Small:
				return AgGameplayTags::GameplayCue_CameraShake_Small;
			case EAgCameraShake::Medium:
				return AgGameplayTags::GameplayCue_CameraShake_Medium;
			case EAgCameraShake::Large:
				return AgGameplayTags::GameplayCue_CameraShake_Large;
			default:
				return FGameplayTag();
		}
	}
}

void AgCameraShake::Play(AActor* Source, EAgCameraShake Shake)
{
	const FGameplayTag CueTag = GetCueTag(Shake);
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Source);
	if (ASC && CueTag.IsValid())
	{
		ASC->ExecuteGameplayCue(CueTag, FGameplayCueParameters());
	}
}
