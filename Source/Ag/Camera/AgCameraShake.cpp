// Copyright Woogle. All Rights Reserved.

#include "Camera/AgCameraShake.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Core/AgGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

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

void AgCameraShake::Play(APawn* Target, EAgCameraShake Shake)
{
	const APlayerController* Controller = Target ? Cast<APlayerController>(Target->GetController()) : nullptr;
	const FGameplayTag CueTag = GetCueTag(Shake);
	if (!Controller || !Controller->IsLocalController() || !CueTag.IsValid())
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
	{
		ASC->ExecuteGameplayCue(CueTag, FGameplayCueParameters());
	}
}
