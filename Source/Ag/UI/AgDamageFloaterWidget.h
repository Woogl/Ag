// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgDamageFloaterWidget.generated.h"

class UCommonTextBlock;
class UCommonTextStyle;
class UWidgetAnimation;

/**
 * WBP_DamageFloater의 부모(전투 HUD '대미지 숫자'). 위젯이 숫자의 모양과 투명해지는 애니메이션을 갖고, 그 길이가
 * 표시 시간입니다. 그동안 AAgDamageFloater가 숫자를 위로 옮깁니다.
 */
UCLASS(Abstract)
class UAgDamageFloaterWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** Amount를 정수로 표시하고 투명해지기 시작합니다. bLarge면 큰 글자 스타일로 바꿉니다. */
	void SetDamage(int32 Amount, bool bLarge, float GameSpeed);

	/** 투명해지는 속도를 게임 시간 배율(슬로우모션)에 맞춥니다. */
	void SetGameSpeed(float GameSpeed);

	/** 표시 시간: 투명해지는 애니메이션의 길이입니다. */
	float GetLifetime() const;

	float GetRiseHeight() const { return RiseHeight; }

protected:
	/** 숫자. 자기 글자 스타일이 일반 크기입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DamageText;

	/** 숫자가 투명해지는 애니메이션. */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeAnimation;

	/** 크게 표시하는 타(스킬·궁극기·처형)의 글자 스타일. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater")
	TSubclassOf<UCommonTextStyle> LargeStyle;

	/** 표시 시간 동안 떠오르는 높이. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater", meta = (Units = "cm", ClampMin = 0))
	float RiseHeight = 0.f;
};
