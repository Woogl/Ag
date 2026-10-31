// Copyright Woogle. All Rights Reserved.

#include "Game/AgPlayerController.h"

#include "Ag.h"
#include "CommonActivatableWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/AgCheatManager.h"
#include "UI/AgNavigationConfig.h"
#include "UI/AgUILayout.h"

AAgPlayerController::AAgPlayerController()
{
	CheatClass = UAgCheatManager::StaticClass();
}

void AAgPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	// 에디터 위젯은 슬레이트가 따로 둔 에디터용 설정을 쓰므로, PIE가 끝나도 되돌릴 필요가 없습니다.
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

void AAgPlayerController::PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass)
{
	if (!ScreenClass)
	{
		UE_LOG(LogAg, Error, TEXT("PushScreen: screen class is not set (check the Blueprint defaults)."));
		return;
	}
	if (UILayout)
	{
		UILayout->PushScreen(Layer, ScreenClass);
	}
}
