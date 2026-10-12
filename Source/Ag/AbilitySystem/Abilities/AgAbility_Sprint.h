// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Sprint.generated.h"

/**
 * 달리기: 회피 후딜에 Shift와 이동 입력이 있으면 회피를 캔슬하고 시작해, 플레이어 캐릭터가 취소하거나(Shift를 뗌, 이동
 * 입력 없음, SP 0) 점프가 아닌 행동이나 피격 반응이 시작될 때까지 이어집니다. 실행 중에는 State.Sprinting이 속도를
 * 올리고, 캐릭터를 이동 방향으로 돌리고, 달리기 SP 소모 효과를 돌립니다.
 */
UCLASS()
class UAgAbility_Sprint : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Sprint();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
};
