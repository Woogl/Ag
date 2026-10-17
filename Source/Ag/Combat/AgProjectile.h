// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "AgProjectile.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AAgCharacterBase;

/**
 * 투사체 (전투 시스템 '검기'): 투사체 발사 태스크(UAgAbilityTask_SpawnProjectile)가 쏩니다. 지정 방향으로 곧게 날아가며, 닿은
 * 적을 한 번씩 판정하고 뚫고 지나갑니다. 벽에 닿거나 최대 사거리에 이르거나 쏜 캐릭터가 죽으면 사라지고, 쏜 캐릭터의 다른
 * 상태 변화와는 관계없이 날아갑니다. 판정 상자의 크기(폭, 높이)와 모습(메시, 머티리얼)은 블루프린트(BP_SwordWave)에서
 * 정하고, 속도와 사거리는 발사할 때 받습니다.
 */
UCLASS(Abstract)
class AAgProjectile : public AActor
{
	GENERATED_BODY()

public:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FAgProjectileEffectTest;
#endif
	AAgProjectile();

	/** 발사 Ability가 만든 피해 스펙을 보관하고, 스폰 변환을 유지한 채 Direction 쪽으로 날립니다. */
	void Launch(const FGameplayEffectSpecHandle& InDamageSpec, const FVector& Direction, const FAgProjectileParams& InParams);
	float GetCollisionHalfHeight() const;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyHit(AAgCharacterBase& Target, const FVector& HitLocation);

	/** 판정 상자. 바닥이 지면에 닿고, 검사에만 씁니다. X는 날아가는 방향의 두께, Y는 폭입니다. */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UBoxComponent> HitBox;

	/** 모습. 블루프린트에서 판정 상자에 겹쳐 놓습니다. */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 이미 닿은 캐릭터. 하나당 한 번만 판정합니다. */
	TArray<TWeakObjectPtr<AActor>> TouchedActors;

	/** 발사 출처와 공격 설정. 대상별 결과와 적중 위치는 이 스펙의 복사본에만 기록합니다. */
	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpec;
	FAgProjectileParams Params;
	FVector FlightDirection = FVector::ForwardVector;
	float Traveled = 0.f;
};
