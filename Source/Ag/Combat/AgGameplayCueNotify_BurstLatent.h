// Copyright Woogle. All Rights Reserved.

#pragma once

#include "GameplayCueNotify_BurstLatent.h"
#include "AgGameplayCueNotify_BurstLatent.generated.h"

class UAgPostProcessComponent;

/** 포스트프로세스 머티리얼이 있으면 실행 중인 슬로우모션의 길이만큼 재생하므로, 슬로우모션 효과를 건 뒤에 실행합니다. */
UCLASS(Abstract)
class AAgGameplayCueNotify_BurstLatent : public AGameplayCueNotify_BurstLatent
{
	GENERATED_BODY()

public:
	AAgGameplayCueNotify_BurstLatent();

protected:
	virtual bool Recycle() override;
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(VisibleAnywhere, Category = "Screen Effect")
	TObjectPtr<UAgPostProcessComponent> PostProcess;
};
