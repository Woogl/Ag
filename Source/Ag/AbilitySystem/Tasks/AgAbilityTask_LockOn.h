// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_LockOn.generated.h"

class UAgLockOnComponent;

/**
 * Follows the lock-on target while the lock-on lasts (카메라 '락온 카메라'): every frame it turns the camera toward the
 * target's lock-on point, and it ends once the lock-on component reports the target lost (플레이어 사양 '락온 해제 조건').
 */
UCLASS()
class UAgAbilityTask_LockOn : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_LockOn();

	/** The target died or went beyond the release range. */
	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnTargetLost;

	static UAgAbilityTask_LockOn* FollowTarget(UGameplayAbility* OwningAbility, UAgLockOnComponent* LockOn);

	virtual void TickTask(float DeltaTime) override;

private:
	/** 구도, 회전: turns the control rotation toward the target. */
	void UpdateCamera(float DeltaTime);

	TWeakObjectPtr<UAgLockOnComponent> LockOn;

	/** The camera's turning speeds (degrees per second). Each lock-on starts from rest. */
	float PitchRate = 0.f;
	float YawRate = 0.f;
};
