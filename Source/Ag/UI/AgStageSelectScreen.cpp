// Copyright Woogle. All Rights Reserved.

#include "UI/AgStageSelectScreen.h"

#include "Game/AgGameMode.h"
#include "UI/AgButton.h"
#include "UI/AgDropdown.h"

UAgStageSelectScreen::UAgStageSelectScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UAgStageSelectScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	StartButton->OnClicked().AddUObject(this, &ThisClass::OpenSelectedStage);
	BackButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);
}

void UAgStageSelectScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// CommonUI 스택은 닫은 화면을 다시 쓰므로, 열 때마다 목록을 다시 채우고 첫 스테이지를 고릅니다.
	TArray<FAgDropdownOption> Options;
	for (const FAgStage& Stage : AAgGameMode::FindStages())
	{
		Options.Add({Stage.Map, Stage.DisplayName});
	}
	StageDropdown->SetOptions(Options);
}

UWidget* UAgStageSelectScreen::NativeGetDesiredFocusTarget() const
{
	return StageDropdown->GetDesiredFocusWidget();
}

void UAgStageSelectScreen::OpenSelectedStage()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->OpenStage(StageDropdown->GetSelectedOption());
	}
}
