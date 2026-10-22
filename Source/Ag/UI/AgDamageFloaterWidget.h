// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgDamageFloaterWidget.generated.h"

class UCommonTextBlock;
class UWidgetAnimation;

UCLASS(Abstract)
class UAgDamageFloaterWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetDamage(int32 Amount);

	float GetLifetime() const;

	float GetRiseHeight() const
	{
		return RiseHeight;
	}

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DamageText;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater", meta = (ClampMin = 0))
	float RiseHeight = 0.f;
};
