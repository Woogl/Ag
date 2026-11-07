// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgDropdownOptionWidget.h"
#include "AgEnemyCard.generated.h"

class UImage;

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
