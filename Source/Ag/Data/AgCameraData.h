// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AgCameraData.generated.h"

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
};
