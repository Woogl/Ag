// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/AgAbilitySpawnedActor.h"
#include "Data/AgAttackData.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "AgShockwave.generated.h"

class UNiagaraComponent;

/**
 * 지면에서 모든 수평 방향으로 퍼지는 고리입니다. Hits마다 한 번씩 퍼지며, 앞 파동이 최대 반경에 닿고 Interval초 뒤에 다음 파동이 퍼집니다.
 * 외형 메시가 있으면 반경·높이 1m 원통으로 만들고, 판정 반경·높이에 맞춰 액터 배율을 바꿉니다. 너비는 사용자 지정 프리미티브 데이터 0번(안쪽 반경 ÷ 바깥 반경)으로 머티리얼에 넘깁니다.
 * 나이아가라 외형에는 최대 반경(User.Radius), 첫 파동부터 마지막 파동까지의 시간(User.Duration), 파동이 시작하는 간격(User.Interval)을 넘깁니다.
 */
UCLASS(Abstract)
class AAgShockwave : public AActor, public IAgAbilitySpawnedActor
{
	GENERATED_BODY()

public:
	AAgShockwave();

	virtual void OnSpawnedByAbility(const UAgGameplayAbility& Ability) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateLook();
	void DestroyWhenEffectsFinish();
	UFUNCTION()
	void HandleEffectFinished(UNiagaraComponent* Effect);

	// 값은 SpawnActors.json의 템플릿에만 둡니다. EditInstanceOnly는 JSON 리더가 채우고 BP 기본값에서는 고칠 수 없습니다.
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Speed = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float MaxRadius = 0.f;

	/** 지면에서 판정이 닿는 높이입니다. */
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Height = 0.f;

	/** 바깥 가장자리에서 안쪽으로 판정이 닿는 너비입니다. */
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Width = 0.f;

	/** 앞 파동이 최대 반경에 닿은 뒤 다음 파동이 퍼지기 시작할 때까지의 시간입니다. */
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Interval = 0.f;

	/** 파동마다 공격 데이터입니다. 파동 수는 배열 길이입니다. */
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	TArray<FAgDamage> Hits;

	/** 파동마다 하나이며 모두 같은 적중 상태를 가리킵니다. 엔진이 적용할 때 스펙을 복사해 실행하므로, 적중마다 적중 위치만 바꿔 다시 씁니다. */
	UPROPERTY()
	TArray<FGameplayEffectSpecHandle> HitSpecs;

	TArray<TWeakObjectPtr<AActor>> TouchedActors;
	float Radius = 0.f;
	int32 Wave = 0;
	float WaitTime = 0.f;
};
