// Copyright Woogle. All Rights Reserved.

#include "UI/AgDropdownOptionWidget.h"

#include "CommonTextBlock.h"

void UAgDropdownOptionWidget::SetOption(FName Key, const FText& Label)
{
	NameText->SetText(Label);
}
