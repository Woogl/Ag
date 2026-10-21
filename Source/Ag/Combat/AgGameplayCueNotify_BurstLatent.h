// Copyright Woogle. All Rights Reserved.

#pragma once

#include "GameplayCueNotify_BurstLatent.h"
#include "AgGameplayCueNotify_BurstLatent.generated.h"

class UAgPostProcessComponent;

UCLASS(Abstract)
class AAgGameplayCueNotify_BurstLatent : public AGameplayCueNotify_BurstLatent
{
	GENERATED_BODY()

public:
	AAgGameplayCueNotify_BurstLatent();

protected:
	virtual bool Recycle() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(VisibleAnywhere, Category = "Screen Effect")
	TObjectPtr<UAgPostProcessComponent> PostProcess;
};
