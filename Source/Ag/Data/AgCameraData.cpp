// Copyright Woogle. All Rights Reserved.

#include "Data/AgCameraData.h"

#include "Core/AgGameplayTags.h"

FGameplayTag UAgCameraData::GetShakeCue(EAgCameraShake Shake)
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
