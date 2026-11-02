// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgDropdownOptionWidget.generated.h"

class UCommonTextBlock;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgDropdownOptionWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetLabel(const FText& Label);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> NameText;
};
