// Copyright Woogle. All Rights Reserved.

#include "UI/AgInteractionPrompt.h"

#include "Components/TextBlock.h"

void UAgInteractionPrompt::SetAction(const FText& Text)
{
	// 상호작용 컴포넌트가 매 틱 부르므로, 글자가 바뀔 때만 넣습니다.
	if (!ActionText->GetText().EqualTo(Text))
	{
		ActionText->SetText(Text);
	}
}
