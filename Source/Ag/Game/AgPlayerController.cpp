// Copyright Woogle. All Rights Reserved.

#include "Game/AgPlayerController.h"

#include "Ag.h"
#include "Camera/AgCameraShake.h"
#include "Camera/CameraModifier_CameraShake.h"
#include "Camera/PlayerCameraManager.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCameraData.h"
#include "CommonActivatableWidget.h"
#include "Game/AgCheatManager.h"
#include "UI/AgHUDWidget.h"
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

void AAgPlayerController::PlayCameraShake(EAgCameraShake Shake)
{
	if (Shake == EAgCameraShake::None || !IsLocalController() || !PlayerCameraManager)
	{
		return;
	}

	const UAgCameraData& Camera = UAgAssetManager::Get().GetCameraData();
	const FAgCameraShakeSettings* Settings = Camera.FindShakeSettings(Shake);
	if (!Settings)
	{
		return;
	}

	FAddCameraShakeParams ShakeParams;
	ShakeParams.Initializer.BindLambda([Settings, &Camera](UCameraShakeBase* ShakeInstance)
	{
		CastChecked<UAgCameraShake>(ShakeInstance)->Initialize(*Settings, Camera);
	});
	PlayerCameraManager->StartCameraShake(UAgCameraShake::StaticClass(), ShakeParams);
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
		HUDWidget->HideStatus();
	}
}

void AAgPlayerController::ShowResult(TSubclassOf<UAgResultScreen> ResultScreenClass, bool bStageCleared)
{
	if (UAgResultScreen* ResultScreen = Cast<UAgResultScreen>(PushScreen(ResultScreenClass, ResultZOrder)))
	{
		ResultScreen->SetResult(bStageCleared);
	}
}
