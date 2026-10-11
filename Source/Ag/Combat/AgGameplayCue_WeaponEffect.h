// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "AgGameplayCue_WeaponEffect.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 큐가 켜진 동안 대상 캐릭터의 무기에 반복 이펙트를 붙입니다(붉은 섬광, 예고 섬광). 이펙트와 색은 큐 블루프린트의 데이터입니다.
 */
UCLASS(Abstract)
class AAgGameplayCue_WeaponEffect : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AAgGameplayCue_WeaponEffect();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TObjectPtr<UNiagaraSystem> Effect;

	/** Socket on the weapon to attach to; none attaches at the weapon's origin. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	FName AttachSocket;

	/** Tints the effect through one of its color user parameters, so cues can share an effect and differ in color. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	bool bSetColor = false;

	/** The effect's color user parameter. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (EditCondition = "bSetColor"))
	FName ColorParameter = TEXT("Color");

	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (EditCondition = "bSetColor"))
	FLinearColor Color = FLinearColor::White;

private:
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedEffect;
};
