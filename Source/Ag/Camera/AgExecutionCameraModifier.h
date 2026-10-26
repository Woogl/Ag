// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "AgExecutionCameraModifier.generated.h"

/**
 * 처형하는 플레이어와 대상을 옆에서 비추는 구도로 게임 카메라를 덮습니다. 컨트롤 회전은 건드리지 않으므로, 끄면 그동안의
 * 게임 카메라(자유 시점 또는 락온 카메라)로 돌아갑니다.
 */
UCLASS()
class UAgExecutionCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	UAgExecutionCameraModifier(const FObjectInitializer& ObjectInitializer);

	void Start(const AActor& InPlayer, const AActor& InTarget);

	void Stop();

protected:
	virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;

private:
	FVector GetLookAtLocation() const;

	/** 바라보는 지점에서 카메라 자리까지 지형에 막히면 막힌 곳을 돌려줍니다. */
	FVector SweepCameraLocation(const FVector& LookAt, float CameraYaw, bool& bOutBlocked) const;

	/** 플레이어가 대상을 바라보는 방향에서 오른쪽으로 잰 카메라 방향입니다. 180이면 플레이어 바로 뒤입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Composition")
	float Yaw = 135.f;

	/** 바라보는 지점에서 카메라까지의 수평 거리입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Composition", meta = (ClampMin = "0"))
	float Distance = 480.f;

	/** 카메라가 바라보는 지점보다 높은 만큼입니다. 음수면 낮은 곳에서 올려다봅니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Composition")
	float CameraHeight = 0.f;

	/** 플레이어와 처형을 시작할 때 대상의 캡슐 중심 사이 가운데에서 바라보는 지점까지의 높이입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Composition")
	float Height = 30.f;

	/** 처형을 시작할 때의 시야각입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Composition", meta = (ClampMin = "5", ClampMax = "170"))
	float FieldOfView = 50.f;

	/** 시작한 뒤 ZoomDuration(게임 시간) 동안 이 시야각까지 좁힙니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Composition", meta = (ClampMin = "5", ClampMax = "170"))
	float ZoomFieldOfView = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Composition", meta = (ClampMin = "0"))
	float ZoomDuration = 1.8f;

	/**
	 * 처형을 시작한 뒤 돌아오기를 시작하는 게임 시간입니다. 처형 모션의 마지막 일격(몽타주 0.8초 ÷ 배속 0.6)에서
	 * 0.2초 뒤라, 일격 시각이나 배속을 바꾸면 함께 고칩니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Timing", meta = (ClampMin = "0"))
	float ReturnTime = 1.53f;

	UPROPERTY(EditDefaultsOnly, Category = "Collision", meta = (ClampMin = "0"))
	float ProbeSize = 12.f;

	TWeakObjectPtr<const AActor> Player;
	TWeakObjectPtr<const AActor> Target;
	/** 걷어차여 밀려나는 대상을 카메라가 따라가지 않도록 처형을 시작할 때의 위치를 씁니다. */
	FVector TargetStartLocation = FVector::ZeroVector;
	float CompositionYaw = 0.f;
	float Elapsed = 0.f;
};
