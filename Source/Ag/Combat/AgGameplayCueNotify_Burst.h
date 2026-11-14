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
};
