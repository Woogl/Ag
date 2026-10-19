// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AgCameraData.generated.h"

UENUM()
enum class EAgCameraShake : uint8
{
	None UMETA(DisplayName = "없음"),
	Small UMETA(DisplayName = "작음"),
	Medium UMETA(DisplayName = "보통"),
	Large UMETA(DisplayName = "큼"),
};

USTRUCT()
struct FAgCameraShakeSettings
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, meta = (ClampMin = 0.001, Units = "s"))
	float Duration = 0.f;

	UPROPERTY(VisibleAnywhere, meta = (ClampMin = 0, Units = "deg"))
	float RotationAmplitude = 0.f;

	UPROPERTY(VisibleAnywhere, meta = (ClampMin = 0, Units = "cm"))
	float LocationAmplitude = 0.f;
};

UCLASS(BlueprintType)
class UAgCameraData : public UObject
{
	GENERATED_BODY()

public:
	const FAgCameraShakeSettings* FindShakeSettings(EAgCameraShake Shake) const;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	FAgCameraShakeSettings SmallShake;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	FAgCameraShakeSettings MediumShake;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	FAgCameraShakeSettings LargeShake;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake", meta = (ClampMin = 0))
	float ShakePitchFrequency = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake", meta = (ClampMin = 0))
	float ShakeYawFrequency = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake", meta = (ClampMin = 0))
	float ShakeYFrequency = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake", meta = (ClampMin = 0))
	float ShakeZFrequency = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnPitchMin = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnPitchMax = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "deg"))
	float LockOnLookDown = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "deg/s", ClampMin = 0))
	float LockOnRotationSpeed = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "s", ClampMin = 0))
	float LockOnSmoothingTime = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake PlayerHitShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake PlayerHitHeavyShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake ParryShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake PlayerHurtShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake PlayerHurtHeavyShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake GuardShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake GuardPushbackShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake GuardBreakShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake ExecutionStabShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake ExecutionBlowShake = EAgCameraShake::None;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Camera Shake")
	EAgCameraShake FinisherShake = EAgCameraShake::None;
};
