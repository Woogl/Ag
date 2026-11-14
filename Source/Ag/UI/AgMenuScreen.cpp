// Copyright Woogle. All Rights Reserved.

#include "UI/AgMenuScreen.h"

#include "Core/AgGameplayTags.h"
#include "Input/CommonUIInputTypes.h"

TOptional<FUIInputConfig> UAgMenuScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UAgMenuScreen::BindEscapeToClose()
{
	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(AgGameplayTags::UI_Action_Escape), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::DeactivateWidget)));
}
