// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "AgGameplayCueNotify_DamageFloater.generated.h"

class UAgDamageFloaterWidget;

UCLASS(Abstract)
class UAgGameplayCueNotify_DamageFloater : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgDamageFloaterWidget> FloaterWidget;
};
