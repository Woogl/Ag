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

	// 마우스를 올리면 그 버튼을 선택합니다(게임 플로우 'UI 조작').
	if (GetIsEnabled())
	{
		SetFocus();
	}
}

void UAgButton::HandleFocusLost()
{
	Super::HandleFocusLost();

	// 선택은 포커스를 가진 버튼에만 남깁니다. SetIsSelected(false)는 토글 버튼만 선택을 풀어 주므로 ClearSelection을 씁니다.
	ClearSelection();
}
