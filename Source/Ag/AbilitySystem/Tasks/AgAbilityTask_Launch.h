// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Launch.generated.h"

class UAnimMontage;

UENUM()
enum class EAgLaunchDirection : uint8
{
	Up,
	Back,
	Target
};

USTRUCT()
struct FAgLaunchParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TOptional<float> AirTime;

	UPROPERTY()
	TOptional<float> Height;

	UPROPERTY()
	TOptional<float> Altitude;

	UPROPERTY()
	EAgLaunchDirection Direction = EAgLaunchDirection::Up;

	UPROPERTY()
	float Distance = 0.f;

	UPROPERTY()
	TOptional<float> MaxDistance;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Launch : public UAbilityTask_ApplyRootMotionJumpForce
{
	GENERATED_BODY()
	friend struct FAgLaunchParams;

public:
	static const FName SourceName;

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	/** 함께 발동한 Task(피격 반응의 즉시 회전 등)가 모두 시작된 뒤의 방향을 쓰도록 첫 틱에 띄웁니다. */
	void Launch();

	/** 블렌드 전환(넉업의 정점)과 함께 발동한 PlayMontage는 새 재생이므로 시작될 때마다 끕니다. */
	UFUNCTION()
	void DisableMontageRootMotion(UAnimMontage* Montage);

	UPROPERTY()
	FAgLaunchParams Params;

	int32 MontageInstanceId = INDEX_NONE;
	bool bLaunched = false;
	bool bArcEnded = false;
};
