// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Data/AgCameraData.h"
#include "AgExecutionCameraModifier.generated.h"

struct FCollisionQueryParams;

/**
 * 처형하는 플레이어와 대상을 옆에서 비추는 구도로 게임 카메라를 덮습니다. 컨트롤 회전은 건드리지 않으므로, 끄면 그동안의
 * 게임 카메라(자유 시점 또는 락온 카메라)로 돌아갑니다.
 */
UCLASS(NotBlueprintable)
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
	/** 바라보는 지점에서 카메라 자리까지 지형에 막히면 막힌 곳을 돌려줍니다. */
	FVector SweepCameraLocation(const FVector& LookAt, float CameraYaw, const FCollisionQueryParams& Params, bool& bOutBlocked) const;

	FAgExecutionCameraSettings Settings;

	FVector CameraLocation = FVector::ZeroVector;
	FRotator CameraRotation = FRotator::ZeroRotator;
	float Elapsed = 0.f;
};
