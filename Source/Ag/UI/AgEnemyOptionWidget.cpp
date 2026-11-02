// Copyright Woogle. All Rights Reserved.

#include "UI/AgEnemyOptionWidget.h"

#include "CommonTextBlock.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCharacterData.h"

void UAgEnemyOptionWidget::SetEnemy(FName CharacterDataId)
{
	if (CharacterDataId.IsNone())
	{
		NameText->SetText(EmptyText);
		return;
	}
	const UAgCharacterData* Data = UAgAssetManager::Get().FindCharacterData(CharacterDataId);
	NameText->SetText(Data ? Data->DisplayName : FText::FromName(CharacterDataId));
}
