// Copyright Woogle. All Rights Reserved.

#include "UI/AgButton.h"

#include "CommonTextBlock.h"

UAgButton::UAgButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSelectable = true;
	bShouldSelectUponReceivingFocus = true;
	bInteractableWhenSelected = true;
	bToggleable = false;
}

void UAgButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (LabelText)
	{
		LabelText->SetText(ButtonText);
	}
	UpdateSelectionMarker();
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

	SetIsSelected(false, false);
}

void UAgButton::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);

	UpdateSelectionMarker();
}

void UAgButton::NativeOnDeselected(bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);

	UpdateSelectionMarker();
}

void UAgButton::UpdateSelectionMarker()
{
	if (SelectionMarker)
	{
		SelectionMarker->SetVisibility(GetSelected() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
