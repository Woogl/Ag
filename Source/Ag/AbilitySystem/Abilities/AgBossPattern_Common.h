// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgBossPattern_Common.generated.h"

/**
 * 보스 공용 패턴(A1, A2, A5, A7): 공격 태스크로 패턴 몽타주를 패턴 재생 속도로 공격 데이터와 함께 재생하고, 정지
 * 간격(보스 사양 '이동')을 지키며 몽타주의 워핑 구간(A1·A5 1타, A7) 동안 다가갑니다. 모션이 끝나면 끝납니다. 공격 예고 섬광은
 * 몽타주의 GameplayCue 노티파이가 켭니다. 부여된 스펙마다 패턴 ID를 동적 태그로 가지며, 쿨다운은 UAgGameplayAbility가 그
 * 패턴 ID로 겁니다.
 */
UCLASS()
class UAgBossPattern_Common : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPattern_Common();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleAttackCompleted();

	UFUNCTION()
	void HandleAttackInterrupted();
};
