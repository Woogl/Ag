// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgBossPattern_Common.generated.h"

/**
 * 보스 공용 패턴(DoubleStrike, Sweep, QuintupleStrike, JumpSlam): 공격 태스크로 패턴 몽타주를 패턴 재생 속도로 공격 데이터와 함께
 * 재생하고, 정지 간격(보스 사양 '이동')을 지키며 몽타주의 워핑 구간 동안 다가갑니다. 모션이 끝나면 끝납니다.
 * 공격 예고는 몽타주의 게임플레이 큐 노티파이가 켜고, 패턴별 쿨다운은 UAgGameplayAbility가 겁니다.
 * 전용 값은 패턴 공통 값(FAgBossPatternParams) 그대로입니다.
 */
UCLASS()
class UAgBossPattern_Common : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPattern_Common();

	virtual const UScriptStruct* GetAbilityParamsType() const override { return FAgBossPatternParams::StaticStruct(); }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleAttackCompleted();

	UFUNCTION()
	void HandleAttackInterrupted();
};
