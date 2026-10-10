// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "AgGameplayCue_CameraShake.generated.h"

class UCameraShakeBase;

/** Plays one camera shake level on the local player's camera (카메라 '카메라 셰이크'). The shake asset is data on the cue blueprint. */
UCLASS(Abstract)
class UAgGameplayCue_CameraShake : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Shake")
	TSubclassOf<UCameraShakeBase> Shake;
};
