// Copyright Woogle. All Rights Reserved.

#include "Data/AgCameraData.h"

const FAgCameraShakeSettings* UAgCameraData::FindShakeSettings(EAgCameraShake Shake) const
{
	switch (Shake)
	{
		case EAgCameraShake::Small:
			return &SmallShake;
		case EAgCameraShake::Medium:
			return &MediumShake;
		case EAgCameraShake::Large:
			return &LargeShake;
		default:
			return nullptr;
	}
}
