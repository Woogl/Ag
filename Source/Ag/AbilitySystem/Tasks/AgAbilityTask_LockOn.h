// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_LockOn.generated.h"

class AAgCharacterBase;
class AAgPlayerCharacter;

/** 락온을 쓰는 행동의 표시(LockOn Task의 Params). 행동 처리가 대상을 정해 시작합니다. */
USTRUCT()
struct FAgLockOnParams : public FAgTaskParams
{
	GENERATED_BODY()

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}
};

/**
 * 락온 대상을 보관하고 카메라를 대상의 락온 지점 쪽으로 돌립니다. 대상 상실 시 알리고, 종료 시 대상과 회전 상태를 정리합니다.
 */
UCLASS()
class UAgAbilityTask_LockOn : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_LockOn();

	/** 대상이 죽거나 해제 거리 밖으로 벗어나면 보냅니다. */
	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnTargetLost;

	/** 화면과 타겟팅 범위 안의 가장 가까운 적을 읽기 전용으로 조회합니다. */
	static AAgCharacterBase* FindTarget(const AAgPlayerCharacter& Player);
	static UAgAbilityTask_LockOn* FollowTarget(UGameplayAbility* OwningAbility, AAgCharacterBase* Target);

	AAgCharacterBase* GetTarget() const;
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	bool IsTargetLost() const;
	FVector GetLockOnPoint() const;
	/** 컨트롤 회전을 대상 쪽으로 돌립니다(카메라 '구도', '회전'). */
	void UpdateCamera(float DeltaTime);

	TWeakObjectPtr<AAgCharacterBase> Target;

	/** 카메라의 회전 속도 (deg/s). 락온할 때마다 0에서 시작합니다. */
	float PitchRate = 0.f;
	float YawRate = 0.f;
};
