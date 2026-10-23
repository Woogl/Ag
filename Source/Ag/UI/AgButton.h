// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "AgButton.generated.h"

class UCommonTextBlock;

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
	virtual void NativeOnUnhovered() override;
	virtual void HandleFocusLost() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditAnywhere, Category = "Button")
	FText ButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LabelText;
};
