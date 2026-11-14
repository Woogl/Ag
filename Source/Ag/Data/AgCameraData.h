// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "AgCameraData.generated.h"

class UCameraShakeBase;

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
	ExecutionStab,
	ExecutionBlow,
	BossKill,
};

// 몽타주의 카메라 연출 노티파이에서도 고치므로 편집 가능하게 둡니다.
USTRUCT()
struct FAgFieldOfViewEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Offset = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float BlendInTime = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float HoldTime = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float BlendOutTime = 0.f;

	bool operator==(const FAgFieldOfViewEffect& Other) const
	{
		return Offset == Other.Offset && BlendInTime == Other.BlendInTime && HoldTime == Other.HoldTime && BlendOutTime == Other.BlendOutTime;
	}
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
