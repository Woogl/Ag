// Copyright Woogle. All Rights Reserved.

#include "UI/AgControlsGuideRow.h"

#include "Components/TextBlock.h"

void UAgControlsGuideRow::NativePreConstruct()
{
	Super::NativePreConstruct();

	KeysText->SetText(Keys);
	ActionText->SetText(Action);
	DescriptionText->SetText(Description);
	DescriptionText->SetVisibility(Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
