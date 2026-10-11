// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AgDamageExecution.generated.h"

/**
 * 대미지 GE의 계산: 전투 시스템 '피격 처리 순서'로 결과를 정하고 '대미지'의 계산식으로 HP·PP·SP를 바꿉니다.
 * 결과는 극한 회피, 무적(아무 일 없음), 패리, 가드 또는 가드 브레이크, 일반 피격 순으로 대상의 상태 태그와 공격의 성질
 * (스펙 태그 Effect.Damage.Unguardable·Unparryable)을 보고 정하며, 스펙에 Effect.Damage.Result 태그로 남깁니다.
 * 공격의 배율은 SetByCaller(Data.DamageMultiplier, Data.PoiseMultiplier)로 받습니다.
 */
UCLASS()
class UAgDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UAgDamageExecution();

	/** 'HP 감소량 계산식'에 가드 경감 배율을 곱해 반올림한 정수 */
	static int32 CalculateHPDamage(float SourceATK, float DamageMultiplier, float TargetDEF, float DefenseConstant, float GuardReduction = 1.f);

	/** 'PP 감소량 계산식'을 반올림한 정수 */
	static int32 CalculatePPDamage(float SourceATK, float PoiseMultiplier, float TargetDEF, float DefenseConstant);

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
