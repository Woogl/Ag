// Copyright Woogle. All Rights Reserved.

#include "UI/AgInteractionPrompt.h"

#include "Components/TextBlock.h"

void UAgInteractionPrompt::SetAction(const FText& Text)
{
	ActionText->SetText(Text);
}
