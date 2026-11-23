// Copyright Woogle. All Rights Reserved.

#include "UI/AgMenuScreen.h"

#include "Input/CommonUIInputTypes.h"

TOptional<FUIInputConfig> UAgMenuScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
