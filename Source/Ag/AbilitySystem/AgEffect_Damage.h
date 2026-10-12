// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_Damage.generated.h"

class AAgCharacterBase;
struct FAgPlayerAttackHit;

/**
 * 타 하나의 적중(전투 시스템 '피격 처리 순서'와 '대미지'). 적용하면 피격 처리가 GE 안에서 끝납니다.
 * - 죽은 대상에는 적용되지 않습니다(대상 태그 조건 컴포넌트).
 * - UAgDamageExecution이 피격 결과를 정해 HP·PP·SP를 바꾸고, 결과를 스펙 태그로 남깁니다.
 * - 사망·그로기 판정은 UAgAttributeSet이, 결과에 따른 반응과 적중 후 처리는 UAgHitResultGameplayEffectComponent가 합니다.
 */
UCLASS()
class UAgEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_Damage();

	/** Attacker가 Hit을 Target에 맞힙니다. Causer는 밀려나는 기준(공격자 또는 투사체), HitLocation은 맞은 위치입니다. */
	static void ApplyAttack(AAgCharacterBase* Attacker, AActor* Causer, AAgCharacterBase* Target, const FAgPlayerAttackHit& Hit, const FVector& HitLocation);

	/**
	 * 타 하나의 공격 데이터(배율, 피격 반응, 가드 가능 여부, 마지막 일격, MP·UP 충전)와 맞은 위치를 대미지 GE
	 * 스펙에 담습니다.
	 */
	static void SetAttack(FGameplayEffectSpec& Spec, const FAgPlayerAttackHit& Hit, const FVector& HitLocation);
};
