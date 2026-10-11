// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "AgHitResultGameplayEffectComponent.generated.h"

/**
 * 대미지 GE의 피격 결과 처리(전투 시스템 '피격 처리 순서'). 실행 계산이 스펙에 남긴 결과 태그를 보고, GE가 실행된 뒤
 * 극한 회피·패리 이벤트, 피격 반응과 가드 반응, 막힘 알림, 적중 후 처리(이펙트, 카메라 셰이크, 대미지 숫자, 히트스톱,
 * 적중의 MP·UP 충전)를 처리합니다. 극한 회피와 패리의 보상은 그 이벤트를 받는 회피·가드 어빌리티가 줍니다.
 * 이번 타격으로 죽었거나 그로기가 되면 반응하지 않습니다(상태 우선순위).
 */
UCLASS(DisplayName = "Ag Hit Result")
class UAgHitResultGameplayEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
