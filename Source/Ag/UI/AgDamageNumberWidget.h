// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgDamageNumberWidget.generated.h"

class UCommonTextBlock;
class UCommonTextStyle;

/** Parent of WBP_DamageNumber (전투 HUD '대미지 숫자'). The widget holds the number's look and timing; AAgDamageNumber moves it. */
UCLASS(Abstract)
class UAgDamageNumberWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** Shows Amount as an integer; bLarge switches to the large text style. */
	void SetDamage(int32 Amount, bool bLarge);

	float GetLifetime() const { return Lifetime; }
	float GetRiseHeight() const { return RiseHeight; }

protected:
	/** The number. Its own text style is the normal size. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DamageText;

	/** Text style of the hits shown large (스킬·궁극기·처형). */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	TSubclassOf<UCommonTextStyle> LargeStyle;

	/** How long the number shows while it rises and fades out. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number", meta = (Units = "s", ClampMin = 0.01))
	float Lifetime = 1.f;

	/** How far the number rises over its lifetime. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number", meta = (Units = "cm", ClampMin = 0))
	float RiseHeight = 0.f;
};
