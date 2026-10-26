// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "AgCameraEffect.generated.h"

UENUM()
enum class EAgCameraSituation : uint8
{
	PlayerHit UMETA(DisplayName = "플레이어의 공격 적중 (움찔)"),
	PlayerHitHeavy UMETA(DisplayName = "플레이어의 공격 적중 (넉업·넉다운)"),
	Parry UMETA(DisplayName = "패리 성공"),
	PerfectDodge UMETA(DisplayName = "극한 회피"),
	DodgeCounterHit UMETA(DisplayName = "회피 반격 적중"),
	PlayerHurt UMETA(DisplayName = "플레이어 피격 (움찔)"),
	PlayerHurtHeavy UMETA(DisplayName = "플레이어 피격 (넉업·넉다운)"),
	Guard UMETA(DisplayName = "가드 움찔"),
	GuardPushback UMETA(DisplayName = "가드 밀림"),
	GuardBreak UMETA(DisplayName = "가드 브레이크"),
	ExecutionStab UMETA(DisplayName = "처형의 찌르기"),
	ExecutionBlow UMETA(DisplayName = "처형의 마지막 일격"),
	BossKill UMETA(DisplayName = "보스 처치"),
};

USTRUCT()
struct FAgCameraEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UCameraShakeBase>> CameraShakes;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bTurnToOpponent = false;

	/** 상대가 화면 가운데에서 좌우 이 각도 안에 들어올 때까지 돕니다. */
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bTurnToOpponent", ClampMin = "0"))
	float TurnAngle = 0.f;
};
