// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

/** Trace channel for weapon sweeps ("AgWeapon" in DefaultEngine.ini). Character meshes overlap it; nothing else responds. */
inline constexpr ECollisionChannel ECC_AgWeapon = ECC_GameTraceChannel1;
