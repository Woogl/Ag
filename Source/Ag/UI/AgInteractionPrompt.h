// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgInteractionPrompt.generated.h"

class UTextBlock;

/** 상호작용 안내입니다. 키 표시와 행동 이름의 모양과 배치는 WBP가 정합니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgInteractionPrompt : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetAction(const FText& Text);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionText;
};
