// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgDamageFloaterWidget.generated.h"

class UCommonTextBlock;
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
	/** Amount를 정수로 표시하고 투명해지기 시작합니다. */
	void SetDamage(int32 Amount);

	/** 표시 시간: 투명해지는 애니메이션의 길이입니다. */
	float GetLifetime() const;

	float GetRiseHeight() const
	{
		return RiseHeight;
	}

protected:
	/** 숫자. 글자 크기는 자기 글자 스타일이 정합니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DamageText;

	/** 숫자가 투명해지는 위젯 애니메이션 */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeAnimation;

	/** 표시 시간 동안 떠오르는 높이 */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater", meta = (Units = "cm", ClampMin = 0))
	float RiseHeight = 0.f;
};
