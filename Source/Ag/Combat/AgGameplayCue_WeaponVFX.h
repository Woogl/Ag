// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "AgGameplayCue_WeaponVFX.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;

/**
 * 큐가 켜진 동안 대상 캐릭터의 무기에 반복 VFX를 붙이고, 켜질 때 효과음을 한 번 재생합니다(가드 불가 예고, 패리 예고).
 * VFX, 색, 효과음은 큐 블루프린트의 데이터입니다.
 */
UCLASS(Abstract)
class AAgGameplayCue_WeaponVFX : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AAgGameplayCue_WeaponVFX();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> VFX;

	/** 붙일 무기 소켓. 비우면 무기 원점에 붙입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FName AttachSocket;

	/** VFX의 색 사용자 파라미터. 큐마다 같은 VFX를 다른 색으로 재생합니다(구현 계획서 '연출'). */
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FName ColorParameter = TEXT("Color");

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FLinearColor Color = FLinearColor::White;

	/** 큐가 켜질 때 무기에서 한 번 재생하는 효과음. 큐가 꺼져도 끝까지 재생합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> Sound;

private:
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedVFX;
};
