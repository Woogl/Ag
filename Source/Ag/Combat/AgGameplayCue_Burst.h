// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "AgGameplayCue_Burst.generated.h"

class UNiagaraSystem;

/** Plays a one-shot effect where the cue says (피격 이펙트, 패리 이펙트). The effect and its scale are data on the cue blueprint. */
UCLASS(Abstract)
class UAgGameplayCue_Burst : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TObjectPtr<UNiagaraSystem> Effect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	FVector Scale = FVector::OneVector;
};
