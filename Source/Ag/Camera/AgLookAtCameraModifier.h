// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Data/AgCameraData.h"
#include "AgLookAtCameraModifier.generated.h"

/**
 * 게임 카메라에서 대상의 락온 지점을 잇는 선을 따라 카메라를 당겨 대상을 화면 한가운데 비춥니다. 컨트롤 회전은 건드리지
 * 않으므로, 끄면 그동안의 게임 카메라로 돌아갑니다.
 */
UCLASS(NotBlueprintable)
class UAgLookAtCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	UAgLookAtCameraModifier(const FObjectInitializer& ObjectInitializer);

	void Start(const AActor& InTarget, const FAgCameraLookAt& LookAt);

	/** 돌아오기를 시작합니다. */
	void Stop();

	/** 돌아오기 전까지만 대상을 돌려줍니다. */
	const AActor* GetLookAtTarget() const;

protected:
	virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;

private:
	TWeakObjectPtr<const AActor> Target;
	float Distance = 0.f;
};
