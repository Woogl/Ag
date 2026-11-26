// Copyright Woogle. All Rights Reserved.

#include "UI/AgConfirmDialog.h"

#include "CommonTextBlock.h"
#include "UI/AgButton.h"

UAgConfirmDialog::UAgConfirmDialog(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UAgConfirmDialog::SetMessage(const FText& Message)
{
	MessageText->SetText(Message);
}

void UAgConfirmDialog::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	YesButton->OnClicked().AddUObject(this, &ThisClass::Confirm);
	NoButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);

	// 뒤의 메뉴 화면도 켜져 있어, 막지 않으면 방향키 선택이 창 밖의 버튼으로 넘어가고 Enter가 그 버튼을 실행합니다.
	for (const EUINavigation Direction : {EUINavigation::Left, EUINavigation::Right, EUINavigation::Up, EUINavigation::Down})
	{
		SetNavigationRuleBase(Direction, EUINavigationRule::Stop);
	}
}

UWidget* UAgConfirmDialog::NativeGetDesiredFocusTarget() const
{
	return NoButton;
}

void UAgConfirmDialog::Confirm()
{
	// 받는 쪽이 포커스를 다시 정하므로 창을 먼저 닫습니다.
	DeactivateWidget();
	OnConfirmed.ExecuteIfBound();
}
