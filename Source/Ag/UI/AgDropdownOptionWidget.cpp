// Copyright Woogle. All Rights Reserved.

#include "UI/AgDropdownOptionWidget.h"

#include "CommonTextBlock.h"

void UAgDropdownOptionWidget::SetLabel(const FText& Label)
{
	NameText->SetText(Label);
}
