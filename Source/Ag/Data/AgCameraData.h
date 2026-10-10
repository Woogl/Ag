// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AgCameraData.generated.h"

/** 카메라 '셰이크 단계'. Each level is a camera shake asset played through its GameplayCue. */
UENUM()
enum class EAgCameraShake : uint8
{
	None UMETA(DisplayName = "없음"),
	Small UMETA(DisplayName = "작음"),
	Medium UMETA(DisplayName = "보통"),
	Large UMETA(DisplayName = "큼"),
};

/** DA_Camera: the values of the 카메라 spec. */
UCLASS(BlueprintType)
class UAgCameraData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 자유 시점: 카메라 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "Free", meta = (Units = "cm", ClampMin = 0))
	float Distance = 0.f;

	/** 자유 시점: 회전 중심 높이 (캐릭터 캡슐 중심에서 위로) */
	UPROPERTY(EditDefaultsOnly, Category = "Free", meta = (Units = "cm"))
	float PivotHeight = 0.f;

	/** 자유 시점: 시야각 (FOV) */
	UPROPERTY(EditDefaultsOnly, Category = "Free", meta = (Units = "deg", ClampMin = 5, ClampMax = 170))
	float FieldOfView = 90.f;

	/** 자유 시점: 위치 추적의 따라잡는 시간. 0 follows without delay. */
	UPROPERTY(EditDefaultsOnly, Category = "Free", meta = (Units = "s", ClampMin = 0))
	float FollowLagTime = 0.f;

	/** 락온 카메라 '구도': lowest pitch of the direction from the pivot to the lock-on point (negative looks down). */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnPitchMin = 0.f;

	/** 락온 카메라 '구도': highest pitch of that direction. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnPitchMax = 0.f;

	/** 락온 카메라 '구도': how much further down the camera looks after the clamp. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnLookDown = 0.f;

	/** 락온 카메라 '회전': maximum turning speed toward the target direction. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On", meta = (Units = "deg/s", ClampMin = 0))
	float LockOnRotationSpeed = 0.f;

	/** 락온 카메라 '회전': 따라잡는 시간, how far the turn lags behind a target that keeps moving. It eases in and out. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On", meta = (Units = "s", ClampMin = 0))
	float LockOnSmoothingTime = 0.f;

	/** 상황별 단계: 플레이어의 공격 적중 (피격 반응 움찔) */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake PlayerHitShake = EAgCameraShake::None;

	/** 상황별 단계: 플레이어의 공격 적중 (피격 반응 넉백·다운) */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake PlayerHitHeavyShake = EAgCameraShake::None;

	/** 상황별 단계: 패리 성공 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake ParryShake = EAgCameraShake::None;

	/** 상황별 단계: 플레이어 피격 (움찔) */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake PlayerHurtShake = EAgCameraShake::None;

	/** 상황별 단계: 플레이어 피격 (넉백·다운) */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake PlayerHurtHeavyShake = EAgCameraShake::None;

	/** 상황별 단계: 가드로 막음 (가드 움찔) */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake GuardShake = EAgCameraShake::None;

	/** 상황별 단계: 가드로 막음 (가드 밀림) */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake GuardPushbackShake = EAgCameraShake::None;

	/** 상황별 단계: 가드 브레이크 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake GuardBreakShake = EAgCameraShake::None;

	/** 상황별 단계: 처형의 마지막 일격 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake ExecutionBlowShake = EAgCameraShake::None;

	/** 상황별 단계: 궁극기의 마지막 일격 (hits marked bFinisher) */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake FinisherShake = EAgCameraShake::None;

	/** 상황별 단계: 보스 A4 착지 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake")
	EAgCameraShake LeapLandingShake = EAgCameraShake::None;

	/** 상황별 단계: the A4 landing shakes only within this distance of the player (capsule centers, horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake", meta = (Units = "cm", ClampMin = 0))
	float LeapLandingShakeRange = 0.f;
};
