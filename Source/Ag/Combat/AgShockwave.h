// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/AgAbilitySpawnedActor.h"
#include "Data/AgAttackData.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "AgShockwave.generated.h"

class AAgCharacterBase;

/** 지면에서 모든 수평 방향으로 퍼지는 고리입니다. BP 외형은 반경·높이 1m 고리로 만들고, 판정 반경·높이에 맞춰 배율을 바꿉니다. */
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
	void UpdateScale();

	// 값은 SpawnActors.json의 템플릿에만 둡니다. EditInstanceOnly는 JSON 리더가 채우고 BP 기본값에서는 고칠 수 없습니다.
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Speed = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float MaxRadius = 0.f;

	/** 지면에서 판정이 닿는 높이입니다. */
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Height = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	FAgDamage Damage;

	/** 엔진이 적용할 때 스펙을 복사해 실행하므로, 적중마다 적중 위치만 바꿔 다시 씁니다. */
	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpec;

	TArray<TWeakObjectPtr<AActor>> TouchedActors;
	float Radius = 0.f;
};
