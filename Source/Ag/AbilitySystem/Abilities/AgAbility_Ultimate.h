// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Ultimate.generated.h"

class UAgAbilityTask_TimeStop;

/**
 * 궁극기 (심판): UP를 모두 쓰고, 궁극기 컷신 뒤 멈춘 시간 속에 베기 선을 새기다가 칼을 칼집에 넣는 순간 범위 안의 적을 모두 벱니다.
 * 시작부터 끝까지 무적이고, 시작부터 마무리까지 시간 정지입니다. 마무리 뒤의 칼 뽑기가 후딜입니다.
 */
UCLASS()
class UAgAbility_Ultimate : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Ultimate();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
	virtual FAgAbilityCost GetCost() const override;

private:
	/** 컷신이 끝나면 게임 카메라로 돌아오고 베기 단계(Judgment 섹션)가 시작됩니다. */
	UFUNCTION()
	void HandleCutsceneFinished();

	/** 베기 선 하나를 새기고 다음 베기 선을 기다립니다. */
	UFUNCTION()
	void HandleSlashTime();

	/** 마무리: 칼이 칼집에 들어가는 순간(Event.UltimateFinish) */
	UFUNCTION()
	void HandleFinish(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

	/** 베기 단계 동안 일정한 간격으로, SlashIndex번째 베기 선이 나올 몽타주 위치를 기다립니다. */
	void WaitNextSlash();

	void SpawnSlash();

	/** 시작부터 마무리까지 있습니다. */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_TimeStop> TimeStopTask;

	int32 SlashIndex = 0;
};
