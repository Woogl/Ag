// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;

/** 실행 소유자가 정하고 캐릭터가 적용하는 이동·회전 제약입니다. */
struct FAgCharacterMotionConstraints
{
	TWeakObjectPtr<AActor> StopTarget;
	float StopDistance = 0.f;
	bool bControlsFacing = false;
	bool bTranslationFromVelocity = false;
};
