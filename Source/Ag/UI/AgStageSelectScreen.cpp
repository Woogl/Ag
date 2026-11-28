// Copyright Woogle. All Rights Reserved.

#include "UI/AgStageSelectScreen.h"

#include "Game/AgGameMode.h"
#include "Game/AgSaveSubsystem.h"
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
	BackButton->OnClicked().AddUObject(this, &ThisClass::Close);
}

void UAgStageSelectScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// CommonUI 스택은 닫은 화면을 다시 쓰므로, 열 때마다 목록을 다시 채워 첫 스테이지를 고릅니다.
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

bool UAgStageSelectScreen::NativeOnHandleBackAction()
{
	// 이 화면부터 타이틀로 나갈 때까지가 슬롯을 불러와 있는 구간입니다. Back 버튼과 Esc가 모두 이리로 옵니다.
	UAgSaveSubsystem::Get(this).Unload();
	return Super::NativeOnHandleBackAction();
}

void UAgStageSelectScreen::OpenSelectedStage()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->OpenStage(StageDropdown->GetSelectedOption());
	}
}

void UAgStageSelectScreen::Close()
{
	NativeOnHandleBackAction();
}
