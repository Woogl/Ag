// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Groggy.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;

/**
 * 그로기 (전투 시스템 '그로기', 보스 사양 '그로기', '처형 피격'): PP가 0이 되면 시작합니다. 모든 행동을 멈추고
 * 그로기 시간과 함께 끝나도록 그로기 모션을 재생합니다. 처형당하면 처형 피격 모션으로 바꾸고, 남은 시간과 관계없이
 * 그 모션이 끝날 때 그로기가 끝납니다. 그로기가 끝나면 PP가 가득 찹니다.
 */
UCLASS()
class UAgAbility_Groggy : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Groggy();

	/** 처형 피격 몽타주의 ID (캐릭터 데이터의 Montages). 그로기 몽타주는 어빌리티 ID로 찾습니다. 처형 사망도 씁니다. */
	static const FName ExecutedMontageId;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** 땅에서 그로기 모션을 재생하고 그로기 시간을 세기 시작합니다. */
	void StartGroggy();

	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	UAbilityTask_PlayMontageAndWait* PlayMontage(UAnimMontage* Montage);

	/** 그로기를 끝내고 PP를 가득 채웁니다. */
	void FinishGroggy();

	UFUNCTION()
	void HandleEndSectionDue();

	UFUNCTION()
	void HandleExecuted(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> EndSectionTask;
};
