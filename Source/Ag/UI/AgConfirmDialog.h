// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgConfirmDialog.generated.h"

class UAgButton;
class UCommonTextBlock;

/**
 * 문구와 Yes, No 버튼이 있는 확인 창입니다. Modal 레이어에 엽니다. Yes면 창을 닫고 OnConfirmed를 부르며, No와 Esc는 창만
 * 닫습니다. 스택이 닫은 창을 다시 쓰므로, 여는 쪽이 열 때마다 문구와 OnConfirmed를 다시 정합니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgConfirmDialog : public UAgMenuScreen
{
	GENERATED_BODY()

public:
	UAgConfirmDialog(const FObjectInitializer& ObjectInitializer);

	void SetMessage(const FText& Message);

	FSimpleDelegate OnConfirmed;

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> MessageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> YesButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> NoButton;

private:
	void Confirm();
};
