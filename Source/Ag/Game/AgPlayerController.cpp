// Copyright Woogle. All Rights Reserved.

#include "Game/AgPlayerController.h"

#include "Ag.h"
#include "CommonActivatableWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/AgCheatManager.h"
#include "UI/AgBattleResultScreen.h"
#include "UI/AgHUDWidget.h"
#include "UI/AgNavigationConfig.h"
#include "UI/AgUILayout.h"

AAgPlayerController::AAgPlayerController()
{
	CheatClass = UAgCheatManager::StaticClass();
}

void AAgPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetNavigationConfig(MakeShared<FAgNavigationConfig>());
	}

	// 게임 모드가 PostLogin에서 첫 화면을 열기 전에 레이아웃이 있어야 합니다.
	// 엔진은 PostLogin 직전의 SetPlayer에서 이 함수를 부릅니다.
	if (!UILayoutClass)
	{
		UE_LOG(LogAg, Error, TEXT("UILayoutClass is not set (BP_PlayerController)."));
		return;
	}
	UILayout = CreateWidget<UAgUILayout>(this, UILayoutClass);
	UILayout->AddToPlayerScreen();
}

UCommonActivatableWidget* AAgPlayerController::PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass)
{
	if (!ScreenClass)
	{
		UE_LOG(LogAg, Error, TEXT("PushScreen: screen class is not set (check the Blueprint defaults)."));
		return nullptr;
	}
	return UILayout ? UILayout->PushScreen(Layer, ScreenClass) : nullptr;
}

void AAgPlayerController::ShowHUD(TSubclassOf<UAgHUDWidget> HUDClass)
{
	HUDWidget = Cast<UAgHUDWidget>(PushScreen(EAgUILayer::HUD, HUDClass));
}

void AAgPlayerController::HideHUD()
{
	if (HUDWidget)
	{
		HUDWidget->HideStatus();
	}
}

void AAgPlayerController::ShowResult(TSubclassOf<UAgBattleResultScreen> BattleResultScreenClass, bool bStageCleared)
{
	if (UAgBattleResultScreen* BattleResultScreen = Cast<UAgBattleResultScreen>(PushScreen(EAgUILayer::Menu, BattleResultScreenClass)))
	{
		BattleResultScreen->SetResult(bStageCleared);
	}
}
