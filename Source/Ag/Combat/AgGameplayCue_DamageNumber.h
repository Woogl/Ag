// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "AgGameplayCue_DamageNumber.generated.h"

class UAgDamageFloaterWidget;

/**
 * 전투 HUD '대미지 숫자': 플레이어가 준 HP 감소량(큐의 RawMagnitude)을 맞은 위치에 띄웁니다. 플레이어가 받은 대미지는
 * 띄우지 않습니다. 위젯은 큐 블루프린트의 데이터입니다.
 */
UCLASS(Abstract)
class UAgGameplayCue_DamageNumber : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

protected:
	/** 대미지 숫자 위젯(WBP_DamageFloater) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgDamageFloaterWidget> FloaterWidget;
};
