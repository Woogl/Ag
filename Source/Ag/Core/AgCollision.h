// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

/** 무기 스윕용 트레이스 채널(DefaultEngine.ini의 AgWeapon). 캐릭터 메시만 겹침으로 응답하고 나머지는 무시합니다. */
inline constexpr ECollisionChannel ECC_AgWeapon = ECC_GameTraceChannel1;
