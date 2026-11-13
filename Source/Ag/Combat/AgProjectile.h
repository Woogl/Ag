// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/AgAbilitySpawnedActor.h"
#include "Data/AgAttackData.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "AgProjectile.generated.h"

class UShapeComponent;
class AAgCharacterBase;

/**
 * 판정 모양은 BP에 둔 모양 컴포넌트(상자·캡슐) 전부이며 섞어 둘 수 있습니다. 생성되는 순간 어빌리티의 행동 대상이 지금 속도로 움직인다고 보고
 * 닿을 위치(예측 지점)를 향해 날아가며, 대상이 없으면 생성 방향으로 날아갑니다. FlightTime이 있으면 거리와 관계없이 그 시간에
 * 닿도록 속도를 정합니다.
 */
UCLASS(Abstract)
class AAgProjectile : public AActor, public IAgAbilitySpawnedActor
{
	GENERATED_BODY()

public:
	AAgProjectile();

	virtual void OnSpawnedByAbility(const UAgGameplayAbility& Ability) override;
	void Reflect(AAgCharacterBase& Defender);
	/** 적중·가드로 피해를 준 뒤 불립니다. */
	void OnDamageApplied();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool IsReflected() const;

	// 값은 SpawnActors.json의 템플릿에만 둡니다. EditInstanceOnly는 JSON 리더가 채우고 BP 기본값에서는 고칠 수 없습니다.
	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float Speed = 0.f;

	/** 0보다 크면 발사 때 예측 지점까지 이 시간(초)에 닿도록 Speed를 다시 정합니다. 대상이 없으면 Speed 그대로 날아갑니다. */
	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float FlightTime = 0.f;

	/** FlightTime으로 정한 속도의 하한과 상한입니다. 한도에 걸리면 한도 속도로 날아가 도착 시간이 FlightTime과 달라집니다. */
	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float MinSpeed = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float MaxSpeed = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float Range = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	FAgDamage Damage;

	/** 꺼져 있으면 피해를 준 대상을 뚫고 지나갑니다. */
	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	bool bDestroyOnHit = false;

	UPROPERTY()
	TArray<TObjectPtr<UShapeComponent>> HitShapes;

	TWeakObjectPtr<AActor> AimTarget;
	TArray<TWeakObjectPtr<AActor>> TouchedActors;
	TWeakObjectPtr<AAgCharacterBase> Reflector;

	/** 엔진이 적용할 때 스펙을 복사해 실행하므로, 적중마다 적중 위치만 바꿔 다시 씁니다. */
	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpec;
	FVector FlightDirection = FVector::ForwardVector;
	float Traveled = 0.f;
};
