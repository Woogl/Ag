// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "AgAbility_Guard.generated.h"

class UAbilityTask_PlayMontageAndWait;

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

	/** State.Guarding: hits are guarded (전투 시스템 '피격 처리 순서'). */
	void SetGuarding(bool bGuarding);

	void CloseParryWindow();
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

	FActiveGameplayEffectHandle ParryWindow;
	bool bGuarding = false;
	bool bInParryMotion = false;
	bool bInParryRecovery = false;
};
