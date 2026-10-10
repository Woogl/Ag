// Copyright Woogle. All Rights Reserved.

#include "Game/AgPlayerController.h"

#include "Ag.h"
#include "CommonActivatableWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/AgCheatManager.h"
#include "UI/AgHUDWidget.h"
#include "UI/AgNavigationConfig.h"
#include "UI/AgResultScreen.h"

namespace
{
	constexpr int32 HUDZOrder = 0;
	constexpr int32 ResultZOrder = 10;
}

AAgPlayerController::AAgPlayerController()
{
	CheatClass = UAgCheatManager::StaticClass();
}

void AAgPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// W·S move between buttons on top of the arrow keys (게임 플로우 'UI 조작').
	if (IsLocalController() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetNavigationConfig(MakeShared<FAgNavigationConfig>());
	}
}

UCommonActivatableWidget* AAgPlayerController::PushScreen(TSubclassOf<UCommonActivatableWidget> ScreenClass, int32 ZOrder)
{
	if (!ScreenClass)
	{
		UE_LOG(LogAg, Error, TEXT("PushScreen: screen class is not set (check the game mode Blueprint)."));
		return nullptr;
	}

	UCommonActivatableWidget* Screen = CreateWidget<UCommonActivatableWidget>(this, ScreenClass);
	Screen->AddToPlayerScreen(ZOrder);
	Screen->ActivateWidget();
	return Screen;
}

void AAgPlayerController::ShowHUD(TSubclassOf<UAgHUDWidget> HUDClass)
{
	HUDWidget = Cast<UAgHUDWidget>(PushScreen(HUDClass, HUDZOrder));
}

void AAgPlayerController::HideHUD()
{
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AAgPlayerController::ShowResult(TSubclassOf<UAgResultScreen> ResultScreenClass, bool bStageCleared)
{
	if (UAgResultScreen* ResultScreen = Cast<UAgResultScreen>(PushScreen(ResultScreenClass, ResultZOrder)))
	{
		ResultScreen->SetResult(bStageCleared);
	}
}
