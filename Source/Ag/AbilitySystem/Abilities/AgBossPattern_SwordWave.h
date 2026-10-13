// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Data/AgAttackData.h"
#include "AgBossPattern_SwordWave.generated.h"

/** SwordWave의 전용 값 (보스 사양 '검기') */
USTRUCT()
struct FAgBossSwordWaveParams : public FAgBossPatternParams
{
	GENERATED_BODY()

	/** 검기의 속도와 최대 사거리 */
	UPROPERTY(EditAnywhere, Category = "Sword Wave")
	FAgProjectileParams Wave;
};

/**
 * SwordWave: 세 번 휘두르며, 휘두를 때마다 발사 노티파이에서 그 타의 공격 데이터로 검기 하나를 정면으로 쏩니다(투사체 발사
 * 태스크). 보스 캐릭터가 발사를 판정 구간처럼 보아 발사마다 회전 정지 시간만큼 앞서 회전을 멈추므로, 검기는 그때 바라본
 * 쪽으로 날아갑니다. 정지 간격은 공용 패턴과 같이 지킵니다.
 */
UCLASS()
class UAgBossPattern_SwordWave : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPattern_SwordWave();

	virtual const UScriptStruct* GetAbilityParamsType() const override { return FAgBossSwordWaveParams::StaticStruct(); }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();
};
