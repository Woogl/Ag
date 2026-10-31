// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/AgCameraReactionSettings.h"
#include "UObject/Object.h"
#include "AgCameraData.generated.h"

USTRUCT()
struct FAgLockOnCameraSettings
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere)
	float PitchMin = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float PitchMax = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float LookDown = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float RotationSpeed = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float SmoothingTime = 0.f;
};

USTRUCT()
struct FAgExecutionCameraSettings
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere)
	float Yaw = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float Distance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float CameraHeight = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float Height = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float FieldOfView = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float ZoomFieldOfView = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float ZoomDuration = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float ReturnTime = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float ProbeSize = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float AlphaInTime = 0.f;

	UPROPERTY(Transient, VisibleAnywhere)
	float AlphaOutTime = 0.f;
};

UCLASS()
class UAgCameraData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere)
	FAgLockOnCameraSettings LockOn;

	UPROPERTY(Transient, VisibleAnywhere)
	TMap<EAgCameraSituation, FAgCameraReactionSettings> ReactionsBySituation;

	UPROPERTY(Transient, VisibleAnywhere)
	FAgFieldOfViewEffect SprintFieldOfView;

	UPROPERTY(Transient, VisibleAnywhere)
	FAgExecutionCameraSettings Execution;
};
