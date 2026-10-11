// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "AgButton.generated.h"

class UCommonTextBlock;

/**
 * 화면 UI 버튼 (UI 스타일 '버튼').
 * 포커스를 가진 버튼이 선택된 버튼입니다. 마우스를 올리면 포커스를 가져오고 키보드 내비게이션도 포커스를 옮기므로,
 * 마우스와 키보드가 늘 같은 버튼에서 이어집니다. 선택된 모양을 포함한 상태별 모양은 버튼 스타일 에셋이 정합니다.
 */
UCLASS(Abstract)
class UAgButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UAgButton(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnCurrentTextStyleChanged() override;
	virtual void NativeOnHovered() override;
	virtual void HandleFocusLost() override;

	/** 버튼에 표시할 글자 */
	UPROPERTY(EditAnywhere, Category = "Button")
	FText ButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LabelText;
};
