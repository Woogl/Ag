// Copyright Woogle. All Rights Reserved.

#pragma once

#include "GameplayCueNotify_Burst.h"
#include "AgGameplayCueNotify_Burst.generated.h"

UCLASS(Abstract)
class UAgGameplayCueNotify_Burst : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

public:
	static void PlayCombatCameraReactions(AActor* Target, const FGameplayCueParameters& Parameters);

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

	/** 비우지 않으면 큐의 RawMagnitude를 생성한 나이아가라의 이 실수 파라미터에 넣습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "GCN Effects")
	FName MagnitudeNiagaraParameter;
};
