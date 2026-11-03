// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgDropdownOptionWidget.h"
#include "AgEnemyCard.generated.h"

class UImage;

/** 적 드롭다운의 닫힌 칸입니다. 고른 적의 모습을, 고른 적이 없으면 빈 자리 표시를 보입니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgEnemyCard : public UAgDropdownOptionWidget
{
	GENERATED_BODY()

public:
	virtual void SetOption(FName Key, const FText& Label) override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ModelImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> EmptyMark;
};
