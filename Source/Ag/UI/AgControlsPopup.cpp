// Copyright Woogle. All Rights Reserved.

#include "UI/AgControlsPopup.h"

#include "UI/AgButton.h"

void UAgControlsPopup::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BindEscapeToClose();
	// 뒤의 타이틀 버튼이 켜진 채 남아 있어, 막지 않으면 방향키가 팝업 밖으로 포커스를 옮깁니다.
	ConfirmButton->SetAllNavigationRules(EUINavigationRule::Stop, NAME_None);
	ConfirmButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);
}

UWidget* UAgControlsPopup::NativeGetDesiredFocusTarget() const
{
	return ConfirmButton;
}
