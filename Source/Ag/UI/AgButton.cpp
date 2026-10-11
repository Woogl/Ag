// Copyright Woogle. All Rights Reserved.

#include "UI/AgButton.h"

#include "CommonTextBlock.h"

UAgButton::UAgButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSelectable = true;
	bShouldSelectUponReceivingFocus = true;
	bInteractableWhenSelected = true;
}

void UAgButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (LabelText)
	{
		LabelText->SetText(ButtonText);
	}
}

void UAgButton::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	if (LabelText)
	{
		LabelText->SetStyle(GetCurrentTextStyleClass());
	}
}

void UAgButton::NativeOnHovered()
{
	Super::NativeOnHovered();

	// Moving the mouse onto a button selects it (게임 플로우 'UI 조작').
	if (GetIsEnabled())
	{
		SetFocus();
	}
}

void UAgButton::HandleFocusLost()
{
	Super::HandleFocusLost();

	// Only the focused button stays selected. SetIsSelected(false) deselects only toggleable buttons, so it would leave this one selected.
	ClearSelection();
}
