// Copyright Woogle. All Rights Reserved.

#include "UI/AgTitleScreen.h"

#include "Game/AgGameMode.h"
#include "Game/AgPlayerController.h"
#include "Game/AgSaveSubsystem.h"
#include "UI/AgButton.h"
#include "UI/AgLoadGameScreen.h"
#include "UI/AgSettingsScreen.h"
#include "UI/AgStageSelectScreen.h"
#include "UI/AgUILayout.h"

UAgTitleScreen::UAgTitleScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRestoreFocus = true;
}

void UAgTitleScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ContinueButton->OnClicked().AddUObject(this, &ThisClass::ContinueGame);
	NewGameButton->OnClicked().AddUObject(this, &ThisClass::StartNewGame);
	LoadGameButton->OnClicked().AddUObject(this, &ThisClass::OpenScreen, TSubclassOf<UCommonActivatableWidget>(LoadGameScreenClass));
	SettingsButton->OnClicked().AddUObject(this, &ThisClass::OpenScreen, TSubclassOf<UCommonActivatableWidget>(SettingsScreenClass));

	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		QuitButton->OnClicked().AddUObject(GameMode, &AAgGameMode::QuitGame);
	}
}

void UAgTitleScreen::NativeOnActivated()
{
	// 위 화면(스테이지 선택, Load Game)에서 돌아올 때마다 켜지고, 그 사이에 저장이 생기거나 지워졌을 수 있습니다.
	// 엔진은 켜질 때 포커스 대상을 묻으므로 버튼을 먼저 정합니다.
	bool bHasSaved = false;
	bool bHasEmpty = false;
	bool bHasCorrupted = false;
	for (const FAgSaveSlotInfo& Info : UAgSaveSubsystem::Get(this).ReadSlots())
	{
		bHasSaved |= Info.State == EAgSaveSlotState::Saved;
		bHasEmpty |= Info.State == EAgSaveSlotState::Empty;
		bHasCorrupted |= Info.State == EAgSaveSlotState::Corrupted;
	}
	// 쓸 수 없는 버튼은 접어, 남은 버튼이 빈자리 없이 위에서부터 놓이게 합니다. 지난번에 포커스가 있던 버튼이 접히면
	// 엔진이 첫 포커스 대상으로 넘어갑니다(DefaultEngine.ini의 CommonUI.FallbackToDesiredOnAutoRestoreFailure).
	ContinueButton->SetVisibility(bHasSaved ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	NewGameButton->SetVisibility(bHasEmpty ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	// 읽을 수 없는 저장은 Load Game 화면에서만 지울 수 있습니다.
	LoadGameButton->SetVisibility(bHasSaved || bHasCorrupted ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	Super::NativeOnActivated();
}

UWidget* UAgTitleScreen::NativeGetDesiredFocusTarget() const
{
	for (UAgButton* Button : {ContinueButton.Get(), NewGameButton.Get(), LoadGameButton.Get()})
	{
		if (Button->IsVisible())
		{
			return Button;
		}
	}
	return SettingsButton;
}

void UAgTitleScreen::ContinueGame()
{
	if (!UAgSaveSubsystem::Get(this).LoadLatestSlot())
	{
		return;
	}
	AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>();
	if (!GameMode || !GameMode->OpenLastStage())
	{
		OpenScreen(StageSelectScreenClass);
	}
}

void UAgTitleScreen::StartNewGame()
{
	if (UAgSaveSubsystem::Get(this).StartNewGame())
	{
		OpenScreen(StageSelectScreenClass);
	}
}

void UAgTitleScreen::OpenScreen(TSubclassOf<UCommonActivatableWidget> ScreenClass)
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		PlayerController->PushScreen(EAgUILayer::Menu, ScreenClass);
	}
}
