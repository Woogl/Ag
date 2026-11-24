// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "AgTabButton.generated.h"

class UCommonTextBlock;
class UWidget;

/** 탭 바의 탭 이름입니다. 고른 탭은 버튼 스타일의 선택 글자와 밑줄로 보입니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgTabButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UAgTabButton(const FObjectInitializer& ObjectInitializer);

	void SetLabel(const FText& Label);

protected:
	virtual void NativeOnCurrentTextStyleChanged() override;
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Underline;
};
