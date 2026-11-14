// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

/** 채널 번호는 DefaultEngine.ini가 AgWeapon, AgInteraction에 준 슬롯과 같아야 합니다. */
inline constexpr ECollisionChannel ECC_AgWeapon = ECC_GameTraceChannel1;
inline constexpr ECollisionChannel ECC_AgInteraction = ECC_GameTraceChannel2;

namespace AgCollision
{
	/** 장치의 상호작용 범위 도형에 주는 프로파일입니다. DefaultEngine.ini에 있고 AgInteraction 채널에만 겹칩니다. */
	inline const FName InteractionRangeProfile(TEXT("AgInteractionRange"));
} // namespace AgCollision
