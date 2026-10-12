// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_LockOn.generated.h"

class UAgLockOnComponent;

/**
 * 락온 중 대상을 따라갑니다(카메라 '락온 카메라'). 매 프레임 카메라를 대상의 락온 지점 쪽으로 돌리고, 락온 컴포넌트가
 * 대상을 잃었다고 하면(플레이어 사양 '락온'의 해제 조건) 끝납니다.
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

	static UAgAbilityTask_LockOn* FollowTarget(UGameplayAbility* OwningAbility, UAgLockOnComponent* LockOn);

	virtual void TickTask(float DeltaTime) override;

private:
	/** 컨트롤 회전을 대상 쪽으로 돌립니다(카메라 '구도', '회전'). */
	void UpdateCamera(float DeltaTime);

	TWeakObjectPtr<UAgLockOnComponent> LockOn;

	/** 카메라의 회전 속도 (deg/s). 락온할 때마다 0에서 시작합니다. */
	float PitchRate = 0.f;
	float YawRate = 0.f;
};
