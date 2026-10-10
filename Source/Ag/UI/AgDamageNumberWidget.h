// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgDamageNumberWidget.generated.h"

class UCommonTextBlock;
class UCommonTextStyle;
class UWidgetAnimation;

/**
 * Parent of WBP_DamageNumber (전투 HUD '대미지 숫자'). The widget holds the number's look and its fade-out animation,
 * whose length is how long the number shows; AAgDamageNumber moves it up meanwhile.
 */
UCLASS(Abstract)
class UAgDamageNumberWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** Shows Amount as an integer and starts the fade; bLarge switches to the large text style. */
	void SetDamage(int32 Amount, bool bLarge, float GameSpeed);

	/** Keeps the fade at the game's time scale (슬로우모션). */
	void SetGameSpeed(float GameSpeed);

	/** How long the number shows: the fade animation's length. */
	float GetLifetime() const;

	float GetRiseHeight() const { return RiseHeight; }

protected:
	/** The number. Its own text style is the normal size. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DamageText;

	/** The number fading out. */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeAnimation;

	/** Text style of the hits shown large (스킬·궁극기·처형). */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	TSubclassOf<UCommonTextStyle> LargeStyle;

	/** How far the number rises over its lifetime. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number", meta = (Units = "cm", ClampMin = 0))
	float RiseHeight = 0.f;
};
