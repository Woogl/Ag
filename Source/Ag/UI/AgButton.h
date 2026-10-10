// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "AgButton.generated.h"

class UCommonTextBlock;

/**
 * Screen UI button (UI 스타일 '버튼').
 * The focused button is the selected one: hovering focuses it, and keyboard navigation moves focus,
 * so mouse and keyboard always continue from the same button. The look comes from the button style asset.
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
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;

	/** 버튼에 표시할 글자 */
	UPROPERTY(EditAnywhere, Category = "Button")
	FText ButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LabelText;

	/** Shown only while the button is selected (the arrow in the screen figures). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SelectionMarker;

private:
	void UpdateSelectionMarker();
};
