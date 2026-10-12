// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Data/AgAttackData.h"
#include "AgBossPattern_Backstep.generated.h"

/** Backstep의 전용 값 (보스 사양 'Backstep'). 발동 조건(FAgBossAICondition_BackstepReady)도 읽습니다. */
USTRUCT()
struct FAgBossBackstepParams : public FAgBossPatternParams
{
	GENERATED_BODY()

	/** 플레이어와의 거리: 플레이어가 이 안에 있어야 발동합니다 (캡슐 중심 사이 수평 거리). */
	UPROPERTY(EditAnywhere, Category = "Backstep", meta = (Units = "cm", ClampMin = 0))
	float Range = 0.f;

	/** 발동 확률: 플레이어가 평타의 각 타나 점프 공격을 시작할 때마다 검사합니다. */
	UPROPERTY(EditAnywhere, Category = "Backstep", meta = (ClampMin = 0, ClampMax = 1))
	float Chance = 0.f;

	/** 물러나는 거리: 모션의 루트 모션 이동을 이 거리에 맞춥니다. */
	UPROPERTY(EditAnywhere, Category = "Backstep", meta = (Units = "cm", ClampMin = 0))
	float Distance = 0.f;
};

/**
 * Backstep: 모션의 루트 모션으로 물러나되, 루트 모션 배율로 거리를 Backstep 거리에 맞춥니다. 공격 패턴이 아니라서 모션 재생
 * 속도는 받지 않습니다. 정지 간격을 두지 않습니다. AI가 플레이어의 평타·점프 공격에 맞춰 시작합니다.
 */
UCLASS()
class UAgBossPattern_Backstep : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPattern_Backstep();

	virtual const UScriptStruct* GetPatternParamsType() const override { return FAgBossBackstepParams::StaticStruct(); }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();
};
