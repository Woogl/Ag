// Copyright Woogle. All Rights Reserved.

#include "UI/AgEnemyCard.h"

#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/AgSimulationRoomGameMode.h"

void UAgEnemyCard::SetOption(FName Key, const FText& Label)
{
	Super::SetOption(Key, Label);

	AAgSimulationRoomGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgSimulationRoomGameMode>();
	UTextureRenderTarget2D* Preview = GameMode ? GameMode->GetEnemyPreview(GameMode->FindEnemyClass(Key)) : nullptr;
	if (Preview)
	{
		ModelImage->SetBrushResourceObject(Preview);
	}
	ModelImage->SetVisibility(Preview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	EmptyMark->SetVisibility(Preview ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
