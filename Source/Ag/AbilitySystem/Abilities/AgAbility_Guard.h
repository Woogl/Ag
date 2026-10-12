// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "AgAbility_Guard.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAgAbilityTask_HoldTag;

/**
 * 가드와 패리 (플레이어 사양 '가드', '패리'): 가드 키를 누르고 있는 동안 가드하며, 그동안 천천히 움직일 수 있습니다.
 * 새 가드 시작은 패리 구간을 열고, 패리 모션이나 가드 밀림 뒤에 가드로 돌아올 때는 열지 않습니다. 패리하면 패리 모션을
 * 재생합니다. 그동안은 키를 누르고 있을 때만 가드하고, 후딜에 키를 다시 누르면 패리 구간과 함께 가드를 새로 시작합니다.
 * 키를 떼면 가드가 끝납니다.
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
