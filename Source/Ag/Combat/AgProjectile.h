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

/** 판정 모양은 BP에 둔 모양 컴포넌트(상자 또는 캡슐) 하나입니다. 생성 방향으로 날아갑니다. */
UCLASS(Abstract)
class AAgProjectile : public AActor, public IAgAbilitySpawnedActor
{
	GENERATED_BODY()

public:
	AAgProjectile();

	virtual void OnSpawnedByAbility(const UAgGameplayAbility& Ability) override;
	void Reflect(AAgCharacterBase& Defender);
	bool IsReflected() const;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyHit(AAgCharacterBase& Target, const FVector& HitLocation);

	// 값은 SpawnActors.json의 템플릿에만 둡니다. EditInstanceOnly는 JSON 리더가 채우고 BP 기본값에서는 고칠 수 없습니다.
	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float Speed = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float Range = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	FAgDamage Damage;

	UPROPERTY()
	TObjectPtr<UShapeComponent> HitShape;

	TArray<TWeakObjectPtr<AActor>> TouchedActors;
	TWeakObjectPtr<AAgCharacterBase> Reflector;

	/** 여러 대상에 다시 쓰므로, 대상별 결과와 적중 위치는 이 스펙의 복사본에만 기록합니다. */
	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpec;
	FVector FlightDirection = FVector::ForwardVector;
	float Traveled = 0.f;
};
