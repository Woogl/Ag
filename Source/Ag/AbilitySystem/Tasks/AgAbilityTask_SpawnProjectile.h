// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAttackData.h"
#include "Templates/SubclassOf.h"
#include "AgAbilityTask_SpawnProjectile.generated.h"

class AAgProjectile;

/**
 * 투사체 발사(보스 SwordWave): 몽타주의 투사체 발사 노티파이(Event.ProjectileRelease)마다 공격 데이터를 차례로 하나씩 실어
 * 투사체를 쏩니다. 투사체는 캐릭터 앞의 바닥에 스폰되어 캐릭터 정면으로 날아갑니다. 발사 전 회전 정지는 캐릭터가 맡습니다.
 */
UCLASS()
class UAgAbilityTask_SpawnProjectile : public UAbilityTask
{
	GENERATED_BODY()

public:
	/** 플레이어 공격의 타마다 InClass 투사체 하나를 InParams(속도, 최대 사거리)로 날립니다. */
	static UAgAbilityTask_SpawnProjectile* SpawnProjectiles(UGameplayAbility* OwningAbility, TSubclassOf<AAgProjectile> InClass, TConstArrayView<FAgPlayerAttackHit> InHits, const FAgProjectileParams& InParams);

	/** 보스 패턴의 타마다 투사체 하나. 플레이어 공격에만 있는 값은 기본값입니다. */
	static UAgAbilityTask_SpawnProjectile* SpawnProjectiles(UGameplayAbility* OwningAbility, TSubclassOf<AAgProjectile> InClass, TConstArrayView<FAgAttackHit> InHits, const FAgProjectileParams& InParams);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleRelease(const FGameplayEventData* Payload);

	TSubclassOf<AAgProjectile> ProjectileClass;

	/** 발사 노티파이마다 하나씩, 몽타주 순서대로 */
	TArray<FAgPlayerAttackHit> Hits;

	FAgProjectileParams Params;

	/** 지금까지 받은 발사 노티파이 수 */
	int32 NumReleased = 0;

	FDelegateHandle ReleaseHandle;
};
