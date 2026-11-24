// Copyright Woogle. All Rights Reserved.

#include "Game/AgPlayerController.h"

#include "Ag.h"
#include "Camera/AgPlayerCameraManager.h"
#include "CommonActivatableWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/AgCheatManager.h"
#include "Item/AgInventoryComponent.h"
#include "UI/AgNavigationConfig.h"
#include "UI/AgUILayout.h"

AAgPlayerController::AAgPlayerController()
{
	CheatClass = UAgCheatManager::StaticClass();
	PlayerCameraManagerClass = AAgPlayerCameraManager::StaticClass();
	Inventory = CreateDefaultSubobject<UAgInventoryComponent>(TEXT("Inventory"));
}

void AAgPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	// 에디터 위젯은 슬레이트가 따로 둔 에디터용 설정을 쓰므로, PIE가 끝나도 되돌릴 필요가 없습니다.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetNavigationConfig(MakeShared<FAgNavigationConfig>());
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// 일시정지 메뉴의 탭 바꾸기(Q)가 게임 매핑(우선순위 0)의 궁극기와 키를 나눠 쓰므로 더 높게 겁니다. Enhanced Input은
		// 매핑을 다시 만들 때 높은 맥락의 소비 액션이 쓴 키를 낮은 맥락에서 지워, 낮으면 CommonUI가 탭 액션의 키를 찾지
		// 못합니다. 탭 액션은 입력을 소비하지 않아 궁극기 매핑도 남습니다.
		InputSubsystem->AddMappingContext(UIInputMapping, 1);
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
