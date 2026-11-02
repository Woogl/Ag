// Copyright Woogle. All Rights Reserved.

#include "UI/AgStageOptionWidget.h"

#include "CommonTextBlock.h"
#include "Game/AgGameMode.h"

void UAgStageOptionWidget::SetStage(const FAgStage& Stage)
{
	NameText->SetText(Stage.DisplayName);
}
