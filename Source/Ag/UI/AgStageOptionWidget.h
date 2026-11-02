// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgStageOptionWidget.generated.h"

class UCommonTextBlock;
struct FAgStage;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgStageOptionWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetStage(const FAgStage& Stage);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> NameText;
};
