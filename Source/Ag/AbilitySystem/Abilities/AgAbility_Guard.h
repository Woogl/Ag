// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "AgAbility_Guard.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAgAbilityTask_HoldTag;

/**
 * 가드 and 패리 (플레이어 사양 '가드', '패리'): guards while the guard key is held, and the player can move slowly.
 * A new guard start opens the 패리 구간; returning to guard after a parry motion or a guard pushback doesn't.
 * A parried attack plays the parry motion. During it the guard holds only while the key is held, and pressing the key
 * again in its recovery (후딜) starts a new guard with a new parry window. Releasing the key ends the guard.
 */
UCLASS()
class UAgAbility_Guard : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Guard();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void StartGuard(bool bOpenParryWindow);

	/** State.Guarding: 이 태그가 있으면 공격을 가드합니다(전투 시스템 '피격 처리 순서'). */
	void SetGuarding(bool bGuarding);

	void CloseParryWindow();

	/** 가드 시작 후 이동할 수 없는 시간을 끝냅니다. */
	void CloseMoveLock();
	void EndParryMotion();

	UFUNCTION()
	void HandleGuardPressed(FGameplayEventData Payload);

	UFUNCTION()
	void HandleGuardReleased(FGameplayEventData Payload);

	UFUNCTION()
	void HandleParry(FGameplayEventData Payload);

	UFUNCTION()
	void HandleParryRecoveryStarted(FGameplayEventData Payload);

	UFUNCTION()
	void HandleParryMotionFinished();

	UFUNCTION()
	void HandleParryMotionCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ParryTask;

	/** 가드 중(State.Guarding)인 동안 있습니다. 패리 모션 중에 가드 키를 떼면 없어집니다. */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> GuardingTag;

	/** 패리 모션이 재생되는 동안 있고, 그동안 이동 입력을 막습니다(State.Acting). */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> ParryMotionTag;

	FActiveGameplayEffectHandle ParryWindow;
	FActiveGameplayEffectHandle MoveLock;
};
