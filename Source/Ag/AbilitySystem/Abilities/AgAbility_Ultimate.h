// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Math/Interval.h"
#include "AgAbility_Ultimate.generated.h"

class UAgAbilityTask_TimeStop;

/** 궁극기의 전용 값 (플레이어 사양 '궁극기', '베기 선') */
USTRUCT()
struct FAgUltimateParams : public FAgActionParams
{
	GENERATED_BODY()

	/** 마무리의 1타 */
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	FAgPlayerAttackHit Hit;

	/** 범위 (플레이어와 적의 캡슐 중심 사이 수평 거리) */
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (Units = "cm", ClampMin = 0))
	float Radius = 0.f;

	/** 베기 선: 위치. 카메라가 보는 쪽으로 플레이어 앞 이 거리 안 (cm) */
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	FFloatInterval SlashDistance = FFloatInterval(0.f, 0.f);

	/** 베기 선: 위치. 바닥에서 이 높이 안 (cm) */
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	FFloatInterval SlashHeight = FFloatInterval(0.f, 0.f);
};

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

	virtual const UScriptStruct* GetActionParamsType() const override { return FAgUltimateParams::StaticStruct(); }
	virtual FAgAbilityCost GetCost() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;

private:
	/** 컷신이 끝나면 게임 카메라로 돌아오고 베기 단계(Judgment 섹션)가 시작됩니다. */
	UFUNCTION()
	void HandleCutsceneFinished();

	/** 베기 선 노티파이(Event.UltimateSlash)마다 베기 선 하나를 새깁니다. 이 궁극기의 몽타주에서 온 것만 받습니다. */
	UFUNCTION()
	void HandleSlash(FGameplayEventData Payload);

	/** 마무리: 칼이 칼집에 들어가는 순간(Event.UltimateFinish) */
	UFUNCTION()
	void HandleFinish(FGameplayEventData Payload);

	/** 후딜 시작 노티파이(칼 뽑기). 이 궁극기의 몽타주에서 온 것만 받습니다. */
	UFUNCTION()
	void HandleRecoveryStarted(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

	void SpawnSlash();

	/** 시작부터 마무리까지 있습니다. */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_TimeStop> TimeStopTask;
};
