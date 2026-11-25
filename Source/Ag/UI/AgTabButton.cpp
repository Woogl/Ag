// Copyright Woogle. All Rights Reserved.

#include "UI/AgTabButton.h"

#include "CommonTextBlock.h"

UAgTabButton::UAgTabButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 방향키는 탭 내용 안에서만 선택을 옮기므로 탭 이름은 포커스를 받지 않습니다. 탭은 Q·E와 좌클릭으로 바꿉니다.
	SetIsFocusable(false);
}

void UAgTabButton::SetLabel(const FText& Label)
{
	LabelText->SetText(Label);
}

void UAgTabButton::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	LabelText->SetStyle(GetCurrentTextStyleClass());
}

void UAgTabButton::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);

	SelectionMark->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UAgTabButton::NativeOnDeselected(bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);

	SelectionMark->SetVisibility(ESlateVisibility::Hidden);
}
