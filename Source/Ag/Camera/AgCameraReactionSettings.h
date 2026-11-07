// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/AgFieldOfViewCameraModifier.h"
#include "Camera/CameraShakeBase.h"
#include "AgCameraReactionSettings.generated.h"

UENUM()
enum class EAgCameraSituation : uint8
{
	PlayerHit,
	PlayerHitHeavy,
	Parry,
	PerfectDodge,
	DodgeCounterHit,
	GuardCounterHit,
	BlinkRushHit,
	PlayerHurt,
	PlayerHurtHeavy,
	Guard,
	GuardPushback,
	GuardBreak,
	ExecutionStab,
	ExecutionBlow,
	BossKill,
};

USTRUCT()
struct FAgCameraReactionSettings
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere)
	TArray<TSubclassOf<UCameraShakeBase>> CameraShakes;

	UPROPERTY(Transient, VisibleAnywhere)
	FAgFieldOfViewEffect FieldOfView;

	UPROPERTY(Transient, VisibleAnywhere)
	bool bTurnToOpponent = false;

	/** 상대가 화면 가운데에서 좌우 이 각도 안에 들어올 때까지 돕니다. */
	UPROPERTY(Transient, VisibleAnywhere)
	float TurnAngle = 0.f;
};
