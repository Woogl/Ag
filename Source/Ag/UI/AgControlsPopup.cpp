// Copyright Woogle. All Rights Reserved.

#include "UI/AgControlsPopup.h"

#include "Core/AgGameplayTags.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/AgButton.h"

void UAgControlsPopup::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(AgGameplayTags::UI_Action_Escape), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::DeactivateWidget)));
	ConfirmButton->SetAllNavigationRules(EUINavigationRule::Stop, NAME_None);
	ConfirmButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);
}

UWidget* UAgControlsPopup::NativeGetDesiredFocusTarget() const
{
	return ConfirmButton;
}

TOptional<FUIInputConfig> UAgControlsPopup::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
