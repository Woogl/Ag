// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "AgHitResultGameplayEffectComponent.generated.h"

/**
 * 대미지 GE의 피격 결과 처리(전투 시스템 '피격 처리 순서'). 실행 계산이 스펙에 남긴 결과 태그를 보고, GE가 실행된 뒤
 * 극한 회피(구간 닫기, 환급, 보상, 회피 반격 기회, 슬로우모션)와 패리(공격한 쪽의 PP 감소, 패리 구간 닫기, 보상, 패리 모션),
 * 피격 반응과 가드 반응, 막힘 알림, 적중 후 처리(이펙트와 효과음, 카메라 셰이크, 대미지 숫자, 히트스톱, 궁극기 마무리의
 * 슬로우모션, 적중의 MP·UP 충전)를 처리합니다. 보상의 값은 효과 데이터에 있습니다. 이번 타격으로 죽었거나 그로기가 되면
 * 반응하지 않습니다(상태 우선순위).
 */
UCLASS(DisplayName = "Ag Hit Result")
class UAgHitResultGameplayEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec,
	                                      FPredictionKey& PredictionKey) const override;
};
