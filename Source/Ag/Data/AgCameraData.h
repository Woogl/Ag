// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AgCameraData.generated.h"

/** 카메라 '셰이크 단계'. 단계마다 카메라 셰이크 에셋 하나를 그 단계의 게임플레이 큐로 재생합니다. */
UENUM()
enum class EAgCameraShake : uint8
{
	None UMETA(DisplayName = "없음"),
	Small UMETA(DisplayName = "작음"),
	Medium UMETA(DisplayName = "보통"),
	Large UMETA(DisplayName = "큼"),
};

/**
 * Camera.json: 카메라 기획서의 락온 카메라 값과 셰이크의 상황별 단계(구현 계획서 'JSON과 DataAsset'). 자유 시점 값은
 * BP_AgPlayer의 스프링 암과 카메라에 있습니다. 모든 필드는 Transient이고 에셋 매니저가 JSON에서 채웁니다. 객체는 에셋
 * 매니저가 들고 있고, UAgAssetManager::GetCameraData로 읽습니다.
 */
UCLASS(BlueprintType)
class UAgCameraData : public UObject
{
	GENERATED_BODY()

public:
	/** 락온 카메라 '구도': 회전 중심에서 락온 지점을 보는 방향의 피치 하한 (음수는 아래) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnPitchMin = 0.f;

	/** 락온 카메라 '구도': 그 방향의 피치 상한 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnPitchMax = 0.f;

	/** 락온 카메라 '구도': 피치를 제한한 뒤 더 내려다보는 각도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnLookDown = 0.f;

	/** 락온 카메라 '회전': 목표 방향으로 도는 최대 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "deg/s", ClampMin = 0))
	float LockOnRotationSpeed = 0.f;

	/** 락온 카메라 '회전': 따라잡는 시간. 계속 움직이는 목표를 회전이 이만큼 늦게 따라가며, 부드럽게 가속·감속합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "s", ClampMin = 0))
	float LockOnSmoothingTime = 0.f;

	/** 상황별 단계: 플레이어의 공격 적중 (피격 반응 움찔) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake PlayerHitShake = EAgCameraShake::None;

	/** 상황별 단계: 플레이어의 공격 적중 (피격 반응 넉업·넉백) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake PlayerHitHeavyShake = EAgCameraShake::None;

	/** 상황별 단계: 패리 성공 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake ParryShake = EAgCameraShake::None;

	/** 상황별 단계: 플레이어 피격 (움찔) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake PlayerHurtShake = EAgCameraShake::None;

	/** 상황별 단계: 플레이어 피격 (넉업·넉백) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake PlayerHurtHeavyShake = EAgCameraShake::None;

	/** 상황별 단계: 가드로 막음 (가드 움찔) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake GuardShake = EAgCameraShake::None;

	/** 상황별 단계: 가드로 막음 (가드 밀림) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake GuardPushbackShake = EAgCameraShake::None;

	/** 상황별 단계: 가드 브레이크 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake GuardBreakShake = EAgCameraShake::None;

	/** 상황별 단계: 처형의 베기 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake ExecutionSlashShake = EAgCameraShake::None;

	/** 상황별 단계: 처형의 마지막 일격 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake ExecutionBlowShake = EAgCameraShake::None;

	/** 상황별 단계: 궁극기 마무리 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake FinisherShake = EAgCameraShake::None;

	/** 상황별 단계: 보스 도약 내려찍기 착지 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake LeapLandingShake = EAgCameraShake::None;

	/** 상황별 단계: 도약 내려찍기 착지는 플레이어가 이 거리 안에 있을 때만 흔듭니다 (캡슐 중심 사이 수평 거리). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake", meta = (Units = "cm", ClampMin = 0))
	float LeapLandingShakeRange = 0.f;
};
