// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "AgTabButton.generated.h"

class UCommonTextBlock;
class UWidget;

/**
 * 탭 목록의 탭 이름입니다. 고른 탭은 버튼 스타일의 선택 글자와 선택 표시(탭 바는 밑줄, 분류 목록은 세로 막대)로 보입니다.
 * 탭 바의 이름은 포커스를 받지 않고, 분류 목록의 이름은 위젯 블루프린트 기본값에서 포커스를 받으면 선택되게 합니다.
 */
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
	TObjectPtr<UWidget> SelectionMark;
};
