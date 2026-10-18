// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Launch.generated.h"

class UCharacterMovementComponent;
struct FHitResult;

/** Launch가 수평으로 보내는 곳 */
UENUM()
enum class EAgLaunchTo : uint8
{
	/** 행동 대상 앞, 그 행동의 간격만큼 떨어진 곳 */
	Target
};

/**
 * 띄우기(Launch Task의 Params). 엔진 루트 모션 점프로 체공 시간이 AirTime인 포물선을 그리고 착지하면 끝납니다. 최고 높이는
 * Height이고, 없으면 엔진 중력의 포물선과 같은 높이(중력 × AirTime² ÷ 8)입니다. To가 없으면 제자리에서 수직으로 띄웁니다.
 */
USTRUCT()
struct FAgLaunchParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float AirTime = 0.f;

	/** 선택(cm) */
	UPROPERTY()
	float Height = 0.f;

	/** 선택 */
	UPROPERTY()
	EAgLaunchTo To = EAgLaunchTo::Target;

	/** 선택. To와 함께 씁니다. 수평 이동 거리(cm)의 상한입니다. */
	UPROPERTY()
	float MaxDistance = 0.f;

	bool bHasHeight = false;
	bool bHasTo = false;
	bool bHasMaxDistance = false;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Point;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	virtual bool NeedsTarget() const override
	{
		return bHasTo;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/**
 * 엔진 루트 모션 점프(FRootMotionSource_JumpForce)로 띄우고, 착지하면 소스를 지우고 속도를 0으로 둡니다. 캐릭터 무브먼트는 몽타주가
 * 루트 모션을 주면 루트 모션 소스를 쓰지 않으므로, 도는 동안 몽타주가 루트 모션을 주면 오류를 남깁니다.
 */
UCLASS()
class UAgAbilityTask_Launch : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_Launch();

	/** 이 Task가 거는 루트 모션 소스의 이름. 캐릭터는 이 소스가 남아 있는 동안의 착지(Launch가 착지로 보지 않음)를 알리지 않습니다. */
	static const FName SourceName;

	static UAgAbilityTask_Launch* Launch(UGameplayAbility* Owner, const FAgLaunchParams& Params);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	void Finish();

	FAgLaunchParams Params;
	TWeakObjectPtr<UCharacterMovementComponent> Movement;
	uint16 SourceID = 0;
	float Elapsed = 0.f;
	bool bReportedRootMotion = false;
};
