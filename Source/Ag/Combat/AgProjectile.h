// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgProjectileData.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "AgProjectile.generated.h"

class UShapeComponent;
class AAgCharacterBase;

/** 판정 모양은 BP에 둔 모양 컴포넌트(상자 또는 캡슐) 하나입니다. */
UCLASS(Abstract)
class AAgProjectile : public AActor
{
	GENERATED_BODY()

public:
	AAgProjectile();

	void Launch(const FAgDamage& InDamage, const FGameplayEffectSpecHandle& InDamageSpec, const FVector& Direction, const FAgProjectileParams& InParams);
	void Reflect(AAgCharacterBase& Defender);
	bool IsReflected() const;

protected:
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyHit(AAgCharacterBase& Target, const FVector& HitLocation);

	UPROPERTY()
	TObjectPtr<UShapeComponent> HitShape;

	TArray<TWeakObjectPtr<AActor>> TouchedActors;
	TWeakObjectPtr<AAgCharacterBase> Reflector;

	FAgDamage Damage;

	/** 여러 대상에 다시 쓰므로, 대상별 결과와 적중 위치는 이 스펙의 복사본에만 기록합니다. */
	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpec;
	FAgProjectileParams Params;
	FVector FlightDirection = FVector::ForwardVector;
	float Traveled = 0.f;
};
